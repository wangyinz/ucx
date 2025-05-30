/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2001-2021. ALL RIGHTS RESERVED.
* Copyright (C) Huawei Technologies Co., Ltd. 2020.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifndef UCT_IB_IFACE_H
#define UCT_IB_IFACE_H

#include "ib_md.h"

#include <uct/api/uct.h>
#include <uct/base/uct_iface.h>
#include <uct/base/uct_iov.inl>
#include <ucs/sys/compiler.h>
#include <ucs/sys/string.h>
#include <ucs/sys/math.h>
#include <ucs/datastruct/mpool.inl>
#include <ucs/datastruct/string_buffer.h>


#define UCT_IB_MAX_IOV                     8UL
#define UCT_IB_IFACE_NULL_RES_DOMAIN_KEY   0u
#define UCT_IB_MAX_ATOMIC_SIZE             sizeof(uint64_t)
#define UCT_IB_ADDRESS_INVALID_GID_INDEX   UINT8_MAX
#define UCT_IB_ADDRESS_INVALID_PATH_MTU    ((enum ibv_mtu)0)
#define UCT_IB_ADDRESS_INVALID_PKEY        0
#define UCT_IB_ADDRESS_DEFAULT_PKEY        0xffff
#define UCT_IB_SL_NUM                      16
#define UCT_IB_COUNTER_SET_ID_INVALID      UINT8_MAX


#define UCT_IB_SEND_OVERHEAD_VALUE(_iface_overhead) \
    "bcopy:5ns,cqe:20ns,db:40ns,wqe_fetch:350ns," \
    "wqe_post:" UCS_PP_MAKE_STRING(_iface_overhead)


#define UCT_IB_SEND_OVERHEAD_DEFAULT(_iface_overhead) \
    UCT_IB_CONFIG_PREFIX "SEND_OVERHEAD=" \
    UCT_IB_SEND_OVERHEAD_VALUE(_iface_overhead)


/* Forward declarations */
typedef struct uct_ib_iface_config   uct_ib_iface_config_t;
typedef struct uct_ib_iface_ops      uct_ib_iface_ops_t;
typedef struct uct_ib_iface          uct_ib_iface_t;


/**
 * IB port active speed.
 */
enum {
    UCT_IB_SPEED_SDR     = 1,
    UCT_IB_SPEED_DDR     = 2,
    UCT_IB_SPEED_QDR     = 4,
    UCT_IB_SPEED_FDR10   = 8,
    UCT_IB_SPEED_FDR     = 16,
    UCT_IB_SPEED_EDR     = 32,
    UCT_IB_SPEED_HDR     = 64,
    UCT_IB_SPEED_NDR     = 128,
    UCT_IB_SPEED_LAST
};


/**
 * IB port/path MTU.
 */
typedef enum uct_ib_mtu {
    UCT_IB_MTU_DEFAULT = 0,
    UCT_IB_MTU_512     = 1,
    UCT_IB_MTU_1024    = 2,
    UCT_IB_MTU_2048    = 3,
    UCT_IB_MTU_4096    = 4,
    UCT_IB_MTU_LAST
} uct_ib_mtu_t;


/**
 * Traffic direction.
 */
typedef enum {
    UCT_IB_DIR_RX,
    UCT_IB_DIR_TX,
    UCT_IB_DIR_LAST
} uct_ib_dir_t;

enum {
    UCT_IB_QPT_UNKNOWN,
#if HAVE_DC_DV
    UCT_IB_QPT_DCI = IBV_QPT_DRIVER,
#else
    UCT_IB_QPT_DCI = UCT_IB_QPT_UNKNOWN,
#endif
};


/**
 * IB address packing flags
 */
enum {
    UCT_IB_ADDRESS_PACK_FLAG_ETH           = UCS_BIT(0),
    UCT_IB_ADDRESS_PACK_FLAG_INTERFACE_ID  = UCS_BIT(1),
    UCT_IB_ADDRESS_PACK_FLAG_SUBNET_PREFIX = UCS_BIT(2),
    UCT_IB_ADDRESS_PACK_FLAG_PATH_MTU      = UCS_BIT(3),
    UCT_IB_ADDRESS_PACK_FLAG_GID_INDEX     = UCS_BIT(4),
    UCT_IB_ADDRESS_PACK_FLAG_PKEY          = UCS_BIT(5)
};


/**
 * Reachability mode.
 */
typedef enum {
    UCT_IB_REACHABILITY_MODE_ROUTE        = 0,
    UCT_IB_REACHABILITY_MODE_LOCAL_SUBNET = 1,
    UCT_IB_REACHABILITY_MODE_ALL          = 2,
    UCT_IB_REACHABILITY_MODE_LAST
} uct_ib_iface_reachability_mode_t;


enum {
    UCT_IB_IFACE_STAT_RX_COMPLETION,
    UCT_IB_IFACE_STAT_TX_COMPLETION,
    UCT_IB_IFACE_STAT_RX_COMPLETION_ZIPPED,
    UCT_IB_IFACE_STAT_TX_COMPLETION_ZIPPED,
    UCT_IB_IFACE_STAT_LAST
};

typedef struct uct_ib_address_pack_params {
    /* Packing flags, UCT_IB_ADDRESS_PACK_FLAG_xx. */
    uint64_t                          flags;
    /* GID address to pack/unpack. */
    union ibv_gid                     gid;
    /* LID address to pack/unpack. */
    uint16_t                          lid;
    /* RoCE version to pack/unpack in case of an Ethernet link layer,
       must be valid if @ref UCT_IB_ADDRESS_PACK_FLAG_ETH is set. */
    uct_ib_roce_version_info_t        roce_info;
    /* path MTU size as defined in enum ibv_mtu,
       must be valid if @ref UCT_IB_ADDRESS_PACK_FLAG_PATH_MTU is set. */
    enum ibv_mtu                      path_mtu;
    /* GID index,
       must be valid if @ref UCT_IB_ADDRESS_PACK_FLAG_GID_INDEX is set. */
    uint8_t                           gid_index;
    /* PKEY value,
       must be valid if @ref UCT_IB_ADDRESS_PACK_FLAG_PKEY is set. */
    uint16_t                          pkey;
} uct_ib_address_pack_params_t;



/** Overhead of send operation of ib interface */
typedef struct uct_ib_iface_send_overhead {
    /** Overhead of allocating a tx buffer */
    double bcopy;
    /** Overhead of processing a work request completion */
    double cqe;
    /** Overhead of writing a doorbell to PCI */
    double db;
    /** Overhead of fetching a wqe */
    double wqe_fetch;
    /** Overhead of posting a wqe */
    double wqe_post;
} uct_ib_iface_send_overhead_t;


struct uct_ib_iface_config {
    uct_iface_config_t      super;

    size_t                  seg_size;      /* Maximal size of copy-out sends */

    struct {
        unsigned            queue_len;       /* Queue length */
        unsigned            max_batch;       /* How many fragments can be batched to one post send */
        unsigned            max_poll;        /* How many wcs can be picked when polling tx cq */
        size_t              min_inline;      /* Inline space to reserve for sends */
        unsigned            min_sge;         /* How many SG entries to support */
        uct_iface_mpool_config_t mp;
    } tx;

    struct {
        unsigned            queue_len;       /* Queue length */
        unsigned            max_batch;       /* How many buffers can be batched to one post receive */
        unsigned            max_poll;        /* How many wcs can be picked when polling rx cq */
        uct_iface_mpool_config_t mp;
    } rx;

    /* Inline space to reserve in CQ */
    size_t                           inl[UCT_IB_DIR_LAST];

    /* Change the address type */
    int                              addr_type;

    /* Force global routing */
    int                              is_global;

    /* Use FLID based routing */
    int                              flid_enabled;

    /* IB SL to use (default: AUTO) */
    unsigned long                    sl;

    /* IB Traffic Class to use */
    unsigned long                    traffic_class;

    /* IB hop limit / TTL */
    unsigned                         hop_limit;

    /* Number of paths to expose for the interface  */
    unsigned long                    num_paths;

    /* Whether to check RoCEv2 reachability by IP address and local subnet */
    int                              rocev2_local_subnet;

    /* Length of subnet prefix for reachability check */
    unsigned long                    rocev2_subnet_pfx_len;

    /* Mode used for performing reachability check */
    uct_ib_iface_reachability_mode_t reachability_mode;

    /* List of included/excluded subnets to filter RoCE GID entries by */
    ucs_config_allow_list_t          rocev2_subnet_filter;

    /* Multiplier for RoCE LAG UDP source port calculation */
    unsigned                         roce_path_factor;

    /* Ranges of path bits */
    UCS_CONFIG_ARRAY_FIELD(ucs_range_spec_t, ranges) lid_path_bits;

    /* IB PKEY to use */
    unsigned                         pkey;

    /* Path MTU size */
    uct_ib_mtu_t                     path_mtu;

    /* QP counter set ID */
    unsigned long                    counter_set_id;

    /* IB reverse SL (default: AUTO - same value as sl) */
    unsigned long                    reverse_sl;

    /**
     * Estimated overhead of preparing a work request and posting it to the NIC
     */
    uct_ib_iface_send_overhead_t     send_overhead;
};


enum {
    UCT_IB_CQ_IGNORE_OVERRUN         = UCS_BIT(0),
    UCT_IB_TM_SUPPORTED              = UCS_BIT(1),

    /* Indicates that TX cq len in uct_ib_iface_init_attr_t is specified per
     * each IB path. Therefore IB interface constructor would need to multiply
     * TX CQ len by the number of IB paths (when it is properly initialized). */
    UCT_IB_TX_OPS_PER_PATH           = UCS_BIT(2),
    /* Whether device and transport supports DDP */
    UCT_IB_DDP_SUPPORTED             = UCS_BIT(3)
};


typedef struct uct_ib_iface_init_attr {
    unsigned    rx_priv_len;             /* Length of transport private data to reserve */
    unsigned    rx_hdr_len;              /* Length of transport network header */
    unsigned    cq_len[UCT_IB_DIR_LAST]; /* CQ length */
    size_t      seg_size;                /* Transport segment size */
    unsigned    fc_req_size;             /* Flow control request size */
    int         qp_type;                 /* IB QP type */
    int         flags;                   /* Various flags (see enum) */
    /* The maximum number of outstanding RDMA Read/Atomic operations per QP */
    unsigned    max_rd_atomic;
    uint8_t     cqe_zip_sizes[UCT_IB_DIR_LAST];
    uint16_t    tx_moderation;           /* TX CQ moderation */
} uct_ib_iface_init_attr_t;


#if HAVE_DECL_IBV_CREATE_QP_EX
typedef struct ibv_qp_init_attr_ex uct_ib_qp_init_attr_t;
#else
typedef struct ibv_qp_init_attr uct_ib_qp_init_attr_t;
#endif


typedef struct uct_ib_qp_attr {
    int                         qp_type;
    struct ibv_qp_cap           cap;
    int                         port;
    struct ibv_srq              *srq;
    uint32_t                    srq_num;
    unsigned                    sq_sig_all;
    unsigned                    max_inl_cqe[UCT_IB_DIR_LAST];
    uct_ib_qp_init_attr_t       ibv;
} uct_ib_qp_attr_t;


typedef ucs_status_t (*uct_ib_iface_create_cq_func_t)(uct_ib_iface_t *iface,
                                                      uct_ib_dir_t dir,
                                                      const uct_ib_iface_init_attr_t *init_attr,
                                                      int preferred_cpu,
                                                      size_t inl);

typedef void (*uct_ib_iface_destroy_cq_func_t)(uct_ib_iface_t *iface,
                                               uct_ib_dir_t dir);

typedef void (*uct_ib_iface_event_cq_func_t)(uct_ib_iface_t *iface,
                                             uct_ib_dir_t dir);

typedef void (*uct_ib_iface_handle_failure_func_t)(uct_ib_iface_t *iface, void *arg,
                                                   ucs_status_t status);

typedef ucs_status_t (*uct_ib_iface_set_ep_failed_func_t)(uct_ib_iface_t *iface, uct_ep_h ep,
                                                          ucs_status_t status);


struct uct_ib_iface_ops {
    uct_iface_internal_ops_t           super;
    uct_ib_iface_create_cq_func_t      create_cq;
    uct_ib_iface_destroy_cq_func_t     destroy_cq;
    uct_ib_iface_event_cq_func_t       event_cq;
    uct_ib_iface_handle_failure_func_t handle_failure;
};


struct uct_ib_iface {
    uct_base_iface_t          super;

    struct ibv_cq             *cq[UCT_IB_DIR_LAST];
    struct ibv_comp_channel   *comp_channel;
    uct_recv_desc_t           release_desc;

    uint8_t                   *path_bits;
    unsigned                  path_bits_count;
    unsigned                  num_paths;
    uint16_t                  pkey_index;
    uint16_t                  pkey;
    uint8_t                   addr_size;
    uint8_t                   addr_prefix_bits;
    uct_ib_device_gid_info_t  gid_info;

    struct {
        /* offset from desc to payload */
        unsigned                         rx_payload_offset;
        /* offset from desc to network header */
        unsigned                         rx_hdr_offset;
        /* offset from desc to user headroom */
        unsigned                         rx_headroom_offset;
        unsigned                         rx_max_batch;
        unsigned                         rx_max_poll;
        unsigned                         tx_max_poll;
        unsigned                         seg_size;
        unsigned                         roce_path_factor;
        uint8_t                          max_inl_cqe[UCT_IB_DIR_LAST];
        uint8_t                          port_num;
        uint8_t                          sl;
        uint8_t                          reverse_sl;
        uint8_t                          traffic_class;
        uint8_t                          hop_limit;
        uint8_t                          qp_type;
        uint8_t                          force_global_addr;
        uint8_t                          flid_enabled;
        enum ibv_mtu                     path_mtu;
        uint8_t                          counter_set_id;
        uct_ib_iface_send_overhead_t     send_overhead;
        uct_ib_iface_reachability_mode_t reachability_mode;
    } config;

    uct_ib_iface_ops_t        *ops;
    UCS_STATS_NODE_DECLARE(stats)
};


typedef struct uct_ib_fence_info {
    uint16_t                    fence_beat; /* 16bit is enough because if it wraps around,
                                             * it means the older ops are already completed
                                             * because QP size is less than 64k */
} uct_ib_fence_info_t;


UCS_CLASS_DECLARE(uct_ib_iface_t, uct_iface_ops_t*, uct_ib_iface_ops_t*,
                  uct_md_h, uct_worker_h, const uct_iface_params_t*,
                  const uct_ib_iface_config_t*,
                  const uct_ib_iface_init_attr_t*);

/*
 * The offset to the payload is the maximum between user-requested headroom
 * and transport-specific data/header. When the active message callback is invoked,
 * it gets a pointer to the beginning of the headroom.
 * The headroom can be either smaller (1) or larger (2) than the transport data.
 *
 * (1)
 *
 * <rx_headroom_offset>
 *                   |
 *                   |
 * uct_recv_desc_t   |
 *               |   |
 *               |   |           am_callback/tag_unexp_callback
 *               |   |           |
 * +------+------+---+-----------+---------+
 * | LKey |  ??? | D | Head Room | Payload |
 * +------+------+---+--+--------+---------+
 * | LKey |     TL data | TL hdr | Payload |
 * +------+-------------+--------+---------+
 *                      |
 *                      post_receive
 *
 * (2)
 *                               am_callback/tag_unexp_callback
 *                               |
 * +------+---+------------------+---------+
 * | LKey | D |     Head Room    | Payload |
 * +------+---+-----+---+--------+---------+
 * | LKey | TL data | ? | TL hdr | Payload |
 * +------+---------+---+--------+---------+
 *                      |
 *                      post_receive
 *        <dsc>
 *            <--- rx_headroom -->
 * <------- rx_payload_offset --->
 * <--- rx_hdr_offset -->
 *
 */
typedef struct uct_ib_iface_recv_desc {
    uint32_t                lkey;
} UCS_S_PACKED uct_ib_iface_recv_desc_t;



extern ucs_config_field_t uct_ib_iface_config_table[];
extern const char *uct_ib_mtu_values[];


/**
 * Create memory pool of receive descriptors.
 */
ucs_status_t uct_ib_iface_recv_mpool_init(uct_ib_iface_t *iface,
                                          const uct_ib_iface_config_t *config,
                                          const uct_iface_params_t *params,
                                          const char *name, ucs_mpool_t *mp);

void uct_ib_iface_release_desc(uct_recv_desc_t *self, void *desc);


static UCS_F_ALWAYS_INLINE void
uct_ib_iface_invoke_am_desc(uct_ib_iface_t *iface, uint8_t am_id, void *data,
                            unsigned length, uct_ib_iface_recv_desc_t *ib_desc)
{
    void *desc = (char*)ib_desc + iface->config.rx_headroom_offset;
    ucs_status_t status;

    status = uct_iface_invoke_am(&iface->super, am_id, data, length,
                                 UCT_CB_PARAM_FLAG_DESC);
    if (status == UCS_OK) {
        ucs_mpool_put_inline(ib_desc);
    } else {
        uct_recv_desc(desc) = &iface->release_desc;
    }
}


/**
 * @return Whether the port used by this interface is RoCE
 */
int uct_ib_iface_is_roce(uct_ib_iface_t *iface);


/**
 * @return Whether the port used by this interface is IB
 */
int uct_ib_iface_is_ib(uct_ib_iface_t *iface);


/**
 * Get the expected size of IB packed address.
 *
 * @param [in]  params   Address parameters as defined in
 *                       @ref uct_ib_address_pack_params_t.
 *
 * @return IB address size of the given link scope.
 */
size_t uct_ib_address_size(const uct_ib_address_pack_params_t *params);


/**
 * @return IB address packing flags of the given iface.
 */
unsigned uct_ib_iface_address_pack_flags(uct_ib_iface_t *iface);


/**
 * @return IB address size of the given iface.
 */
size_t uct_ib_iface_address_size(uct_ib_iface_t *iface);


int uct_ib_iface_is_connected(uct_ib_iface_t *ib_iface,
                              const uct_ib_address_t *ib_addr,
                              unsigned path_index, struct ibv_ah *peer_ah);


/**
 * Pack IB address.
 *
 * @param [in]     params   Address parameters as defined in
 *                          @ref uct_ib_address_pack_params_t.
 * @param [in/out] ib_addr  Filled with packed ib address. Size of the structure
 *                          must be at least what @ref uct_ib_address_size()
 *                          returns for the given scope.
 */
void uct_ib_address_pack(const uct_ib_address_pack_params_t *params,
                         uct_ib_address_t *ib_addr);



/**
 * Pack the IB address of the given iface.
 *
 * @param [in]  iface      Iface whose IB address to pack.
 * @param [in/out] ib_addr Filled with packed ib address. Size of the structure
 *                         must be at least what @ref uct_ib_address_size()
 *                         returns for the given scope.
 */
void uct_ib_iface_address_pack(uct_ib_iface_t *iface, uct_ib_address_t *ib_addr);


/**
 * Unpack IB address.
 *
 * @param [in]  ib_addr    IB address to unpack.
 * @param [out] params_p   Filled with address attributes as in
 *                         @ref uct_ib_address_pack_params_t.

 * @return UCS_OK if the address was unpacked successfully, UCS_ERR_INVALID_PARAM
 *         if the address is invalid.
 */
ucs_status_t uct_ib_address_unpack(const uct_ib_address_t *ib_addr,
                                   uct_ib_address_pack_params_t *params_p);


/**
 * Convert IB address to a human-readable string.
 */
const char *uct_ib_address_str(const uct_ib_address_t *ib_addr, char *buf,
                               size_t max);

ucs_status_t uct_ib_iface_get_device_address(uct_iface_h tl_iface,
                                             uct_device_addr_t *dev_addr);

int uct_ib_iface_is_same_device(const uct_ib_address_t *ib_addr, uint16_t dlid,
                                const union ibv_gid *dgid);

int uct_ib_iface_is_reachable_v2(const uct_iface_h tl_iface,
                                 const uct_iface_is_reachable_params_t *params);

/*
 * @param xport_hdr_len       How many bytes this transport adds on top of IB header (LRH+BTH+iCRC+vCRC)
 */
ucs_status_t uct_ib_iface_query(uct_ib_iface_t *iface, size_t xport_hdr_len,
                                uct_iface_attr_t *iface_attr);


ucs_status_t
uct_ib_iface_estimate_perf(uct_iface_h tl_iface, uct_perf_attr_t *perf_attr);


int uct_ib_iface_is_roce_v2(uct_ib_iface_t *iface);


/**
 * Select the IB gid index and RoCE version to use for a RoCE port.
 *
 * @param iface                 IB interface
 * @param md_config_index       Gid index from the md configuration.
 * @param subnets_list          Subnets list to filter GIDs by.
 */
ucs_status_t
uct_ib_iface_init_roce_gid_info(uct_ib_iface_t *iface,
                                unsigned long cfg_gid_index,
                                const ucs_config_allow_list_t *subnets_list);


static inline uct_ib_md_t* uct_ib_iface_md(uct_ib_iface_t *iface)
{
    return ucs_derived_of(iface->super.md, uct_ib_md_t);
}

static inline uct_ib_device_t* uct_ib_iface_device(uct_ib_iface_t *iface)
{
    return &uct_ib_iface_md(iface)->dev;
}

static inline struct ibv_port_attr* uct_ib_iface_port_attr(uct_ib_iface_t *iface)
{
    return uct_ib_device_port_attr(uct_ib_iface_device(iface), iface->config.port_num);
}

static inline void* uct_ib_iface_recv_desc_hdr(uct_ib_iface_t *iface,
                                               uct_ib_iface_recv_desc_t *desc)
{
    return (void*)((char *)desc + iface->config.rx_hdr_offset);
}

typedef struct uct_ib_recv_wr {
    struct ibv_recv_wr ibwr;
    struct ibv_sge     sg;
} uct_ib_recv_wr_t;

/**
 * prepare a list of n work requests that can be passed to
 * ibv_post_recv()
 *
 * @return number of prepared wrs
 */
int uct_ib_iface_prepare_rx_wrs(uct_ib_iface_t *iface, ucs_mpool_t *mp,
                                uct_ib_recv_wr_t *wrs, unsigned n);

ucs_status_t uct_ib_iface_create_ah(uct_ib_iface_t *iface,
                                    struct ibv_ah_attr *ah_attr,
                                    const char *usage, struct ibv_ah **ah_p);

void uct_ib_iface_fill_ah_attr_from_gid_lid(uct_ib_iface_t *iface, uint16_t lid,
                                            const union ibv_gid *gid,
                                            uint8_t gid_index,
                                            unsigned path_index,
                                            struct ibv_ah_attr *ah_attr);

ucs_status_t
uct_ib_iface_fill_ah_attr_from_addr(uct_ib_iface_t *iface,
                                    const uct_ib_address_t *ib_addr,
                                    unsigned path_index,
                                    struct ibv_ah_attr *ah_attr,
                                    enum ibv_mtu *path_mtu);

ucs_status_t uct_ib_iface_pre_arm(uct_ib_iface_t *iface);

ucs_status_t uct_ib_iface_event_fd_get(uct_iface_h iface, int *fd_p);

ucs_status_t uct_ib_iface_arm_cq(uct_ib_iface_t *iface,
                                 uct_ib_dir_t dir,
                                 int solicited_only);

ucs_status_t uct_ib_verbs_create_cq(uct_ib_iface_t *iface, uct_ib_dir_t dir,
                                    const uct_ib_iface_init_attr_t *init_attr,
                                    int preferred_cpu, size_t inl);

void uct_ib_verbs_destroy_cq(uct_ib_iface_t *iface, uct_ib_dir_t dir);

ucs_status_t uct_ib_iface_create_qp(uct_ib_iface_t *iface,
                                    uct_ib_qp_attr_t *attr,
                                    struct ibv_qp **qp_p);

void uct_ib_iface_fill_attr(uct_ib_iface_t *iface,
                            uct_ib_qp_attr_t *attr);

uint8_t uct_ib_iface_config_select_sl(const uct_ib_iface_config_t *ib_config);

void uct_ib_iface_set_reverse_sl(uct_ib_iface_t *ib_iface,
                                 const uct_ib_iface_config_t *ib_config);

uint16_t uct_ib_iface_resolve_remote_flid(uct_ib_iface_t *iface,
                                          const union ibv_gid *gid);

#define UCT_IB_IFACE_FMT \
    "%s:%d/%s"
#define UCT_IB_IFACE_ARG(_iface) \
    uct_ib_device_name(uct_ib_iface_device(_iface)), \
    (_iface)->config.port_num, \
    uct_ib_iface_is_roce(_iface) ? "RoCE" : "IB"


#define UCT_IB_IFACE_VERBS_COMPLETION_MSG(_type, _iface, _i, _wc) \
            "%s completion[%d] with error on %s/%p: %s," \
            " vendor_err 0x%x wr_id 0x%lx", \
            _type, _i, uct_ib_device_name(uct_ib_iface_device(_iface)), \
            _iface, uct_ib_wc_status_str(_wc[_i].status), _wc[_i].vendor_err, \
            _wc[_i].wr_id

#define UCT_IB_IFACE_VERBS_COMPLETION_LOG(_log_lvl, _type, _iface, _i, _wc) \
    ucs_log(_log_lvl, UCT_IB_IFACE_VERBS_COMPLETION_MSG(_type,  _iface, _i, _wc))

#define UCT_IB_IFACE_VERBS_COMPLETION_FATAL(_type, _iface, _i, _wc) \
    ucs_fatal(UCT_IB_IFACE_VERBS_COMPLETION_MSG(_type,  _iface, _i, _wc))

#define UCT_IB_IFACE_VERBS_FOREACH_RXWQE(_iface, _i, _hdr, _wc, _wc_count) \
    for (_i = 0; _i < _wc_count && ({ \
        if (ucs_unlikely(_wc[_i].status != IBV_WC_SUCCESS)) { \
            UCT_IB_IFACE_VERBS_COMPLETION_FATAL("receive", _iface, _i, _wc); \
        } \
        _hdr = (typeof(_hdr))uct_ib_iface_recv_desc_hdr(_iface, \
                                                      (uct_ib_iface_recv_desc_t *)(uintptr_t)_wc[_i].wr_id); \
        VALGRIND_MAKE_MEM_DEFINED(_hdr, _wc[_i].byte_len); \
               1; }); ++_i)

#define UCT_IB_MAX_ZCOPY_LOG_SGE(_iface) \
    (uct_ib_iface_device(_iface)->max_zcopy_log_sge)

/**
 * Fill ibv_sge data structure by data provided in uct_iov_t
 * The function avoids copying IOVs with zero length
 *
 * @return Number of elements in sge[]
 */
static UCS_F_ALWAYS_INLINE
size_t uct_ib_verbs_sge_fill_iov(struct ibv_sge *sge, const uct_iov_t *iov,
                                 size_t iovcnt)
{
    size_t iov_it, sge_it = 0;

    for (iov_it = 0; iov_it < iovcnt; ++iov_it) {
        sge[sge_it].length = uct_iov_get_length(&iov[iov_it]);
        if (sge[sge_it].length > 0) {
            sge[sge_it].addr   = (uintptr_t)(iov[iov_it].buffer);
        } else {
            continue; /* to avoid zero length elements in sge */
        }

        if (iov[iov_it].memh == UCT_MEM_HANDLE_NULL) {
            sge[sge_it].lkey = 0;
        } else {
            sge[sge_it].lkey = uct_ib_memh_get_lkey(iov[iov_it].memh);
        }
        ++sge_it;
    }

    return sge_it;
}

static UCS_F_ALWAYS_INLINE
size_t uct_ib_iface_hdr_size(size_t max_inline, size_t min_size)
{
    return (size_t)ucs_max((ssize_t)(max_inline - min_size), 0);
}

static UCS_F_ALWAYS_INLINE void
uct_ib_fence_info_init(uct_ib_fence_info_t* fence)
{
    fence->fence_beat = 0;
}

static UCS_F_ALWAYS_INLINE unsigned
uct_ib_cq_size(uct_ib_iface_t *iface, const uct_ib_iface_init_attr_t *init_attr,
               uct_ib_dir_t dir)
{
    if (dir == UCT_IB_DIR_RX) {
        return init_attr->cq_len[UCT_IB_DIR_RX];
    } else if (init_attr->flags & UCT_IB_TX_OPS_PER_PATH) {
        return init_attr->cq_len[UCT_IB_DIR_TX] * iface->num_paths;
    } else {
        return init_attr->cq_len[UCT_IB_DIR_TX];
    }
}

static UCS_F_ALWAYS_INLINE unsigned
uct_ib_iface_roce_dscp(uct_ib_iface_t *iface)
{
    ucs_assert(uct_ib_iface_is_roce(iface));
    return iface->config.traffic_class >> 2;
}

#if HAVE_DECL_IBV_CREATE_CQ_EX
static UCS_F_ALWAYS_INLINE void
uct_ib_fill_cq_attr(struct ibv_cq_init_attr_ex *cq_attr,
                    const uct_ib_iface_init_attr_t *init_attr,
                    uct_ib_iface_t *iface, int preferred_cpu, unsigned cq_size)
{
    int num_comp_vectors =
            uct_ib_iface_device(iface)->ibv_context->num_comp_vectors;

    cq_attr->cqe         = cq_size;
    cq_attr->channel     = iface->comp_channel;
    cq_attr->comp_vector = preferred_cpu % num_comp_vectors;
#if HAVE_DECL_IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN
    /* Always check CQ overrun if assert mode enabled. */
    /* coverity[dead_error_condition] */
    if (!UCS_ENABLE_ASSERT && (init_attr->flags & UCT_IB_CQ_IGNORE_OVERRUN)) {
        cq_attr->comp_mask = IBV_CQ_INIT_ATTR_MASK_FLAGS;
        cq_attr->flags     = IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN;
    }
#endif /* HAVE_DECL_IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN */
}
#endif /* HAVE_DECL_IBV_CREATE_CQ_EX */

static UCS_F_ALWAYS_INLINE ucs_status_t
uct_ib_wc_to_ucs_status(enum ibv_wc_status status)
{
    switch (status)
    {
    case IBV_WC_SUCCESS:
        return UCS_OK;
    case IBV_WC_REM_ACCESS_ERR:
    case IBV_WC_REM_OP_ERR:
    case IBV_WC_REM_INV_RD_REQ_ERR:
        return UCS_ERR_CONNECTION_RESET;
    case IBV_WC_RETRY_EXC_ERR:
    case IBV_WC_RNR_RETRY_EXC_ERR:
    case IBV_WC_REM_ABORT_ERR:
        return UCS_ERR_ENDPOINT_TIMEOUT;
    case IBV_WC_WR_FLUSH_ERR:
        return UCS_ERR_CANCELED;
    default:
        return UCS_ERR_IO_ERROR;
    }
}

#endif

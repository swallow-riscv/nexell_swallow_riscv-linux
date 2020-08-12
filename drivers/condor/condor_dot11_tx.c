#include "condor.h"
#include "condor_dev.h"
#include "condor_convergence_pkt.h"

#define HW_TX_CTRL_TX_POWER_SET (1 << 1)

/* datarate, txpower, IP ACI는 임시로 고정한다. */
#define DEFAULT_IFIDX 0
#define DEFAULT_TIMESLOT 0
#define DEFAULT_DATARATE 6
#define DEFAULT_TXPOWER 10
/**
 * IP ACI
 * BE, Best Effort: 0
 * BK, Background:  1
 * VI, Video:       2
 * VO, Voice:       3
 * */
#define DEFAULT_IPACI 1
/**
 * @brief Condor 하드웨어를 제어하기 위한 제어 정보
 *          - MPDU 앞에 붙여서 하드웨어로 전달한다.
 */
struct condor_hw_tx_ctrl
{
    u32 plcp_hdr_low32;
    u8 plcp_hdr_high8;
    u8 tx_ctrl;    // 송신파워 자동설정, AMC 기능 설정
    u16 len_power; // MAC헤더 길이 및 txgain(max2829) 값
} __attribute__((packed));

/**
 * @brief dBm 단위의 송신파워에 해당되는 max2829 설정(txgain) 값
 */
struct condor_hw_tx_power_map
{
    s32 txpower; // 송신파워(dBm)
    u32 mW;      // mW
    u8 txgain;   // txgain
};

/* 데이터레이트 코드
 * 	- PHY PLCP 헤더에서 사용되는 데이터레이트별 코드 값(RATE 필드)이며, PHY하드웨어로 전달된다. */
typedef enum
{
    plcp_rate_code_3mbps = 0xb,
    plcp_rate_code_4p5mbps = 0xf,
    plcp_rate_code_6mbps = 0xa,
    plcp_rate_code_9mbps = 0xe,
    plcp_rate_code_12mbps = 0x9,
    plcp_rate_code_18mbps = 0xd,
    plcp_rate_code_24mbps = 0x8,
    plcp_rate_code_27mbps = 0xc,
} plcp_rate_code;

/**
 * @brief dBm 단위 송신파워를 Max2829용 세팅값과 맵핑 시킨 테이블
 */
static struct condor_hw_tx_power_map g_tx_power_map[MAX2829_SUPPORT_TXPOWER_MAXNUM] =
    {
        /* dBm, mW, txgain */
        {-11, 0.079, 0x00},
        {-10, 0.1, 0x02},
        {-9, 0.126, 0x04},
        {-8, 0.158, 0x06},
        {-7, 0.200, 0x08},
        {-6, 0.251, 0x0a},
        {-5, 0.316, 0x0c},
        {-4, 0.398, 0x0e},
        {-3, 0.500, 0x10},
        {-2, 0.631, 0x12},
        {-1, 0.794, 0x14},
        {0, 1, 0x16},
        {1, 1.259, 0x18},
        {2, 1.585, 0x1a},
        {3, 2.000, 0x1c},
        {4, 2.512, 0x1e},
        {5, 3.162, 0x20},
        {6, 3.980, 0x22},
        {7, 5.012, 0x24},
        {8, 6.310, 0x26},
        {9, 7.943, 0x28},
        {10, 10, 0x2a},
        {11, 12.589, 0x2c},
        {12, 15.849, 0x2e},
        {13, 19.953, 0x30},
        {14, 25.119, 0x32},
        {15, 31.623, 0x34},
        {16, 39.811, 0x36},
        {17, 50.119, 0x38},
        {18, 63.096, 0x3a},
        {19, 79.433, 0x3c},
        {20, 100, 0x3e},
};

/**
 * @brief 500kbps 단위의 Datarate 값을 PLCP 헤더의 코드 값으로 변환한다.
 * @param datarate (입력) 500kbps 단위의 Datarate 값
 * @return 변환된 코드 값
 */
static plcp_rate_code condor_get_plcp_rate_code(u8 datarate)
{
    plcp_rate_code code;

    switch (datarate)
    {
    case 6:
        code = plcp_rate_code_3mbps;
        break;
    case 9:
        code = plcp_rate_code_4p5mbps;
        break;
    case 12:
        code = plcp_rate_code_6mbps;
        break;
    case 18:
        code = plcp_rate_code_9mbps;
        break;
    case 24:
        code = plcp_rate_code_12mbps;
        break;
    case 36:
        code = plcp_rate_code_18mbps;
        break;
    case 48:
        code = plcp_rate_code_24mbps;
        break;
    case 54:
        code = plcp_rate_code_27mbps;
        break;
    default:
        code = -1;
        break;
    }
    return code;
}

/**
 * @brief 하드웨어 제어 헤더를 생성한다.
 * @param hw 패킷을 전송할 하드웨어
 * @param skb MPDU(PSDU)가 포함된 소켓버퍼
 * @param datarate 송신데이터레이트
 * @param txpower 송신출력
 * @return 성공: 0, 실패: -1
 * */
int construct_hw_control_header(struct sk_buff *skb, u8 datarate, s32 txpower, u32 ether_len)
{
    struct platform_device *pdev = g_pdev;

    /* FPGA 테스트 */
    BYTE llc_snap[6] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};

    u32 i;

    /* FPGA 테스트 */
    u32 psdu_len = sizeof(dot11QoSDataHdr_t) + sizeof(llc_snap) + sizeof(llcHdr_t) + ether_len - ETH_HLEN + WAVE_FCS_LEN;
    plcp_rate_code plcp_rate;
    struct condor_hw_tx_ctrl *ctrl;
    struct condor_hw_tx_power_map *entry = NULL;

    // dev_info(&pdev->dev, "Under construction of HW Control Header for %d-bytes PSDU\n", psdu_len);

    ctrl = (struct condor_hw_tx_ctrl *)skb_push(skb, sizeof(*ctrl));
    if (!ctrl)
    {
        // dev_err(&pdev->dev, "Fail to skb_push() for hw_ctrl\n");
        dev_kfree_skb(skb);
        return -1;
    }

    // 초기화
    memset(ctrl, 0, sizeof(*ctrl));

    for (i = 0; i < MAX2829_SUPPORT_TXPOWER_MAXNUM; i++)
    {
        if (g_tx_power_map[i].txpower == txpower)
        {
            entry = &g_tx_power_map[i];
            break;
        }
    }
    if (!entry)
    {
        // dev_err(&pdev->dev, "Fail to find txgain map for txpower: %d\n", txpower);
        dev_kfree_skb(skb);
        return -1;
    }
    // dev_info(&pdev->dev, "Success to find txgain(0x%02X) map for txpower: %d\n", entry->txgain, entry->txpower);
    ctrl->len_power = __cpu_to_le16((entry->txgain << 8) | WAVE_HDR_LEN);
    ctrl->tx_ctrl = HW_TX_CTRL_TX_POWER_SET;

    /*----------------------------------------------------------------------------------*/
    /* PHY PLCP 헤더의 일부 필드를 채운다(나머지는 하드웨어가 채운다) */
    /*----------------------------------------------------------------------------------*/
    /* PSDU의 길이 및 데이터레이트 설정
   * 	- 모뎀은 리틀엔디안으로 입력을 받고, MPC5125는 빅엔디안이므로 변환해서 설정한다.
   * 	- CPU가 빅엔디안이므로 hton*() 함수는 무의미하다. */
    plcp_rate = condor_get_plcp_rate_code(datarate);
    ctrl->plcp_hdr_low32 = __cpu_to_le32((psdu_len << 5) | plcp_rate);

    // dev_info(&pdev->dev, "Success to construct HW control header for PSDU");
    return 0;
}

/**
 * @brief 이더넷 패킷을 받아 802.11 패킷 형태로 변형한다.
 * @param skb (입력) 변형할 패킷이 저장된 소켓 버퍼
 * @return 802.11 MPDU가 저장된 소켓버퍼
 */
struct sk_buff *convert_eth_to_80211(struct sk_buff *skb, txMetaData_t *tx_meta, int32_t *result)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    dataUnitLenRange pkt_len;
    struct ethhdr *ehdr = (struct ethhdr *)skb->data;
    struct sk_buff *new_skb;
    dot11QoSDataHdr_t *dot11Hdr;
    llcHdr_t *llcHdr;
    
    /* sk_buff 포인터 */
    BYTE *ptr;
    BYTE broadcastMacAddress[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    BYTE wildcardBssid[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    /* FPGA 테스트 */
    BYTE llc_snap[6] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};

    // dev_info(&pdev->dev, "Converting ethernet packet to 802.11 packet\n");

    /* 송신 메타데이터 설정 */
    tx_meta->netIfIndex = DEFAULT_IFIDX;
    memcpy(tx_meta->src, ehdr->h_source, WAVE_MAC_ALEN);
    memcpy(tx_meta->dst, ehdr->h_dest, WAVE_MAC_ALEN);
    tx_meta->priority = DEFAULT_IPACI;
    tx_meta->timeSlot = DEFAULT_TIMESLOT;
    tx_meta->dataRate = pdata->datarate;
    tx_meta->txPower = pdata->tx_power;
    tx_meta->expiry = 0;
    if (!memcmp(tx_meta->dst, broadcastMacAddress, WAVE_MAC_ALEN))
        tx_meta->serviceClass = dot11ServiceClass_QoSNoAck;
    else
        tx_meta->serviceClass = dot11ServiceClass_QoSAck;

    /* MPDU 구성 */
    /* 802.11 패킷을 저장히기 위한 소켓 버퍼를 할당*/
    /* FPGA 테스트 */
    pkt_len = HW_SPECIFIC_HDR_LEN + sizeof(dot11QoSDataHdr_t) + sizeof(llc_snap) + sizeof(llcHdr_t) + skb->len - ETH_HLEN + WAVE_FCS_LEN;

    new_skb = alloc_skb(pkt_len, GFP_KERNEL);
    if (!new_skb)
    {
        // err
        // dev_err(&pdev->dev, "Fail to allocate memory for new socket buffer\n");
        *result = -1;
        return NULL;
    }

    /* [HW_CONTROL_HEADER][QoS][LLC][MSDU][WAVE_FCS] */
    memset(new_skb->data, 0, pkt_len);
    skb_reserve(new_skb, HW_SPECIFIC_HDR_LEN);    

    /**
     * 하드웨어 헤더를 구성한다.
     * IP 패킷 전송 시, txProfile의 datarate, txpower 필요로 한다.
     * */
    *result = construct_hw_control_header(new_skb, tx_meta->dataRate, tx_meta->txPower, skb->len);
    if (*result < 0)
        goto release;

    /**
     * 802.11 MAC헤더 설정
     * */
    ptr = (BYTE *)skb_put(new_skb, sizeof(dot11QoSDataHdr_t));
    dot11Hdr = (dot11QoSDataHdr_t *)ptr;
    memset(dot11Hdr, 0, sizeof(*dot11Hdr));

    /* Frame control */
    dot11Hdr->fc = DOT11_SET_FC_FTYPE(dot11MacHdrFcType_data) | DOT11_SET_FC_FSTYPE(dot11MacHdrFcSubType_qosData);

#if 1 // DroneSoC
#else
#if defined(_BIG_ENDIAN_)
    dot11Hdr->fc = __cpu_to_le16(dot11Hdr->fc);
#elif defined(_LITTLE_ENDIAN_)
#else
#error "ENDIAN is not defined"
#endif
#endif

    if (pdata->dev_type == dev_type_normal) 
    {
        dot11Hdr->fc |= (DOT11_SET_FC_TODS(0) | DOT11_SET_FC_FROMDS(0));

        /* ADDR1(RA) = DA, ADDR2(TA) = SA, ADDR3(BSSID) = wildcard BSSID */
        memcpy(dot11Hdr->addr1, ehdr->h_dest, ETH_ALEN);
        memcpy(dot11Hdr->addr2, ehdr->h_source, ETH_ALEN);
        memcpy(dot11Hdr->addr3, wildcardBssid, ETH_ALEN);
    }
    else if (pdata->dev_type == dev_type_rse) 
    {
        dot11Hdr->fc |= (DOT11_SET_FC_TODS(0) | DOT11_SET_FC_FROMDS(1));

        /* ADDR1(RA) == DA, ADDR2(TA) = BSSID, ADDR3(SA) = SA */
        memcpy(dot11Hdr->addr1, ehdr->h_dest, ETH_ALEN);
        memcpy(dot11Hdr->addr2, pdata->hw.bssid, ETH_ALEN);
        memcpy(dot11Hdr->addr3, ehdr->h_source, ETH_ALEN);
    }
    else if (pdata->dev_type == dev_type_obe) 
    {
        dot11Hdr->fc |= (DOT11_SET_FC_TODS(1) | DOT11_SET_FC_FROMDS(0));

        /* ADDR1(RA) = BSSID(RSE MAC), ADDR2(TA) = SA, ADDR3(DA) = DA */
        memcpy(dot11Hdr->addr1, pdata->hw.bssid, ETH_ALEN);
        memcpy(dot11Hdr->addr2, ehdr->h_source, ETH_ALEN);
        memcpy(dot11Hdr->addr3, ehdr->h_dest, ETH_ALEN);
    }
    else if (pdata->dev_type == dev_type_extobe)
    {
        dot11Hdr->fc |= (DOT11_SET_FC_TODS(1) | DOT11_SET_FC_FROMDS(0));

        /* ADDR1(RA) = BSSID(RSE MAC), ADDR2(TA) = SA, ADDR3(DA) = DA */
        memcpy(dot11Hdr->addr1, pdata->hw.bssid, ETH_ALEN);
        memcpy(dot11Hdr->addr2, ehdr->h_source, ETH_ALEN);
        memcpy(dot11Hdr->addr3, ehdr->h_dest, ETH_ALEN);
    }
    
    /* ACK_POLICY는 destination addr이 broadcast인지 확인하고 처리한다. */
    dot11Hdr->qos = DOT11_SET_QOS_UP(DEFAULT_IPACI) | DOT11_SET_QOS_ACK_POLICY(tx_meta->serviceClass);

    /* FPGA 테스트 */
    ptr = (BYTE *)skb_put(new_skb, sizeof(llc_snap));
    memcpy(ptr, llc_snap, sizeof(llc_snap));

    /* LLC 헤더 설정 */
    ptr = (BYTE *)skb_put(new_skb, sizeof(llcHdr_t));
    // ptr += sizeof(dot11QoSDataHdr_t);
    llcHdr = (llcHdr_t *)ptr;
    memset(llcHdr, 0, sizeof(*llcHdr));
    llcHdr->type = ehdr->h_proto;

    /* MSDU 설정 */
    ptr = (BYTE *)skb_put(new_skb, skb->len - ETH_HLEN);
    // ptr += sizeof(llcHdr_t);
    memcpy(ptr, skb->data + ETH_HLEN, skb->len - ETH_HLEN);

    /* 하드웨어에서 FCS 계산기능을 제공하지 않으면, 여기서 계산해서 넣어야 한다. */
    ptr = (BYTE *)skb_put(new_skb, WAVE_FCS_LEN);
    /* ------ */

    // dev_info(&pdev->dev, "HW: %d | QoS: %d | LLC: %d | MSDU: %d | FCS: %d", HW_SPECIFIC_HDR_LEN, sizeof(dot11QoSDataHdr_t), sizeof(llcHdr_t), skb->len - ETH_HLEN, WAVE_FCS_LEN);
    
    // dev_info(&pdev->dev, "Success to convert ethernet packet to 802.11 packet\n");
    return new_skb;

release:
    dev_kfree_skb(new_skb);
    return NULL;
}

#include "condor_dev.h"
#include "condor_hw_regs.h"

rcpiRange ConvertPowerToRcpi(powerRange power);
static void hw_memcpy(char *dst, char *src, u32 len);
dataRateRange condor_get_datarate(dot11PlcpRateCode code);
static BYTE *MacAddressStr(BYTE MacAddress[], BYTE MacAddressStr[], int strSize);
static int32_t condor_update_rx_mmpdu_meta_data(dot11MgmtHdr_t *hdr, uint16_t fc, rxMetaData_t *rx_meta);
static int32_t condor_update_rx_data_mpdu_meta_data(dot11MacHdr_t *hdr, uint16_t fc, uint16_t fc_fstype, rxMetaData_t *rx_meta);
static int32_t check_wave_rx_data_ocb_communication_validity(dot11MacHdr_t *hdr, uint16_t fc, uint16_t fc_fstype, rxMetaData_t *rx_meta);
static int32_t condor_process_rx_data_mpdu(struct sk_buff *skb, dot11MacHdr_t *hdr, uint16_t fc, rxMetaData_t *rx_meta, bool *is_skb_free);
static int32_t condor_process_rx_mmpdu(struct sk_buff *skb, dot11MgmtHdr_t *hdr, uint16_t fc, rxMetaData_t *rx_meta);
static int32_t MA_UNITDATA_indication(struct sk_buff *skb, dataUnitLenRange mac_hdr_len, rxMetaData_t *rx_meta, bool *is_skb_free);

/**
 * @brief 수신된 MPDU를 처리한다.
 * @param MPDU가 저장된 소켓버퍼
 * @param rx_meta 수신 메타데이터
 * @param is_skb_free 소켓버퍼를 해제해야 하는지 여부가 저장되어 반환된다.
 * @return 성공시 0, 실패시 음수
 * */
int32_t condor_process_rx_mpdu(struct sk_buff *skb, rxMetaData_t *rx_meta, bool *is_skb_free)
{
    struct platform_device *pdev = g_pdev;

    int32_t result;
    dot11MgmtHdr_t *hdr;
    dot11QoSDataHdr_t *qos_hdr;
    uint16_t fc, fc_ftype;

    // dev_info(&pdev->dev, "Processing received MPDU\n");
    // print_packet_dump(skb);

    /* 데이터 길이 확인 - 최소 MAC 헤더 길이보다 작으면 실패 */
    if (skb->len < sizeof(dot11MgmtHdr_t))
    {
        // dev_err(&pdev->dev, "Too short frame - %d\n", skb->len);
        return -1;
    }

    /* 패킷 헤더 파싱 */
    hdr = (dot11MgmtHdr_t *)(skb->data);
    fc = hdr->fc; /* Frame Control 영역은 모든 유형의 헤더에서 동일하다. */

#if 1 // DronSoc
#else
#if defined(_BIG_ENDIAN_)
    fc = __cpu_to_le16(fc);
#elif defined(_LITTLE_ENDIAN_)
#else
#error "Endian is not defined"
#endif
#endif
    fc_ftype = DOT11_GET_FC_FTYPE(fc);

    /* 프로토콜 버전 확인 */
    if (DOT11_GET_FC_PVER(fc) != DOT11_PROTOCOL_VERSION)
    {
        // dev_err(&pdev->dev, "Invalid protocol version: %d\n", DOT11_GET_FC_PVER(fc));
        return -1;
    }

    /* Frame type 별로 파싱해서 처리 */
    /* 데이터 패킷 처리 */
    if (fc_ftype == dot11MacHdrFcType_data)
    {
        // dev_info(&pdev->dev, "Data mpdu is received from hw - len: %d, chan: %d, rxpower: %d, rcpi: %d, datarate: %d\n", skb->len, rx_meta->ChannelNumber, rx_meta->rxPower, rx_meta->rcpi, rx_meta->rxDataRate);
        if (skb->len < sizeof(dot11QoSDataHdr_t))
        {
            // dev_info(&pdev->dev, "Too short data packet(%d). Must be QoS data packet\n", skb->len);
            return -1;
        }
        qos_hdr = (dot11QoSDataHdr_t *)(skb->data);
        result = condor_process_rx_data_mpdu(skb, (dot11MacHdr_t *)qos_hdr, fc, rx_meta, is_skb_free);
    }
    else if (fc_ftype == dot11MacHdrFcType_mgmt)
    {
        // dev_info(&pdev->dev, "Management mmpdu is received from hw - len: %d, chan: %d, rxpower: %d, rcpi: %d, datarate: %d\n", skb->len, rx_meta->ChannelNumber, rx_meta->rxPower, rx_meta->rcpi, rx_meta->rxDataRate);
        result = condor_process_rx_mmpdu(skb, hdr, fc, rx_meta);
    }
    /* 지원되지 않은 패킷 */
    else
    {
        // dev_err(&pdev->dev, "Not supported type(%d) frame is received from hw - len: %d, chan: %d, rxpower: %d, rcpi: %d, datarate: %d\n", fc_ftype, skb->len, rx_meta->ChannelNumber, rx_meta->rxPower, rx_meta->rcpi, rx_meta->rxDataRate);
        result = -1;
    }

    return result;
}

/**
 * @brief 수신된 관리 MPDU를 처리한다.
 * @param skb 데이터 MPDU가 저장된 소켓버퍼
 * @param hdr MAC 헤더
 * @param fc Frame Control 필드값
 * @param rx_meta 수신 메타데이터
 * @param is_skb_free 소켓버퍼를 해지해야 하는지 여부가 저장되어 반환된다.
 * @return 성공시 0, 실패시 음수
 * */
static int32_t condor_process_rx_data_mpdu(struct sk_buff *skb, dot11MacHdr_t *hdr, uint16_t fc, rxMetaData_t *rx_meta, bool *is_skb_free)
{
    struct platform_device *pdev = g_pdev;

    int32_t result;
    dataUnitLenRange mac_hdr_len, msdu_len;
    uint16_t fc_fstype;

    // dev_info(&pdev->dev, "Processing received data MPDU\n");

    /* Frame subtype 파싱 */
    fc_fstype = DOT11_GET_FC_FSTYPE(fc);

    /* Frame Subtype에 따라 MSDU 추출 */
    /* QoS */
    if (fc_fstype == dot11MacHdrFcSubType_qosData)
    {
        mac_hdr_len = sizeof(dot11QoSDataHdr_t);
        msdu_len = skb->len - mac_hdr_len - WAVE_FCS_LEN;
    }
    // /* Non-QoS 데이터  */
    // else if(fstype == dot11MacHdrFcSubType_data) {
    //     offset	=	sizeof(dot11DataHdr_t);
    //     msduLen	=	skb->len - sizeof(dot11DataHdr_t) - sizeof(uint32_t)/*FCS*/;
    // }
    /* 유효하지 않은 subtype */
    else
    {
        // dev_err(&pdev->dev, "Not supported fstype(%d)\n", fc_fstype);
        return -1;
    }
    /**
     * MSDU 길이 유효성 확인
     *  - MAC헤더 및 CRC를 제거했는데 0보다 작으면 실패
     *  - 너무 큰 길이에 대해서는 ReceiveHwPacket() 함수에서 유효성체크를 이미 했다.
     * */
    if (msdu_len <= 0)
    {
        // dev_err(&pdev->dev, "Too short MSDU length(%d)\n", msdu_len);
        return -1;
    }

    /**
     * 수신 메타데이터 업데이트
     *  - 송/수신 MAC주소
     *  - QoS패킷이 경우, 우선순위 및 서비스 클래스
     * */
    result = condor_update_rx_data_mpdu_meta_data(hdr, fc, fc_fstype, rx_meta);
    if (result < 0)
        return result;

    /* OCB통신에 관련된 패킷 헤더 필드 검증 */
    // if (ocb 사용하는가?)
    // result = check_wave_rx_data_ocb_communication_validity(hdr, fc, fc_fstype, rx_meta);
    // if (result < 0)
    //     return result;

    return MA_UNITDATA_indication(skb, mac_hdr_len, rx_meta, is_skb_free);
}

/**
 * @brief 수신된 관리 MPDU를 처리한다.
 * @param skb 관리 mpdu가 저장된 소켓버퍼
 * @param hdr MAC 헤더
 * @param fc Frame Control 필드값
 * @param rx_meta 수신 메타데이터
 * @return 성공시 0, 실패시 음수
 * */
static int32_t condor_process_rx_mmpdu(struct sk_buff *skb, dot11MgmtHdr_t *hdr, uint16_t fc, rxMetaData_t *rx_meta)
{
    struct platform_device *pdev = g_pdev;

    int32_t result;
    dataUnitLenRange mmpdu_len;
    BYTE *mmpdu;
    uint16_t fc_fstype;

    // dev_info(&pdev->dev, "Processing received management MPDU\n");

    /* 패킷 헤더 파싱 및 MGMT MPDU 추출 */
    fc_fstype = DOT11_GET_FC_FSTYPE(fc);
    mmpdu = skb->data + sizeof(dot11MgmtHdr_t);
    mmpdu_len = skb->len - sizeof(dot11MgmtHdr_t) - WAVE_FCS_LEN /* FCS */;

    /**
     * Frame Subtype에 따라 MGMT MPDU 처리
     *  - 현재 WAVE에서 지원되는 Management 패킷은 TA 패킷이 유일하다.
     * */
    /* TA 패킷 처리 */
    if (fc_fstype == dot11MacHdrFcSubType_ta)
    {
        /* 수신 메타데이터 업데이트 - 송/수신 MAC주소 */
        result = condor_update_rx_mmpdu_meta_data(hdr, fc, rx_meta);
        if (result < 0)
            return result;

        /* dot4 계층으로 전달 */
        // return MLME_TIMING_ADVERTISEMENT_indication(mmpdu, mmpdulen);
    }
    /* 유효하지 않은 subtype */
    else
    {
        // dev_err(&pdev->dev, "Not supported fstype(%d)", fc_fstype);
        return -1;
    }
    return 0;
}

/**
 * @brief 수신된 관리 MPDU에 관련된 메타데이터를 업데이트한다.
 * 손신자/목적지 MAC 주소
 * @param hdr 패킷 헤더
 * @param fc Frame Control 필드값
 * @param meta 수신 메타데이터
 * @return 성공시 0, 실패시 -1
 * */
static int32_t condor_update_rx_mmpdu_meta_data(dot11MgmtHdr_t *hdr, uint16_t fc, rxMetaData_t *rx_meta)
{
    struct platform_device *pdev = g_pdev;

    uint16_t to_ds, from_ds, sc;
    BYTE mac_addr_str[3][17 + 1];

    /**
     * 송신지/목적지 MAC 주소를 메타데이터에 저장
     *  - Management 패킷의 to_ds 및 from_ds는 항상 0이어야 한다.
     * */
    to_ds = DOT11_GET_FC_TODS(fc);
    from_ds = DOT11_GET_FC_FROMDS(fc);
    if ((to_ds == 0) && (from_ds == 0))
    {
        memcpy(rx_meta->srcAddress, hdr->addr2, WAVE_MAC_ALEN);
        memcpy(rx_meta->dstAddress, hdr->addr1, WAVE_MAC_ALEN);
    }
    else
    {
        // dev_err(&pdev->dev, "Not supported to_ds(%d) and from_ds(%d) combination\n", to_ds, from_ds);
        return -1;
    }

    /* 순서번호 및 분할번호 - 큰 의미는 없다(정규 SW에서는 딱히 활용되지 않는다.) */
    sc = hdr->seq;
#if 1 // DroneSoC
#else
#if defined(_BIG_ENDIAN_)
    sc = __cpu_to_le16(sc);
#elif defined(_LITTLE_ENDIAN_)
#else
#error "Endian is not defined"
#endif
#endif
    rx_meta->seqNumber = DOT11_GET_SEQ_SEQ(sc);
    rx_meta->fragNumber = DOT11_GET_SEQ_FRAG(sc);

    // printk("Rx Management MPDU meta data\n");
    // printk("  V: %d, T: %d, ST: %d, ToDS: %d, FromDS: %d, MF: %d, "
    //        "R: %d, PM: %d, MD: %d, P: %d, O: %d\n",
    //        DOT11_GET_FC_PVER(fc), DOT11_GET_FC_FTYPE(fc), DOT11_GET_FC_FSTYPE(fc),
    //        to_ds, from_ds, DOT11_GET_FC_MOREFRAG(fc), DOT11_GET_FC_RETRY(fc),
    //        DOT11_GET_FC_PWRMGT(fc), DOT11_GET_FC_MOREDATA(fc),
    //        DOT11_GET_FC_ISWEP(fc), DOT11_GET_FC_ORDER(fc));
    // printk("  SN: %d, FN: %d\n", rx_meta->seqNumber, rx_meta->fragNumber);
    // printk("  DstAddr: %s, SrcAddr: %s, ADDR3: %s\n",
    //        MacAddressStr(rx_meta->dstAddress, mac_addr_str[0], 17),
    //        MacAddressStr(rx_meta->srcAddress, mac_addr_str[1], 17),
    //        MacAddressStr(hdr->addr3, mac_addr_str[2], 17));

    return 0;
}

/**
 * @brief 수신된 데이터 MPDU에 관련된 메타데이터를 업데이트한다.
 *  - 송신자/목적지 MAC주소
 *  - QoS 패킷인 경우, 우선순위 및 서비스 클래스
 * @param hdr 패킷 헤더
 * @param fc Frame Control 필드값
 * @param fc_fstype Frame subtype (QoS or non-QoS)
 * @param rx_meta 수신 메타데이터
 * @return 성공시 0, 실패시 음수
 * */
static int32_t condor_update_rx_data_mpdu_meta_data(dot11MacHdr_t *hdr, uint16_t fc, uint16_t fc_fstype, rxMetaData_t *rx_meta)
{
    struct platform_device *pdev = g_pdev;

    uint16_t to_ds, from_ds, sc, qc = 0;
    BYTE mac_addr_str[3][18];

    /**
     * 송신지/목적지 MAC주소를 메타데이터에 저장
     *  - to_ds 및 from_ds 갑셍 따라 ADDR1, 2, 3의 의미가 달라지므로, 해당 조합에 맞춰
     * 송신지/목적지 MAC주소를 구한다.
     * */
    to_ds = DOT11_GET_FC_TODS(fc);
    from_ds = DOT11_GET_FC_FROMDS(fc);
    if ((to_ds == 0) && (from_ds == 0))
    {
        memcpy(rx_meta->srcAddress, hdr->u.dataHdr.addr2, WAVE_MAC_ALEN);
        memcpy(rx_meta->dstAddress, hdr->u.dataHdr.addr1, WAVE_MAC_ALEN);
    }
    else if ((to_ds == 0) && (from_ds == 1))
    {
        memcpy(rx_meta->srcAddress, hdr->u.dataHdr.addr3, WAVE_MAC_ALEN);
        memcpy(rx_meta->dstAddress, hdr->u.dataHdr.addr1, WAVE_MAC_ALEN);
    }
    else if ((to_ds == 1) && (from_ds == 0))
    {
        memcpy(rx_meta->srcAddress, hdr->u.dataHdr.addr2, WAVE_MAC_ALEN);
        memcpy(rx_meta->dstAddress, hdr->u.dataHdr.addr3, WAVE_MAC_ALEN);
    }
    else
    {
        // dev_err(&pdev->dev, "Not supported to_ds(%d) and from_ds(%d) combination\n", to_ds, from_ds);
        return -1;
    }

    /* 순서번호 및 분할번호 - 큰 의미는 없다.(정규 어플리케이션에서는 딱히 활용되지 않는다.) */
    sc = hdr->u.dataHdr.seq;
#if 1 // DroneSoC
#else
#if defined(_BIG_ENDIAN_)
#ifdef _KERNEL_SPACE_
    sc = __cpu_to_le16(sc);
#endif
#elif defined(_LITTLE_ENDIAN_)
    //cpu_to_be16s(&sc);
#else
#error "Endian is not defined"
#endif
#endif
    rx_meta->seqNumber = DOT11_GET_SEQ_SEQ(sc);
    rx_meta->fragNumber = DOT11_GET_SEQ_FRAG(sc);

    /* QoS 데이터인 경우, 우선순위 및 서비스 클래스를 메타데이터에 저장한다. */
    if (fc_fstype == dot11MacHdrFcSubType_qosData)
    {
        qc = hdr->u.qosDataHdr.qos;
#if 1 // DroneSoC
#else
#if defined(_BIG_ENDIAN_)
#ifdef _KERNEL_SPACE_
        qc = __cpu_to_le16(qc);
#endif
#elif defined(_LITTLE_ENDIAN_)
        //cpu_to_be16s(&qc);
#else
#error "Endian is not defined"
#endif
#endif
        rx_meta->priority = DOT11_GET_QOS_TID(qc);
        rx_meta->serviceClass = DOT11_GET_QOS_ACK_POLICY(qc);
    }
    else
    {
        rx_meta->priority = -1;
        rx_meta->serviceClass = -1;
    }

    // printk("RxDataMpdu meta data\n");
    // printk("  V: %d, T: %d, ST: %d, ToDS: %d, FromDS: %d, MF: %d\n",
    //        DOT11_GET_FC_PVER(fc), DOT11_GET_FC_FTYPE(fc), DOT11_GET_FC_FSTYPE(fc),
    //        to_ds, from_ds, DOT11_GET_FC_MOREFRAG(fc));
    // printk("  R: %d, PM: %d, MD: %d, P: %d, O: %d\n",
    //        DOT11_GET_FC_RETRY(fc), DOT11_GET_FC_PWRMGT(fc), DOT11_GET_FC_MOREDATA(fc),
    //        DOT11_GET_FC_ISWEP(fc), DOT11_GET_FC_ORDER(fc));
    // printk("  SN: %d, FN: %d\n", rx_meta->seqNumber, rx_meta->fragNumber);
    // printk("  DstAddr: %s, SrcAddr: %s, ADDR3: %s\n",
    //        MacAddressStr(rx_meta->dstAddress, mac_addr_str[0], 17 + 1),
    //        MacAddressStr(rx_meta->srcAddress, mac_addr_str[1], 17 + 1),
    //        MacAddressStr(hdr->u.dataHdr.addr3, mac_addr_str[2], 17 + 1));
    // if (fc_fstype == dot11MacHdrFcSubType_qosData)
    //     printk("  TID: %d, EOSP: %d, AP: %d, AMSDU: %d, TxOpDurReq: %d\n",
    //            rx_meta->priority, DOT11_GET_QOS_EOSP(qc), rx_meta->serviceClass,
    //            DOT11_GET_QOS_AMSDU_PRESENT(qc), DOT11_GET_QOS_TXOP_DUR_REQ(qc));

    return 0;
}

static int32_t check_wave_rx_data_ocb_communication_validity(dot11MacHdr_t *hdr, uint16_t fc, uint16_t fc_fstype, rxMetaData_t *rx_meta)
{
    struct platform_device *pdev = g_pdev;

    // dev_info(&pdev->dev, "Checking WAVE data OCB communication validity\n");

    /* QoS 패킷이 아니면 실패 */
    if (fc_fstype != dot11MacHdrFcSubType_qosData)
    {
        // dev_err(&pdev->dev, "Not QoS data - fc_fstype: %d\n", fc_fstype);
        return -1;
    }

    /* ADDR3가 wildcard BSSID가 아니면 실패 */
    if (memcmp(hdr->u.qosDataHdr.addr3, wildcardBssid, WAVE_MAC_ALEN))
    {
        // dev_err(&pdev->dev, "addr3 is not wildcard bssid - %02x:%02x:%02x:%02x:%02x:%02x\n", hdr->u.qosDataHdr.addr3[0], hdr->u.qosDataHdr.addr3[1], hdr->u.qosDataHdr.addr3[2], hdr->u.qosDataHdr.addr3[3], hdr->u.qosDataHdr.addr3[4], hdr->u.qosDataHdr.addr3[5]);
        return -1;
    }

    /* ToDS/FromDS/MoreFrag/Retry/PwrMgmt/MoreData/Wep/Order가 모두 0이 아니면 실패 */
    if (((fc >> 8) & 0xff) != 0)
    {
        // dev_err(&pdev->dev, "header flag(s) are invalid - 0x%02x\n", (uint8_t)((fc >> 8) & 0xff));
        return -1;
    }

    /* Priority 범위 확인 */
    if ((rx_meta->priority < 0) || (rx_meta->priority > userPriority_max))
    {
        // dev_err(&pdev->dev, "Invalid priority %d\n", rx_meta->priority);
        return -1;
    }

    // dev_info(&pdev->dev, "Success to check WAVE data OCB communication validity\n");
    return 0;
}

/**
 * @brief HW 버퍼에 수신된 패킷을 소켓버퍼에 복사한다.
 * @param hw PHY하드웨어
 * @param q_index 수신큐 번호
 * @param rx_meta 수신 메타데이터가 저장된 변수
 * @param result 결과가 저장된다.
 * @reutrn 수신 패킷이 저장된 소켓버퍼 (실패시 NULL)
 * */
struct sk_buff *condor_receive_hw_packet(struct condor_hw *hw, int32_t q_index, rxMetaData_t *rx_meta, int32_t *result)
{
    struct platform_device *pdev = g_pdev;

    uint32_t rx_ctrl_data;
    uint8_t *cfg_iobase = hw->radio[0].cfg_iobase;
    uint8_t *rxbuf_iobase = hw->radio[0].rxbuf_iobase;
    bool local_time_exist = false;
    uint32_t fc_dur;
    BYTE *ptr;
    dot11PlcpRateCode rate_code;
    dataUnitLenRange rx_len, mpdu_len;

    struct sk_buff *skb = NULL;

    // dev_info(&pdev->dev, "Trying to receive hw packet - hw rxq[%d]\n", q_index);

    /**
     * 하드웨어로부터 부가정보를 읽은다.
     * 수신버퍼의 첫 4바이트에는 실제 수신된 패킷이 아닌 부가정보가 들어있다.
     * localtime의 존재유무, 수신패킷의 데이터레이트, 수신패킷의 길이, 수신파워 정보가 들어있다.
     * */
    /* 수신부가정보를 읽는다. */
    rx_ctrl_data = readl(rxbuf_iobase + RXBUF(q_index, 0));

    /* 수신데이터에 localtime 필드가 있는지 확인 -> localtime 필드는 TA 패킷 수신 시에 존재한다. */
    if (rx_ctrl_data & 1)
        local_time_exist = true;

    /* 수신 패킷의 데이터레이트 코드 */
    rate_code = (rx_ctrl_data >> 8) & 0xf;

    /**
     * 수신 데이터의 길이 확인
     * = 부가정보(4) + MAC헤더(24 or 26) + MSDU(n) + CRC(4) + localtime(8: optional)
     * */
    rx_len = (rx_ctrl_data >> 12) & 0xfff;

    /* MPDU 길이 계산 */
    if (local_time_exist)
        mpdu_len = rx_len - sizeof(uint32_t) /* 부가정보 */ - sizeof(uint64_t) /* localtime */;
    else
        mpdu_len = rx_len - sizeof(uint32_t) /* 부가정보 */;
    if (mpdu_len > mpduLen_max)
    {
        // dev_err(&pdev->dev, "Too long frame length: %d > %d\n", mpdu_len, mpduLen_max);
        *result = -1;
        goto release;
    }

    /* 수신파워 확인 및 보정 -> RCPI 계산 */
    rx_meta->rxPower = ((rx_ctrl_data >> 24) & 0xff) * -1;
    rx_meta->rxPower += 15; /* 수정필요 */
    rx_meta->rcpi = ConvertPowerToRcpi(rx_meta->rxPower);

    /* 소켓 버퍼 할당 및 패킷 복사 */
    skb = alloc_skb(mpdu_len + 4 /* skb_reserve()를 위한 공간 확보 */, GFP_KERNEL);
    if (!skb)
    {
        // dev_err(&pdev->dev, "No memory for socket buffer\n");
        *result = -1;
        goto release;
    }

    /* MAC헤더가 26바이트이므로 IP패킷인 경우 커널로 올려줄 때 IP 헤더 정렬을 위해 앞에 공간을 추가한다. */
    skb_reserve(skb, NET_IP_ALIGN);

    /* 패킷을 복사할 공간 확보 및 초기화 */
    ptr = skb_put(skb, mpdu_len);
    memset(ptr, 0, mpdu_len);

    /**
     * Frame Control 및 Duration 필드 복사
     * 모뎀은 리틀엔디안으로 출력하고, MPC5125는 빅엔디안이므로 받은 값을 변환한다.
     * CPU가 빅엔디안이므로 ntoh*() 함수는 무의미하다.
     * */
    fc_dur = __cpu_to_le32(readl(rxbuf_iobase + RXBUF(q_index, 1)));
    *(uint32_t *)ptr = fc_dur;
    ptr += sizeof(uint32_t);

    /* local_time 존재 유무에 따라 결정된 길이 만큼 MPDU를 소켓버퍼에 복사 */
    if (local_time_exist)
    {
        hw_memcpy(ptr + sizeof(uint32_t) /* FC + Dur */,
                  rxbuf_iobase + RXBUF(q_index, 2),
                  mpdu_len - sizeof(uint32_t) /* FC _ Dur*/ + sizeof(uint64_t) /* local_time */);
        rx_meta->localtime = *(uint64_t *)(ptr + mpdu_len);
        skb->tail -= sizeof(uint64_t); /* local_time */
        skb->len -= sizeof(uint64_t);  /* local_time */
    }
    else
    {
#define WORD_ALIGN 4
        uint32_t i;
        uint32_t len = skb->len;
        uint32_t remain = len % WORD_ALIGN;
        if (remain != 0)
        {
            len += (WORD_ALIGN - remain);
        }

        for (i = 2; i < len / 4; i++)
        {
            /* 리틀엔디안으로 변환하고, 4바이트 씩 복사한다. */
            *(uint32_t *)ptr = __cpu_to_le32(readl(rxbuf_iobase + RXBUF(q_index, i)));
            ptr += sizeof(uint32_t);
        }
        // hw_memcpy(ptr + sizeof(uint32_t) /* Fc + Dur */, rxbuf_iobase + RXBUF(q_index, 2), mpdu_len - sizeof(uint32_t) /* FC + Dur */);
        rx_meta->localtime = 0;
    }

    /* 메타데이터 저장 */
    rx_meta->phyIndex = 0;
    rx_meta->netIfIndex = 0;
    rx_meta->rxDataRate = condor_get_datarate(rate_code);

    *result = 0;

release:
    writel(RELEASE_RXBUF(q_index), cfg_iobase + REG_RX_BUF_RELEASE);
    return skb;
}

/**
 * @brief 수신된 802.11 패킷을 이더넷 패킷으로 변환한다.
 * @param skb 802.11 MPDU가 들어있으며, 변화하여 이더넷 MPDU가 된다.
 * @param rx_meta 수신 메타데이터
 * @return 성공시 0, 실패시 음수
 * */
int32_t convert_80211_to_eth(struct sk_buff *skb, rxMetaData_t *rx_meta)
{
    struct platform_device *pdev = g_pdev;

    struct ethhdr *e_hdr;
    
    /* FPGA 테스트 */
    BYTE llc_snap[6] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
    llcHdr_t *llc_hdr;
    uint16_t proto;

    // dev_info(&pdev->dev, "Converting 802.11 packet to ethernet packet\n");

    // print_packet_dump(skb);

    /* 패킷 최소 길이 확인 */
    /* FPGA 테스트 */
    if (skb->len < sizeof(dot11QoSDataHdr_t) + sizeof(llc_snap) + sizeof(llcHdr_t))
    {
        // dev_err(&pdev->dev, "Too short packet - %d-bytes\n", skb->len);
        return -1;
    }

    /* 이더넷 패킷 구성 */
    /* FPGA 테스트 */
    llc_hdr = (llcHdr_t *)(skb->data + sizeof(llc_snap) + sizeof(dot11QoSDataHdr_t));
    proto = llc_hdr->type;

    /* 802.11 MAC 및 LLC 헤더 제거 */
    skb_pull(skb, sizeof(dot11QoSDataHdr_t) + sizeof(llc_snap) + sizeof(llcHdr_t));

    /* 이더넷 헤더 추가 */
    e_hdr = (struct ethhdr *)skb_push(skb, ETH_HLEN);
    e_hdr->h_proto = proto;
    memcpy(e_hdr->h_dest, rx_meta->dstAddress, WAVE_MAC_ALEN);
    memcpy(e_hdr->h_source, rx_meta->srcAddress, WAVE_MAC_ALEN);
    skb_trim(skb, skb->len - MAC_FCS_LEN);

    // dev_info(&pdev->dev, "Success to convert\n");
    // print_packet_dump(skb);

    skb->protocol = eth_type_trans(skb, skb->dev);
    skb->mac_header = (BYTE *)e_hdr;
    
    return 0;
}

/**
 * @brief 수신된 데이터패킷을 LLC계층으로 전달한다.
 * @param skb 수신된 MPDU가 담긴 소켓버퍼
 * @param mac_hdr_len 수신된 MPDU의 MAC헤더 길이
 * @param rx_meta 수신 메타데이터
 * @param is_skb_free 소켓버퍼를 해지해야 하는지 여부가 저장된다.
 * IP 패킷인 경우에는 해지해서는 안된다.
 * @return 성공시 0, 실패시 음수
 * */
static int32_t MA_UNITDATA_indication(struct sk_buff *skb, dataUnitLenRange mac_hdr_len, rxMetaData_t *rx_meta, bool *is_skb_free)
{
    struct platform_device *pdev = g_pdev;
    /* FPGA 테스트 */
    BYTE llc_snap[6] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};

    int32_t result;
    llcHdr_t *llc_hdr;

    /* FPGA 테스트 */
    dataUnitLenRange lsdu_len = skb->len - mac_hdr_len - sizeof(llc_snap) - sizeof(llcHdr_t) - WAVE_FCS_LEN;
    BYTE lsdu;
    UNITDATA_indication_params_t ind_params;

    // dev_info(&pdev->dev, "Trying to process MA-UNITDATA.indication - lsdu_len: %d\n", lsdu_len);

    /* LSDU 추출 */
    /* LSDU 길이 유효성 확인 -> 최대값에 대해서는 이미 체크되었다. */
    if (lsdu_len <= 0)
    {
        // dev_err(&pdev->dev, "Too short LSDU length %d\n", lsdu_len);
        return -1;
    }
    /* FPGA 테스트 */
    lsdu = skb->data + mac_hdr_len + sizeof(llc_snap) + sizeof(llcHdr_t);

    /* llc 헤더 */
    /* FPGA 테스트 */
    llc_hdr = (llcHdr_t *)(skb->data + mac_hdr_len + sizeof(llc_snap));

    /* EtherType에 따라 패킷 분류 - WSMP/GeoNetworking or IPv6 */
    /* WSMP/GeoNetworking이면 DL-UNITDATA.indication을 호출하여 WNE로 전달한다. */
    if (llc_hdr->type == htons(LLCTYPE_WSMP))
    {
        // dev_info(&pdev->dev, "WSMP packet\n");

        /* 파라미터 설정 */
        memcpy(ind_params.source_address, rx_meta->srcAddress, WAVE_MAC_ALEN);
        memcpy(ind_params.destination_address, rx_meta->dstAddress, WAVE_MAC_ALEN);
        ind_params.MacHeaderSize = mac_hdr_len;
        ind_params.LlcHeaderSize = sizeof(llcHdr_t);
        ind_params.HeaderSize = ind_params.MacHeaderSize + ind_params.LlcHeaderSize;
        memcpy(ind_params.data, skb->data, skb->len);
        ind_params.data_len = skb->len;
        ind_params.reception_status = true;
        ind_params.priority = rx_meta->priority;
        ind_params.service_class = rx_meta->serviceClass;
        ind_params.channelNumber = rx_meta->ChannelNumber;
        ind_params.rcpi = rx_meta->rcpi;
        ind_params.rxPower = rx_meta->rxPower;
        ind_params.dataRate = rx_meta->rxDataRate;
        ind_params.netIfIndex = 0;
        ind_params.phyIndex = 0;
        ind_params.etherType = llc_hdr->type;

        /* 수정 필요 - WNE로 전달 */
        result = 0;
    }

    /* IP-based 패킷*/
    else
    {
        
        result = convert_80211_to_eth(skb, rx_meta);
        if (result < 0)
        {
            *is_skb_free = true;
            return result;
        }
        
        /* 커널로 전달한다. */
        netif_rx(skb);
        result = 0;
        *is_skb_free = false;
    }
    return result;
}

/**
 * @brief dBm단위의 파워 값(-128~127)을 RCPI값으로 변환한다.
 * RCPI = Int{(Power in dBm + 110) x 2} for 0dBm > Power > -110dBm
 * @param power dBm단위의 파워 값
 * @return RCPI 값
 * */
rcpiRange ConvertPowerToRcpi(powerRange power)
{
    rcpiRange rcpi;

    /* WPM에서는 1단위의 dBm만을 사용하므로, 반올림을 수행할 필요 없다
	power	=	RoundOff(power, 0); */

    rcpi = (power + 110) * 2;
    if (rcpi < rcpi_min)
        rcpi = rcpi_min;
    else if (rcpi > rcpi_max)
        rcpi = rcpi_max;

    return rcpi;
}

/**
 * @brief Condor 하드웨어로 블록 메모리를 복사한다.
 *          복사할 데이터의 길이가 4바이트 단위가 아니면, 4바이트 단위로 맞춰준다(패딩 개념)
 *          MPC5125와 KETI 모뎀HW간 메모리 인터페이스의 특성에 따른 기능으로,
 *          플랫폼 변경 시 삭제되거나 달라질 수 있다.
 * @param dst (입력) 복사할 목적지
 * @param src (입력) 복사할 근원지
 * @param len (입력) 복사할 크기
 */
static void hw_memcpy(char *dst, char *src, u32 len)
{
#define WORD_ALIGN 4
    u32 remain = len % WORD_ALIGN;
    if (remain != 0)
    {
        len += (WORD_ALIGN - remain);
    }

    /* 데이터를 하드웨어에 복사한다 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
    memcpy(dst, src, len);
#else
    memcpy_fromio(dst, src, len);
#endif
}

/**
 * @brief PLCP 헤더의 RATE 필드 코드 값을 500kbps 단위의 데이터레이트 값으로 변환한다.
 * @param PLCP 헤더의 RATE 필드 코드 값
 * @return 데이터레이트 값, 실패시 -1
 * */
dataRateRange condor_get_datarate(dot11PlcpRateCode code)
{
    dataRateRange dataRate;

    if (code == dot11PlcpRate_3mbps)
        dataRate = 6;
    else if (code == dot11PlcpRate_4p5mbps)
        dataRate = 9;
    else if (code == dot11PlcpRate_6mbps)
        dataRate = 12;
    else if (code == dot11PlcpRate_9mbps)
        dataRate = 18;
    else if (code == dot11PlcpRate_12mbps)
        dataRate = 24;
    else if (code == dot11PlcpRate_18mbps)
        dataRate = 36;
    else if (code == dot11PlcpRate_24mbps)
        dataRate = 48;
    else if (code == dot11PlcpRate_27mbps)
        dataRate = 54;
    else
        dataRate = -1;
    return dataRate;
}

static BYTE *MacAddressStr(BYTE MacAddress[], BYTE MacAddressStr[], int strSize)
{
    memset(MacAddressStr, 0, strSize);
    snprintf(MacAddressStr, strSize, "%02x:%02x:%02x:%02x:%02x:%02x",
             MacAddress[0], MacAddress[1], MacAddress[2],
             MacAddress[3], MacAddress[4], MacAddress[5]);
    return MacAddressStr;
}

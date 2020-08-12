//
// Created by gyun on 2019-03-23.
//

#include "condor.h"
#include "condor_dev.h"
#include "if.h"

static int condor_netdev_open(struct net_device *ndev);
static int condor_netdev_stop(struct net_device *ndev);
static int condor_netdev_tx_ip(struct sk_buff *skb, struct net_device *ndev);
static int condor_netdev_ioctl(
    struct net_device *ndev,
    struct ifreq *req,
    int code);

/**
 * @brief Condor 네트워크 디바이스 정보를 할당, 초기화하고 반환한다.
 *          struct net_device를 할당 및 서로 연결하고 콜백함수를 등록한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @return 생성된 Condor 네트워크 디바이스
 */
struct condor_netdev *condor_init_netdev(struct platform_device *pdev)
{
    struct condor_netdev *cndev;
    struct net_device *ndev;

    // dev_info(&pdev->dev, "Intializing net device - %s\n", DEV_NAME);

    // net_device 정보를 할당한다.
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
    ndev = alloc_netdev(sizeof(*cndev), DEV_NAME, ether_setup);
#else
    ndev = alloc_netdev(sizeof(*cndev), DEV_NAME, NET_NAME_UNKNOWN, ether_setup);
#endif
    if (!ndev)
    {
        // dev_err(&pdev->dev, "Fail to allocate net device\n");
        return NULL;
    }
    cndev = netdev_priv(ndev);

    // 콜백함수를 등록한다.
    cndev->ndev = ndev;
    cndev->ndev_ops.ndo_open = condor_netdev_open;
    cndev->ndev_ops.ndo_stop = condor_netdev_stop;
    cndev->ndev_ops.ndo_start_xmit = condor_netdev_tx_ip;
    cndev->ndev_ops.ndo_do_ioctl = condor_netdev_ioctl;
    ndev->netdev_ops = &cndev->ndev_ops;

    // dev_info(&pdev->dev, "Success to intialize net device\n");
    return cndev;
}

/**
 * @brief Condor 네트워크 디바이스를 커널에 등록한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param cndev (입력) Condor 네트워크 디바이스
 * @return 성공시 0, 실패시 -1
 */
int condor_register_netdev(
    struct platform_device *pdev,
    struct condor_netdev *cndev)
{
    // dev_info(&pdev->dev, "Registering net device\n");

    if (register_netdev(cndev->ndev))
    {
        // dev_err(&pdev->dev, "Fail to register_netdev()\n");
        return -1;
    }

    // dev_info(&pdev->dev, "Success to register net device\n");
    return 0;
}

/**
 * @brief 네트워크 디바이스에 MAC 주소를 설정한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param ndev (입력) Condor 네트워크 디바이스
 * @return 성공시 0, 실패시 -1
 */
int condor_set_netdev_mac_addr(
    struct platform_device *pdev,
    struct net_device *ndev,
    u8 addr[ETH_ALEN])
{
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    // dev_info(&pdev->dev, "Set net device mac addr - %02X:%02X:%02X:%02X:%02X:%02X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // MAC 주소 유효성 확인
    if ((addr[0] & 1) == 1)
    {
        // dev_err(&pdev->dev, "I/G bit is not individual - it's %d\n", (addr[0] & 1));
        return -1;
    }

    // MAC 주소 설정
    memcpy(ndev->dev_addr, addr, ETH_ALEN);
    condor_set_radio_hw_mac_addr(pdev, &pdata->hw, 0, addr);
#if (RADIO_NUM_ == 2)
    condor_set_radio_hw_mac_addr(pdev, &pdata->hw, 1, addr);
#endif

    // dev_info(&pdev->dev, "Success to set mac addr\n");
    return 0;
}

/**
 * @brief 커널로부터의 요청에 따라 Condor 네트워크 디바이스를 연다.
 *          ifconfig ** up 시 호출 된다.
 * @param ndev (입력) Condor 네트워크 디바이스
 * @return 성공시 0, 실패시 -1
 */
static int condor_netdev_open(struct net_device *ndev)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    // dev_info(&pdev->dev, "Open condor net device\n");

    // 상위계층 큐를 중지시킨다. (IP 패킷 전송 금지)
    netif_stop_queue(ndev);

    // 모뎀 하드웨어를 활성화 시킨다.
    condor_activate_hw(pdev, &pdata->hw);

    // dev_info(&pdev->dev, "Success to open condor net device\n");
    return 0;
}

/**
 * @brief 커널로부터의 요청에 따라 Condor 네트워크 디바이스를 닫는다.
 *          ifconfig ** down 시 호출 된다.
 * @param ndev (입력) Condor 네트워크 디바이스
 * @return 성공시 0, 실패시 -1
 */
static int condor_netdev_stop(struct net_device *ndev)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    // dev_info(&pdev->dev, "Stop condor net device\n");

    // 상위계층 큐를 중지시킨다. (IP 패킷 전송 금지)
    netif_stop_queue(ndev);
    // 모뎀 하드웨어를 비활성화 시킨다.
    condor_deactivate_hw(pdev, &pdata->hw);

    // 송신 큐를 비운다
    // TO DO

    // dev_info(&pdev->dev, "Success to stop condor net device\n");
    return 0;
}

/**
 * @brief 커널로부터의 요청에 따라 IP 패킷 전송 요청을 처리한다.
 * @param skb (입력) 전송할 패킷이 저장된 소켓 버퍼
 * @param ndev (입력) Condor 네트워크 디바이스
 * @return 성공시 0, 실패시 -1
 */
static int condor_netdev_tx_ip(struct sk_buff *skb, struct net_device *ndev)
{
    struct platform_device *pdev = g_pdev;
    /* Debug */
    // dev_info(&pdev->dev, "1. IP Packet Callback\n");

    
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    txMetaData_t tx_meta;
    int32_t result;
    struct sk_buff *new_skb = NULL;

    pdata->netstats.tx_try_cnt++;
    pdata->netstats.tx_ip_try_cnt++;

    /* 이더넷 패킷을 802.11 패킷으로 변환, new_skb에 MPDU가 저장된다. */
    // dev_info(&pdev->dev, "Trying to convert from Ethernet to 802.11p\n");
    new_skb = convert_eth_to_80211(skb, &tx_meta, &result);
    if (!new_skb)
    {
        pdata->netstats.tx_drop_cnt++;
        pdata->netstats.tx_ip_drop_cnt++;
        goto out;
    }
/* ---------------- */

/* 패킷 송신*/
#ifdef _WAE_SHARED_HW_TXBUF_
    result = condor_tx_ppdu_shared_buf(pdev, &pdata->hw, 0, tx_meta.timeSlot * 4, new_skb, &tx_meta);
#else
    result = condor_tx_ppdu(pdev, &pdata->hw, 0, tx_meta.timeSlot * 4, new_skb);
#endif
    if (result == 1)
    {        
        /* Debug */
        // dev_info(&pdev->dev, "netif_stop_queue()");
        netif_stop_queue(pdata->cndev->ndev);
    }
    else if (result < 0)
    {
        pdata->netstats.tx_drop_cnt++;
        pdata->netstats.tx_ip_drop_cnt++;
        dev_kfree_skb(new_skb);
        goto out;
    }
    /* ---------------- */
    // dev_info(&pdev->dev, "Ethernet packet dump\n");
    // print_packet_dump(skb);

    // dev_info(&pdev->dev, "Transmit IP packet via condor net device\n");
    // print_packet_dump(new_skb);

out:
    if (skb)
        dev_kfree_skb(skb);
    return 0;
}

/**
 * @brief 어플리케이션으로부터의 ioctl 명령어를 처리한다.
 * @param ndev (입력) Condor 네트워크 디바이스
 * @param req (입력) 요청 정보
 * @param code (입력) ioctl code (나에게로의 명령인지 확인용)
 * @return 성공시 0, 실패시 -1
 */
static int condor_netdev_ioctl(
    struct net_device *ndev,
    struct ifreq *req,
    int code)
{
    u8 *req_data = NULL;
    int ret;
    bool updata = false;
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    struct ioctl_ifreq *ifreq = (struct ioctl_ifreq *)req;
    struct ioctl_ifreq_params *ifreq_params;

    // dev_info(&pdev->dev, "Control condor net device\n");

    /*
   * 코드 및 매직넘버 검사
   */
    if (code != CONDOR_IOCTL_CODE)
    {
        // dev_err(&pdev->dev, "Invalid ioctl code %d, must be %d\n", code, CONDOR_IOCTL_CODE);
        return -1;
    }
    if (ifreq->magic != CONDOR_IOCTL_MAGIC)
    {
        // dev_err(&pdev->dev, "Invalid ioctl magic %d, must be %d\n", ifreq->magic, CONDOR_IOCTL_MAGIC);
        return -1;
    }

    /*
   * 요청 데이터 파라미터를 사용자 공간으로부터 복사해 온다.
   */
    ifreq_params = (struct ioctl_ifreq_params *)
        kmalloc(sizeof(*ifreq_params), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (!ifreq_params)
    {
        // dev_err(&pdev->dev, "No memory\n");
        ifreq->result = -1;
        return -1;
    }
    ret = copy_from_user(ifreq_params, ifreq->params, sizeof(*ifreq_params));
    if (ret)
    {
        // dev_err(&pdev->dev, "Cannot copy ifreq_params from user\n");
        ifreq->result = -1;
        kfree(ifreq_params);
        return -1;
    }
    // dev_info(&pdev->dev, "Success to copy ifreq_params from user\n");

    /*
   * 요청 데이터가 있는 경우, 요청 데이터를 사용자 공간으로부터 복사해 온다.
   */
    if (ifreq_params->data_size)
    {
        // dev_info(&pdev->dev, "Copying %d bytes user data\n", ifreq_params->data_size);
        req_data = (u8 *)kmalloc(ifreq_params->data_size,
                                 in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
        if (!req_data)
        {
            // dev_err(&pdev->dev, "Cannot allocate memory for req_data\n");
            kfree(ifreq_params);
            return -1;
        }
        ret = copy_from_user(req_data, ifreq_params->data, ifreq_params->data_size);
        if (ret)
        {
            // dev_err(&pdev->dev, "Cannot copy req_data from user\n");
            ifreq->result = -1;
            kfree(req_data);
            kfree(ifreq_params);
            return -1;
        }
        // // dev_info(&pdev->dev, "Success to copy req_data from user\n");
        //    for (i = 0; i < ifreq_params->data_size; i++)
        //    {
        //      if ((i != 0) && (i % 32 == 0))
        //      {
        //        printk("\n");
        //      }
        //      printk("%02X ", req_data[i]);
        //    }
    }
    else
    {
        // dev_info(&pdev->dev, "There is no req_data\n");
    }

    /*
   * 각 요청 메시지 처리
   */
    // dev_info(&pdev->dev, "Process ioctl msg - id: %d\n", ifreq_params->msgid);
    switch (ifreq_params->msgid)
    {
    /*
     * 레지스터 READ 명령 수행
     */
    case IOCTL_MSG_ID_REG_READ:
    {
        struct ioctl_ifreq_reg_access *reg_access =
            (struct ioctl_ifreq_reg_access *)req_data;
        reg_access->val = readl(
            pdata->hw.radio[reg_access->radio_idx].cfg_iobase + reg_access->offset);
        // dev_info(&pdev->dev, "Reading radio%u register(0x%08X) : 0x%08X\n", reg_access->radio_idx, (int)reg_access->offset, (int)reg_access->val);
        updata = true;
        break;
    }
    /*
     * 레지스터 WRITE 명령 수행
     */
    case IOCTL_MSG_ID_REG_WRITE:
    {
        struct ioctl_ifreq_reg_access *reg_access =
            (struct ioctl_ifreq_reg_access *)req_data;
        writel(
            reg_access->val,
            pdata->hw.radio[reg_access->radio_idx].cfg_iobase + reg_access->offset);
        // dev_info(&pdev->dev, "Writing radio%u register(0x%08X) : 0x%08X\n", reg_access->radio_idx, (int)reg_access->offset, (int)reg_access->val);
        break;
    }
    /*
     * 채널 접속 명령 수행
     */
    case IOCTL_MSG_ID_CHANNEL_ACCESS:
    {
        struct ioctl_ifreq_chan_access *chan_access =
            (struct ioctl_ifreq_chan_access *)req_data;
        // dev_info(&pdev->dev, "Accessing radio%u channel - ts0:%u, ts1:%u\n", chan_access->radio_idx, chan_access->ts0_chan, chan_access->ts1_chan);

        ret = condor_access_channel(pdev,
                                    chan_access->radio_idx,
                                    chan_access->ts0_chan,
                                    chan_access->ts1_chan);
        ifreq->result = ret;
        break;
    }
    /*
     * MPDU 전송 명령 수행
     */
    case IOCTL_MSG_ID_MPDU_TX:
    {
        struct ioctl_ifreq_mpdu_tx *mpdu_tx =
            (struct ioctl_ifreq_mpdu_tx *)req_data;
        // dev_info(&pdev->dev, "Tx %lu-bytes mpdu at radio%u timeslot%u - datarate: %u, power: %d\n", mpdu_tx->mpdu_size, mpdu_tx->radio_idx, mpdu_tx->timeslot, mpdu_tx->datarate, mpdu_tx->txpower);

        ifreq->result = condor_tx_mpdu(pdev,
                                       mpdu_tx->radio_idx,
                                       mpdu_tx->timeslot,
                                       mpdu_tx->datarate,
                                       mpdu_tx->txpower,
                                       mpdu_tx->mpdu,
                                       mpdu_tx->mpdu_size);
        break;
    }
    /*
     * Check network statistics
     */
    case IOCTL_MSG_ID_GET_NETSTATS:
    {
        struct ioctl_ifreq_get_netstats *get_netstats =
            (struct ioctl_ifreq_get_netstats *)req_data;
        get_netstats->tx_success_cnt = pdata->netstats.tx_success_cnt;
        get_netstats->tx_fail_cnt = pdata->netstats.tx_fail_cnt;
        get_netstats->rx_success_cnt = pdata->netstats.rx_success_cnt;
        get_netstats->rx_fail_cnt = pdata->netstats.rx_fail_cnt;
        // dev_info(&pdev->dev, "Get network statistics - tx_success:%u, tx_fail: %u, rx_success:%u, rx_fail:%u\n", get_netstats->tx_success_cnt, get_netstats->tx_fail_cnt, get_netstats->rx_success_cnt, get_netstats->rx_fail_cnt);

        updata = true;
        break;
    }
    /*
     * Clear network statistics
     */
    case IOCTL_MSG_ID_CLEAR_NETSTATS:
    {
        pdata->netstats.tx_success_cnt = 0;
        pdata->netstats.tx_fail_cnt = 0;
        pdata->netstats.rx_success_cnt = 0;
        pdata->netstats.rx_fail_cnt = 0;
        // dev_info(&pdev->dev, "Clear network statistics\n");
        break;
    }
    /* Setup txpower and datarate */
    case IOCTL_MSG_ID_SET_POWER_DATARATE:
    {
        struct ioctl_ifreq_set_power_datarate *setting = (struct ioctl_ifreq_set_power_datarate *)req_data;
        pdata->tx_power = setting->tx_power;
        pdata->datarate = setting->datarate;
        // dev_info(&pdev->dev, "Setup tx_power: %d-dBm, datarate: %u-kbps\n", pdata->tx_power, pdata->datarate);
        break;
    }
    /* Setup bssid */
    case IOCTL_MSG_ID_SET_TYPE:
    {
        struct ioctl_ifreq_set_type *setting = (struct ioctl_ifreq_set_type *)req_data;

        // dev_info(&pdev->dev, "Set device type: %u, BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n", setting->dev_type, setting->bssid[0], setting->bssid[1], setting->bssid[2], setting->bssid[3], setting->bssid[4], setting->bssid[5]);
        pdata->dev_type = setting->dev_type;
        memcpy(pdata->hw.bssid, setting->bssid, ETH_ALEN);
        condor_set_radio_hw_bssid(pdev, &pdata->hw, 0, setting->bssid);
        break;
    }
    /* Setup MAC addr */
    case IOCTL_MSG_ID_SET_MAC:
    {
        struct ioctl_ifreq_set_mac *setting = (struct ioctl_ifreq_set_mac *)req_data;

        // dev_info(&pdev->dev, "Set MAC addr : %02x:%02x:%02x:%02x:%02x:%02x\n", setting->mac_addr[0], setting->mac_addr[1], setting->mac_addr[2], setting->mac_addr[3], setting->mac_addr[4], setting->mac_addr[5]);
        memcpy(pdata->hw.mac_addr, setting->mac_addr, ETH_ALEN);
        condor_set_netdev_mac_addr(pdev, ndev, setting->mac_addr);
        break;
    }
    case IOCTL_MSG_ID_SET_UPPER_LAYER:
    {
        unsigned int *setting = (unsigned int *)req_data;
        flush_all_condor_tx_queues();
        // dev_info(&pdev->dev, "TX packet number: %d, TX interrupt number: %d", pdata->tx_pkt_num, pdata->tx_intr_num);
        pdata->tx_intr_num = 0;
        pdata->tx_pkt_num = 0;

        if (*setting == 1)
        {
            // dev_info(&pdev->dev, "netif_wake_queue()\n");
            netif_wake_queue(ndev);
        }
        else
        {
            // dev_info(&pdev->dev, "netif_stop_queue()\n");
            netif_stop_queue(ndev);
        }
        break;
    }

    default:
        // dev_err(&pdev->dev, "Unknown msg id %d\n", ifreq_params->msgid);
        break;
    }

    /*
   * 반환할 데이터가 있으면 반환한다.
   */
    if (updata)
    {
        ret = copy_to_user(ifreq_params->data, req_data, ifreq_params->data_size);
        if (ret)
        {
            // dev_err(&pdev->dev, "Fail to copy %u-bytes data to user\n", ifreq_params->data_size);
            ifreq->result = -1;
        }
        // dev_info(&pdev->dev, "Success to copy %u-bytes data to user\n", ifreq_params->data_size);
    }

    if (req_data)
    {
        kfree(req_data);
    }
    kfree(ifreq_params);
    return 0;
}

/**
 * @brief 수신 성공 인터럽트를 처리한다.
 * 수신 패킷을 큐에 넣는 작업을 한다.
 * @param hw 인터럽트가 발생한 PHY하드웨어
 * @param q_index 하드웨어 수신버퍼 인덱스
 * @return
 * */
void condor_process_rx_success_interrupt(struct condor_hw *hw, int32_t q_index)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    int32_t result;
    struct sk_buff *skb;
    rxMetaData_t rx_meta;

    // dev_info(&pdev->dev, "Processing Rx success interrupt - hw buf[%d]\n", q_index);

    /* 하드웨어 레지스터로부터 패킷 복사 */
    skb = condor_receive_hw_packet(hw, q_index, &rx_meta, &result);
    if (!skb)
        return;

    /* 수신 정보 저장 */
    /* 소켓버퍼 정보 저장 */
    skb->dev = pdata->cndev->ndev;
    skb->mac_header = skb->data;
    /* 수신 채널번호 저장 */
    rx_meta.ChannelNumber = pdata->current_channel;

    /* 수신큐에 삽입 */
    spin_lock(&pdata->rx_lock);
    result = push_condor_rx_queue(&pdata->rxq, skb, &rx_meta);
    spin_unlock(&pdata->rx_lock);

    /* 삽입에 실패하면 소켓버퍼를 해제한다. */
    if (result < 0)
    {
        /* 오버플로우(꽉참) */
        if (result == -2)
            pdata->netstats.rx_queue_full_cnt++;
        // dev_kfree_skb(skb);
        dev_kfree_skb_irq(skb);
        pdata->netstats.rx_drop_cnt++;
    }

    /* 상태변수를 참으로 설정하여 수신 쓰레드에서 패킷을 처리하도록 한다. */
    pdata->rx_pending = true;
    wake_up_interruptible(&pdata->rx_wait);
}

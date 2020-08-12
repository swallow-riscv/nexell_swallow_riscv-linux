//
// Created by gyun on 2019-03-24.
//

#include "condor.h"
#include "condor_types.h"
#include "condor_hw_regs.h"

#define HW_TX_CTRL_TX_POWER_SET (1 << 1)

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

static plcp_rate_code condor_get_plcp_rate_code(u8 datarate);
static void hw_memcpy(char *dst, char *src, u32 len);
#ifdef _WAE_SHARED_HW_TXBUF_
int condor_tx_ppdu_shared_buf(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx,
    struct sk_buff *skb, txMetaData_t *meta);
#else
static void condor_tx_ppdu(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx,
    struct sk_buff *skb);
#endif

/**
 * @brief MPDU를 전송한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param timeslot (입력) MPDU를 전송할 시간슬롯 (0,1)
 * @param datarate (입력) MPDU를 전송할 데이터레이트
 * @param txpower (입력) MPDU를 전송할 파워
 * @param mpdu (입력) 전송할 MPDU
 * @param mpdu_size (입력) 전송할 MPDU 길이
 * @return 성공시 0, 실패시 -1
 */
int condor_tx_mpdu(
    struct platform_device *pdev,
    u32 radio_idx,
    u8 timeslot,
    u8 datarate,
    s32 txpower,
    u8 mpdu[],
    u32 mpdu_size)
{
    u32 i;
    plcp_rate_code plcp_rate;
    u32 q_idx = timeslot * 4;
    u32 psdu_len;
    int result = 0;
    struct sk_buff *skb;
    struct condor_hw_tx_ctrl *ctrl;
    struct condor_hw_tx_power_map *entry = NULL;
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    struct condor_hw *hw = &pdata->hw;

    txMetaData_t meta;
    meta.expiry = 0;

    // dev_info(&pdev->dev, "Transmit %d bytes mpdu - radio%u, timeslot%u, q_idx: %u, datarate:%u, txpower:%d\n", mpdu_size, radio_idx, timeslot, q_idx, datarate, txpower);

    /* src addr 수정*/
    memcpy(mpdu + 10, hw->mac_addr, 6);
    /*
   * 소켓버퍼 생성
   */
    skb = alloc_skb(
        HW_SPECIFIC_HDR_LEN + mpdu_size + MAC_FCS_LEN,
        in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (!skb)
    {
        // dev_err(&pdev->dev, "Fail to allocate socket buffer\n");
        return -1;
    }
    skb_reserve(skb, HW_SPECIFIC_HDR_LEN);
    memcpy(skb_put(skb, mpdu_size), mpdu, mpdu_size);
    // dev_info(&pdev->dev, "Success to construct socket buffer - len: %d\n", skb->len);
    skb_put(skb, MAC_FCS_LEN);
    psdu_len = mpdu_size + MAC_FCS_LEN;

    /*
   * 하드웨어 헤더를 구성한다.
   */
    ctrl = (struct condor_hw_tx_ctrl *)skb_push(skb, sizeof(*ctrl));
    if (!ctrl)
    {
        // dev_err(&pdev->dev, "Fail to skb_push() for hw_ctrl\n");
        dev_kfree_skb(skb);
        return -1;
    }
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

    /*
   * 전송한다.
   */
#ifdef _WAE_SHARED_HW_TXBUF_
    result = condor_tx_ppdu_shared_buf(pdev, hw, radio_idx, q_idx, skb, &meta);
#else
    condor_tx_ppdu(pdev, hw, radio_idx, q_idx, skb);
#endif
    if (result < 0)
        dev_kfree_skb(skb);
    return 0;
}

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

#ifdef _WAE_SHARED_HW_TXBUF_
/**
 * @brief 공유버퍼에 저장된 패킷의 수를 확인한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 디바이스
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param q_idx (입력) 큐 번호 (0~3)
 * @return 공유버퍼에 저장된 패킷수, 오류시 -1
 */
int condor_get_hw_buf_pkt_num(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx)
{
    int ret;
    u32 reg =
        readl(hw->radio[radio_idx].cfg_iobase + TX_QUEUE_STAT(q_idx)) & 0xff;

    // dev_info(&pdev->dev, "q_idx = %d, offset: 0x%08x, reg = 0x%08x\n", q_idx, TX_QUEUE_STAT(q_idx), reg);

    switch (reg)
    {
    case 0:
        ret = 0;
        break;
    case 1:
        ret = 1;
        break;
    case 3:
        ret = 2;
        break;
    case 7:
        ret = 3;
        break;
    case 15:
        ret = 4;
        break;
    case 31:
        ret = 5;
        break;
    case 63:
        ret = 6;
        break;
    case 127:
        ret = 7;
        break;
    default:
        ret = -1;
    }
    return ret;
}

/**
 * @brief 공유버퍼에 패킷을 쓸 수 있을 정도의 공간이 있는지 확인한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 디바이스
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param block_num (입력) 전송패킷이 차지하는 블록개수
 * @return 패킷을 쓸 시작블록인덱스, 공간이 없을 시 -1
 */
int condor_check_buf_space(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    int block_num)
{
    u32 reg;
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    int i, j, emptyBlockNum = 0;

    for (i = 0; i < 4; i++)
    {
        reg = readl(iobase + REG_TX_QUEUE_DATA_MAP(i));
        for (j = 0; j < 32; j++)
        {
            if (((reg >> j) & 1) == 0)
            {
                emptyBlockNum++;
            }
            else
            {
                emptyBlockNum = 0;
            }
            if (emptyBlockNum > block_num)
            {
                return (i * 32) + j - (emptyBlockNum - 1);
            }
        }
    }
    return -1;
}

/**
 * @brief PPDU를 전송한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 디바이스
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param q_idx (입력) 큐 번호 (0~3)
 * @param start_block_idx (입력)
 * @param block_num (입력)
 * @param skb (입력) 전송할 PPDU
 */
void condor_tx_ppdu_shared_buffer_go(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx,
    u32 start_block_idx,
    u32 block_num,
    struct sk_buff *skb)
{
    /* Debug */
    // dev_info(&pdev->dev, "4444. Write HW Buffer\n");
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    u32 i, len, remain;
    int offset = (start_block_idx * TX_UNIT_BLOCK_SIZE) / 4;
    // dev_info(&pdev->dev, "Transmitting %d-bytes packet using radio[%d]'s queue[%d] offset[%d] - StartIndx: %d, BlockNum: %d\n", skb->len, radio_idx, q_idx, offset, start_block_idx, block_num);

    // 전송할 PPDU를 하드웨어 송신버퍼에 복사한다. */
#if 0
  hw_memcpy(hw->radio[radio_idx].txbuf_iobase + TXBUF(q_idx, offset),
            skb->data, skb->len);
#else

#define WORD_ALIGN 4
    len = skb->len;
    remain = len % WORD_ALIGN;
    if (remain != 0)
    {
        len += (WORD_ALIGN - remain);
    }

    for (i = 0; i < len / 4; i++)
    {
        writel(*(u32 *)(skb->data + (4 * i)),
               hw->radio[radio_idx].txbuf_iobase + TXBUF(q_idx, i + offset));
    }
#endif

    // 전송할 PPDU의 AC/TS, 블록개수, 시작블록인덱스를 설정한다. */
    writel((q_idx << 12) | (block_num << 7) | (start_block_idx << 0),
           hw->radio[radio_idx].cfg_iobase + REG_TX_QUEUE_DATA_INFO);

    // 송신명령을 내린다 */
    writel(SET_TX_QUEUE_DATA_LOAD,
           hw->radio[radio_idx].cfg_iobase + REG_TX_QUEUE_DATA_LOAD);
    pdata->hw_pkt_num++;
    pdata->tx_pkt_num++;
    /* Debug */
    // dev_info(&pdev->dev, "4444. Write HW Buffer END\n");
}

/**
 * @brief PPDU를 공유버퍼 하드웨어 기반으로 전송한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 디바이스
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param q_idx (입력) 큐 번호 (0~3)
 * @param skb (입력) 전송할 PPDU
 */
int condor_tx_ppdu_shared_buf(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx,
    struct sk_buff *skb, txMetaData_t *meta)
{
    int start_block_index, hw_buffer_pkt_num, block_num;
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    condorNetIfTxQueue_t *txq = &pdata->txq[0];
    unsigned long flags;
    int32_t result = 0;

    spin_lock_irqsave(&pdata->tx_lock, flags);
    // hw_buffer_pkt_num = condor_get_hw_buf_pkt_num(pdev, hw, radio_idx, q_idx);
    /* Debug */
    // dev_info(&pdev->dev, "condr_tx_ppdu_shared_buf");
    if (TAILQ_EMPTY(&txq->head))
    {
        // dev_info(&pdev->dev, "Tx %d bytes ppdu to radio%u queue%u with shared buffer - buf pktnum: %u\n", skb->len, radio_idx, q_idx, hwBufferPktNum);

        /* H/W버퍼 내에 패킷이 없으면 H/W버퍼에 패킷을 쓴다
    * 	- H/W 버퍼에 공간이 있는 경우 가능하며, 공간이 없는 경우에는 SW 큐에 삽입한다. */
        if (pdata->hw_pkt_num == 0)
        {
            // dev_info(&pdev->dev, "hw_pkt_num: %u", pdata->hw_pkt_num);
            block_num = (skb->len % TX_UNIT_BLOCK_SIZE) ? ((skb->len / TX_UNIT_BLOCK_SIZE) + 1) : (skb->len / TX_UNIT_BLOCK_SIZE);
            start_block_index = condor_check_buf_space(pdev, hw, radio_idx, block_num);

            if (start_block_index < 0)
            {
                // dev_info(&pdev->dev, "push queue: %u", txq->pktNum);
                result = push_condor_net_if_tx_queue(txq, skb, meta);
                
            }
            else
            {
                condor_tx_ppdu_shared_buffer_go(pdev, hw, radio_idx, q_idx, start_block_index, block_num, skb); 
                dev_kfree_skb(skb);
                skb = NULL;
            }
        }
        else
        {
            // dev_info(&pdev->dev, "push queue: %u", txq->pktNum);
            result = push_condor_net_if_tx_queue(txq, skb, meta);
        }
    }
    else
    {
        // dev_info(&pdev->dev, "push queue: %u", txq->pktNum);
        result = push_condor_net_if_tx_queue(txq, skb, meta);
    }

    spin_unlock_irqrestore(&pdata->tx_lock, flags);

    // dev_info(&pdev->dev, "Success to tx ppdu\n");
    return result;
}

#else // _WAE_SHARED_HW_TXBUF_

/**
 * @brief PPDU를 전송한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 디바이스
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param q_idx (입력) 큐 번호 (0~3)
 * @param skb (입력) 전송할 PPDU
 */
static void condor_tx_ppdu(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx,
    struct sk_buff *skb)
{
    // dev_info(&pdev->dev, "Tx %d bytes ppdu to radio%u queue%u\n", skb->len, radio_idx, q_idx);

    // PPDU를 송신버퍼에 복사한다.
    hw_memcpy(hw->radio[radio_idx].txbuf_iobase + TXBUF(q_idx, 0),
              skb->data,
              skb->len);

    // PPDU 전송 명령을 내린다.
    writel((1 << q_idx), hw->radio[radio_idx].cfg_iobase + REG_TX_START);
}

#endif // _WAE_SHARED_HW_TXBUF_

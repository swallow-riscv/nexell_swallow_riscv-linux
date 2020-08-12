//
// Created by gyun on 2019-03-23.
//

#include "condor.h"
#include "condor_hw_max2829.h"
#include "condor_hw_regs.h"

/**
 * @brief MAX2829 RF칩을 각 채널로 설정하기 위한 설정값 정보
 */
struct condor_max2829_chan_conf_entry
{
    u8 channel;   // 채널번호
    u32 freq;     // 채널 중심주파수
    u32 int_div;  // 채널 별 RF침 레지스터 설정 값
    u32 fra_div0; // 채널 별 RF침 레지스터 설정 값
    u32 fra_div1; // 채널 별 RF침 레지스터 설정 값
};

/**
 * @brief MAX2829 RF칩의 각 채널별 설정값들.
 */
#define MAX2829_SUPPORT_CHAN_NUM 15
static struct condor_max2829_chan_conf_entry
    g_max2829_chan_conf[MAX2829_SUPPORT_CHAN_NUM] =
        {/* 채널번호, 중심주파수, MAX2829 레지스터 세팅값 1, 2, 3 */
         {160, 5800, 0xE8, 0x0000, 0},
         {162, 5810, 0xE8, 0x1999, 2},
         {164, 5820, 0xE8, 0x3333, 0},
         {166, 5830, 0xE9, 0x0CCC, 3},
         {168, 5840, 0xE9, 0x2666, 1},
         {170, 5850, 0xEA, 0x0000, 0},
         {172, 5860, 0xEA, 0x1999, 2},
         {174, 5870, 0xEA, 0x3333, 0},
         {176, 5880, 0xEB, 0x0CCC, 3},
         {178, 5890, 0xEB, 0x2666, 1},
         {180, 5900, 0xEC, 0x0000, 0},
         {182, 5910, 0xEC, 0x1999, 2},
         {184, 5920, 0xEC, 0x3333, 0},
         {186, 5930, 0xED, 0x0CCC, 3},
         {188, 5940, 0xED, 0x2666, 1}};

/**
 * @brief 각 라디오 하드웨어의 TSF 타이머 값을 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param time (입력) 설정할 시간값
 */
void condor_set_radio_hw_tsf(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u64 time)
{
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;

    // TSF 타이머 값 저장
    writel(time & 0xffffffff, iobase + REG_TSF_LOW);
    writel((time >> 32) & 0xffffffff, iobase + REG_TSF_HIGH);

    // TSF 타이머 값을 방금 저장한 값으로 사용하도록 설정
    writel(MAC_CMD_UPD_TS_REG, iobase + REG_TSF_UPDATE_CMD);
}

/**
 * @brief 각 라디오 하드웨어의 MAC주소를 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param addr (입력) MAC 주소
 */
void condor_set_radio_hw_mac_addr(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 addr[ETH_ALEN])
{
    u32 reg, i;
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    // dev_info(&pdev->dev, "Setting radio%u Mac addr - %02X:%02X:%02X:%02X:%02X:%02X\n", radio_idx, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // MAC 주소 설정한다.
    writel(
        (addr[0] << 0) | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24),
        iobase + REG_MACADDR_LOW32);
    writel(
        (addr[4] << 0) | (addr[5] << 8),
        iobase + REG_MACADDR_HIGH16);

    // 설정된 MAC주소를 기반으로 난수발생기 시드 값을 설정한다.
    reg = 0;
    for (i = 0; i < ETH_ALEN; i++)
    {
        reg ^= addr[i];
    }
    writel(reg, iobase + REG_RN_INIT);
}

/**
 * @brief 각 라디오 하드웨어의 BSSID 를 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param bssid (입력) BSSID
 */
void condor_set_radio_hw_bssid(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 bssid[ETH_ALEN])
{
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    // dev_info(&pdev->dev, "Setting radio%u BSSID - %02X:%02X:%02X:%02X:%02X:%02X\n", radio_idx, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

    // MAC 주소 설정한다.
    writel(
        (bssid[0] << 0) | (bssid[1] << 8) | (bssid[2] << 16) | (bssid[3] << 24),
        iobase + REG_BSSID_LOW32);
    writel(
        (bssid[4] << 0) | (bssid[5] << 8),
        iobase + REG_BSSID_HIGH16);
        
    memcpy(pdata->hw.bssid, bssid, ETH_ALEN);
}

/**
 * @brief 각 라디오의 각 시간슬롯별로 설정된 명령을 MAX2829 RF 칩으로 전송한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param timeslot (입력) 전송할 설정명령에 해당되는 시간슬롯 (0,1)
 */
void condor_send_radio_hw_rf_cmd(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 timeslot)
{
    // dev_info(&pdev->dev, "Sending radio%u timeslot%u command to rf\n", radio_idx, timeslot);

    if (timeslot == 0)
    {
        writel(RF_PARAMS_SET_TIMESLOT0,
               hw->radio[radio_idx].cfg_iobase + REG_RF_PARAMS_SET);
    }
    else
    {
        writel(RF_PARAMS_SET_TIMESLOT1,
               hw->radio[radio_idx].cfg_iobase + REG_RF_PARAMS_SET);
    }
    // dev_info(&pdev->dev, "Success to send\n");
}

/**
 * @brief MAX2829 RF 칩을 PHY 하드웨어에서 병렬IF를 통해 제어하도록 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 */
void condor_set_radio_hw_rf_parallel_control(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx)
{
    u32 reg1, reg2;
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;

    // MAX2829의 rxgain 병렬 인터페이스를 통해 제어하도록 설정한다.

    // TimeSlot0 접속 기간 동안 MAX2829에 적용되도록 명령어 저장
    reg1 = readl(iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXCTRL);
    reg1 = reg1 >> 4;
    reg2 = reg1 & ~(1 << 12);
    reg2 = (reg2 << 4) | MAX2829_REG_RXCTRL_ADDR;
    writel(reg2, iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXCTRL);

    // TimeSlot1 접속 기간 동안 MAX2829에 적용되도록 명령어 저장
    reg1 = readl(iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXCTRL);
    reg1 = reg1 >> 4;
    reg2 = reg1 & ~(1 << 12);
    reg2 = (reg2 << 4) | MAX2829_REG_RXCTRL_ADDR;
    writel(reg2, iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXCTRL);

    //  MAX2829로 명령어 전송 - 이제 MAX2829의 rxgain은
    //  모뎀 내의 AGC로부터 병렬인터페이스를 통해 제어된다.
    //    - 이 시점에서 굳이 각 시간슬롯 별 명령어를
    //      둘 다 MAX2829로 전송할 필요는 없다(동일하므로)
    condor_send_radio_hw_rf_cmd(pdev, hw, radio_idx, 0);
}

/**
 * @brief 라디오의 채널 스위칭을 시작한다. (alternating 접속)
 *          * 각 시간슬롯 별로 채널설정 파라미터를 레지스터에 로딩한다.
 *              - 채널스위칭 변경시점에 자동으로 RF칩으로 전송된다.
 *          * 채널스위칭 인터벌 레지스터를 설정한다.
 *          * 채널스위칭 레지스터를 설정하여 스위칭을 시작한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param ts0_chan (입력) Timeslot0 채널
 * @param ts1_chan (입력) Timeslot1 채널
 */
int condor_start_radio_hw_chan_switching(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 ts0_chan,
    u8 ts1_chan)
{
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    // dev_info(&pdev->dev, "Start radio%u channel switching - ts0:%u, ts1:%u\n", radio_idx, ts0_chan, ts1_chan);

    /*
   * 각 시간슬롯별 채널 설정 파라미터를 로딩한다.
   */
    if (condor_load_radio_hw_rf_cmd(pdev, hw, radio_idx, ts0_chan, 0))
    {
        return -1;
    }
    if (condor_load_radio_hw_rf_cmd(pdev, hw, radio_idx, ts1_chan, 1))
    {
        return -1;
    }

    /*
   * 채널스위칭 인터벌을 설정한다.
   */
    condor_set_radio_hw_chan_switching_interval(pdev, hw, radio_idx, 50, 50, 2, 2);

    /*
   * 채널스위칭을 시작한다.
   */
    writel(readl(iobase + REG_ENABLE) | (1 << 3), iobase + REG_ENABLE);
    // 채널스위칭을 PPS에 동기화해서 수행하도록 설정
    //  - 초기화 절차(InitHwChannelSwitching())에서 이미 설정되었지만,
    //    HW 버그로 인해 채널스위칭 시작 명령 후 다시 한번 해 줘야 한다.
    writel(readl(iobase + REG_COORD_CTRL) | (1 << 0), iobase + REG_COORD_CTRL);

    // dev_info(&pdev->dev, "Success to start channel switching\n");
    return 0;
}

/**
 * @brief 라디오의 채널 스위칭을 중지한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 */
void condor_stop_radio_hw_chan_switching(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx)
{
    u32 reg;
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    // dev_info(&pdev->dev, "Stop radio%u channel switching\n", radio_idx);

    reg = readl(iobase + REG_ENABLE);
    reg &= ~(1 << 3);
    writel(reg, iobase + REG_ENABLE);
}

/**
 * @brief 특정 라디오를 특정 채널로 접속한다. (=continuous 접속)
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param chan (입력) 접속할 채널
 */
int condor_access_radio_hw_chan(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 chan)
{
    // dev_info(&pdev->dev, "Access continuously radio%u channel %u\n", radio_idx, chan);

    // 시간슬롯0 채널접속을 위한 명령어를 로딩한다 - continous이므로 TS0만 하면 됨.
    if (condor_load_radio_hw_rf_cmd(pdev, hw, radio_idx, chan, 0))
    {
        return -1;
    }

    // 로딩된 명령어를 RF칩으로 전송하여 세팅한다.
    condor_send_radio_hw_rf_cmd(pdev, hw, radio_idx, 0);

    // dev_info(&pdev->dev, "Success to access channel\n");
    return 0;
}

/**
 * @brief 각 라디오의 각 시간슬롯 별로 채널 접속을 위한 설정들을 레지스터에 로딩한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param chan (입력) 접속할 채널
 * @param timeslot (입력) 접속할 시간슬롯
 * @return 성공시 0, 실패시 -1
 */
int condor_load_radio_hw_rf_cmd(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 chan,
    u8 timeslot)
{
    u32 i, reg1, reg2;
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    struct condor_max2829_chan_conf_entry *entry = NULL;
    // dev_info(&pdev->dev, "Load rf chip command for radio%u, timeslot%u, chan:%u\n", radio_idx, timeslot, chan);

    /*
   * MAX2829 채널설정 파라미터 엔트리 탐색
   */
    for (i = 0; i < MAX2829_SUPPORT_CHAN_NUM; i++)
    {
        if (g_max2829_chan_conf[i].channel == chan)
        {
            entry = &g_max2829_chan_conf[i];
            break;
        }
    }
    if (!entry)
    {
        // dev_err(&pdev->dev, "No configuration entry for chan %u\n", chan);
        return -1;
    }

    /*
   * 레지스터 설정
   */
    reg1 = ((entry->fra_div1 << 12) | (entry->int_div));
    reg1 = (reg1 << 4) | MAX2829_REG_INTDIVRATIO_ADDR;
    reg2 = (entry->fra_div0 << 4) | MAX2829_REG_FRADIVRATIO_ADDR;
    if (timeslot == 0)
    {
        writel(reg1, iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_INTDIVRATIO);
        writel(reg2, iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_FRADIVRATIO);
    }
    else
    {
        writel(reg1, iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_INTDIVRATIO);
        writel(reg2, iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_FRADIVRATIO);
    }

    // dev_info(&pdev->dev, "Success - int:0x%02X, fra0:0x%04X, fra1:%u, REG1:0x%08X, REG2:0x%08X\n", entry->int_div, entry->fra_div0, entry->fra_div1, reg1, reg2);
    return 0;
}

/**
 * @brief 특정 라디오의 채널스위칭 인터벌 파라미터값들을 레지스터에 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param ts0_interval (입력) 시간슬롯0 인터벌
 * @param ts1_interval (입력) 시간슬롯1 인터벌
 * @param sync_tolerance (입력) Sync tolerance
 * @param switching_max_time (입력) Max channel switching time
 */
void condor_set_radio_hw_chan_switching_interval(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 ts0_interval,
    u32 ts1_interval,
    u32 sync_tolerance,
    u32 switching_max_time)
{
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    u32 reg;
    // dev_info(&pdev->dev, "Set radio%u channel switching interval - ts0:%u, ts1:%u, sync:%u, maxtime:%u\n", radio_idx, ts0_interval, ts1_interval, sync_tolerance, switching_max_time);
    reg = (switching_max_time << 28) | (sync_tolerance << 24) |
          (ts1_interval << 12) | (ts0_interval << 0);
    writel(reg, iobase + REG_CHN_INTERVAL);
}

/**
 * @brief 특정 라디오의 RTS Threshold 값을 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param threshold (입력) RTS threshold
 */
void condor_set_radio_hw_mac_rts_thr(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 threshold)
{
    u32 reg;
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    // dev_info(&pdev->dev, "Set radio%u rts threshold %u\n", radio_idx, threshold);
    reg = readl(iobase + REG_RTS_FRAG_THR);
    reg &= ~(0xfff);
    reg |= threshold;
    writel(reg, iobase + REG_RTS_FRAG_THR);
}

/**
 * @brief 특정 라디오의 Fragmentation Threshold 값을 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param threshold (입력) Fragmentation threshold
 */
void condor_set_radio_hw_mac_frag_thr(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 threshold)
{
    u32 reg;
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    // dev_info(&pdev->dev, "Set radio%u fragmentation threshold %u\n", radio_idx, threshold);
    reg = readl(iobase + REG_RTS_FRAG_THR);
    reg &= ~(0xfff << 16);
    reg |= (threshold << 26);
    writel(reg, iobase + REG_RTS_FRAG_THR);
}

/**
 * @brief 특정 라디오의 시간슬롯별/AC 별 EDCA Parameter Set 값을 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 식별자 (0,1)
 * @param timeslot (입력) 시간슬롯 (0,1)
 * @param ac (입력) Access category (0~3)
 * @param cwmin (입력) CWmin
 * @param cwmax (입력) CWmax
 * @param aifsn (입력) AIFSN
 */
void condor_set_radio_hw_mac_edca(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 timeslot,
    u32 ac,
    u32 cwmin,
    u32 cwmax,
    u32 aifsn)
{
    u8 *iobase = hw->radio[radio_idx].cfg_iobase;
    u32 offset = ac + (timeslot * 4);
    // dev_info(&pdev->dev, "Setting radio%u timeslot%u ac%u edca - cwmin: %u, cwmax: %u, aifsn: %u\n", radio_idx, timeslot, ac, cwmin, cwmax, aifsn);

    writel(
        ((aifsn << 24) | (cwmax << 8) | cwmin),
        iobase + MIB_DOT11OP_REG((8 + offset))); //8=other register offset
}

uint64_t condor_get_hw_tsf_time(struct condor_hw *hw)
{
    uint64_t reg, reg_l, reg_h;
    uint8_t *iobase = hw->radio[0].cfg_iobase;
    reg_l = readl(iobase + REG_TSF_LOW);
    reg_h = readl(iobase + REG_TSF_HIGH);
    reg = (reg_h << 32) | reg_l;
    return reg;
}
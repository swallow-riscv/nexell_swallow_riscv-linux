//
// Created by gyun on 2019-03-23.
//

#include "condor.h"
#include "condor_dev.h"
#include "condor_hw_max2829.h"
#include "condor_hw_regs.h"

/**
 * @brief AC 별 EDCA 파라미터값
 */
struct edca_param_set_entry
{
  u32 cwmin;
  u32 cwmax;
  u32 aifsn;
  u32 txop;
};

/**
 * @brief 4개의 AC에 대한 EDCA 파라미터 셋
 */
static struct edca_param_set_entry g_default_edca_param_set[4] =
  {
    { 15, 1023, 9, 0}, // AC_BK
    { 15, 1023, 6, 0}, // AC_BE
    { 7, 15, 3, 0}, // AC_VI
    { 3, 7, 2, 0}  // AC_VO
  };


static int condor_map_hw_io(struct platform_device *pdev, struct condor_hw *hw);
static void condor_unmap_hw_io(
  struct platform_device *pdev,
  struct condor_hw *hw);
static int condor_map_radio_hw_io(
  struct platform_device *pdev,
  struct condor_hw *hw,
  u32 radio_idx);

static void condor_init_mac_hw(
  struct platform_device *pdev,
  struct condor_hw *hw);
static void condor_init_phy_hw(
  struct platform_device *pdev,
  struct condor_hw *hw);
static void condor_init_rf_hw(
  struct platform_device *pdev,
  struct condor_hw *hw);
static void condor_init_hw_intr(
  struct platform_device *pdev,
  struct condor_hw *hw);
static void condor_init_hw_chan_switching(
  struct platform_device *pdev,
  struct condor_hw *hw);
static int condor_check_init_hw_state(
  struct platform_device *pdev,
  struct condor_hw *hw);

static void condor_start_hw(struct platform_device *pdev, struct condor_hw *hw);
static void condor_stop_hw(struct platform_device *pdev, struct condor_hw *hw);
static void condor_activate_hw_intr(
  struct platform_device *pdev,
  struct condor_hw *hw);
static void condor_deactivate_hw_intr(
  struct platform_device *pdev,
  struct condor_hw *hw);
static void condor_set_hw_no_diversity(
  struct platform_device *pdev,
  struct condor_hw *hw);



/**
 * @brief Condor 하드웨어를 초기화한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘 모두 초기화한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @return 성공시 0, 실패시 -1
 */
int condor_init_hw(struct platform_device *pdev)
{
  struct condor_platform_data *pdata = pdev->dev.platform_data;
  struct condor_hw *hw = &pdata->hw;

  // dev_info(&pdev->dev, "Init condor hardware\n");

  // 각 라디오별 레지스터 시작 주소
  hw->radio[0].iobase = hw->iobase;
#if (RADIO_NUM_ == 2)
  hw->radio[1].iobase = hw->iobase + HW_REGS_CONTAINER_LEN;
#endif

  // 하드웨어 레지스터 블록 맵핑
  if (condor_map_hw_io(pdev, hw))
  {
    return -1;
  }

  // 하드웨어 초기상태 확인
  if (condor_check_init_hw_state(pdev, hw))
  {
    condor_unmap_hw_io(pdev, hw);
    return -1;
  }

  // MAC 하드웨어 초기화
  condor_init_mac_hw(pdev, hw);

  // PHY 하드웨어 초기화
  condor_init_phy_hw(pdev, hw);

  // 인터럽트 초기화
  condor_init_hw_intr(pdev, hw);

  // 채널스위칭 동작 초기화 - 실제로 채널스위칭을 수행하지는 않는다
  condor_init_hw_chan_switching(pdev, hw);

  // dev_info(&pdev->dev, "Success to init condor hardware\n");
  return 0;
}

/**
 * @brief Condor 하드웨어의 각 레지스터 블록을 맵핑한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘 모두 매핑한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @return 성공시 0, 실패시 -1
 */
static int condor_map_hw_io(struct platform_device *pdev, struct condor_hw *hw)
{
  int ret;
  // dev_info(&pdev->dev, "Map condor hardware io\n");

  /*
   * 각 라디오 별로 레지스터 블럭 맵핑
   */
  ret = condor_map_radio_hw_io(pdev, hw, 0);
  if (ret)
  {
    return -1;
  }
#if (RADIO_NUM_ == 2)
  ret = condor_map_radio_hw_io(pdev, hw, 1);
  if (ret)
  {
    condor_unmap_hw_io(pdev, hw);
    return -1;
  }
#endif

  // dev_info(&pdev->dev, "Success to map condor hardware io\n");
  return 0;
}


/**
 * @brief Condor 각 라디오의 레지스터 블록을 맵핑한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @param radio_idx (입력) 라디오 인덱스 (0,1)
 * @return 성공시 0, 실패시 -1
 */
static int condor_map_radio_hw_io(
  struct platform_device *pdev,
  struct condor_hw *hw,
  u32 radio_idx)
{
  u8 *iobase = NULL;
  // dev_info(&pdev->dev, "Map condor radio%u hardware io\n", radio_idx);

  // 설정 레지스터 블럭
  iobase = ioremap_nocache(hw->radio[radio_idx].iobase + CONFIG_REG_BLK_OFFSET,
                           CONFIG_REGS_BLK_LEN);
  if (!iobase)
  {
    // dev_err(&pdev->dev, "Fail to ioremap_nocache(0x%08x, %u)\n", hw->radio[radio_idx].iobase + CONFIG_REG_BLK_OFFSET,  CONFIG_REGS_BLK_LEN);
    goto fail;
  }
  hw->radio[radio_idx].cfg_iobase = iobase;
  // dev_info(&pdev->dev, "Success to ioremap_nocache(0x%08x, %u)\n", hw->radio[radio_idx].iobase + CONFIG_REG_BLK_OFFSET, CONFIG_REGS_BLK_LEN);

  // 수신버퍼 블럭
  iobase = ioremap_nocache(hw->radio[radio_idx].iobase + RXBUF_BLK_OFFSET,
                           RXBUF_BLK_LEN);
  if (!iobase)
  {
    // dev_err(&pdev->dev, "Fail to ioremap_nocache(0x%08x, %u)\n", hw->radio[radio_idx].iobase + RXBUF_BLK_OFFSET,  RXBUF_BLK_LEN);
    goto fail;
  }
  hw->radio[radio_idx].rxbuf_iobase = iobase;
  // dev_info(&pdev->dev, "Success to ioremap_nocache(0x%08x, %u)\n", hw->radio[radio_idx].iobase + RXBUF_BLK_OFFSET, RXBUF_BLK_LEN);

  // 송신버퍼 블럭
  iobase = ioremap_nocache(hw->radio[radio_idx].iobase + TXBUF_BLK_OFFSET,
                           TXBUF_BLK_LEN);
  if (!iobase)
  {
    // dev_err(&pdev->dev, "Fail to ioremap_nocache(0x%08x, %u)\n", hw->radio[radio_idx].iobase + TXBUF_BLK_OFFSET, TXBUF_BLK_LEN);
    goto fail;
  }
  hw->radio[radio_idx].txbuf_iobase = iobase;
  // dev_info(&pdev->dev, "Success to ioremap_nocache(0x%08x, %u)\n", hw->radio[radio_idx].iobase + TXBUF_BLK_OFFSET, TXBUF_BLK_LEN);

  // dev_err(&pdev->dev, "Success to map condor radio hardware io\n");
  return 0;

fail:
  condor_unmap_hw_io(pdev, hw);
  return -1;
}


/**
 * @brief Condor 하드웨어의 각 레지스터 블록의 맵핑을 해제한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘 모두 해제한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @return 성공시 0, 실패시 -1
 */
static void condor_unmap_hw_io(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  // dev_info(&pdev->dev, "Unmap condor hardware io\n");

  if (hw->radio[0].cfg_iobase)
  {
    iounmap(hw->radio[0].cfg_iobase);
    hw->radio[0].cfg_iobase = NULL;
  }
  if (hw->radio[0].rxbuf_iobase)
  {
    iounmap(hw->radio[0].rxbuf_iobase);
    hw->radio[0].rxbuf_iobase = NULL;
  }
  if (hw->radio[0].txbuf_iobase)
  {
    iounmap(hw->radio[0].txbuf_iobase);
    hw->radio[0].txbuf_iobase = NULL;
  }

#if (RADIO_NUM_ == 2)
  if (hw->radio[1].cfg_iobase)
  {
    iounmap(hw->radio[1].cfg_iobase);
    hw->radio[1].cfg_iobase = NULL;
  }
  if (hw->radio[1].rxbuf_iobase)
  {
    iounmap(hw->radio[1].rxbuf_iobase);
    hw->radio[1].rxbuf_iobase = NULL;
  }
  if (hw->radio[1].txbuf_iobase)
  {
    iounmap(hw->radio[1].txbuf_iobase);
    hw->radio[1].txbuf_iobase = NULL;
  }
#endif
}


/**
 * @brief Condor 하드웨어의 초기 기본값을 확인한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 * @return 성공시 0, 실패시 -1
 */
static int condor_check_init_hw_state(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  int ret = 0;
  u32 version, man_id, prod_id;
  u8 *iobase;

  // dev_info(&pdev->dev, "Check condor hw initial status\n");

  iobase = hw->radio[0].cfg_iobase;

  // 레지스터 초기화
  writel(INIT_ALL_REGISTERS, iobase + REG_INIT);
  mdelay(100);

  // 버전, 제조사식별자, 제품식별자 확인
  version = readl(iobase + REG_HW_VERSION);
  man_id = readl(iobase + REG_MANID);
  prod_id = readl(iobase + REG_PRODID);
  // dev_info(&pdev->dev, "HW version: 0x%08x\n", version);
  if (man_id != DEFAULT_MANID)
  {
    // dev_err(&pdev->dev, "Invalid manufacturer id 0x%08x\n", man_id);
    ret = -1;
  }
  // dev_info(&pdev->dev, "Manufacturer id 0x%08x\n", man_id);
  if (prod_id != DEFAULT_PRODID)
  {
    // dev_err(&pdev->dev, "Invalid product id 0x%08x\n", prod_id);
    ret = -1;
  }
  // dev_info(&pdev->dev, "Product id 0x%08x\n", prod_id);

  if (ret)
  {
    // dev_err(&pdev->dev, "Fail to check hw initial state\n");
  }

#if (RADIO_NUM_ == 2)
  iobase = hw->radio[1].cfg_iobase;

  // 레지스터 초기화
  writel(INIT_ALL_REGISTERS, iobase + REG_INIT);
  mdelay(4);

  // 버전, 제조사식별자, 제품식별자 확인
  version = readl(iobase + REG_HW_VERSION);
  man_id = readl(iobase + REG_MANID);
  prod_id = readl(iobase + REG_PRODID);
  // dev_info(&pdev->dev, "HW version: 0x%08x\n", version);
  if (man_id != DEFAULT_MANID)
  {
    // dev_err(&pdev->dev, "Invalid manufacturer id 0x%08x\n", man_id);
    ret = -1;
  }
  // dev_info(&pdev->dev, "Manufacturer id 0x%08x\n", man_id);
  if (prod_id != DEFAULT_PRODID)
  {
    // dev_err(&pdev->dev, "Invalid product id 0x%08x\n", prod_id);
    ret = -1;
  }
  // dev_info(&pdev->dev, "Product id 0x%08x\n", prod_id);

  if (ret)
  {
    // dev_err(&pdev->dev, "Fail to check hw initial state\n");
  }
#endif

  return ret;
}


/**
 * @brief Condor MAC 하드웨어를 초기화한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 초기화 한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_init_mac_hw(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  u32 i, j;
  u8 mac_addr[ETH_ALEN];
  u8 bssid[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  // dev_info(&pdev->dev, "Initializing condor mac hardware\n");

  // TSF 타이머 초기화
  condor_set_radio_hw_tsf(pdev, hw, 0, 0);
#if (RADIO_NUM_ == 2)
  condor_set_radio_hw_tsf(pdev, hw, 1, 0);
#endif

  // MAC 주소 및 Wildcard BSSID 레지스터 설정
  random_ether_addr(mac_addr);
  memcpy(hw->mac_addr, mac_addr, ETH_ALEN);
  condor_set_radio_hw_mac_addr(pdev, hw, 0, mac_addr);
  condor_set_radio_hw_bssid(pdev, hw, 0, bssid);
#if (RADIO_NUM_ == 2)
  condor_set_radio_hw_mac_addr(pdev, hw, 1, mac_addr);
  condor_set_radio_hw_bssid(pdev, hw, 1, bssid);
#endif

  // EDCA 파라미터 셋 설정
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 4; j++)
    {
      condor_set_radio_hw_mac_edca(
        pdev,
        hw,
        0,  // radio_idx
        i,  // timeslot
        j,  // access category
        g_default_edca_param_set[j].cwmin, // cwmin
        g_default_edca_param_set[j].cwmax, // cwmax
        g_default_edca_param_set[j].aifsn); // aifsn
    }
  }
#if (RADIO_NUM_ == 2)
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 4; j++)
    {
      condor_set_radio_hw_mac_edca(
        pdev,
        hw,
        1,  // radio_idx
        i,  // timeslot
        j,  // access category
        g_default_edca_param_set[j].cwmin, // cwmin
        g_default_edca_param_set[j].cwmax, // cwmax
        g_default_edca_param_set[j].aifsn); // aifsn
    }
  }
#endif

  // RTS, Fragementation 임계값 설정
  condor_set_radio_hw_mac_rts_thr(pdev, hw, 0, 4095);
  condor_set_radio_hw_mac_frag_thr(pdev, hw, 0, 4095);
#if (RADIO_NUM_ == 2)
  condor_set_radio_hw_mac_rts_thr(pdev, hw, 1, 4095);
  condor_set_radio_hw_mac_frag_thr(pdev, hw, 1, 4095);
#endif
}


/**
 * @brief Condor PHY 하드웨어를 초기화한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 초기화 한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_init_phy_hw(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  u32 reg;
  u8 *iobase;
  // dev_info(&pdev->dev, "Initialize condor phy hardware\n");

  /*
   * 라디오별/시간슬롯 별 MAX2829 명령어 레지스터 로딩 및 전송
   */
  condor_init_rf_hw(pdev, hw);

  /*
   * AGC 초기화
   *  - max2829를 병렬 인터페이스로 제어하도록 설정
   *  - PHY 모뎀 AGC 활성화
   */
  // dev_info(&pdev->dev, "Initialize condor phy agc\n");
  condor_set_radio_hw_rf_parallel_control(pdev, hw, 0);
  iobase = hw->radio[0].cfg_iobase;
  reg = readl(iobase + REG_PHY_AGC_LIMIT);
  reg |= (1 << 24);
  writel(reg, iobase + REG_PHY_AGC_LIMIT);
#if (RADIO_NUM_ == 2)
  condor_set_radio_hw_rf_parallel_control(pdev, hw, 1);
  iobase = hw->radio[1].cfg_iobase;
  reg = readl(iobase + REG_PHY_AGC_LIMIT);
  reg |= (1 << 24);
  writel(reg, iobase + REG_PHY_AGC_LIMIT);
#endif
  // dev_info(&pdev->dev, "Success to initialize condor phy hardware\n");
}


/**
 * @brief RF 하드웨어를 초기화한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 초기화 한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_init_rf_hw(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  u32 reg;
  u8 *iobase;
  // dev_info(&pdev->dev, "Initialize rf hardware\n");

  /*
   * 라디오 0의 TimeSlot0 및 TimeSlot1에서 max2829 RF칩에 적용될 명령어들을
   * 레지스터에 저장해 둔다.
   *  - 저장해 둔 명령어들은 차후 send 명령 시, 또는 채널 스위칭 시점에
   *    자동으로 RF 칩으로 전송된다.
   */
  iobase = hw->radio[0].cfg_iobase;

  // TimeSlot0
  writel(MAX2829_REG_0_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_0);
  writel(MAX2829_REG_1_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_1);
  writel(MAX2829_REG_STANDBY_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_STANDBY);
  writel(MAX2829_REG_INTDIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_INTDIVRATIO);
  writel(MAX2829_REG_FRADIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_FRADIVRATIO);
  writel(MAX2829_REG_BANDSEL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_BANDSEL);
  writel(MAX2829_REG_CALIB_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_CALIB);
  writel(MAX2829_REG_LFILTER_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_LFILTER);
  writel(MAX2829_REG_RXCTRL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXCTRL);
  writel(MAX2829_REG_TXLINEAR_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_TXLINEAR);
  writel(MAX2829_REG_PA_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_PA);
  writel(MAX2829_REG_RXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXGAIN);
  writel(MAX2829_REG_TXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_TXGAIN);

  //  MAX2829로 전송될 명령어 레지스터의 범위를 모든 레지스터로 설정한다.
  //    - 즉, MAX2829로 명령어가 전송될 때
  //      모든 명령어 레지스터의 명령어들이 MAX2829로 전송된다.
  reg = readl(iobase + REG_RF_PARAMS_ADDR(0));
  reg &= ~((0x1f << 5) | (0x1f << 0));
  reg |= ((MAX2829_REG_TXGAIN_ADDR << 5) |
          (MAX2829_REG_0_ADDR << 0)); // -> RF_CCH_CMD0~RF_CCH_CMD12
  writel(reg, iobase + REG_RF_PARAMS_ADDR(0));

  // TimeSlot1
  writel(MAX2829_REG_0_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_0);
  writel(MAX2829_REG_1_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_1);
  writel(MAX2829_REG_STANDBY_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_STANDBY);
  writel(MAX2829_REG_INTDIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_INTDIVRATIO);
  writel(MAX2829_REG_FRADIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_FRADIVRATIO);
  writel(MAX2829_REG_BANDSEL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_BANDSEL);
  writel(MAX2829_REG_CALIB_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_CALIB);
  writel(MAX2829_REG_LFILTER_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_LFILTER);
  writel(MAX2829_REG_RXCTRL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXCTRL);
  writel(MAX2829_REG_TXLINEAR_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_TXLINEAR);
  writel(MAX2829_REG_PA_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_PA);
  writel(MAX2829_REG_RXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXGAIN);
  writel(MAX2829_REG_TXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_TXGAIN);

  //  MAX2829로 전송될 명령어 레지스터의 범위를 모든 레지스터로 설정한다.
  //    - 즉, MAX2829로 명령어가 전송될 때 모든 명령어 레지스터의 명령어들이
  //      MAX2829로 전송된다.
  reg = readl(iobase + REG_RF_PARAMS_ADDR(0));
  reg &= ~((0x1f << 15) | (0x1f << 10));
  reg |= (((MAX2829_REG_TXGAIN_ADDR | (1 << 4)) << 15) |
          ((MAX2829_REG_0_ADDR | (1 << 4)) << 10));
  writel(reg, iobase + REG_RF_PARAMS_ADDR(0));

  //  TimeSlot0의 명령어들을 MAX2829로 보내 초기화한다.
  //    - 이 시점에서는 timeSlot0이나 timeSlot1이나 동일한 설정이므로
  //      두 명령어 세트를 모두  MAX2829로 보낼 필요는 없다
  condor_send_radio_hw_rf_cmd(pdev, hw, 0, 0);


#if (RADIO_NUM_ == 2)
  /*
   * 라디오 1의 TimeSlot0 및 TimeSlot1에서 max2829 RF칩에 적용될 명령어들을
   * 레지스터에 저장해 둔다.
   *  - 저장해 둔 명령어들은 차후 send 명령 시, 또는 채널 스위칭 시점에
   *    자동으로 RF 칩으로 전송된다.
   */
  iobase = hw->radio[1].cfg_iobase;

  // TimeSlot0
  writel(MAX2829_REG_0_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_0);
  writel(MAX2829_REG_1_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_1);
  writel(MAX2829_REG_STANDBY_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_STANDBY);
  writel(MAX2829_REG_INTDIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_INTDIVRATIO);
  writel(MAX2829_REG_FRADIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_FRADIVRATIO);
  writel(MAX2829_REG_BANDSEL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_BANDSEL);
  writel(MAX2829_REG_CALIB_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_CALIB);
  writel(MAX2829_REG_LFILTER_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_LFILTER);
  writel(MAX2829_REG_RXCTRL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXCTRL);
  writel(MAX2829_REG_TXLINEAR_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_TXLINEAR);
  writel(MAX2829_REG_PA_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_PA);
  writel(MAX2829_REG_RXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXGAIN);
  writel(MAX2829_REG_TXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_TXGAIN);

  //  MAX2829로 전송될 명령어 레지스터의 범위를 모든 레지스터로 설정한다.
  //    - 즉, MAX2829로 명령어가 전송될 때
  //      모든 명령어 레지스터의 명령어들이 MAX2829로 전송된다.
  reg = readl(iobase + REG_RF_PARAMS_ADDR(0));
  reg &= ~((0x1f << 5) | (0x1f << 0));
  reg |= ((MAX2829_REG_TXGAIN_ADDR << 5) |
          (MAX2829_REG_0_ADDR << 0)); // -> RF_CCH_CMD0~RF_CCH_CMD12
  writel(reg, iobase + REG_RF_PARAMS_ADDR(0));

  // TimeSlot1
  writel(MAX2829_REG_0_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_0);
  writel(MAX2829_REG_1_DEFVAL, iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_1);
  writel(MAX2829_REG_STANDBY_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_STANDBY);
  writel(MAX2829_REG_INTDIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_INTDIVRATIO);
  writel(MAX2829_REG_FRADIVRATIO_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_FRADIVRATIO);
  writel(MAX2829_REG_BANDSEL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_BANDSEL);
  writel(MAX2829_REG_CALIB_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_CALIB);
  writel(MAX2829_REG_LFILTER_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_LFILTER);
  writel(MAX2829_REG_RXCTRL_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXCTRL);
  writel(MAX2829_REG_TXLINEAR_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_TXLINEAR);
  writel(MAX2829_REG_PA_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_PA);
  writel(MAX2829_REG_RXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXGAIN);
  writel(MAX2829_REG_TXGAIN_DEFVAL,
         iobase + REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_TXGAIN);

  //  MAX2829로 전송될 명령어 레지스터의 범위를 모든 레지스터로 설정한다.
  //    - 즉, MAX2829로 명령어가 전송될 때 모든 명령어 레지스터의 명령어들이
  //      MAX2829로 전송된다.
  reg = readl(iobase + REG_RF_PARAMS_ADDR(0));
  reg &= ~((0x1f << 15) | (0x1f << 10));
  reg |= (((MAX2829_REG_TXGAIN_ADDR | (1 << 4)) << 15) |
          ((MAX2829_REG_0_ADDR | (1 << 4)) << 10));
  writel(reg, iobase + REG_RF_PARAMS_ADDR(0));

  //  TimeSlot0의 명령어들을 MAX2829로 보내 초기화한다.
  //    - 이 시점에서는 timeSlot0이나 timeSlot1이나 동일한 설정이므로
  //      두 명령어 세트를 모두  MAX2829로 보낼 필요는 없다
  condor_send_radio_hw_rf_cmd(pdev, hw, 1, 0);
#endif
}


/**
 * @brief Condor 하드웨어의 인터럽트를 활성화 한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 초기화 한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_init_hw_intr(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  u8 *iobase;
  // dev_info(&pdev->dev, "Initialize condor hardware interrupt\n");

  // 모든 인터럽트 비활성화 및 펜딩 인터럽트 클리어
  iobase = hw->radio[0].cfg_iobase;
  writel(INTR_EN_ALL_DISABLED, iobase + REG_INTR_ENABLE);
  writel(INTR_CLEAR_ALL, iobase + REG_INTR_STATUS);
#if (RADIO_NUM_ == 2)
  iobase = hw->radio[1].cfg_iobase;
  writel(INTR_EN_ALL_DISABLED, iobase + REG_INTR_ENABLE);
  writel(INTR_CLEAR_ALL, iobase + REG_INTR_STATUS);
#endif
}


/**
 * @brief 각 라디오의 채널스위칭 설정을 초기화한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 초기화 한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_init_hw_chan_switching(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  // dev_info(&pdev->dev, "Initialize condor hardware channel switching config\n");

  // b0: 채널스위칭을 PPS에 동기화해서 수행하도록 설정
  // b1: 가상채널스위칭 타이머 on
  writel((1<<1) | (1<<0), hw->radio[0].cfg_iobase + REG_COORD_CTRL);
#if (RADIO_NUM_ == 2)
  writel((1<<1) | (1<<0), hw->radio[1].cfg_iobase + REG_COORD_CTRL);
#endif
}


/**
 * @brief Condor 하드웨어를 활성화 시킨다.
 *          모뎀 하드웨어를 시작하고 인터럽트를 활성화시킨다.
 *          비 다이버시티 모드로 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
void condor_activate_hw(struct platform_device *pdev, struct condor_hw *hw)
{
  // dev_info(&pdev->dev, "Activate condor hardware\n");
  condor_start_hw(pdev, hw);
  condor_activate_hw_intr(pdev, hw);
  condor_set_hw_no_diversity(pdev, hw);
}


/**
 * @brief Condor 하드웨어를 비활성화 시킨다.
 *          모뎀 하드웨어를 중지하고 인터럽트를 비활성화시킨다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
void condor_deactivate_hw(struct platform_device *pdev, struct condor_hw *hw)
{
  // dev_info(&pdev->dev, "Deactivate condor hardware\n");

  condor_deactivate_hw_intr(pdev, hw);
  condor_stop_hw(pdev, hw);
}


/**
 * @brief Condor 하드웨어의 동작을 시작한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 시작한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_start_hw(struct platform_device *pdev, struct condor_hw *hw)
{
  u32 reg = ENABLE_MAC | ENABLE_PHY_TX | ENABLE_PHY_RX;
  // dev_info(&pdev->dev, "Start condor hardware\n");
  writel(reg, hw->radio[0].cfg_iobase + REG_ENABLE);
#if (RADIO_NUM_ == 2)
  writel(reg, hw->radio[1].cfg_iobase + REG_ENABLE);
#endif
}


/**
 * @brief Condor 하드웨어의 동작을 중지한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 중지한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_stop_hw(struct platform_device *pdev, struct condor_hw *hw)
{
  // dev_info(&pdev->dev, "Stop condor hardware\n");
  writel(MAC_OP_ALL_DISABLE, hw->radio[0].cfg_iobase + REG_ENABLE);
#if (RADIO_NUM_ == 2)
  writel(MAC_OP_ALL_DISABLE, hw->radio[1].cfg_iobase + REG_ENABLE);
#endif
}


/**
 * @brief Condor 하드웨어의 인터럽트를 활성화 한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 활성화 한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_activate_hw_intr(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  u32 reg = INTR_EN_TRX_ENABLED | INTR_EN_CHAN_SW_ENABLED | INTR_EN_PPS |
            INTR_EN_TX_BUF_FULL_ERR;
  // dev_info(&pdev->dev, "Activate condor hardware interrupt - 0x%08X\n", reg);
  writel(reg, hw->radio[0].cfg_iobase + REG_INTR_ENABLE);
#if (RADIO_NUM_ == 2)
  writel(reg, hw->radio[1].cfg_iobase + REG_INTR_ENABLE);
#endif
}


/**
 * @brief Condor 하드웨어의 인터럽트를 비활성화 한다.
 *          하드웨어가 두개의 라디오를 지원하는 경우, 둘을 동일하게 비활성화 한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_deactivate_hw_intr(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  // dev_info(&pdev->dev, "Deactivate condor hardware interrupt\n");
  writel(INTR_EN_ALL_DISABLED, hw->radio[0].cfg_iobase + REG_INTR_ENABLE);
#if (RADIO_NUM_ == 2)
  writel(INTR_EN_ALL_DISABLED, hw->radio[1].cfg_iobase + REG_INTR_ENABLE);
#endif
}


/**
 * @brief Condor 하드웨어를 비 다이버시티 모드로 설정한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param hw (입력) Condor 하드웨어 정보
 */
static void condor_set_hw_no_diversity(
  struct platform_device *pdev,
  struct condor_hw *hw)
{
  u32 reg;
  // dev_info(&pdev->dev, "Set condor hardware no diversity\n");

  reg = readl(hw->radio[0].cfg_iobase + REG_DIVERSITY);
  reg &= ~3;
  writel(reg, hw->radio[0].cfg_iobase + REG_DIVERSITY);
#if (RADIO_NUM_ == 2)
  reg = readl(hw->radio[1].cfg_iobase + REG_DIVERSITY);
  reg &= ~3;
  writel(reg, hw->radio[1].cfg_iobase + REG_DIVERSITY);
#endif
}
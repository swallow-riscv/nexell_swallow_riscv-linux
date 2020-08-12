//
// Created by gyun on 2019-03-24.
//

#include "condor.h"
#include "condor_dev.h"

/**
 * @brief 채널접속을 수행한다.
 *          ts0_chan, ts1_chan이 동일할 경우 continuous 접속을 수행한다.
 *          ts0_chan, ts1_chan이 다를 경우 alternating 접속을 수행한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @param radio_idx (입력) 라디오 식별자(0,1)
 * @param ts0_chan (입력) 시간슬롯0 채널
 * @param ts1_chan (입력) 시간슬롯1 채널
 * @return 성공시 0, 실패시 -1
 */
int condor_access_channel(
    struct platform_device *pdev,
    unsigned int radio_idx,
    unsigned char ts0_chan,
    unsigned char ts1_chan)
{
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    struct condor_hw *hw = &pdata->hw;
    // dev_info(&pdev->dev, "Accessing radio%u channel - ts0:%u, ts1:%u\n", radio_idx, ts0_chan, ts1_chan);

    /*
   * Continuous 접속
   *  1. 채널스위칭 중지
   *  2. 채널접속
   */
    if (ts0_chan == ts1_chan)
    {
        condor_stop_radio_hw_chan_switching(pdev, hw, radio_idx);
        if (condor_access_radio_hw_chan(pdev, hw, radio_idx, ts0_chan))
        {
            return -1;
        }
    }
    /*
   * Alternating 접속
   */
    else
    {
        if (condor_start_radio_hw_chan_switching(pdev,
                                                 hw,
                                                 radio_idx,
                                                 ts0_chan,
                                                 ts1_chan))
        {
            return -1;
        }
    }

    pdata->current_channel = ts1_chan;
    // dev_info(&pdev->dev, "Success to access channel\n");
    return 0;
}

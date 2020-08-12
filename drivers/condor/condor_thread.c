#include "condor_dev.h"

/**
 * @brief Condor 쓰레드들을 초기화한다.
 * @param
 * @return 성공시 0, 실패시 -1
 * */
int32_t init_condor_threads(void)
{
    struct platform_device *pdev = g_pdev;
    int32_t result;

    // dev_info(&pdev->dev, "Trying to initialize threads\n");

    /* 각 쓰레드들을 초기화 */
    /* 수신 패킷 처리 쓰레드 */
    condor_init_rx_thread();
    if (result < 0)
        goto fail;

#ifdef _CONDOR_USE_TXTHREAD_
    /* 송신 패킷 처리 쓰레드 */
    result = init_condor_tx_thread();
    if (result < 0)
        goto fail;
#endif
    /* WAE-notification 처리 쓰레드 */
    /* -------------------------- */

    // dev_info(&pdev->dev, "Success to init threads\n");
    return 0;

fail:
#ifdef _CONDOR_USE_TXTHREAD_
    release_condor_tx_thread();
#endif
    return result;
}

/**
 * @brief Condor 쓰레드들을 해제한다.
 * @param
 * @return 
 * */
void release_condor_threads(void)
{
    struct platform_device *pdev = g_pdev;

    // dev_info(&pdev->dev, "Releasing threads\n");

    /* 각 쓰레드들을 해제 */
    /* 송신 패킷 처리 쓰레드 해제 */
#ifdef _CONDOR_USE_TXTHREAD_
    release_condor_tx_thread();
#endif
    /* 수신 패킷 처리 쓰레드 해제 */
    condor_release_rx_thread();
    /* WAE-notification 처리 쓰레드 해제 */
    /* ------------------------------ */
}
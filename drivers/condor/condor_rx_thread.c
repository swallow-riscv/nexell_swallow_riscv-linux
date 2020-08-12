#include "condor_dev.h"

static int32_t condor_process_rx_thread(void *none);

/**
 * @brief 수신 패킷 처리 쓰레드를 초기화한다.
 * @param 
 * @return 성공시 0, 실패시 -1
 * */
int32_t condor_init_rx_thread(void) 
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    // dev_info(&pdev->dev, "Trying to initialize CONDOR rx thread\n");

    /* 수신패킷 처리 쓰레드 생성 */
    pdata->rx_thread = kthread_run(condor_process_rx_thread, NULL, "condor_rx_thread");
    if (!pdata->rx_thread)
    {
        // dev_err(&pdev->dev, "Fail to run rx thread\n");
        return -1;
    }

    /* 수신패킷 처리 대기 큐 초기화 */
    init_waitqueue_head(&pdata->rx_wait);

    // dev_info(&pdev->dev, "Success to initialize rx thread\n");
    return 0;
}

/**
 * @brief 수신패킷 처리 쓰레드를 해제한다.
 * @param 
 * @return
 * */
void condor_release_rx_thread(void)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    // dev_info(&pdev->dev, "Releasing CONDOR rx thread\n");
    if (pdata->rx_thread)
    {
        kthread_stop(pdata->rx_thread);
        pdata->rx_thread = NULL;
    }
}

/**
 * @brief 수신패킷 처리 쓰레드 수행 함수
 * @param none 사용 안함(kthread_run()함수에서 NULL을 전달하였으므로)
 * @return 0
 * */
static int32_t condor_process_rx_thread(void *none)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    int32_t result;
    bool is_skb_free;
    unsigned long flags;
    struct sk_buff *skb;
    condorRxQueue_t *rxq = &pdata->rxq;
    rxMetaData_t rx_meta;

    /* 루프를 돌면서 수신 큐에서 패킷을 꺼내 처린한다. */
    while (1)
    {
        /**
         * 수신큐에 패킷이 생길 때까지 대기한다.
         *  - rx_pending은 다음과 같은 경우에 참이 된다.
         *     - 인터럽트서비스루틴에서 수싵큐에 패킷이 삽입되었을 때
         * */
        if (!pdata->rx_pending)
            wait_event_interruptible(pdata->rx_wait, pdata->rx_pending);
        
        /**
         * 윗 줄과 아랫 줄 사이 동안에 큐에 패킷이 삽딥되면 rx_pending = false 상태로 stuck 되지 않는지?
         *  - 어차피 아래 루틴에서 큐에 있는 모든 패킷을 모두 처리한다.
         * */
        pdata->rx_pending = false;

        /* 수신큐에서 패킷을 꺼내 처린한다.(큐 내의 모든 패킷을 처리한다.) */
        do {
            /* 큐에서 패킷을 꺼낸다. */
            spin_lock_irqsave(&pdata->rx_lock, flags);
            skb = pop_condor_rx_queue(rxq, &rx_meta, &result);
            spin_unlock_irqrestore(&pdata->rx_lock, flags);

            /* 패킷이 없으면 종료 */
            if (!skb) break;

            /* 수신 패킷을 처리한다. */
            is_skb_free = true;
            result = condor_process_rx_mpdu(skb, &rx_meta, &is_skb_free);
            if (result < 0) 
                pdata->netstats.rx_drop_cnt++;
            if (is_skb_free)
                dev_kfree_skb(skb);
        } while(1);

    }
}
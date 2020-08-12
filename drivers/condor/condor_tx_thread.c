#include "condor_dev.h"
#include "condor_hw_regs.h"

#ifdef _CONDOR_USD_TXTHREAD_

static int32_t process_condor_tx_thread(void *none);

/**
 * @brief 송신 패킷 처리 쓰레드를 초기화한다.
 * @param
 * @return 성공시 0, 실패시 -1
 * */
int32_t init_condor_tx_thread(void)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    /* 송신 패킷 처리 쓰레드 생성 */
    pdata->tx_thread = kthread_run(process_condor_tx_thread, NULL, "condor_tx_thread");
    if (!pdata->tx_thread)
    {
        // dev_err(&pdev->dev, "Fail to run tx kthread\n");
        return -1;
    }

    /* 송신 패킷 처리 대기 큐 초기화 */
    init_waitqueue_head(&pdata->tx_wait);

    // dev_info(&pdev->dev, "Success to initialize tx kthread\n");
    return 0;
}

/**
 * @brief 송신 패킷 처리 쓰레드를 해제한다.
 * @param
 * @return
 * */
void release_condor_tx_thread(void)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    // dev_info(&pdev->dev, "Releasing tx thread\n");
    if (pdata->tx_thread)
    {
        kthread_stop(pdata->tx_thread);
        pdata->tx_thread = NULL;
    }
}

/**
 * @brief 송신 패킷 처리 쓰레드 수행 함수
 * 송신버퍼에 패킷이 생기면, 하드웨어 버퍼에 복사한다.(하드웨어 버퍼가 비어있는 경우에만)
 * @param none 사용 안함 (ktrhead_run() 함수에서 NULL을 전달하였으므로)
 * @return 0
 * */
static int32_t process_condor_tx_thread(void *none)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    int32_t result, i, j;
    int hw_buffer_pkt_num, start_block_index, block_num;
    unsigned long flags;
    netIfTxQueueIndexRange q_index;

    struct condor_hw *hw;
    struct sk_buff *skb;
    condorNetIfTxQueue_t *txq;
    condorNetIfTxQueueEntry_t *entry;

    /* 루프를 돌면서 수신 큐에서 패킷을 꺼내 처리한다. */
    while (1)
    {
        /**
         * 송신큐에 패킷이 생길 때까지 대기한다.
         * tx_pending은 다음과 같은 경우에 참이 된다.
         * - 송신 루틴에서 송신큐에 패킷이 삽입되었을 때
         * - 송신인터럽트가 발생했을 때
         * */
        if (pdata->tx_pending == false)
            wait_event_interruptible(pdata->tx_wait, pdata->tx_pending);
        /**
         * 윗 줄과 아랫 줄 사이 동안에 큐에 패킷이 삽입되면 tx_pending = false 상태로 stuck되지 않는지?
         * 어차피 인터럽트에서 다시 tx_pending이 true로 설정되므로 다시 깨어날 수 있다.
         * */
        pdata->tx_pending = false;

        /**
         * 송신큐에서 패킷이 있는지 확인해서,
         * 하드웨어 큐가 idle하면, 송신큐에서 패킷을 꺼내서 하드웨어 큐에 삽입한다.
         * */
        spin_lock_irqsave(&pdata->tx_lock, flags);
        hw = &pdata->hw;
        for (i = 0; i < WAVE_TIMESLOT_NUM; i++)
        {
            for (j = (dot11AcNum - 1); j >= 0; j--)
            {
                q_index = (i * dot11AcNum) + j;
                txq = &pdata->txq[q_index];

                /* SW큐에 패킷이 없으면 다음 큐로 넘어간다. */
                entry = TAILQ_FIRST(&txq->head);
                if (!entry)
                {
                    // dev_info(&pdev->dev, "There is no more packet in tx queue[%d]\n", q_index);
                    continue;
                }
                skb = entry->skb;

                /* 관련 HW버퍼 내의 패킷 개수를 확인한다. */
                hw_buffer_pkt_num = get_hw_buffer_pkt_num(hw, q_index);

                /**
                 * HW버퍼 내의 패킷이 없으면 HW버퍼에 패킷을 쓴다.
                 * HW버퍼에 공간이 있는 경우 가능하며, 공간이 없는 경우에는 종료한다.
                 * 공간이 없으면 어차피 다음 SW큐의 패킷도 처리 못하므로 루틴을 완전 종료한다.
                 * */
                if (hw_buffer_pkt_num == 0)
                {
                    block_num = (skb->len % TX_UNIT_BLOCK_SIZE) ? ((skb->len / TX_UNIT_BLOCK_SIZE) + 1) : (skb->len / TX_UNIT_BLOCK_SIZE);
                    start_block_index = check_hw_buffer_space(hw, block_num);
                    if (start_block_index < 0)
                    {
                        // dev_info(&pdev->dev, "No packet in H/W buffer[%d][%d], but no space - break routine\n", i, j);
                        break;
                    }
                    else
                    {
                        /* SW큐에서 패킷을 꺼내서 전송한다. */
                        skb = pop_condor_net_if_tx_queue(txq, &result);
                        // dev_info(&pdev->dev, "No packet in H/W buffer[%d][%d], push H/W queue - start_block_index: %d, block_num: %d\n", i, j, start_block_index, block_num);
                        transmit_hw_pkt(hw, skb, q_index, start_block_index, block_num);
                        dev_kfree_skb(skb);

                        /**
                         * TxProfile이 등록되어 있고, 큐가 많이 비어있으면 상위계층네트워크 큐를 재개한다.
                         * (중지되어 있지 않을 수도 있다. result == 1 almostEmpty)
                         * */
                        /* ----------------------- */
                    }
                }
                /**
                 * H/W버퍼 내에 패킷이 1개 있으면 AC_V0인 경우에만 H/W버퍼에 패킷을 쓴다.
                 * 이 기능은 최상위 AC에 대해서만 듀얼버퍼의 기능을 제공하는 것이며,
                 * 최대 Throughput 성능을 측정할 때 사용될 수 있다.
                 * H/W버퍼에 공간이 있는 경우 가능하며, 공간이 없는 경우에는 종료한다.
                 * 공간이 없으면 어차피 다음 SW큐의 패킷도 처리 못하므로 루틴을 완전 종료한다.
                 * */
                else if ((hw_buffer_pkt_num == 1) && (j == dot11Aci_vo))
                {
                    block_num = (skb->len % TX_UNIT_BLOCK_SIZE) ? ((skb->len / TX_UNIT_BLOCK_SIZE) + 1) : (skb->len / TX_UNIT_BLOCK_SIZE);
                    start_block_index = check_hw_buffer_space(hw, block_num);
                    if (start_block_index < 0)
                    {
                        // dev_info(&pdev->dev, "1 packet in H/W buffer[%d][%d], but no space - break routine\n", i, j);
                        break;
                    }
                    else 
                    {
                        /* SW큐에서 패킷을 꺼내서 전송한다. */
                        skb = pop_condor_net_if_tx_queue(txq, &result);
                        // dev_info(&pdev->dev, "1 packet in H/W buffer[%d][%d], push H/W queue - start_block_index: %d, block_num: %d\n", i, j, start_block_index, block_num);
                        transmit_hw_pkt(hw, skb, q_index, start_block_index, block_num);
                        dev_kfree_skb(skb);

                        /**
                         * TxProfile이 등록되어 있고, 큐가 많이 비어있으면 상위계층네트워크 큐를 재개한다.
                         * (중지되어 있지 않을 수도 있다. result == 1 almostEmpty)
                         * */
                        /* ----------------------- */
                    }
                }
                else 
                    // dev_info(&pdev->dev, "%d packets in H/W buffer[%d][%d] - do nothing\n", hw_buffer_pkt_num, i, j);
            }
        }
        spin_unlock_irqrestore(&pdata->tx_lock, flags);
    }
    return 0;
}
#endif
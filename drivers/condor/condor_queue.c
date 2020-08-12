#include "condor_dev.h"

/**
 * @brief Condor의 모든 송신큐들을 초기화 한다.
 * @param
 * @return 
 */
void init_all_condor_tx_queues(void)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    int32_t i;
    // dev_info(&pdev->dev, "Initializing condor's tx queues\n");

    /* 네트워크인터페이스의 각 송신큐들을 초기화한다. */
    for (i = 0; i < 1/*CONDOR_PHY_TX_QUEUE_NUM*/; i++)
        init_condor_tx_queue(&pdata->txq[i], i);
}

/**
 * @brief Condor 내 각 송신큐를 초기화 한다.
 * @param txq 초기화할 송신큐
 * @apram q_index 이 송신큐의 번호 (동일 네트워크인터페이스에 할당된 8개 버퍼 사이에서의 번호)
 * @return 
 */
void init_condor_tx_queue(condorNetIfTxQueue_t *txq, netIfTxQueueIndexRange q_index)
{
    struct platform_device *pdev = g_pdev;

    // dev_info(&pdev->dev, "Initializing condor's txq[%d]\n", q_index);

    memset(txq, 0, sizeof(condorNetIfTxQueue_t));
    txq->netIfIndex = 0;
    txq->qIndex = q_index;
    txq->maxPktNum = CONDOR_TXQ_PKT_MAXNUM;
    txq->almostFullPktNum = CONDOR_TXQ_ALMOST_FULL_PKTNUM;
    txq->almostEmptyPktNum = CONDOR_TXQ_ALMOST_EMPTY_PKTNUM;
    TAILQ_INIT(&txq->head);
}

/**
 * @brief Condor 내 모든 송신큐들을 비운다.
 * @param
 * @return
 * */
void flush_all_condor_tx_queues(void)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    int32_t i;

    // dev_info(&pdev->dev, "Flushing condor's tx queues\n");

    for (i = 0; i < 1/*CONDOR_PHY_TX_QUEUE_NUM*/; i++)
        flush_condor_tx_queue(&pdata->txq[i]);
}

/**
 * @brief Condor 내 각 네트워크인터페이스의 송신큐들을 비운다.
 * @param
 * @return
 * */
void flush_condor_tx_queue(condorNetIfTxQueue_t *txq)
{
    struct platform_device *pdev = g_pdev;
    condorNetIfTxQueueEntry_t *entry, *tmp;

    // dev_info(&pdev->dev, "Flushing condor's tx queue[%d]\n", txq->qIndex);

    /* 송신 큐 내의 모든 엔트리를 삭제한다. */
    TAILQ_FOREACH_SAFE(entry, &txq->head, entries, tmp)
    {
        TAILQ_REMOVE(&txq->head, entry, entries);
        if (entry->skb)
            dev_kfree_skb_any(entry->skb);
        kfree(entry);
    }
    txq->pktNum = 0;
}
/**
 * @brief Condor 내 송신큐에 패킷을 삽입한다.
 * @param txq 패킷을 삽입할 송신큐
 * @param skb 삽입할 PPDU가 포함된 소켓버퍼
 * @param tx_meta 송신 메타데이터
 * @return almostfull 1, 성공인 경우 0, 실패인 경우 -1
 * */
int32_t push_condor_net_if_tx_queue(condorNetIfTxQueue_t *txq, struct sk_buff *skb, txMetaData_t *tx_meta)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    condorNetIfTxQueueEntry_t *entry;

    // dev_info(&pdev->dev, "Trying to push %d-bytes PPDU into condor's tx queue[%d]\n", skb->len, txq->qIndex);

    /* 큐 오버플로우 확인 */
    if (txq->pktNum >= txq->maxPktNum)
    {
        // dev_err(&pdev->dev, "Queue full\n");
        pdata->netstats.tx_queue_full_cnt[txq->qIndex]++;
        txq->queueFullCnt++;
        return -1;
    }

    /* 큐 엔트리 할당 */
    entry = (condorNetIfTxQueueEntry_t *)kmalloc(sizeof(condorNetIfTxQueueEntry_t), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (!entry)
    {
        dev_err(&pdev->dev, "No memory for entry\n");
        return -1;
    }

    memset(entry, 0, sizeof(condorNetIfTxQueueEntry_t));

    /* 엔트리에 정보 저장 */
    entry->skb = skb;
    memcpy(&entry->meta, tx_meta, sizeof(txMetaData_t));

    /* 엔트리를 큐에 삽입 */
    /* Debug */
    // dev_info(&pdev->dev, "22. Push Queue");
    TAILQ_INSERT_TAIL(&txq->head, entry, entries);
    txq->pktNum++;

    /* 임계점 확인 */
    if (txq->pktNum >= txq->almostFullPktNum)
        return 1;

    // dev_info(&pdev->dev, "%u in queue", pdata->txq[0].pktNum);
    pdata->tx_pending = true;
    // if (pdata->tx_pending)
    //     wake_up(&pdata->tx_wait);
        
    // dev_info(&pdev->dev, "Success to push PPDU tx queue\n");
    return 0;
}

/**
 * @brief Condor 내 송신큐의 제일 앞 패킷을 꺼낸다.
 * 패킷이 만기되었으면 폐기하고 NULL을 반환한다.
 * @param txq 패킷을 꺼낼 송신큐
 * @param result 결과가 저장될 변수(almost empty 1, 성공시 0, 실패시 -1)
 * @return 꺼낸 패키시 포함된 소켓버퍼에 대한 포인터
 * */
struct sk_buff *pop_condor_net_if_tx_queue(condorNetIfTxQueue_t *txq, int32_t *result)
{
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;

    struct condor_hw *hw;
    condorNetIfTxQueueEntry_t *entry;
    uint64_t current_tsf;
    struct sk_buff *skb;

    // dev_info(&pdev->dev, "Trying to pop packet from tx queue[%d]\n", txq->qIndex);

    /* 큐 언더플로우 확인 */
    if (txq->pktNum == 0)
    {
        // dev_info(&pdev->dev, "Queue empty\n");
        *result = -1;
        return NULL;
    }

    /* 큐에서 엔트리를 꺼낸다. */
    entry = TAILQ_FIRST(&txq->head);
    if (!entry)
    {
        // dev_err(&pdev->dev, "Queue empty - cannot be here\n");
        *result = -1;
        txq->pktNum = 0;
        return NULL;
    }

    /* Debug */
    // dev_info(&pdev->dev, "333. Pop queue");
    TAILQ_REMOVE(&txq->head, entry, entries);
    txq->pktNum--;

    hw = &pdata->hw;

    /* 패킷 만기시각 확인 */
    /* 저장된 만기시각이 0이 아니면, 현재시점의 TSF 값과 비교하여 만기시각이 작은 경우(=과거) 패킷을 폐기한다. */
    if (entry->meta.expiry)
    {
        current_tsf = condor_get_hw_tsf_time(hw);
        if (entry->meta.expiry < current_tsf)
        {
            // dev_info(&pdev->dev, "Packet is expired\n");
            *result = -1;
            if (entry->skb)
                dev_kfree_skb_any(entry->skb);
            kfree(entry);
            return NULL;
        }
    }

    /* 꺼낸 패킷 반환 */
    skb = entry->skb;
    kfree(entry);

    /* 임계점 확인 */
    if (txq->pktNum <= txq->almostEmptyPktNum)
        *result = 1;
    else 
        *result = 0;
    
    return skb;
}

/**
 * @brief 수신큐를 초기화 한다.
 * @param rxq 초기화할 수신큐
 * @return
 * */
void init_condor_rx_queue(condorRxQueue_t *rxq)
{
    struct platform_device *pdev = g_pdev;
    
    // dev_info(&pdev->dev, "Initializing CONDOR rx queue\n");

    memset(rxq, 0, sizeof(condorRxQueue_t));
    rxq->maxPktNum = CONDOR_RXQ_PKT_MAXNUM;
    TAILQ_INIT(&rxq->head);
}

/**
 * @brief 수신큐 내의 모든 패킷을 삭제한다.
 * @param rxq 삭제할 수신큐
 * @return
 * */
void flush_condor_rx_queue(condorRxQueue_t *rxq)
{
    struct platform_device *pdev = g_pdev;
    condorRxQueueEntry_t *entry, *tmp;

    // dev_info(&pdev->dev, "Flushing CONDOR rx queue\n");

    /* 수신 큐 내의 모든 엔트리를 삭제한다. */
    TAILQ_FOREACH_SAFE(entry, &rxq->head, entries, tmp) {
        TAILQ_REMOVE(&rxq->head, entry, entries);
        if (entry->skb)
            dev_kfree_skb_any(entry->skb);
        kfree(entry);
    }
    rxq->pktNum = 0;
}

/**
 * @brief 수신큐가 비어있는지 확인한다.
 * @param rxq 확인할 수신큐
 * @return true 비어있음, false 비어있지않음
 * */
bool is_empty_condor_rx_queue(condorRxQueue_t *rxq)
{
    if (rxq->pktNum == 0) return true;
    else return false;
}

/**
 * @brief 수신큐에 패킷을 삽입한다.
 * @param rxq 패킷을 삽입할 rxq
 * @param skb 삽입할 패킷(소켓 버퍼 형식)
 * @param rx_meta 수신 메타 데이터
 * @return 성공시 0, 실패시 음수(기본: -1, 오버플로우: -2)
 * */
int32_t push_condor_rx_queue(condorRxQueue_t *rxq, struct sk_buff *skb, rxMetaData_t *rx_meta)
{
    struct platform_device *pdev = g_pdev;
    condorRxQueueEntry_t *entry;

    // dev_info(&pdev->dev, "Trying to push packet into rx queue\n");

    /* 큐 오버플로우 확인 */
    if (rxq->pktNum >= rxq->maxPktNum) {
        // dev_err(&pdev->dev, "queue full\n");
        return -2;
    }

    /* 큐 엔트리 할당 */
    entry = (condorRxQueueEntry_t *)kmalloc(sizeof(condorRxQueueEntry_t), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (!entry)
    {
        // dev_err(&pdev->dev, "No memory for entry\n");
        return -1;
    }
    memset(entry, 0, sizeof(condorRxQueueEntry_t));

    /* 엔트리 정보 저장 */
    entry->skb = skb;
    memcpy(&entry->meta, rx_meta, sizeof(rxMetaData_t));

    /* 엔트리를 큐에 삽입 */
    TAILQ_INSERT_TAIL(&rxq->head, entry, entries);
    rxq->pktNum++;

    return 0;
}

/**
 * @brief 수신큐에서 패킷을 꺼낸다.
 * @param rxq 패킷을 꺼낼 수신큐
 * @param rx_meta 수신 메타데이터가 저장된 변수
 * @param result 결과가 저장될 변수
 * @return 꺼낸 패킷에 대한 포인터(소켓 버퍼 형식)
 * */
struct sk_buff* pop_condor_rx_queue(condorRxQueue_t *rxq, rxMetaData_t *rx_meta, int32_t *result)
{
    struct platform_device *pdev = g_pdev;
    condorRxQueueEntry_t *entry;
    struct sk_buff *skb;

    // dev_info(&pdev->dev, "Trying to pop packet from rx queue\n");

    /* 수신 큐 언더플로우 확인 */
    if (rxq->pktNum == 0) {
        // dev_err(&pdev->dev, "Queue empty");
        return NULL;
    }

    /* 수신 큐에서 엔트리를 꺼낸다. */
    entry = TAILQ_FIRST(&rxq->head);
    if(!entry)
    {
        // dev_err(&pdev->dev, "Queue empty - cannot be here\n");
        *result = -1;
        rxq->pktNum = 0;
        return NULL;
    }
    TAILQ_REMOVE(&rxq->head, entry, entries);
    rxq->pktNum--;

    /* 꺼낸 패킷 반환 */
    skb = entry->skb;
    memcpy(rx_meta, &entry->meta, sizeof(rxMetaData_t));
    kfree(entry);

    return skb;
}
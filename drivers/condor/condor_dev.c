//
// Created by gyun on 2019-03-22.
//

#include "condor.h"
#include "condor_dev.h"
#include "condor_hw_regs.h"

struct platform_device *g_pdev;

/**
 * @brief Condor 플랫폼 드라이버 정보
 */
static int condor_probe(struct platform_device *pdev);
static int condor_remove(struct platform_device *pdev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)

static struct platform_driver condor_driver = {
    .driver = {
        .name = DEV_NAME,
    },
    .probe = condor_probe,
    .remove = __devexit_p(condor_remove),
};

#else // LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)

static struct condor_platform_data g_pdata;

/**
 * @brief Condor 정보를 디바이스 트리에서 탐색하기 위한 정보
 *          compatible이 일치하는 정보를 탐색한다.
 */
static const struct of_device_id condor_match_table[] = {
    {
        .compatible = "keti,keti-c2dl",
        .data = &g_pdata,
    },
    {},
};
MODULE_DEVICE_TABLE(of, condor_match_table);

/**
 * @brief Condor driver information
 */
static struct platform_driver condor_driver = {
    .driver = {
        .name = DEV_NAME,
        .of_match_table = condor_match_table,
    },
    .probe = condor_probe,
    .remove = condor_remove,
};

#endif // LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)

static irqreturn_t condor_isr(int irq, void *dev_id);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)

/**
 * @brief 플랫폼 드라이버의 .probe 콜백함수
 *          Condor 디바이스를 초기화한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @return 성공 시 0, 실패 시 음수
 */
static int condor_probe(struct platform_device *pdev)
{

    int ret = -ENODEV;
    struct resource *mem;
    u32 mem_size;
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    struct condor_netdev *cndev;
    struct net_device *ndev;
    // dev_info(&pdev->dev, "Probing condor\n");

    /*
   * 네트워크 디바이스를 초기화한다.
   */
    cndev = condor_init_netdev(pdev);
    if (!cndev)
    {
        goto req_err;
    }
    pdata->cndev = cndev;
    ndev = cndev->ndev;
    platform_set_drvdata(pdev, cndev);

    /*
   * 디바이스 트리로부터 메모리영역 정보를 가져 와서 메모리 접근을 요청한다.
   */
    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem)
    {
        // dev_err(&pdev->dev, "Fail to platform_get_resource(IORESOURCE_MEM)\n");
        goto req_err;
    }
    mem_size = mem->end - mem->start + 1;
    match->data if (!request_mem_region(mem->start, mem_size, pdev->dev.driver->name))
    {
        // dev_err(&pdev->dev, "resource available\n");
        goto req_err;
    }

    /*
   * 하드웨어를 초기화한다.
   */
    pdata->hw.iobase = (u32)mem->start;
    if (condor_init_hw(pdev))
    {
        goto map_err;
    }

    /*
   * 네트워크 디바이스에 MAC 주소를 설정한다.
   */
    condor_set_netdev_mac_addr(pdev, ndev, pdata->hw.mac_addr);

    //  pdata->hw.io_start = mem->start;
    //  pdata->hw.io_end = mem->end;
    //  ndev->base_addr = (unsigned long) ioremap_nocache(mem->start, mem_size);
    //  if (!ndev->base_addr)
    //  {
    //    // dev_err(&pdev->dev, "Fail to ioremap_nocache()\n");
    //    ret = -ENOMEM;
    //    goto map_err;
    //  }

    /*
   * 디바이스 트리로부터 인터럽트 정보를 가져 와서 커널에 등록한다.
   */
    pdata->irq = platform_get_irq(pdev, 0);
    if (!(pdata->irq))
    {
        // dev_err(&pdev->dev, "Fail to platform_get_irq()\n");
        ret = -ENODEV;
        goto map_err;
    }
    ndev->irq = pdata->irq;
    // 인터럽트 등록
    ret = request_irq(
        pdata->irq,                     // 인터럽트 번호
        condor_isr,                     // 인터럽트 서비스 루틴
        IRQF_SHARED | IRQF_TRIGGER_LOW, // 인터럽트 특성
        condor_driver.driver.name,      // 디바이스 이름
        pdata);                         // Condor 플랫폼데이터 (ISR에서 전달된다)
    if (ret)
    {
        // dev_err(&pdev->dev, "Fail to request_irq(%d)\n", pdata->irq);
        goto map_err;
    }
    // dev_info(&pdev->dev, "Success to request_irq(%d)\n", pdata->irq);

    /*
   * 네트워크 디바이스를 커널에 등록한다.
   */
    if (condor_register_netdev(pdev, cndev))
    {
        goto map_err;
    }

    /* 기타 주요 기능 초기화 */
    spin_lock_init(&pdata->lock);
    spin_lock_init(&pdata->tx_lock);
    init_all_condor_tx_queues();

    /* 쓰레드 초기화 */
    init_condor_threads();

    // dev_info(&pdev->dev, "Success to probe\n");
    return 0;

map_err:
    release_mem_region(mem->start, mem_size);

req_err:
    // dev_err(&pdev->dev, "Fail to probe\n");
    return ret;
}

#else // #if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)

/**
 * @brief 플랫폼 드라이버의 .probe 콜백함수
 *          Condor 디바이스를 초기화한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @return 성공 시 0, 실패 시 음수
 */
static int condor_probe(struct platform_device *pdev)
{
    int ret;
    u32 mem_size;
    struct resource res;
    const struct of_device_id *match;
    struct condor_platform_data *pdata;
    struct condor_netdev *cndev;
    struct net_device *ndev;
    struct device_node *node = pdev->dev.of_node;

    // dev_info(&pdev->dev, "Probing condor\n");

    // Searching matched device
    match = of_match_device(condor_match_table, &pdev->dev);
    if (!match)
    {
        // dev_err(&pdev->dev, "Fail to of_match_device()\n");
        return -EINVAL;
    }
    // dev_info(&pdev->dev, "Success to of_match_device()\n");

    // Condor 전용 플랫폼데이터를 플랫폼 디바이스에 추가한다.
    //  match->data = g_pdata; (referenced in condor_match_table)
    if (platform_device_add_data(pdev, match->data, sizeof(struct condor_platform_data)))
    {
        // dev_err(&pdev->dev, "Fail to platform_device_add_data()\n");
        return -EINVAL;
    }
    // dev_info(&pdev->dev, "Success to platform_device_add_data()\n");
    pdata = pdev->dev.platform_data;

    // Get address range
    if (of_address_to_resource(node, 0, &res))
    {
        // dev_err(&pdev->dev, "Fail to of_address_to_resource()\n");
        return -EINVAL;
    }
    // dev_info(&pdev->dev, "Success to of_address_to_resource() - start: 0x%08x, end: 0x%08x\n", (int)res.start, (int)res.end);

    // Find irq number
    pdata->irq = irq_of_parse_and_map(node, 0);
    if (!pdata->irq)
    {
        // dev_err(&pdev->dev, "No irq found\n");
        return -ENODEV;
    }
    // dev_info(&pdev->dev, "Success to irq_of_parse_and_map() - %d\n", pdata->irq);

    // Request memory region
    mem_size = res.end - res.start + 1;
    if (!request_mem_region(res.start, mem_size, pdev->dev.driver->name))
    {
        // dev_err(&pdev->dev, "resource available\n");
        return -EINVAL;
    }
    // dev_info(&pdev->dev, "Success to request_mem_region()\n");

    /*
   * 네트워크 디바이스를 초기화한다.
   */
    cndev = condor_init_netdev(pdev);
    if (!cndev)
    {
        return -EINVAL;
    }
    pdata->cndev = cndev;
    ndev = cndev->ndev;
    platform_set_drvdata(pdev, cndev);

    /*
   * 하드웨어를 초기화한다.
   */
    pdata->hw.iobase = (u32)res.start;
    if (condor_init_hw(pdev))
    {
        return -EINVAL;
    }

    /*
   * 네트워크 디바이스에 MAC 주소를 설정한다.
   */
    condor_set_netdev_mac_addr(pdev, ndev, pdata->hw.mac_addr);

    /*
   * 인터럽트를 커널에 등록한다.
   */
    ndev->irq = pdata->irq;
    ret = request_irq(
        pdata->irq,                     // 인터럽트 번호
        condor_isr,                     // 인터럽트 서비스 루틴
        IRQF_SHARED | IRQF_TRIGGER_LOW, // 인터럽트 특성
        condor_driver.driver.name,      // 디바이스 이름
        pdata);                         // Condor 플랫폼데이터 (ISR에서 전달된다)
    if (ret)
    {
        // dev_err(&pdev->dev, "Fail to request_irq(%d)\n", pdata->irq);
        return -EINVAL;
    }
    // dev_info(&pdev->dev, "Success to request_irq(%d)\n", pdata->irq);

    /*
   * 네트워크 디바이스를 커널에 등록한다.
   */
    if (condor_register_netdev(pdev, cndev))
    {
        return -EINVAL;
    }

    g_pdev = pdev;

    /*
   * 기타 주요 기능 초기화
   */
    spin_lock_init(&pdata->lock);
    spin_lock_init(&pdata->rx_lock);

    spin_lock_init(&pdata->tx_lock);
    init_all_condor_tx_queues();

    init_condor_rx_queue(&pdata->rxq);

    /* pdata init */
    pdata->tx_power = 10;
    pdata->datarate = 6;

    /* dev_type_normal == 0 */
    pdata->dev_type = dev_type_normal;
    /* H/W 버퍼 초기화 */
    pdata->hw_pkt_num = 0;
    /* 쓰레드 초기화 */
    init_condor_threads();
    pdata->current_channel = channelNumber_cch;

    // dev_info(&pdev->dev, "Success to probe\n");
    return 0;
}

#endif // LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)

/**
 * @brief 플랫폼 드라이버의 .remove 콜백함수
 *          Condor 디바이스를 해제하고 시스템에서 제거한다.
 * @param pdev (입력) Condor 플랫폼 디바이스
 * @return 0
 */
static int condor_remove(struct platform_device *pdev)
{
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    struct condor_netdev *cndev = platform_get_drvdata(pdev);

    // dev_info(&pdev->dev, "Trying to condor_remove()\n");
    if (!cndev)
    {
        // dev_err(&pdev->dev, "Fail to platform_get_drvdata(pdev)\n");
        return 0;
    }

    free_irq(pdata->irq, pdata);

    // unregister net device
    // iounmap

    // release_mem_region

    // free_device()...

    platform_set_drvdata(pdev, NULL);
    // dev_info(&pdev->dev, "Success to condor_remove()\n");

    return 0;
}

/**
 * @brief Condor 디바이스 인터럽트 서비스 루틴
 * @param irq (입력) 인터럽트 번호
 * @param dev_id (입력) request_irq() 함수에서 전달된 Condor 플랫폼데이터
 * @return 처리 시 IRQ_HANDLED, 미 처리 시(해당되는 인터럽트 없을 경우) IRQ_NONE
 */
static irqreturn_t condor_isr(int irq, void *dev_id)
{
    u8 *iobase;
    u32 status;
    u32 i, j, intr;
    struct platform_device *pdev = g_pdev;
    struct condor_platform_data *pdata = pdev->dev.platform_data;
    struct condor_hw *hw = &pdata->hw;

    // printk("Interrupt(%d)!!!\n", irq);

    /*
   * 각 라디오에 대한 인터럽트를 처리한다.
   */
    for (i = 0; i < RADIO_NUM_; i++)
    {
        iobase = hw->radio[i].cfg_iobase;

        // 하드웨어로부터 상태 인터럽트를 읽고 클리어한다.
        intr = readl(iobase + REG_INTR_STATUS);
        writel(intr, iobase + REG_INTR_STATUS);

        if (!intr)
        {
            return IRQ_NONE;
        }

        // dev_err(&pdev->dev, "intr: 0x%08X\n", intr);

        if (intrAsserted(intr, INTR_PPS))
        {
            // dev_info(&pdev->dev, "PPS interrupt - no need\n");
        }

        /*
     * 수신 실패 인터럽트 처리
     */
        if (intrAsserted(intr, INTR_RX_FAIL))
        {
            // dev_info(&pdev->dev, "Rx fail interrupt\n");
            pdata->netstats.rx_fail_cnt++;
        }

        /* 수신 성공 인터럽트 처리 */
        for (j = 0; j < 4 /*HW_RXQ_NUM*/; j++)
        {
            if (intrAsserted(intr, INTR_RX_SUCCESS(j)))
            {
                // dev_info(&pdev->dev, "Rx success(%d) interrupt\n", j);
                pdata->netstats.rx_success_cnt++;

                /* 수신 성공했을 때, 수신한 skb를 큐에 넣는다. */
                condor_process_rx_success_interrupt(&pdata->hw, j);
            }
        }

        /*
     * 송신 인터럽트 처리
     */
        for (j = 0; j < 8; j++)
        {
            if (intrAsserted(intr, INTR_TX_SUCCESS(j)))
            {
                // dev_info(&pdev->dev, "Tx success(%d) interrupt\n", j);
                // netif_wake_queue(pdata->cndev->ndev);

                /* Debug */
                // dev_info(&pdev->dev, "55555. Tx Success Interrupt\n");

                pdata->netstats.tx_success_cnt++;
            }
            if (intrAsserted(intr, INTR_TX_FAIL(j)))
            {

                /* Debug */
                // dev_info(&pdev->dev, "55555. Tx Fail Interrupt\n");

                // netif_wake_queue(pdata->cndev->ndev);

                status = readl(iobase + REG_FAIL_INTR_STATUS);
                pdata->netstats.tx_fail_cnt++;
                // dev_info(&pdev->dev, "Tx fail(%d) interrupt(0x%08X)\n", j, status);
                if (intrAsserted(status, INTR_TX_FAIL_CHAN_SW))
                {
                    // dev_info(&pdev->dev, "  dut to channel switching\n");
                }
                if (intrAsserted(status, INTR_TX_FAIL_UNDEF_RATE))
                {
                    // dev_info(&pdev->dev, "  dut to undefined rate\n");
                }
                if (intrAsserted(status, INTR_TX_FAIL_SYNC_ERR))
                {
                    // dev_info(&pdev->dev, "  dut to sync error\n");
                }
                if (intrAsserted(status, INTR_TX_FAIL_TX_TIMEOUT))
                {
                    // dev_info(&pdev->dev, "  dut to tx timeout\n");
                }
                if (intrAsserted(status, INTR_TX_FAIL_SRLIMIT_OVER))
                {
                    // dev_info(&pdev->dev, "  dut to short retry limit\n");
                }
                if (intrAsserted(status, INTR_TX_FAIL_LRLIMIT_OVER))
                {
                    // dev_info(&pdev->dev, "  dut to long retry limit\n");
                }
            }
            if (intrAsserted(intr, INTR_TX_SUCCESS(j)) || intrAsserted(intr, INTR_TX_FAIL(j)))
            {
                condorNetIfTxQueue_t *txq;
                struct sk_buff *skb;

                int32_t result = 0;
                int block_num, start_block_index, hw_buffer_pkt_num;

                // spin_lock(&pdata->tx_lock);
                pdata->tx_intr_num++;
                txq = &pdata->txq[0];

                spin_lock(&pdata->tx_lock);
                pdata->hw_pkt_num--;

                if (!TAILQ_EMPTY(&txq->head))
                {
                    // hw_buffer_pkt_num = condor_get_hw_buf_pkt_num(pdev, hw, 0, 0);
                    if (pdata->hw_pkt_num != 0)
                        goto unlock;

                    skb = TAILQ_FIRST(&txq->head)->skb;
                    if (!skb)
                        goto unlock;
                    
                    block_num = (skb->len % TX_UNIT_BLOCK_SIZE) ? ((skb->len / TX_UNIT_BLOCK_SIZE) + 1) : (skb->len / TX_UNIT_BLOCK_SIZE);
                    start_block_index = condor_check_buf_space(pdev, hw, 0, block_num);

                    if (start_block_index < 0)
                        goto unlock;
                    
                    skb = pop_condor_net_if_tx_queue(txq, &result);
                    if (result < 0)
                        goto unlock;
                    else if (result == 1)
                        netif_wake_queue(pdata->cndev->ndev);

                    condor_tx_ppdu_shared_buffer_go(pdev, hw, 0, 0, start_block_index, block_num, skb);
                    dev_kfree_skb_irq(skb);
                    skb = NULL;
                }
                
            unlock:
                spin_unlock(&pdata->tx_lock);
            }
        }
    }
    return IRQ_HANDLED;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)

/**
 * @brief 디바이스 트리를 탐색하여 Condor 플랫폼 디바이스를 생성한다.
 * @return 성공 시 0, 실패 시 -1
 */
static int __init condor_of_to_pdev(void)
{
    int ret;
    struct resource r[2] = {};
    struct device_node *node = NULL;
    struct condor_platform_data pdata;
    char condor_compatible_name[] = "keti,keti-c2dl";

    printk(KERN_INFO "Trying to condor_of_to_pdev()\n");

    // 디바이스 트리에서 동일한 Compatible 이름을 탐색한다.
    node = of_find_compatible_node(node, NULL, condor_compatible_name);
    if (node)
    {
        of_node_put(node);

        // 디바이스 트리에서 메모리 주소 및 인터럽트 정보를 가져온다.
        ret = of_address_to_resource(node, 0, &r[0]);
        if (ret)
        {
            printk(KERN_ERR "Fail to of_address_to_resource()\n");
            return ret;
        }
        of_irq_to_resource(node, 0, &r[1]);

        // 플랫폼 디바이스를 커널에 등록한다.
        g_pdev = platform_device_register_simple("condor", 0, r, 2);
        if (IS_ERR(g_pdev))
        {
            printk(KERN_ERR "Fail to platform_device_register_simple()\n");
            return PTR_ERR(g_pdev);
        }

        // Condor 전용 플랫폼데이터를 플랫폼 디바이스에 추가한다.
        ret = platform_device_add_data(g_pdev, &pdata, sizeof(pdata));
        if (ret)
        {
            printk(KERN_ERR "Fail to platform_device_add_data()\n");
            platform_device_unregister(g_pdev);
            return ret;
        }
    }
    else
    {
        printk(KERN_ERR "Fail to of_find_compatible_node()\n");
        return -1;
    }

    printk(KERN_INFO "Success to condor_of_to_pdev()\n");
    return 0;
}

#endif // #if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)

/**
 * @brief Condor 디바이스 드라이버 모듈을 초기화한다.
 *          커널 부팅 시 또는 insmod 명령 입력 시에 호출된다.
 *          플랫폼 디바이스를 만들고 커널에 등록한다.
 * @return platform_driver_register()의 리턴값.
 */
static int __init condor_init(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)

    if (condor_of_to_pdev())
    {
        printk(KERN_ERR "Fail to load %s\n", condor_driver.driver.name);
        return -ENODEV;
    }
    return platform_driver_register(&condor_driver);

#else // #if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)

    return platform_driver_probe(&condor_driver, condor_probe);

#endif // #if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
}

/**
 * @brief Condor 디바이스 드라이버 모듈을 종료한다.
 *          커널 종료 시 또는 rmmod 명령 입력 시에 호출된다.
 *          등록, 할당 되어 있는 정보를 해제한다.
 */
static void __exit condor_exit(void)
{
    // printk(KERN_INFO "Unloading %s\n", condor_driver.driver.name);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)

    platform_driver_unregister(&condor_driver);
    platform_device_unregister(g_pdev);

#else // #if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)

    platform_driver_unregister(&condor_driver);

#endif // #if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
}

module_init(condor_init);
module_exit(condor_exit);

MODULE_AUTHOR("Han-Gyun Jung <junghg@keti.re.kr>");
MODULE_DESCRIPTION("KETI condor network driver");
MODULE_LICENSE("GPL v2");

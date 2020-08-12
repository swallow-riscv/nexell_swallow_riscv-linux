//
// Created by gyun on 2019-03-24.
//

#ifndef LIBCONDOR_CONDOR_TYPES_H
#define LIBCONDOR_CONDOR_TYPES_H

#include "condor.h"

/**
 * @brief Condor 네트워크 디바이스 정보.
 *          Condor 디바이스의 네트워크 관련 정보가 저장된다.
 *          struct net_device의 private data로 저장된다.
 */
struct condor_netdev
{
    struct net_device *ndev;
    struct net_device_ops ndev_ops;
};

/**
 * @brief Condor 디바이스의 하드웨어 정보
 */
struct condor_hw
{
    u32 iobase;            // 모뎀 하드웨어 레지스터 시작 주소
    u8 mac_addr[ETH_ALEN]; // 하드웨어 MAC 주소 (두 개의 라디오에서 동일하다)
    u8 bssid[ETH_ALEN];    // BSSID

    struct
    {
        u32 iobase;       // 각 라디오 하드웨어 레지스터 시작 주소
        u8 *cfg_iobase;   // 각 라디오 설정 레지스터 블럭 시작 주소
        u8 *rxbuf_iobase; // 각 라디오 수신버퍼 블럭 시작 주소
        u8 *txbuf_iobase; // 각 라디오 송신버퍼 블럭 시작 주소
    } radio[RADIO_NUM_];
};

/**
 * @breif Condor 플랫폼 데이터.
 *          Condor 디바이스에 의존적인 정보가 저장된다.
 *          platform_device의 platform_data로 등록된다.
 */
struct condor_platform_data
{
    struct condor_netdev *cndev; // Condor 네트워크 디바이스 정보
    struct condor_hw hw;         // Condor 하드웨어 정보

    unsigned int tx_pkt_num;
    unsigned int tx_intr_num;

    /* H/W buffer 가 비어있는지 여부 */
    volatile unsigned int hw_pkt_num;

    /* 현재 접속중인 채널 */
    channelNumberRange current_channel;

    int irq;
    spinlock_t lock;

    /* 송신 큐 - 네트워크인터페이스 별 송신큐는 8개 존재한다. CCH 4개, SCH 4개 */
    spinlock_t tx_lock;
    condorNetIfTxQueue_t txq[CONDOR_PHY_TX_QUEUE_NUM];

    /* 수신 큐 */
    spinlock_t rx_lock;
    condorRxQueue_t rxq;

    /* 커널 쓰레드 */
    // struct task_struct *tx_thread;
    // wait_queue_head_t tx_wait;
    struct task_struct *rx_thread;
    wait_queue_head_t rx_wait;

    /* 송/수신 대기 중인 패킷이 있는지 여부 */
    bool tx_pending;
    bool rx_pending;
    
    /* 패킷을 전송할 데이터레이트 */
    dataRateRange datarate;
    /* 패킷을 전송할 파워레벨(0~7) */
    /* 수정 powerlevel -> txpower integer */
    int32_t tx_power;

    /* condor device type(0 normal, 1 obe, 2 rse, 3 extobe) */
    condor_device_type dev_type;
    
    struct // Network statistics
    {
        u32 tx_try_cnt;
        u32 rx_try_cnt;

        u32 tx_ip_try_cnt;
        u32 rx_ip_try_cnt;

        u32 tx_drop_cnt;
        u32 tx_ip_drop_cnt;
        u32 rx_drop_cnt;
        
        u32 tx_queue_full_cnt[CONDOR_PHY_TX_QUEUE_NUM];
        u32 rx_queue_full_cnt;
        
        u32 tx_success_cnt;
        u32 tx_fail_cnt;
        u32 rx_success_cnt;
        u32 rx_fail_cnt;
    } netstats;
};

#endif //LIBCONDOR_CONDOR_TYPES_H

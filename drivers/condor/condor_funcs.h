//
// Created by gyun on 2019-03-24.
//

#ifndef LIBCONDOR_CONDOR_FUNCS_H
#define LIBCONDOR_CONDOR_FUNCS_H

#include "condor_defines.h"
#include "condor_types.h"

// condor_thread.c
int32_t init_condor_threads(void);

// condor_tx_thread.c;
// void release_condor_tx_thread(void);
// int32_t init_condor_tx_thread(void);

// condor_rx_thread.c
void condor_release_rx_thread(void);
int32_t condor_init_rx_thread(void);
int32_t condor_process_rx_mpdu(struct sk_buff *skb, rxMetaData_t *rx_meta, bool *is_skb_free);

// condor_dot11_rx.c
struct sk_buff *condor_receive_hw_packet(struct condor_hw *hw, int32_t q_index, rxMetaData_t *rx_meta, int32_t *result);

// condor_netif.c
void print_packet_dump(struct sk_buff *skb);

// condor_queue.c
// tx_queue
void init_all_condor_tx_queues(void);
void init_condor_tx_queue(condorNetIfTxQueue_t *txq, netIfTxQueueIndexRange q_index);
void flush_all_condor_tx_queues(void);
void flush_condor_tx_queue(condorNetIfTxQueue_t *txq);
int32_t push_condor_net_if_tx_queue(condorNetIfTxQueue_t *txq, struct sk_buff *skb, txMetaData_t *tx_meta);
struct sk_buff *pop_condor_net_if_tx_queue(condorNetIfTxQueue_t *txq, int32_t *result);

// rx_queue
void init_condor_rx_queue(condorRxQueue_t *rxq);
void flush_condor_rx_queue(condorRxQueue_t *rxq);
bool is_empty_condor_rx_queue(condorRxQueue_t *rxq);
int32_t push_condor_rx_queue(condorRxQueue_t *rxq, struct sk_buff *skb, rxMetaData_t *rx_meta);
struct sk_buff* pop_condor_rx_queue(condorRxQueue_t *rxq, rxMetaData_t *rx_meta, int32_t *result);


// condor_dot11_tx.c (수정 필요)
struct sk_buff *convert_eth_to_80211(struct sk_buff *skb, txMetaData_t *tx_meta, int32_t *result);
int construct_hw_control_header(struct sk_buff *skb, u8 datarate, s32 txpower, u32 ether_len);

// condor_netdev.c
struct condor_netdev *condor_init_netdev(struct platform_device *pdev);
int condor_register_netdev(
    struct platform_device *pdev,
    struct condor_netdev *cndev);
int condor_set_netdev_mac_addr(
    struct platform_device *pdev,
    struct net_device *ndev,
    u8 addr[ETH_ALEN]);
void condor_process_rx_success_interrupt(
    struct condor_hw *hw, 
    int32_t q_index);

// condor_channel.c
int condor_access_channel(
    struct platform_device *pdev,
    unsigned int radio_idx,
    unsigned char ts0_chan,
    unsigned char ts1_chan);

// condor_tx.c
int condor_tx_mpdu(
    struct platform_device *pdev,
    u32 radio_idx,
    u8 timeslot,
    u8 datarate,
    s32 txpower,
    u8 mpdu[],
    u32 mpdu_size);
int condor_tx_ppdu_shared_buf(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx,
    struct sk_buff *skb, txMetaData_t *meta);
void condor_tx_ppdu_shared_buffer_go(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx,
    u32 start_block_idx,
    u32 block_num,
    struct sk_buff *skb);
int condor_check_buf_space(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    int block_num);
int condor_get_hw_buf_pkt_num(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 q_idx);
    
// condor_hw.c
int condor_init_hw(struct platform_device *pdev);
void condor_activate_hw(struct platform_device *pdev, struct condor_hw *hw);
void condor_deactivate_hw(struct platform_device *pdev, struct condor_hw *hw);

// condor_hw_radio.c
void condor_set_radio_hw_tsf(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u64 time);
void condor_set_radio_hw_mac_addr(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 addr[ETH_ALEN]);
void condor_set_radio_hw_bssid(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 bssid[ETH_ALEN]);
void condor_send_radio_hw_rf_cmd(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 timeslot);
void condor_set_radio_hw_rf_parallel_control(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx);
int condor_start_radio_hw_chan_switching(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 ts0_chan,
    u8 ts1_chan);
void condor_stop_radio_hw_chan_switching(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx);
int condor_access_radio_hw_chan(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 chan);
int condor_load_radio_hw_rf_cmd(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u8 chan,
    u8 timeslot);
void condor_set_radio_hw_chan_switching_interval(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 ts0_interval,
    u32 ts1_interval,
    u32 sync_tolerance,
    u32 switching_max_time);
void condor_set_radio_hw_mac_rts_thr(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 threshold);
void condor_set_radio_hw_mac_frag_thr(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 threshold);
void condor_set_radio_hw_mac_edca(
    struct platform_device *pdev,
    struct condor_hw *hw,
    u32 radio_idx,
    u32 timeslot,
    u32 ac,
    u32 cwmin,
    u32 cwmax,
    u32 aifsn);
uint64_t condor_get_hw_tsf_time(struct condor_hw *hw);

#endif //LIBCONDOR_CONDOR_FUNCS_H

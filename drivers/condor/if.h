//
// Created by gyun on 2019-03-23.
//

#ifndef LIBCONDOR_IF_H
#define LIBCONDOR_IF_H

#include <linux/sockios.h>

#define CONDOR_IOCTL_CODE (SIOCDEVPRIVATE+1)
#define	CONDOR_IOCTL_MAGIC (0x4b45)

enum e_ioctl_msg_id
{
  IOCTL_MSG_ID_REG_READ,
  IOCTL_MSG_ID_REG_WRITE,
  IOCTL_MSG_ID_CHANNEL_ACCESS,
  IOCTL_MSG_ID_MPDU_TX,
  IOCTL_MSG_ID_GET_NETSTATS,
  IOCTL_MSG_ID_CLEAR_NETSTATS,
  IOCTL_MSG_ID_SET_POWER_DATARATE,
  IOCTL_MSG_ID_SET_TYPE,
  IOCTL_MSG_ID_SET_MAC,
  IOCTL_MSG_ID_SET_UPPER_LAYER
};
typedef unsigned int ioctl_msg_id;

/**
 * @brief ioctl()을 통해 condor.ko로 전달되는 데이터 파라미터.
 *          struct ioctl_ifreq에 실려서 전달된다.
 */
struct ioctl_ifreq_params
{
  void *data;  // 전달되는 데이터
  unsigned int data_size;  // 전달되는 데이터 크기
  ioctl_msg_id msgid;  // 전달 메시지 식별자
} __attribute__ ((packed));

/**
 * @brief ioctl()을 통해 condor.ko로 전달되는 요청 데이터
 */
struct ioctl_ifreq
{
  char ifname[16];  // 디바이스 인터페이스 명
  struct ioctl_ifreq_params *params;  // 디바이스로 전달되는 데이터 파라미터
  short magic;  // 사전에 정해진 매직번호 (유효성 판단용)
  int result;  // 제어처리 반환
} __attribute__ ((packed));


/**
 * @brief
 */
struct ioctl_ifreq_reg_access
{
  unsigned int radio_idx;
  unsigned long offset;
  unsigned long val;
};

struct ioctl_ifreq_chan_access
{
  unsigned int radio_idx;
  unsigned char ts0_chan;
  unsigned char ts1_chan;
};

struct ioctl_ifreq_mpdu_tx
{
  unsigned int radio_idx;
  unsigned char timeslot;
  unsigned char datarate;
  int txpower;
  unsigned char mpdu[2000];
  unsigned long mpdu_size;
};

struct ioctl_ifreq_get_netstats
{
  unsigned int tx_success_cnt;
  unsigned int tx_fail_cnt;
  unsigned int rx_success_cnt;
  unsigned int rx_fail_cnt;
};

struct ioctl_ifreq_set_power_datarate
{
    dataRateRange datarate;
    int32_t tx_power;
};

struct ioctl_ifreq_set_type
{
    condor_device_type dev_type;
    unsigned char bssid[6];
};

struct ioctl_ifreq_set_mac
{
    unsigned char mac_addr[6];
};

#endif //LIBCONDOR_IF_H

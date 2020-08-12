//
// Created by gyun on 2019-03-23.
//

#ifndef LIBCONDOR_CONDOR_DEFINES_H
#define LIBCONDOR_CONDOR_DEFINES_H

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <stdbool.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
#include <asm/of_platform.h>
#else
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#endif

#undef BYTE
#define BYTE char

// #define CONDOR3_
// #define DRONESOC_
#define RADIO_NUM_ 1
#define _WAE_SHARED_HW_TXBUF_

// 디바이스 드라이버 및 네트워크 인터페이스 명
#define DEV_NAME "condor"

#define MAC_FCS_LEN 4

#define HW_SPECIFIC_HDR_LEN 10

/* MAX2829가 지원하는 전송파워의 개수
 * 	- 원래 MAX2829는 0x00~0x3f 범위의 txgain 값을 통해 64개의 스텝을 지원하나, txgain의 각 스텝은 0.5dBm의 차이가 있으므로,
 * 	   정수 dBm단위로 단순화하기 위해 txgain의 두 스텝 당 하나씩만을 사용한다. */
#define MAX2829_SUPPORT_TXPOWER_MAXNUM 32

#define WAVE_HDR_LEN 26
#define WAVE_FCS_LEN 4
#define WAVE_MAC_ALEN 6

/* WAVE 10MHz 채널 개수 */
#define	WAVE_10M_CHANNEL_NUM 7
/* 최대 PHY개수 - 이 장치가 지원하는 PHY장치의 개수
 * 	- 최대치의 제한은 따로 없으나, 일단 WAVE에서의 기본 채널 개수로 설정되며, 초기화 시 구현에 맞게 지원 개수를 입력한다. */
#define	CONDOR_PHY_MAX_NUM WAVE_10M_CHANNEL_NUM
/* WAVE(802.11)에서 사용되는 Access Category의 개수 */
#define	WAVE_AC_NUM	4
/* CONDOR의 각 PHY 당 송신 큐 개수 - 보통 CCH용 4개, SCH 4개를 사용한다. */
#define	CONDOR_PHY_TX_QUEUE_NUM (WAVE_AC_NUM * 2)
/* 시간슬롯의 개수 - WAVE 표준에 따라 시간슬롯은 2개이다 */
#define WAVE_TIMESLOT_NUM					2

/* WSMP용 LLC type */
#define	LLCTYPE_WSMP						0x88DC

typedef enum
{
    dev_type_normal     = 0,
    dev_type_obe        = 1,
    dev_type_rse        = 2,
    dev_type_extobe     = 3
} condor_device_type;

typedef enum
{
	netIfTxQueueIndex_min	=	0,
	netIfTxQueueIndex_max	=	(CONDOR_PHY_TX_QUEUE_NUM-1)
} netIfTxQueueIndexRange;

typedef enum
{
	dataUnitLen_min			=	0,
	wsmMaxLen_min			=	1,		/* WsmMaxLength 최소값(WsmMaxLength값은 헤더를 포함) */
	wsmMaxLen_default		=	1400,	/* WsmMaxLength 기본값(WsmMaxLength값은 헤더를 포함) */
	wsmHdrLen_min			=	4,		/* WSM 헤더의 최소길이(필수영역만 최소길이로 포함) */
	wsmHdrLen_max			=	18,		/* WSM 헤더의 최대길이(확장영역 모두 포함) */
	lsduLen_max				=	2302,	/* LLC service data unit 최대길이 (MSDU - EtherType) */
	msduLen_max				=	2304,	/* MAC service data unit 최대길이 */
	mpduLen_max				=	2334,	/* MAC protocol data unit 최대길이 (MSDU길이 + QoS MAC헤더(26) + CRC(4)) */
#define mmpduLen_max	msduLen_max		/* MAC management protocol data unit 최대길이(MAC헤더 불포함)*/
#define lpduLen_max		msduLen_max		/* LLC protocol data unit 최대길이 */
#define wsmMaxLen_max	lsduLen_max		/* WsmMaxLength 최대값(WsmMaxLength값은 헤더를 포함) */
} dataUnitLenRange;

/* 네트워크 인터페이스 식별번호 */
typedef enum
{
	netIfIndex_unknown	=	-1,
	netIfIndex_min		=	0,
	netIfIndex_max		=	(CONDOR_PHY_MAX_NUM-1)
} netIfIndexRange;

/* MAC레벨 사용자 우선순위 범위 */
typedef enum
{
	userPriority_min			=	0,
	userPriority_max			=	7,
	userPriority_unknown
#define userPriority_wsa_default	userPriority_max
} userPriorityRange;

/* 시간슬롯 범위 */
typedef enum
{
	timeSlot_0				=	0,		/* ts0 접속 (alternating) */
	timeSlot_1				=	1,		/* ts1 접속 (alternating) */
	timeSlot_either			=	2,		/* 둘 중 하나에 접속 */
	timeSlot_0_1			=	3,		/* 표준에는 정의되어 있지 않으나, 필요에 의해 추가 (=각 시간슬롯에서 서로다른 채널접속) */
	timeSlot_continuous		=	4,		/* 표준에는 정의되어 있지 않으나, 필요에 의해 추가 (=continuous) */
	timeSlot_none			=	5,		/* 표준에는 정의되어 있지 않으나, 필요에 의해 추가 (=CCH접속) */
	timeSlot_max
#define timeSlot_both	timeSlot_either
} timeSlotRange;

/* Service Class - MA-UNITDATAX.request 파라미터에서 사용된다. */
typedef enum
{
	dot11ServiceClass_QoSAck	=	0,
	dot11ServiceClass_QoSNoAck	=	1,
	dot11ServiceClass_unknown,
} dot11ServiceClassRange;

/* DataRate 값 범위(500kbps 단위) */
typedef enum
{
	dataRate_unknown	=	-999,
	dataRate_min		=	2,
	dataRate_3Mbps		=	6,
	dataRate_4p5Mbps	=	9,
	dataRate_6Mbps		=	12,
	dataRate_9Mbps		=	18,
	dataRate_12Mbps		=	24,
	dataRate_18Mbps		=	36,
	dataRate_24Mbps		=	48,
	dataRate_27Mbps		=	54,
	dataRate_max		=	127
#define	dataRate_default		dataRate_6Mbps
#define dataRate_wsa_default	dataRate_default
} dataRateRange;

/* powerLevel(=전송출력레벨) 인덱스
 * 	- dot11PhyTxPowerTable 내의 8개의 엔트리 중 하나의 번호로 설정된다.
 * 	- 표준에서는 1~8의 값을 갖도록 정의되어 있으나 편의 및 일관성을 위해 0~7로 사용한다. */
typedef enum
{
	powerLevelIndex_min			=	0,
	powerLevelIndex_std_default	=	3,	/* 표준에 명시된 기본값(표준 상에서는 4) */
	powerLevelIndex_max			=	7
#define powerLevelIndex_default	powerLevelIndex_max	/* 실제 기본값 - 전송거리의 확보를 위해 */
} powerLevelIndexRange;

/* powerLevel(=전송출력레벨) 개수
 * 	- 802.11에서는 기본적으로 8개의 레벨을 지원하도록 규정하고 있다. */
typedef enum
{
	powerLeveLNum_min	=	1,
	powerLevelNum_max	=	(powerLevelIndex_max+1)
#define powerLevelNum_std_default	powerLevelNum_max	/* 표준 상의 기본값 */
#define powerLevelNum_default		powerLevelNum_std_default
} powerLevelNumRange;

/* 파워 값(= dBm단위 전송출력/수신파워) 범위 */
typedef enum
{
	power_min			=	-128,
	power_tx_default	=	20,		/* C class에서 전송할 수 있는 최대 파워를 기본값으로 한다. */
	power_max			=	127,
	power_unknown		=	255
#define power_wsa_tx_default	power_tx_default
} powerRange;

/* Access Category Indicator */
typedef enum
{
	dot11Aci_min	=	0,
	dot11Aci_max	=	(WAVE_AC_NUM-1),
#define dot11Aci_be		0
#define	dot11Aci_bk		1
#define	dot11Aci_vi		2
#define	dot11Aci_vo		3
} dot11AciRange;

/* AC 개수 */
typedef enum
{
	dot11AcNum		=	WAVE_AC_NUM
} dot11AcNumRange;

/* 802.11 MAC헤더 sequence number */
typedef enum
{
	dot11MpduSeqNumber_min	=	0,
	dot11MpduSeqNumber_max	=	4095	/* 12비트 길이 */
} dot11MpduSeqNumberRange;

/* 802.11 MAC헤더 fragnement number */
typedef enum
{
	dot11MpduFragNumber_min	=	0,
	dot11MpduFragNumber_max	=	15		/* 4비트 길이 */
} dot11MpduFragNumberRange;

/* PHY인터페이스 식별번호 */
typedef enum
{
	phyIndex_min	=	0,
	phyIndex_max	=	(CONDOR_PHY_MAX_NUM-1)
} phyIndexRange;

/* 채널번호 값 범위 */
typedef enum
{
	channelNumber_unknown	=	-999,
	channelNumber_min		=	0,
	channelNumber_waveMin	=	172,	/* WAVE에서 사용되는 최저 채널번호 */
	channelNumber_cch		=	180,	/* CCH */
	channelNumber_waveMax	=	184,	/* WAVE에서 사용되는 최고 채널번호 */
	channelNumber_max		=	200,
	channelNumber_any		=	201,	/* 임의의 채널 */
} channelNumberRange;

typedef enum
{
	rcpi_min			=	0,		/* = -110dBm */
	rcpi_max			=	220,	/* = 0dBm */
	rcpi_not_available	=	255
#define rcpi_threshold_min	rcpi_min
#define rcpi_threshold_max	rcpi_max
} rcpiRange;

/* 데이터레이트 코드
 * 	- PHY PLCP 헤더에서 사용되는 데이터레이트별 코드 값(RATE 필드)이며, PHY하드웨어로 전달된다. */
typedef enum
{
	dot11PlcpRate_3mbps		=	0xb,
	dot11PlcpRate_4p5mbps	=	0xf,
	dot11PlcpRate_6mbps		=	0xa,
	dot11PlcpRate_9mbps		=	0xe,
	dot11PlcpRate_12mbps	=	0x9,
	dot11PlcpRate_18mbps	=	0xd,
	dot11PlcpRate_24mbps	=	0x8,
	dot11PlcpRate_27mbps	=	0xc,
} dot11PlcpRateCode;

/* UNITDATA.indication */
typedef struct
{
	/* 네트워크인터페이스 식별번호
	 * 	표준에는 명시되어 있지 않은 파라미터이지만, 2개 이상의 RF하드웨어 구현 시 필요하다. */
	netIfIndexRange			netIfIndex;
	/* PHY식별번호
	 * 	표준에는 명시되어 있지 않은 파라미터이지만, USDOT CV COC 수신테스트에서 Radio인터페이스번호로 활용하기 위해
	 * 	추가하였다. 수신된 hw의 번호가 저장된다. */
	phyIndexRange			phyIndex;

	/* 표준에 정의된 UNITDATA.indication 파라미터 */
	BYTE					source_address[WAVE_MAC_ALEN];					/* MA- & DL- */
	BYTE					destination_address[WAVE_MAC_ALEN];				/* MA- & DL- */
	BYTE					routing_information;/* 사용되지 않음 */				/* MA- */
	dataUnitLenRange		data_len;	/* = MPDU 길이 */						/* MA- */
	BYTE					data[mpduLen_max];	/* =MPDU */					/* MA- & DL- */
	bool					reception_status;	/* 항상 success */			/* MA- */
	userPriorityRange		priority;										/* MA- & DL- */
	dot11ServiceClassRange	service_class;									/* MA- */

	/* 구현 필요에 의해 추가된 필드 */
	//OUT dataUnitLenRange		dataOffset;			/* data 버퍼 중 패킷이 몇번째 바이트부터 저장되어 있는지 */
	uint16_t				etherType;

	/* Rich implementation을 위해 추가된 필드 -> 어플리케이션까지 전달된다. */
	channelNumberRange		channelNumber;
	powerRange				rxPower;
	rcpiRange				rcpi;
	dataRateRange			dataRate;
	dataUnitLenRange		MacHeaderSize;
	dataUnitLenRange		LlcHeaderSize;
	dataUnitLenRange		HeaderSize;	/* 헤더 길이의 합 */
} UNITDATA_indication_params_t;
#endif //LIBCONDOR_CONDOR_DEFINES_H

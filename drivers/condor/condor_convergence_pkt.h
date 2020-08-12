/*
 * convergence_pkt.h
 *
 *  Created on: 2017. 7. 11.
 *      Author: gyun
 * 	Modified on: 2020-06-19 
 */

#ifndef CONVERGENCE_PKT_H
#define CONVERGENCE_PKT_H

#include "condor_defines.h"

/* 802.11 MAC 프로토콜 버전 */
#define DOT11_PROTOCOL_VERSION		0


/****************************************************************************************
	매크로

****************************************************************************************/

/*----------------------------------------------------------------------------------*/
/* MAC 헤더의 각 필드를 설정/확인하는 매크로 */
/*----------------------------------------------------------------------------------*/
/* Frame Control 필드 설정/확인 매크로 */
#define DOT11_GET_FC_PVER(fc)	 	(fc & 3)
#define DOT11_GET_FC_FTYPE(fc)		((fc >> 2) & 3)
#define DOT11_GET_FC_FSTYPE(fc)		((fc >> 4) & 0xf)
#define DOT11_GET_FC_TODS(fc) 		((fc >> 8) & 1)
#define DOT11_GET_FC_FROMDS(fc)		((fc >> 9) & 1)
#define DOT11_GET_FC_MOREFRAG(fc) 	((fc >> 10) & 1)
#define DOT11_GET_FC_RETRY(fc)		((fc >> 11) & 1)
#define DOT11_GET_FC_PWRMGT(fc)		((fc >> 12) & 1)
#define DOT11_GET_FC_MOREDATA(fc) 	((fc >> 13) & 1)
#define DOT11_GET_FC_ISWEP(fc)		((fc >> 14) & 1)
#define DOT11_GET_FC_ORDER(fc)		((fc >> 15) & 1)
#define DOT11_SET_FC_PVER(n)		(uint16_t)(n & 3)
#define DOT11_SET_FC_FTYPE(n)		(uint16_t)((n & 3) << 2)
#define DOT11_SET_FC_FSTYPE(n)		(uint16_t)((n & 0xf) << 4)
#define DOT11_SET_FC_TODS(n) 		(uint16_t)((n & 1) << 8)
#define DOT11_SET_FC_FROMDS(n)		(uint16_t)((n & 1) << 9)
#define DOT11_SET_FC_MOREFRAG(n) 	(uint16_t)((n & 1) << 10)
#define DOT11_SET_FC_RETRY(n)		(uint16_t)((n & 1) << 11)
#define DOT11_SET_FC_PWRMGT(n)		(uint16_t)((n & 1) << 12)
#define DOT11_SET_FC_MOREDATA(n) 	(uint16_t)((n & 1) << 13)
#define DOT11_SET_FC_ISWEP(n)		(uint16_t)((n & 1) << 14)
#define DOT11_SET_FC_ORDER(n)		(uint16_t)((n & 1) << 15)

/* Sequence Control 필드 설정/확인 매크로 */
#define	DOT11_GET_SEQ_SEQ(sc)		((sc >> 4) & 0xfff)
#define	DOT11_GET_SEQ_FRAG(sc)		(sc & 0xf)
#define DOT11_SET_SEQ_SEQ(n)		(uint16_t)((n & 0xfff) << 4)
#define DOT11_SET_SEQ_FRAG(n)		(uint16_t)(n & 0xf)

/* QoS 필드 설정/확인 매크로 */
#define	DOT11_GET_QOS_TID(qc)			(qc & 0xf)
#define	DOT11_GET_QOS_EOSP(qc)			((qc >> 4) & 1)
#define	DOT11_GET_QOS_ACK_POLICY(qc)	((qc >> 5) & 3)
#define	DOT11_GET_QOS_AMSDU_PRESENT(qc)	((qc >> 7) & 1)
#define	DOT11_GET_QOS_TXOP_DUR_REQ(qc)	((qc >> 8) & 0xff)
#define	DOT11_SET_QOS_TID(n)			(uint16_t)(n & 0xf)
#define	DOT11_SET_QOS_EOSP(n)			(uint16_t)((n & 1) << 4)
#define	DOT11_SET_QOS_ACK_POLICY(n)		(uint16_t)((n & 3) << 5)
#define	DOT11_SET_QOS_AMSDU_PRESENT(n)	(uint16_t)((n & 1) << 7)
#define	DOT11_SET_QOS_TXOP_DUR_REQ(n)	(uint16_t)((n & 0xff) << 8)
#define	DOT11_SET_QOS_UP(n)				DOT11_SET_QOS_TID(n)
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 개별/그룹 MAC주소 확인 매크로 */
/*----------------------------------------------------------------------------------*/
#define DOT11_GET_MAC_ADDR_IG(addr)		(addr[0]&1)
#define DOT11_MAC_ADDR_IG_INDIVIDUAL	0
#define DOT11_MAC_ADDR_IG_GROUP			1
/*----------------------------------------------------------------------------------*/


/****************************************************************************************
	유형 정의

****************************************************************************************/


/****************************************************************************************
	열거형

****************************************************************************************/
/*----------------------------------------------------------------------------------*/
/* 802.11 MAC 패킷 유형 값 */
/*----------------------------------------------------------------------------------*/
/* Frame Control 영역의 Type 영역 */
typedef enum
{
	dot11MacHdrFcType_mgmt	=	0x00,
	dot11MacHdrFcType_ctrl	=	0x01,
	dot11MacHdrFcType_data	=	0x02,
	dot11MacHdrFcType_unknown
} dot11MacHdrFcType;

/* Frame Control 영역의 Subtype 영역 */
typedef enum
{
	dot11MacHdrFcSubType_data		=	0x00,
	dot11MacHdrFcSubType_ta			=	0x06,
	dot11MacHdrFcSubType_qosData	=	0x08,
	dot11MacHdrFcSubType_rts		=	0x0b,
	dot11MacHdrFcSubType_cts		=	0x0c,
	dot11MacHdrFcSubType_ack		=	0x0d,
} dot11MacHdrFcSubType;
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 802.11 MAC 헤더 */
/*----------------------------------------------------------------------------------*/
/* Management 패킷 헤더 */
typedef struct
{
	uint16_t		fc;
	uint16_t		dur;
	BYTE			addr1[WAVE_MAC_ALEN];
	BYTE			addr2[WAVE_MAC_ALEN];
	BYTE			addr3[WAVE_MAC_ALEN];
	uint16_t		seq;
} __attribute__ ((packed)) dot11MgmtHdr_t;

/* QoS 데이터 패킷 헤더 */
typedef struct
{
	uint16_t		fc;
	uint16_t		dur;
	BYTE			addr1[WAVE_MAC_ALEN];
	BYTE			addr2[WAVE_MAC_ALEN];
	BYTE			addr3[WAVE_MAC_ALEN];
	uint16_t		seq;
	uint16_t		qos;
} __attribute__ ((packed)) dot11QoSDataHdr_t;

/* non-QoS 데이터 패킷 헤더 */
typedef dot11MgmtHdr_t	dot11DataHdr_t;

/* 802.11p MAC 헤더 */
typedef struct
{
	union {
		dot11MgmtHdr_t		mgmtHdr;
		dot11QoSDataHdr_t	qosDataHdr;
		dot11DataHdr_t		dataHdr;
	} u;
} __attribute__ ((packed)) dot11MacHdr_t;
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* LLC 헤더 */
/*----------------------------------------------------------------------------------*/
typedef struct
{
	uint16_t		type;
} __attribute__ ((packed)) llcHdr_t;
/*----------------------------------------------------------------------------------*/


/* 송신 패킷 메타 데이터 - 송신에 관련된 부가정보 포함 */
typedef struct
{
    /* 패킷을 전송할 네트워크 인터페이스의 식별번호 */
    netIfIndexRange netIfIndex;
    /* 패킷 송신지 */
    BYTE src[WAVE_MAC_ALEN];
    /* 패킷 목적지 */
    BYTE dst[WAVE_MAC_ALEN];
    /* 패킷 우선순위 */
    userPriorityRange priority;
    /* 패킷을 전송할 시간슬롯 */
    timeSlotRange timeSlot;
    /* 패킷을 전송할 데이터레이트 */
    dataRateRange dataRate;
    /* 패킷을 전송할 파워레벨(0~7) */
    /* 수정 powerlevel -> txpower integer */
    int32_t txPower;
    /* 만기시각 - 전송시점에 TSF 타이머값과 비교하여 지났으면 패킷을 폐기한다 */
    uint64_t expiry;
    /* 서비스클래스 */
    dot11ServiceClassRange serviceClass;
} txMetaData_t;

/* 수신 패킷 메타 데이터 - 수신에 관련된 부가정보 포함 */
typedef struct
{
	/* 수신된 PHY의 번호 */
	phyIndexRange		phyIndex;
	/* 수신된 네트워크인터페이스의 번호 */
	netIfIndexRange		netIfIndex;
	/* 수신된 채널번호 */
	channelNumberRange	ChannelNumber;
	/* 수신 세기 */
	powerRange			rxPower;
	/* RCPI - 수신세기로부터 변환됨 */
	rcpiRange			rcpi;
	/* 수신 데이터레이트 */
	dataRateRange		rxDataRate;
	/* 로컬 타임 - WAVE에서는 TA 프레임인 경우에만 존재 */
	uint64_t			localtime;
	/* 송신지 주소 */
	BYTE				srcAddress[WAVE_MAC_ALEN];
	/* 목적지 주소 */
	BYTE				dstAddress[WAVE_MAC_ALEN];
	/* 우선순위 */
	userPriorityRange	priority;
	/* 서비스클래스 */
	dot11ServiceClassRange	serviceClass;
	/* 순서번호 */
	dot11MpduSeqNumberRange	seqNumber;
	/* 분할번호 */
	dot11MpduFragNumberRange	fragNumber;
} rxMetaData_t;

/****************************************************************************************
	전역변수

****************************************************************************************/
extern BYTE wildcardBssid[];
extern BYTE broadcastMacAddress[];


#endif /* !CONVERGENCE_PKT_H */

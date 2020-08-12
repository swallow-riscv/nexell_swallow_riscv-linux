//
// Created by gyun on 2019-03-23.
//

#ifndef LIBCONDOR_CONDOR_HW_REGS_H
#define LIBCONDOR_CONDOR_HW_REGS_H

/*----------------------------------------------------------------------------------*/
/* 하드웨어 기본 주소 */
/*----------------------------------------------------------------------------------*/
#define		HW_ADDRESS_BASE			0xf0000000
//#define		HW1_ADDRESS_BASE			0xf0010000
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 하드웨어 레지스터 블록 정보 */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* 레지스터 블록 크기 */
/*-----------------------------------------------------------------*/
#define		CONFIG_REGS_BLK_LEN		0x1000
#define		SEC_REG_BLK_LEN			0x1000
#define		SEC_IN_BUF_BLK_LEN		0x1000
#define		SEC_OUT_BUF_BLK_LEN		0x1000
#define		RXBUF_BLK_LEN			0x4000
#ifdef _WAE_SHARED_HW_TXBUF_
#define		TXBUF_BLK_LEN			0x4000
#else
#define		TXBUF_BLK_LEN			0x8000
#endif
#define 	HW_REGS_LEN				(CONFIG_REGS_BLK_LEN + SEC_REG_BLK_LEN + \
									SEC_IN_BUF_BLK_LEN + SEC_OUT_BUF_BLK_LEN + \
									RXBUF_BLK_LEN + TXBUF_BLK_LEN)
#define		HW_REGS_CONTAINER_LEN	0x10000
#define		HW_REGS_TOTAL_LEN		(HW_REGS_LEN*2)	/* 2개의 PHY */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 각 레지스터 블록 오프셋 */
/*-----------------------------------------------------------------*/
/* 모뎀 설정 레지스터 블록 오프셋 */
#define		CONFIG_REG_BLK_OFFSET	0x0000
/* 보안가속기 레지스터 블록 오프셋 */
#define		SEC_REG_BLK_OFFSET		(CONFIG_REG_BLK_OFFSET+CONFIG_REGS_BLK_LEN) 	/*=0x1000*/
/* 보안입력버퍼 블록 오프셋 */
#define		SEC_IN_BUF_BLK_OFFSET	(SEC_REG_BLK_OFFSET+SEC_REG_BLK_LEN)			/*=0x2000*/
/* 보안출력버퍼 블록 오프셋 */
#define		SEC_OUT_BUF_BLK_OFFSET	(SEC_IN_BUF_BLK_OFFSET+SEC_IN_BUF_BLK_LEN)		/*=0x3000*/
/* 모뎀수신버퍼 블록 오프셋 */
#define		RXBUF_BLK_OFFSET		(SEC_OUT_BUF_BLK_OFFSET+SEC_OUT_BUF_BLK_LEN)	/*=0x4000*/
/* 모뎀송신버퍼 블록 오프셋 */
#define		TXBUF_BLK_OFFSET		(RXBUF_BLK_OFFSET+RXBUF_BLK_LEN)				/*=0x8000*/
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 모뎀버퍼 주소 오프셋 */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* 모뎀송신버퍼 주소 오프셋 */
/*-----------------------------------------------------------------*/
#define		TXBUF_NUM					8
#define		TXBUF0						(0)
#define		TXBUF1						(0x1000)
#define		TXBUF2						(0x2000)
#define		TXBUF3						(0x3000)
#define		TXBUF4						(0x4000)
#define		TXBUF5						(0x5000)
#define		TXBUF6						(0x6000)
#define		TXBUF7						(0x7000)
#define		TXBUF(i, j)					(((i)*0x1000) + (4*(j)))
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 모뎀수신버퍼 주소 오프셋 */
/*-----------------------------------------------------------------*/
#define 	RXBUF_NUM					4
#define		RXBUF0						(0)
#define		RXBUF1						(0x1000)
#define		RXBUF2						(0x2000)
#define		RXBUF3						(0x3000)
#define		RXBUF(i, j)					(((i)*0x1000) + (4*(j)))
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 현재 접속 중인 시간 슬롯 확인 레지스터 */
/*----------------------------------------------------------------------------------*/
#define		MAC_CMD_REG(i)	   			(0x000 + (4*(i)))			// 0 <= i <= 1
#define		REG_CUR_TIME_SLOT			MAC_CMD_REG(0)
#define	MAC_CMD_CUR_TIME_SLOT_1		0
#define MAC_CMD_CUR_TIME_SLOT_0		1
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 타임스탬프 업데이트 방법 설정 레지스터
 * 	1. 수신프레임을 이용한 방법
 * 	2. 레지스터에 직접 설정하는 방법 */
/*----------------------------------------------------------------------------------*/
#define		REG_TSF_UPDATE_CMD			MAC_CMD_REG(1)
#define	MAC_CMD_UPD_TS_RX			(1<<0)
#define MAC_CMD_UPD_TS_REG			(1<<1)
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 모뎀 하드웨어 버전 레지스터 */
/*----------------------------------------------------------------------------------*/
#define		WAVE_VERSION				(0x008)
#define		REG_HW_VERSION				WAVE_VERSION
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 모뎀 초기화 레지스터 - 모든 레지스터들을 기본 값으로 설정한다. */
/*----------------------------------------------------------------------------------*/
#define		DEFAULT_LOAD				(0x00C)
#define		REG_INIT					DEFAULT_LOAD
#define INIT_ALL_REGISTERS			1
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 미사용 */
/*----------------------------------------------------------------------------------*/
#define 	AHB_WAIT					(0x010)
#define		REG_AHB_CONFIG				AHB_WAIT
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 기능 활성화 레지스터
 * 	- 각 비트별로 기능을 활성화/비활성화 한다. */
/*----------------------------------------------------------------------------------*/
/* Enable functions */
#define		MAC_OP_REG              	(0x100)
#define		REG_ENABLE					MAC_OP_REG
#define	ENABLE_MAC					(1<<0)		/* MAC활성화 */
#define	ENABLE_PHY_TX				(1<<1)		/* PHY송신활성화 */
#define ENABLE_PHY_RX				(1<<2)		/* PHY수신활성화 */
#define	ENABLE_COORD				(1<<3)		/* 채널스위칭활성화 */
#define	ENABLE_PORT_CHANGE			(1<<4)		/* 포트변경기능활성화(사용안함) */
#define MAC_OP_ALL_ENABLE			0xffffffff	/* 모든 기능 활성화 */
#define MAC_OP_ALL_DISABLE			0x00000000	/* 모든 기능 비활성화 */
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 인터럽트 관련 레지스터 */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* 인터럽트 관련 레지스터 셋 (i=0~3)
 * 	- HOST_IF_REG(0): 인터럽트 상태 레지스터
 * 	- HOST_IF_REG(1): 인터럽트 활성화 레지스터
 * 	- HOST_IF_REG(2): 수신실패인터럽트 상태 레지스터
 * 	- HOST_IF_REG(3): 수신버퍼 해제 레지스터 */
/*-----------------------------------------------------------------*/
#define		HOST_IF_REG(i)				(0x104 + (4*(i)))
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 인터럽트 상태 레지스터 - 발생하여 pending 되어 있는 인터럽트 확인 */
/*-----------------------------------------------------------------*/
#define		REG_INTR_STATUS				HOST_IF_REG(0)
#define	INTR_TX_SUCCESS(i)			(1<<(i))				/* 송신성공인터럽트. 각 비트는 각 버퍼별 인터럽트를 나타낸다(i=0~7) */
#define	INTR_TX_FAIL(i)				(1<<(8+(i)))			/* 송신실패인터럽트. 각 비트는 각 버퍼별 인터럽트를 나타낸다(i=0~7) */
#define	INTR_RX_SUCCESS(i)			(1<<(16+(i)))			/* 수신성공인터럽트. 각 비트는 각 버퍼별 인터럽트를 나타낸다(i=0~3) */
#define INTR_RX_FAIL				(1<<20)				/* 수신실패인터럽트 */
#define	INTR_TO_TIMESLOT0			(1<<21)				/* 시간슬롯0으로의 채널스위칭 발생 인터럽트 */
#define	INTR_TO_TIMESLOT1			(1<<22)				/* 시간슬롯1으로의 채널스위칭 발생 인터럽트 */
#define	INTR_RF_CMD_SENT			(1<<23)				/* MAX2829로의 설정 명령 전송 완료 인터럽트 */
#ifdef _WAE_OLD_FPGA_
#define	INTR_SW_PUSHED				(1<<24)				/* 스위치 입력 인터럽트 */
	#define	INTR_ANT_PORT_CHANGE		(1<<26)				/* 안테나포트 변경 인터럽트 */
	#define	INTR_TX_BUF_FULL_ERR		(1<<27)				/* 송신버퍼 Full 인터럽트 */
	#define	INTR_EN_SEC_INTR			(1<<28)				/* 보안블럭 인터럽트 */
	#define INTR_PPS					(1<<29)				/* PPS 인터럽트 */
#else
#define	INTR_PPS					(1<<24)				/* PPS 인터럽트 */
#define	INTR_TX_BUF_FULL_ERR		(1<<25)				/* 송신버퍼 Full 인터럽트 */
#define	INTR_PORT_CHANGE			(1<<26)				/* 안테나 포트 변경 인터럽트(deprecated) */
#define	INTR_CH_INTERVAL_POINT1		(1<<27)				/* 채널 인터벌 중 설정한 특정 시점(SyncInterval시작지점부터) 도달 인터럽트 */
#define	INTR_CH_INTERVAL_POINT2		(1<<28)				/* 채널 인터벌 중 설정한 특정 시점(SyncInterval시작지점부터) 도달 인터럽트 */
#endif
#define	INTR_CLEAR_ALL				0xffffffff			/* 이 레지스터에 이 값을 쓰면 모든 인터럽트는 클리어된다. */
#define	INTR_CLEAR_TX_ALL			0xffff				/* 이 레지스터에 이 값을 쓰면 모든 송신 인터럽트는 클리어된다. */
#define INTR_CLEAR_RX_ALL			(0x1f<<16)			/* 이 레지스터에 이 값을 쓰면 모든 수신 인터럽트는 클리어된다. */
#define	intrAsserted(vec, bit)		(((vec) & (bit)) == (bit))	/* 각 인터럽트가 발생했는지 확인하는 매크로 */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 인터럽트 활성화 레지스터 */
/*-----------------------------------------------------------------*/
#define		REG_INTR_ENABLE				HOST_IF_REG(1)
#define	INTR_EN_TX_SUCCESS(i)		(1<<(i))			/* 송신성공인터럽트 활성화. 각 비트는 각 버퍼별 인터럽트를 나타낸다(i=0~7) */
#define INTR_EN_TX_FAIL				(1<<(8+(i)))		/* 송신실패인터럽트 활성화. 각 비트는 각 버퍼별 인터럽트를 나타낸다(i=0~7) */
#define	INTR_EN_RX_SUCCESS			(1<<(16+(i)))		/* 수신성공인터럽트 활성화. 각 비트는 각 버퍼별 인터럽트를 나타낸다(i=0~3) */
#define INTR_EN_RX_FAIL				(1<<20)			/* 수신실패인터럽트  활성화 */
#define	INTR_EN_TO_TIMESLOT0		(1<<21)			/* 시간슬롯0으로의 채널스위칭 발생 인터럽트 활성화 */
#define	INTR_EN_TO_TIMESLOT1		(1<<22)			/* 시간슬롯1으로의 채널스위칭 발생 인터럽트 활성화 */
#define	INTR_EN_RF_CMD_SENT			(1<<23)			/* MAX2829로의 설정 명령 전송 완료 인터럽트 활성화 */
#ifdef _WAE_OLD_FPGA_
#define	INTR_EN_SW_PUSHED			(1<<24)				/* 스위치 입력 인터럽트 */
	#define	INTR_EN_ANT_PORT_CHANGE		(1<<26)				/* 안테나포트 변경 인터럽트 */
	#define	INTR_EN_TX_BUF_FULL_ERR		(1<<27)				/* 송신버퍼 Full 인터럽트 */
	#define	INTR_EN_SEC_INTR			(1<<28)				/* 보안블럭 인터럽트 */
	#define INTR_EN_PPS					(1<<29)				/* PPS 인터럽트 */
#else
#define	INTR_EN_PPS					(1<<24)			/* PPS 인터럽트 활성화 */
#define	INTR_EN_TX_BUF_FULL_ERR		(1<<25)			/* 송신버퍼 Full 인터럽트 활성화 */
#define	INTR_EN_PORT_CHANGE			(1<<26)			/* 안테나 포트 변경 인터럽트 활성화(deprecated) */
#define	INTR_EN_CH_SW_POINT1		(1<<27)			/* 채널 스위칭 중 설정한 특정 시점 도달 인터럽트 */
#define	INTR_EN_CH_SW_POINT2		(1<<28)			/* 채널 스위칭 중 설정한 특정 시점 도달 인터럽트 */
#endif
#define INTR_EN_ALL_DISABLED		0				/* 이 레지스터에 이 값을 쓰면 모든 인터럽트는 비활성화된다. */
#define INTR_EN_ALL_ENABLED			0xffffffff		/* 이 레지스터에 이 값을 쓰면 모든 인터럽트는 활성화된다. */
#define	INTR_EN_TRX_ENABLED			0x1fffff		/* 이 레지스터에 이 값을 쓰면 모든 송수신 관련 인터럽트는 활성화된다. */
#define	INTR_EN_CHAN_SW_ENABLED		(3<<21)			/* 시간슬롯0/시간슬롯1 채널스위칭 발생 인터럽트 활성화 */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 송신 및 수신 실패 인터럽트 상태 레지스터
 * 	- 각 비트별로 송신 및 수신 실패 이유를 나타낸다. */
/*-----------------------------------------------------------------*/
#define		REG_FAIL_INTR_STATUS		HOST_IF_REG(2)
#define INTR_RX_FAIL_RATE			(1<<0)		/* PHY계층에서 지원되지 않는 데이터 레이트 */
#define	INTR_RX_FAIL_PARITY			(1<<1)		/* PHY계층 패리티 에러 */
#define	INTR_RX_FAIL_VERSION		(1<<2)		/* PHY계층 버전 에러 */
#define	INTR_RX_FAIL_LENGTH			(1<<3)		/* PHY계층 길이 에러 */
#define INTR_RX_FAIL_MAX_LENGTH		(1<<4)		/* PHY계층에서 지원하는 최대 길이 초과 에러 */
#define INTR_RX_FAIL_FRAG_NUM		(1<<5)		/* 분할프레임 번호 에러 */
#define INTR_RX_FAIL_CRC32			(1<<6)		/* CRC 에러 */
#define	INTR_TX_FAIL_CHAN_SW		(1<<8)		/* 채널스위칭으로 인한 송신 에러 */
#define	INTR_TX_FAIL_UNDEF_RATE		(1<<9)		/* PHY계층에서 지원되지 않는 데이터 레이트 */
#define	INTR_TX_FAIL_SYNC_ERR		(1<<10)		/* HW 내부 동기 문제로 인한 에러 */
#define	INTR_TX_FAIL_TX_TIMEOUT		(1<<11)		/* 송신 타임아웃 */
#define	INTR_TX_FAIL_SRLIMIT_OVER	(1<<12)		/* 짧은 패킷에 대한 재전송 실패(최대재전송회수 초과) */
#define	INTR_TX_FAIL_LRLIMIT_OVER	(1<<13)		/* 긴 패킷에 대한 재전송 실패(최대재전송회수 초과) */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 수신 버퍼 해제 레지스터
 * 	- SW는 수신된 패킷을 별도의 버퍼로 복사한 후, 이 레지스터의 설정을 통해 하드웨어 버퍼를 비워야 한다. */
/*-----------------------------------------------------------------*/
#define		REG_RX_BUF_RELEASE			HOST_IF_REG(3)
#define	RELEASE_RXBUF(i)			(1<<(i))			/* 각 비트별로 해당되는 수신버퍼를 비운다. (i=0~3) */
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* MAC H/W 설정 레지스터 셋 */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* MAC H/W 설정 관련 레지스터 셋 (i=0~2)
 * 	- MAC_CFG_REG(0): 난수 생성기의 초기값 설정 레지스터
 * 	- MAC_CFG_REG(1): MPDU 최대길이 설정 레지스터
 * 	- MAC_CFG_REG(2): 채널 대역폭 설정 레지스터  */
/*-----------------------------------------------------------------*/
#define		MAC_CFG_REG(i)            	(0x120 + (4*(i)))
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* MAC에서 사용되는 난수 생성기의 초기값 설정 레지스터 */
/*-----------------------------------------------------------------*/
#define		REG_RN_INIT					MAC_CFG_REG(0)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* MPDU 최대허용길이 설정 레지스터 */
/*-----------------------------------------------------------------*/
#define		REG_MPDU_MAXLEN				MAC_CFG_REG(1)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 채널 대역폭 설정 레지스터 - 현재 10MHz만 지원된다. */
/*-----------------------------------------------------------------*/
#define		REG_CHN_BW					MAC_CFG_REG(2)
#define	CHN_BW_10M					1
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* BSSID 설정 레지스터 */
/*----------------------------------------------------------------------------------*/
#define		MBSSID_REG(i)				(0x12C + (4*(i)))		/* i=0~1*/
/* BSSID의 하위 4바이트를 설정/확인한다.(배열 상의 앞 4바이트)
 * 	- little endian 형식을 따른다.
 * 	- bssid[0]은 b0~7, bssid[1]은 b8~15, bssid[2]는 b16~23, bssid[3]은 b24~31에 쓴다.*/
#define		REG_BSSID_LOW32				MBSSID_REG(0)
/* BSSID의 상위 2바이트를 설정/확인한다.(배열 상의 뒤 2바이트)
 * 	- little endian 형식을 따른다.
 * 	- bssid[5]는 b0~7, bssid[6]은 b8~15에 쓴다. */
#define		REG_BSSID_HIGH16			MBSSID_REG(1)
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 프로토콜 시간 레지스터 셋
 * 	- 이 레지스터들은  PROTO_TIME_REG(11~13)을 제외하고는 HW 기본값을 사용하므로,
 * 	  특별한 상황 외에는 SW가 사용할 일이 없다. */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* 프로토콜 시간 파라미터 관련 레지스터 (i=0~13) *
 * 	- PROTO_TIME_REG(0): MAC/PHY 처리지연시간 레지스터
 * 	- PROTO_TIME_REG(1): MAC slottime 레지스터
 * 	- PROTO_TIME_REG(2): SIFS 시간 설정 레지스터
 * 	- PROTO_TIME_REG(3): EIFS 시간 설정 레지스터
 * 	- PROTO_TIME_REG(4): PSK(3/6Mbps) 응답 시간 설정 레지스터
 * 	- PROTO_TIME_REG(5): QAM(12/24Mbps) 응답 시간 설정 레지스터
 * 	- PROTO_TIME_REG(6): PSK(3/6Mbps) RTS 타임아웃시간 설정 레지스터
 * 	- PROTO_TIME_REG(7): QAM(12/24Mbps) RTS 타임아웃시간 설정 레지스터
 * 	- PROTO_TIME_REG(8): 마이크로초에 대한 H/W 카운터 기준값 설정 레지스터
 * 	- PROTO_TIME_REG(9): TU에 대한 H/W 카운터 기준값 설정 레지스터
 * 	- PROTO_TIME_REG(10): RF 지연 값 설정 레지스터
 * 	- PROTO_TIME_REG(11~12): 타임스탬프 설정/확인 레지스터
 * 	- PROTO_TIME_REG(13): 채널 스위칭 파라미터 설정 레지스터 */
/*-----------------------------------------------------------------*/
#define		PROTO_TIME_REG(i)         	(0x134 + (4*(i)))
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* MAC 처리 시간 및 수신 PLCP 지연 시간을 설정/확인한다.
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_PROC_TIME				PROTO_TIME_REG(0)
#define	DEFAULT_MAC_PROCTIME_40M	80					// 2usec
#define DEFAULT_MAC_PROCTIME_50M	100					// 2usec
#define DEFAULT_RX_PLCPTIME_40M		1080				// 27usec
#define DEFAULT_RX_PLCPTIME_50M		1350				// 27usec
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* MAC SlotTime 레지스터
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_SLOT_TIME				PROTO_TIME_REG(1)
#define DEFAULT_SLOTTIME_40M		640
#define DEFAULT_SLOTTIME_50M		800
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* SIFS 시간 설정 레지스터
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_SIFS_TIME				PROTO_TIME_REG(2)
#define DEFAULT_SIFSTIME_TX_40M		1120
#define DEFAULT_SIFSTIME_TX_50M		1400
#define DEFAULT_SIFSTIME_RX_40M		40
#define DEFAULT_SIFSTIME_RX_50M		50
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* EIFS 시간 설정 레지스터
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_EIFS_TIME				PROTO_TIME_REG(3)
#define	DEFAULT_EIFSTIME_TX_10_40M	5920
#define	DEFAULT_EIFSTIME_TX_20_40M	4960
#define DEFAULT_EIFSTIME_TX_10_50M	7400
#define DEFAULT_EIFSTIME_TX_20_50M	6200
#define	DEFAULT_EIFSTIME_RX_10_40M	4840
#define	DEFAULT_EIFSTIME_RX_20_40M	3880
#define DEFAULT_EIFSTIME_RX_10_50M	6050
#define DEFAULT_EIFSTIME_RX_20_50M	4850
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* PSK(3/6Mbps) 응답 시간 설정 레지스터
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_RSP_TOUT_3M_6M			PROTO_TIME_REG(4)
#define	DEFAULT_RSPTOUT3M_10_40M	5440
#define DEFAULT_RSPTOUT3M_20_40M	4480
#define DEFAULT_RSPTOUT3M_10_50M	6800
#define DEFAULT_RSPTOUT3M_20_50M	5600
#define	DEFAULT_RSPTOUT6M_10_40M	5440
#define DEFAULT_RSPTOUT6M_20_40M	4000
#define DEFAULT_RSPTOUT6M_10_50M	5600
#define DEFAULT_RSPTOUT6M_20_50M	5000
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* QAM(12/24Mbps) 응답 시간 설정 레지스터
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_RSP_TOUT_12M_24M		PROTO_TIME_REG(5)
#define	DEFAULT_RSPTOUT12M_10_40M	4160
#define DEFAULT_RSPTOUT12M_20_40M	3840
#define DEFAULT_RSPTOUT12M_10_50M	5200
#define DEFAULT_RSPTOUT12M_20_50M	4800
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* PSK(3/6Mbps) RTS 타임아웃시간 설정 레지스터
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_RTS_TOUT_3M_6M			PROTO_TIME_REG(6)
#define	DEFAULT_RTSTOUT3M_10_40M	7360
#define DEFAULT_RTSTOUT3M_20_40M	6000
#define DEFAULT_RTSTOUT3M_10_50M	9200
#define DEFAULT_RTSTOUT3M_20_50M	7500
#define	DEFAULT_RTSTOUT6M_10_40M	6400
#define DEFAULT_RTSTOUT6M_20_40M	5920
#define DEFAULT_RTSTOUT6M_10_50M	8000
#define DEFAULT_RTSTOUT6M_20_50M	7400
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* QAM(12/24Mbps) RTS 타임아웃시간 설정 레지스터
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_RTS_TOUT_12M_24M		PROTO_TIME_REG(7)
#define	DEFAULT_RTSTOUT12M_10_40M	6080
#define DEFAULT_RTSTOUT12M_20_40M	5760
#define DEFAULT_RTSTOUT12M_10_50M	7600
#define DEFAULT_RTSTOUT12M_20_50M	7200
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 마이크로초에 대한 H/W 카운터 기준값을 설정한다.
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_USEC_TIME				PROTO_TIME_REG(8)
#define DEFAULT_USECTIME_40M		40
#define DEFAULT_USECTIME_50M		50
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* TU에 대한 H/W 카운터 기준값을 설정한다.
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_TU_TIME					PROTO_TIME_REG(9)
#define	DEFAULT_TUTIME_40M			40960
#define DEFAULT_TUTIME_50M			51200
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* RF 지연 값을 설정한다.
 * 	- HW 기본값이 사용되므로 SW가 설정할 필요 없다. */
/*-----------------------------------------------------------------*/
#define		REG_RF_DELAY				PROTO_TIME_REG(10)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 타임스탬프 설정/확인 레지스터 */
/*-----------------------------------------------------------------*/
#define		REG_TSF_LOW					PROTO_TIME_REG(11)
#define		REG_TSF_HIGH				PROTO_TIME_REG(12)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 채널 스위칭 파라미터 설정 레지스터
 * 	- Ts0Duration, Ts1Duration, SyncTolerance, MaxChanSwTime 설정 */
/*-----------------------------------------------------------------*/
#define		REG_CHN_INTERVAL			PROTO_TIME_REG(13)
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 재시도 횟수 확인 레지스터 */
/*----------------------------------------------------------------------------------*/
#define		RETRY_CNT_REG           	(0x20C)
#define		REG_SSRC_SLRC				RETRY_CNT_REG
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 수신 패킷의 타임스탬프 영역이 수신된 시점의 로컬 타임스탬프 값
 * 	- 수신패킷에 같이 전달되는 것으로 변경되면서 이 레지스터는 더 이상 사용되지 않는다. */
/*----------------------------------------------------------------------------------*/
#define		LOCAL_TIME_REG(i)         	(0x210 + (4*(i)))
#define		REG_LOCAL_TIME_LOW			LOCAL_TIME_REG(0)
#define		REG_LOCAL_TIME_HIGH			LOCAL_TIME_REG(1)
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* 미사용 레지스터들 */
/*----------------------------------------------------------------------------------*/
/* Read dulication cache */
#define		CNT_DCACHE_REG          	(0x218)
#define		REG_DCACHE_CNT				CNT_DCACHE_REG
/* Read Rx buffer fail */
#define		CNT_RXBUF_FAIL_REG      	(0x21C)
#define		REG_RXBUF_FAIL_CNT			CNT_RXBUF_FAIL_REG
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* MAC 동작 관련 레지스터 */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* MAC 동작 관련 레지스터 셋
 * 	- MIB_DOT11OP_REG(0): 제조사 식별자 레지스터 (R/W)
 * 	- MIB_DOT11OP_REG(1): 제품 식별자 레지스터 (R/W)
 * 	- MIB_DOT11OP_REG(2~3): MAC 주소 레지스터 (R/W)
 * 	- MIB_DOT11OP_REG(4): 없음
 * 	- MIB_DOT11OP_REG(5): RTS 및 Fragmenetation 임계길이 설정 레지스터 (R/W)
 * 	- MIB_DOT11OP_REG(6): 재전송 최대회수 설정 레지스터 (R/W)
 * 	- MIB_DOT11OP_REG(7): 분할 프레임들을 수신할 때 기다리는 시간 설정 레지스터
 * 	- MIB_DOT11OP_REG(8~11): 시간슬롯0으로 패킷 전송 시의 EDCA Parameter Set 설정 레지스터
 * 	- MIB_DOT11OP_REG(12~15): 시간슬롯1으로 패킷 전송 시의 EDCA Parameter Set 설정 레지스터 */
/*-----------------------------------------------------------------*/
#define		MIB_DOT11OP_REG(i)        	(0x300 + (4*(i)))
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 제조사 식별자 레지스터 (R/W) */
/*-----------------------------------------------------------------*/
#define		REG_MANID					MIB_DOT11OP_REG(0)
#define DEFAULT_MANID				0x4954454b		/* KETI 아스키코드 */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 제품 식별자 레지스터 (R/W) */
/*-----------------------------------------------------------------*/
/* Produc ID register (R/W) */
#define 	REG_PRODID					MIB_DOT11OP_REG(1)
#define DEFAULT_PRODID				0x45564157		/* WAVE 아스키코드 */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* MAC 주소 레지스터 (R/W) */
/*-----------------------------------------------------------------*/
/* MAC주소의 하위 4바이트를 설정/확인한다.(배열 상의 앞 4바이트)
 * 	- little endian 형식을 따른다.
 * 	- addr[0]은 b0~7, addr[1]은 b8~15, addr[2]는 b16~23, addr[3]은 b24~31에 쓴다.*/
#define		REG_MACADDR_LOW32			MIB_DOT11OP_REG(2)
/* MAC주소의 상위 2바이트를 설정/확인한다.(배열 상의 뒤 2바이트)
 * 	- little endian 형식을 따른다.
 * 	- addr[5]는 b0~7, addr[6]은 b8~15에 쓴다. */
#define		REG_MACADDR_HIGH16			MIB_DOT11OP_REG(3)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* RTS 및 Fragmenetation 임계길이 설정 레지스터 (R/W)
 *  - RTS Threshold의 IEEE 802.11-2012 표준 상의 기본값은 65536이므로 거의 사용되지 않는다.
 *  - Fragement Threshold의 IEEE 802.11-2012 표준 상의 기본값은 aMPDUMaxLength이므로 거의 사용되지 않는다.
 *  - 단, 하드웨어 레지스터는 각각 12비트의 길이를 가지므로, 4095까지만 설정 가능하다.
 *    4095값을 설정하여 RTS/Fragmentaion이 발생하지 않도록 한다. */
/*-----------------------------------------------------------------*/
#define		REG_RTS_FRAG_THR			MIB_DOT11OP_REG(5)
#define	DEFAULT_RTSTHR				4095
#define DEFAULT_FRAGTHR				4095
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 재전송 최대회수 설정 레지스터 (R/W)
 * 	- IEEE 802.11-2012 표준 상의 기본값은 각각 7,4 이다. */
/*-----------------------------------------------------------------*/
#define		REG_RETRY_LIMIT				MIB_DOT11OP_REG(6)
#define DEFAULT_DOT11SRETRYLIMIT	7
#define DEFAULT_DOT11LRETRYLIMIT	4

/*-----------------------------------------------------------------*/
/* 분할 프레임들을 수신할 때 기다리는 시간 설정 레지스터
 * 	- 최초 분할패킷을 받은 후부터 이 값이 지날 때까지 마지막 분할패킷을 받지 못하면,
 * 	   해당 패킷들은 폐기된다. */
/*-----------------------------------------------------------------*/
#define		REG_RX_FRAG_LIFETIME		MIB_DOT11OP_REG(7)
#define	DEFAULT_DOT11MAXRXTLIFE		20000			/* us단위 */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* AC별 EDCA Parameter Set 설정 레지스터 */
/*-----------------------------------------------------------------*/
/* 시간슬롯0으로 AC_BK 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT0_AC_BK			MIB_DOT11OP_REG(8)
#define	DEFAULT_TIMESLOT0_AC_BK_CWMIN		15
#define	DEFAULT_TIMESLOT0_AC_BK_CWMAX		1023
#define DEFAULT_TIMESLOT0_AC_BK_AIFSN		9

/* 시간슬롯0으로 AC_BE 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT0_AC_BE			MIB_DOT11OP_REG(9)
#define	DEFAULT_TIMESLOT0_AC_BE_CWMIN		7
#define	DEFAULT_TIMESLOT0_AC_BE_CWMAX		1023
#define DEFAULT_TIMESLOT0_AC_BE_AIFSN		6

/* 시간슬롯0으로 AC_VI 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT0_AC_VI			MIB_DOT11OP_REG(10)
#define	DEFAULT_TIMESLOT0_AC_VI_CWMIN		3
#define	DEFAULT_TIMESLOT0_AC_VI_CWMAX		15
#define DEFAULT_TIMESLOT0_AC_VI_AIFSN		3

/* 시간슬롯0으로 AC_VO 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT0_AC_VO			MIB_DOT11OP_REG(11)
#define	DEFAULT_TIMESLOT0_AC_VO_CWMIN		3
#define	DEFAULT_TIMESLOT0_AC_VO_CWMAX		7
#define DEFAULT_TIMESLOT0_AC_VO_AIFSN		2

/* 시간슬롯1으로 AC_BK 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT1_AC_BK			MIB_DOT11OP_REG(12)
#define	DEFAULT_TIMESLOT1_AC_BK_CWMIN		15
#define	DEFAULT_TIMESLOT1_AC_BK_CWMAX		1023
#define DEFAULT_TIMESLOT1_AC_BK_AIFSN		9

/* 시간슬롯1으로 AC_BE 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT1_AC_BE			MIB_DOT11OP_REG(13)
#define	DEFAULT_TIMESLOT1_AC_BE_CWMIN		7
#define	DEFAULT_TIMESLOT1_AC_BE_CWMAX		1023
#define DEFAULT_TIMESLOT1_AC_BE_AIFSN		6

/* 시간슬롯1으로 AC_VI 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT1_AC_VI			MIB_DOT11OP_REG(14)
#define	DEFAULT_TIMESLOT1_AC_VI_CWMIN		3
#define	DEFAULT_TIMESLOT1_AC_VI_CWMAX		15
#define DEFAULT_TIMESLOT1_AC_VI_AIFSN		3

/* 시간슬롯1으로 AC_VO 패킷 전송 시 사용되는 EDCA Parameter Set */
#define		REG_EDCA_TIMESLOT1_AC_VO			MIB_DOT11OP_REG(15)
#define	DEFAULT_TIMESLOT1_AC_VO_CWMIN		3
#define	DEFAULT_TIMESLOT1_AC_VO_CWMAX		7
#define DEFAULT_TIMESLOT1_AC_VO_AIFSN		2
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* Promiscuous mode 설정 레지스터 */
/*----------------------------------------------------------------------------------*/
#define		REG_PROMISC					(0x348)
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* MAC 통계정보 레지스터 (RO) */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* 송신관련 통계정보 레지스터 셋 */
/*-----------------------------------------------------------------*/
#define		MIB_DOT11CNT_REGT(i)       	(0x400 + (4*(i)))
#define		REG_TX_TRY_CNT				MIB_DOT11CNT_REGT(0)	// Tx try
#define		REG_TX_FRM_SUCCESS_CNT		MIB_DOT11CNT_REGT(1)	// Tx frame success
#define		REG_TX_FRAG_SUCCESS_CNT		MIB_DOT11CNT_REGT(2)	// Tx fragment success
#define		REG_TX_MCAST_SUCCESS_CNT	MIB_DOT11CNT_REGT(3)	// Tx multicast frame success
#define		REG_TX_RETRY_SUCCESS_CNT	MIB_DOT11CNT_REGT(4)	// Tx success after retry
#define		REG_TX_RTS_SUCCESS_CNT		MIB_DOT11CNT_REGT(5)	// RTS success
#define		REG_TX_FAIL_CNT				MIB_DOT11CNT_REGT(6)	// Tx fail
#define		REG_TX_ACK_FAIL_CNT			MIB_DOT11CNT_REGT(7)	// ACK fail
#define		REG_TX_RTS_FAIL_CNT			MIB_DOT11CNT_REGT(8)	// RTS fail
#define		REG_TX_EXCEPTION_CNT		MIB_DOT11CNT_REGT(9)	// Exception
#define		REG_TX_RETRY_CNT			MIB_DOT11CNT_REGT(10)	// Retry

/*-----------------------------------------------------------------*/
/* 수신관련 통계정보 레지스터 셋 */
/*-----------------------------------------------------------------*/
#define		MIB_DOT11CNT_REGR(i)		(0x440 + (4*(i)))
#define		REG_RX_ALLFRM_CNT			MIB_DOT11CNT_REGR(0)	// All Rx frame
#define		REG_RX_MYFRM_CNT			MIB_DOT11CNT_REGR(1)	// My Rx frame
#define		REG_RX_FRAG_CNT				MIB_DOT11CNT_REGR(2)	// Rx fragment
#define		REG_RX_MCAST_CNT			MIB_DOT11CNT_REGR(3)	// Rx multicast frame(excludes wildcard)
#define		REG_RX_WCARD_CNT			MIB_DOT11CNT_REGR(4)	// Rx wildcard frame
#define		REG_RX_DUPLICATED_CNT		MIB_DOT11CNT_REGR(5)	// Rx dupllicated frame
#define		REG_RX_NOTMYFRM_CNT			MIB_DOT11CNT_REGR(6)	// Rx dupllicated frame
#define		REG_RX_FAIL_CNT				MIB_DOT11CNT_REGR(7)	// Rx fail
#define		REG_RX_FCS_ERROR_CNT		MIB_DOT11CNT_REGR(8)	// Rx FCS error
#define		REG_RX_BUF_FULL_CNT			MIB_DOT11CNT_REGR(9)	// Rx buffer full error
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 통계정보 클리어 레지스터 */
/*-----------------------------------------------------------------*/
#define		MIB_DOT11CMD_REG			(0x0480)
#define		REG_CNT_CLEAR				MIB_DOT11CMD_REG
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 패킷 전송 관련 레지스터 셋 */
/*----------------------------------------------------------------------------------*/

#ifdef _WAE_SHARED_HW_TXBUF_

/* 유닛블록의 크기 및 개수 */
#define		TX_UNIT_BLOCK_SIZE			128
#define		TX_UNIT_BLOCK_NUM			128

#define		TXFRM_INFO_REG(i)         	(0x530 + (4*(i)))

/*-----------------------------------------------------------------*/
/* Tx buffer에서다 음 fragment의 시작 위치를 나타내는 레지스터(RO) */
/*-----------------------------------------------------------------*/
#define		REG_TXFRM_PTR_NFRG			TXFRM_INFO_REG(1)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 전송 중인 프레임의 남아 있는 길이 (RW)
 * 	- 이 register는 TXHDR_REG를 설정할 때 함께 설정된다.
 * 	- Fragmentation된 프레임의 경우에는 프레임이 송신 성공할 때마다 값이 줄어든다. */
/*-----------------------------------------------------------------*/
#define		REG_TXFRM_LENGTH			TXFRM_INFO_REG(3)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 송신데이터 로드 레지스터
 * 	TX_QUEUE_DATA_INFO 레지스터에 전송할 패킷정보를 저장한 후, 1로 설정한다.
 * 	그러면 해당 패킷은 시간슬롯/AC 별 송신큐에 저장된다.
 * 	각 큐에는 최대 8개의 데이터를 저장할 수 있다. */
/*-----------------------------------------------------------------*/
#define		TX_QUEUE_DATA_LOAD			(0x540)
#define		REG_TX_QUEUE_DATA_LOAD		TX_QUEUE_DATA_LOAD
	#define	SET_TX_QUEUE_DATA_LOAD		(1<<0)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 송신큐에 저장될 데이터의 인덱스블록 위치 및 길이를 설정하는 레지스터
 * 	- b6~0: TX_START_INDEX - 송신데이터가 저장된 인덱스블록들 중 시작 인덱스블록의 번호
 * 	- b11~7: TX_INDEX_LEN - 송신데이터의 길이 (송신데이터가 차지하는 인덱스블록의 개수)
 * 		- 2^5=32 -> 32개의 블록이 가능 -> 32*128=4,096길이의 바이트까지 가능
 * 	- b14~12: 송신데이터의 TimeSlot/AC */
/*-----------------------------------------------------------------*/
#define		TX_QUEUE_DATA_INFO			(0x544)
#define		REG_TX_QUEUE_DATA_INFO		TX_QUEUE_DATA_INFO
	#define TX_QUEUE_DATA_INFO_TS0_BK	(0<<12)	/* TS0의 BK AC로 송신 */
	#define TX_QUEUE_DATA_INFO_TS0_BE	(1<<12)	/* TS0의 BE AC로 송신 */
	#define TX_QUEUE_DATA_INFO_TS0_VI	(2<<12)	/* TS0의 VI AC로 송신 */
	#define TX_QUEUE_DATA_INFO_TS0_VO	(3<<12)	/* TS0의 VO AC로 송신 */
	#define TX_QUEUE_DATA_INFO_TS1_BK	(4<<12)	/* TS1의 BK AC로 송신 */
	#define TX_QUEUE_DATA_INFO_TS1_BE	(5<<12)	/* TS1의 BE AC로 송신 */
	#define TX_QUEUE_DATA_INFO_TS1_VI	(6<<12)	/* TS1의 VI AC로 송신 */
	#define TX_QUEUE_DATA_INFO_TS1_VO	(7<<12)	/* TS1의 VO AC로 송신 */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 에러 레지스터
 * 	- 송신패킷을 송신큐에 저장할 때 에러가 발생한 경우, 에러의 원인을 알려준다. */
/*-----------------------------------------------------------------*/
#define		TX_QUEUE_ERROR				(0x548)
#define		REG_TX_QUEUE_ERROR			TX_QUEUE_ERROR
	#define	GET_TX_ERR_CNT(reg)			((reg) & 0xffff)		/* 에러카운트: 에러 발생시마다 증가 */
	#define	GET_TX_Q_ERR_TYPE(reg)		(((reg) >> 16) & 0xf)	/* 발생에러 종류 */
	#define TX_QUEUE_ERR_TYPE_QUEUE_FULL		(1<<0)		/* Queue Full */
	#define TX_QUEUE_ERR_TYPE_INDEX_OVERWRITE	(1<<1)		/* 이미 사용 중인 인덱스 */
	#define TX_QUEUE_ERR_TYPE_QUEUE_EMPTY		(1<<3)		/* 큐가 비었는데 송신 완료/실패 요청이 들어옴 */
	#define	GET_TX_ACC_CATEGORY			(((reg) >> 20) & 0xf)	/* 에러가 발생한 AC를 알려 줌 */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 송신버퍼에 저장된 송신데이터의 분포를 확인할 수 있는 레지스터 (RO)
 * 	- 각 비트는 각 128byte 블록이 채워져 있는지 여부를 나타내며,
 * 		전체 128-bit로 구성(128개의 블록)되어 있다. */
/*-----------------------------------------------------------------*/
#define		TX_QUEUE_DATA_MAP(i)		(0x550 + (4*(i)))
#define		REG_TX_QUEUE_DATA_MAP		TX_QUEUE_DATA_MAP
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 각 시간슬롯/AC 별 송신큐의 데이터 저장 상태 확인 레지스터
 * 	- b0~7의 각 비트가 설정되어 있으면 해당번째 큐에 데이터가 저장되어 있음을 나타낸다.
 * 	- 각 큐는 최대 8개까지 저장 가능. 1st 데이터가 가장 먼저 송신되며,
 * 		송신된 후에 2nd -> 1st, 3rd->2nd, … , 8th -> 7th 로 shift down된다. */
/*-----------------------------------------------------------------*/
#define		TX_QUEUE_STAT(i)			(0x560 + (4*(i)))
#define		REG_TX_QUEUE_STAT_TS0_BK	TX_QUEUE_STAT(0)
#define		REG_TX_QUEUE_STAT_TS0_BE	TX_QUEUE_STAT(1)
#define		REG_TX_QUEUE_STAT_TS0_VI	TX_QUEUE_STAT(2)
#define		REG_TX_QUEUE_STAT_TS0_VO	TX_QUEUE_STAT(3)
#define		REG_TX_QUEUE_STAT_TS1_BK	TX_QUEUE_STAT(4)
#define		REG_TX_QUEUE_STAT_TS1_BE	TX_QUEUE_STAT(5)
#define		REG_TX_QUEUE_STAT_TS1_VI	TX_QUEUE_STAT(6)
#define		REG_TX_QUEUE_STAT_TS1_VO	TX_QUEUE_STAT(7)
	#define	TX_1ST_DATA_FILL			(1<<0)	/* 1번째 송신데이터 로드 준비 완료 플래그 */
	#define	TX_2ND_DATA_FILL			(1<<1)	/* 2번째 송신데이터 로드 준비 완료 플래그 */
	#define	TX_3RD_DATA_FILL			(1<<2)	/* 3번째 송신데이터 로드 준비 완료 플래그 */
	#define	TX_4TH_DATA_FILL			(1<<3)	/* 4번째 송신데이터 로드 준비 완료 플래그 */
	#define	TX_5TH_DATA_FILL			(1<<4)	/* 5번째 송신데이터 로드 준비 완료 플래그 */
	#define	TX_6TH_DATA_FILL			(1<<5)	/* 6번째 송신데이터 로드 준비 완료 플래그 */
	#define	TX_7TH_DATA_FILL			(1<<6)	/* 7번째 송신데이터 로드 준비 완료 플래그 */
	#define	TX_8TH_DATA_FILL			(1<<7)	/* 8번째 송신데이터 로드 준비 완료 플래그 */
/*-----------------------------------------------------------------*/

#else

/*-----------------------------------------------------------------*/
/* 패킷 송신 명령 레지스터 - 각 비트를 1로 설정하면 해당되는 버퍼의 패킷이 전송된다. */
/*-----------------------------------------------------------------*/
#define		TXFRM_INFO_REG(i)         	(0x530 + (4*(i)))
#define		REG_TX_START				TXFRM_INFO_REG(0)
#define	TX_START_TIMESLOT0_AC_BK			(1<<0)
#define	TX_START_TIMESLOT0_AC_BE			(1<<1)
#define TX_START_TIMESLOT0_AC_VI			(1<<2)
#define TX_START_TIMESLOT0_AC_VO			(1<<3)
#define TX_START_TIMESLOT1_AC_BK			(1<<4)
#define TX_START_TIMESLOT1_AC_BE			(1<<5)
#define TX_START_TIMESLOT1_AC_VI			(1<<6)
#define TX_START_TIMESLOT1_AC_VO			(1<<7)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 재전송회수 설정 레지스터 */
/*-----------------------------------------------------------------*/
#define		REG_CUR_FRAME_RETRY_CNT		TXFRM_INFO_REG(2)
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 송신버퍼 상태 레지스터
 * 	- 현재 버퍼의 상태를 확인한다. */
/*-----------------------------------------------------------------*/
#define		TX_BUF_STATUS				(0x540)
#define		REG_TXBUF_STATUS			TX_BUF_STATUS
/*-----------------------------------------------------------------*/

#endif
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
/* RF/Analog 설정 레지스터 셋 */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* MAX2829 명령 설정 레지스터 */
/*-----------------------------------------------------------------*/
#define		RF_COMMAND  		   		(0x700)
#define		REG_RF_PARAMS_SET			RF_COMMAND
#define	RF_PARAMS_SET_TIMESLOT0		(1<<0)
#define	RF_PARAMS_SET_TIMESLOT1		(1<<1)
#define	RF_PARAMS_SET_AGC			(1<<2)
#define RF_PARAMS_SET_REGION1		(1<<3)
#define	RF_PARAMS_SET_REGION2		(1<<4)
#define	RF_PARAMS_SET_SINGLE1		(1<<5)
#define RF_PARAMS_SET_SINGLE2		(1<<6)
#define RF_PARAMS_SET_SINGLE3		(1<<7)
#define RF_PARAMS_SET_SINGLE4		(1<<8)

#define		RF_CONTROL			   		(0x704)
#define		REG_RF_CONTROL				RF_CONTROL
#define	RF_CONTROL_POWERON			(1<<12)
#define RF_CONTROL_TX_FIX			(1<<24)
#define RF_CONTROL_RX_FIX			(1<<25)

#define		PHY_TEST			   		(0x710)
#define		REG_PHY_TEST				PHY_TEST
#define	LOOPBACK_TEST_EN			(1<<0)
#define	COSINE_TEST_EN				(1<<1)
#define PREAMBLE_TEST_EN			(1<<2)

/*-----------------------------------------------------------------*/
/* RF command address */
/*-----------------------------------------------------------------*/
#define		RF_CMD_SET1			   		(0x718)
#define		RF_CMD_SET2			   		(0x71C)
#define		REG_RF_PARAMS_ADDR(i)		(RF_CMD_SET1 + (4*(i)))			// 0<= i <= 1
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* 미사용 */
/*-----------------------------------------------------------------*/
#define		RF_PULSE_SET  		   		(0x720)
#define		REG_RF_PULSE_SET
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* MAX2829 레지스터 명령어
 * 	- MAX2829로 전송되는 명령어들이 저장되는 레지스터 */
/*-----------------------------------------------------------------*/
#define		RF_TIMESLOT0_CMD(x)				(0x740 + ((x)*4))
#define		REG_RF_PARAMS_TIMESLOT0(x)							RF_TIMESLOT0_CMD(x)			// 0x38000740
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_0				REG_RF_PARAMS_TIMESLOT0(0)	// 0x38000740
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_1				REG_RF_PARAMS_TIMESLOT0(1)	// 0x38000744
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_STANDBY			REG_RF_PARAMS_TIMESLOT0(2)    // 0x38000748
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_INTDIVRATIO 	REG_RF_PARAMS_TIMESLOT0(3)    // 0x3800074c
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_FRADIVRATIO   	REG_RF_PARAMS_TIMESLOT0(4)    // 0x38000750
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_BANDSEL       	REG_RF_PARAMS_TIMESLOT0(5)    // 0x38000754
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_CALIB         	REG_RF_PARAMS_TIMESLOT0(6)    // 0x38000758
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_LFILTER       	REG_RF_PARAMS_TIMESLOT0(7)    // 0x3800075c
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXCTRL        	REG_RF_PARAMS_TIMESLOT0(8)    // 0x38000760
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_TXLINEAR      	REG_RF_PARAMS_TIMESLOT0(9)    // 0x38000764
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_PA            	REG_RF_PARAMS_TIMESLOT0(10)   // 0x38000768
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_RXGAIN        	REG_RF_PARAMS_TIMESLOT0(11)   // 0x3800076c
#define		REG_RF_PARAMS_TIMESLOT0_MAX2829_REG_TXGAIN        	REG_RF_PARAMS_TIMESLOT0(12)   // 0x38000770

#define		RF_TIMESLOT1_CMD(x)				(0x780 + ((x)*4))
#define		REG_RF_PARAMS_TIMESLOT1(x)							RF_TIMESLOT1_CMD(x)			// 0x38000780
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_0				REG_RF_PARAMS_TIMESLOT1(0)    // 0x38000780
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_1				REG_RF_PARAMS_TIMESLOT1(1)    // 0x38000784
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_STANDBY			REG_RF_PARAMS_TIMESLOT1(2)    // 0x38000788
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_INTDIVRATIO   	REG_RF_PARAMS_TIMESLOT1(3)    // 0x3800078c
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_FRADIVRATIO   	REG_RF_PARAMS_TIMESLOT1(4)    // 0x38000790
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_BANDSEL       	REG_RF_PARAMS_TIMESLOT1(5)    // 0x38000794
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_CALIB         	REG_RF_PARAMS_TIMESLOT1(6)    // 0x38000798
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_LFILTER       	REG_RF_PARAMS_TIMESLOT1(7)    // 0x3800079c
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXCTRL        	REG_RF_PARAMS_TIMESLOT1(8)    // 0x380007a0
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_TXLINEAR      	REG_RF_PARAMS_TIMESLOT1(9)    // 0x380007a4
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_PA            	REG_RF_PARAMS_TIMESLOT1(10)   // 0x380007a8
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_RXGAIN        	REG_RF_PARAMS_TIMESLOT1(11)   // 0x380007ac
#define		REG_RF_PARAMS_TIMESLOT1_MAX2829_REG_TXGAIN        	REG_RF_PARAMS_TIMESLOT1(12)   // 0x380007b0
/*-----------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/




/*----------------------------------------------------------------------------------*/
/* PHY 설정 레지스터들  */
/*----------------------------------------------------------------------------------*/
#define		PHY_AGC_LIMIT		   		(0x800)
#define		REG_PHY_AGC_LIMIT			PHY_AGC_LIMIT

#define		PHY_CONTROL			   		(0x804)
#define		REG_PHY_CONTROL				PHY_CONTROL

#define		PHY_RSSI_STATUS		   		(0x808)
#define		REG_PHY_RSSI				PHY_RSSI_STATUS

#define		PHY_RSSI_THRD		   		(0x80C)
#define		REG_PHY_RSSI_THR			PHY_RSSI_THRD

#define		RF_AGC_SET					(0x810)
#define		REG_PHY_EAGC_SET			RF_AGC_SET

#define		RF_RXAGC_STATUS				(0x820)
#define		REG_PHY_EAGC_STATUS			RF_RXAGC_STATUS

#define		RF_RXAGC_TIMING				(0x824)
#define		REG_PHY_EAGC_TIMING			RF_RXAGC_TIMING

#define		RF_RXAGC_CONTROL			(0x828)
#define		REG_PHY_EAGC_CTRL			RF_RXAGC_CONTROL
#define		DEFAULT_INITIAL_EAGC_CONF	((0x46 << 24) | (0x49 << 16) | (0x3c << 8) | (0x30 <<0))

#define 	RF_RXAGC_RECEIVE_DB			(0x82c)
#define		REG_PHY_EAGC_DB				RF_RXAGC_RECEIVE_DB

#define		TXENA_DELAY					(0x0b00)
#define		REG_PHY_TXENA_DELAY			TXENA_DELAY
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 채널 스위칭 설정 레지스터 셋 */
/*----------------------------------------------------------------------------------*/
/* 채널스위칭 관련 명령 레지스터
 * 	- sw가 채널스위칭을 직접 할 수 있는 기능 제공.
 * 	- 사용할 일 없음 */
#define		CH_COORD_CMD				(0x840)
#define		REG_COORD_CMD				CH_COORD_CMD

/* 채널스위칭 제어 레지스터
 * 	- b0: 채널스위칭을 pps에 동기화할 지 여부 (1: pps에 동기화, 0: 내부 타이머에 동기화)
 * 		- 어차피 pps가 안 들어올 때에는 내부 타이머에 동기화되므로, 항상 1로 두는 것이 낫다.
 * 	- b1: 가상채널스위칭타이머 on
 * 		- 가상의 채널스위칭타이머가 동작하며, 각 인터벌 시점에서 rf칩 제어 신호가 발생하지 않는다.
 * 		- b0값에 따라 gps에 동기화될 수도 있다.
 * 		- MAC_OP_REG 레지스터의 채널스위칭활성화 비트가 비활성화되어 있어야 의미가 있다.
 * 		  (해당 비트가 활성화되어 있으면, 실제 채널스위칭이 수행되므로)
 * 		- 가상채널스위칭타이머만이 동작 중인 상태에서는 채널변경시점에서 인터럽트가 발생하지 않는다.
 * 		- 부팅 시에 활성화해 둔다.(초기에 내부타이머에 의해 돌다가 pps신호가 들어오면 동기된다) */
#define		CH_COORD_CONTROL			(0x844)
#define		REG_COORD_CTRL				CH_COORD_CONTROL
#define		GPS_PPS_EN					(1<<0)
#define		TX_COORD_TIMER_EN			(1<<1)

/* Tx coordination timer 값을 읽거나 쓴다.
 * 	- 15:0  : usec 단위 값 (0~999)
 * 	- 31:16 : msec 단위 값 (0~999)
 * 	- 여기에 값을 쓰거 Tx_COORD_TIMER_SET명령을 내리면 이 레지스터의 값으로
 * 	  채널스위칭타이머가 세팅된다.
 * 	- SW에서 사용할 일 없음.  */
#define		CH_COORD_TIMER				(0x848)
#define		REG_COORD_TIMER				CH_COORD_TIMER

#define		ABS_TIMER_LOW				(0x84c)
#define		REG_COORD_ABS_TIMER_LOW32	ABS_TIMER_LOW

#define 	ABS_TIMER_HIGH				(0x850)
#define		REG_COORD_ABS_TIMER_HIGH32	ABS_TIMER_HIGH

/* SyncInterval 시작점(=TimeSlot0 시작점)으로부터 point1, point2 시점에 인터럽트를 발생시키기 위한
 * point1, point2 시간값을 써주는 레지스터.
 * 	- 9:0   : usec단위 시간값
 * 	- 19:10 : msec단위 시간
 * 	- 두 값을 더한 시간이 point가 된다.
 * 	- point값이 Ts0Duration보다 길면 TimeSlot1구간에 발생되는 셈이며,
 * 	  SyncInterval보다 길면 인터럽트가 발생하지 않는다.
 * 	- 이는 실제 채널스위칭을 수행하지 않는 상태(=가상채널스위칭 상태)에서도 사용 가능하다. */
#define		CH_SW_POINT1				(0x860)
#define		REG_CH_SW_POINT1			CH_SW_POINT1
#define		CH_SW_POINT2				(0x864)
#define		REG_CH_SW_POINT2			CH_SW_POINT2
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 다이버시트 설정 레지스터 */
/*----------------------------------------------------------------------------------*/
#define		REG_DIVERSITY				(0xA08)
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 보안 레지스터 셋 */
/*----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/* 공통 레지스터 */
/*------------------------------------------------------------------*/
/* 버전레지스터 */
#define		SEC_VERSION					(0)

/* 기본값 로딩 레지스터(초기화) */
#define		SEC_DEFAULT_LOAD			(0x0004)

/* 기능 On/Off */
#define		SEC_ENABLE_REG				(0x0008)
#define	SEC_OP_ENABLE				1
#define	SEC_OP_DISABLE				0

/* 인터럽트 상태 */
#define		SEC_INTERRUPT_STATUS		(0x0010)

/* 인터럽트 마스크 */
#define		SEC_INTERRUPT_MASK			(0x0014)
#define	SEC_INTR_AES_ENC_DONE		(1<<0)
#define	SEC_INTR_AES_DEC_VALID		(1<<1)
#define SEC_INTR_AES_DEC_ERROR		(1<<2)
#define SEC_INTR_HASH_DONE			(1<<3)
#define SEC_INTR_ECC256_DONE		(1<<4)
#define SEC_INTR_ECC224_DONE		(1<<5)
#define SEC_INTR_RND_GEN_DONE		(1<<6)
#define	SEC_INTR_ENABLE_ALL			0xffffffff

/* Manufacturer ID */
#define		SEC_MANUFACTURER			(0x0020)
#define		DEFAULT_SEC_MANID		0x4954454b

/* Product ID */
#define		SEC_PRODUCT_ID				(0x0024)
#define		DEFAULT_SEC_PRODID		0x57534543

/* 제어(Control) */
#define		SEC_CONTROL					(0x0030)
#define RANDOM_SKEY_USE				(1<<0)		// Use Random Symm. Key in AES, Hash block
#define	RANDOM_SKEY_NOT_USE			(0<<0)

/* 입력 데이터 길이(바이트단위) */
#define		SEC_DATA_IN_LEN				(0x0050)

/* 출력 데이터 길이(바이트단위) */
#define		SEC_DATA_OUT_LEN			(0x0054)
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/* AES system register */
/*------------------------------------------------------------------*/
/* AES Command */
#define		AES_COMMAND					(0x0100)
#define	AES_ENC_START				(1<<0)
#define	AES_DEC_START				(1<<1)

/* AES Control */
#define		AES_CONTROL					(0x0104)
#define	MANUAL_KEY_USE				(1<<2)
#define MANUAL_KEY_NOT_USE			(0<<2)
#define REG_NONCE_USE				(1<<3)
#define REG_NONCE_NOT_USE			(0<<3)
#define	REG_IV_USE					(1<<4)
#define	REG_IV_NOT_USE				(0<<4)
#define	AES_MANUAL_KEY_BS			(1<<8)
#define AES_DEC_TAG_BS				(1<<9)
#define AES_ENC_TAG_BS				(1<<10)
#define	AES_REG_NONCE_BS			(1<<11)
#define	AES_REG_IV_BS				(1<<12)
#ifdef _BYTE_SHUFFLE_
#define	set_aes_control(x)			((0x1f<<8) | (x))
#else
#define	set_aes_control(x)			(x)
#endif

/* AES status */
#define		AES_STATUS					(0x0108)
#define	AES_STATUS_LOAD				(1<<0)
#define	AES_STATUS_DEC				(1<<1)
#define	AES_STATUS_WORKING			(1<<2)
#define	AES_STATUS_DONE				(1<<3)
#define	AES_STATUS_VALID			(1<<4)
#define	AES_STATUS_EN				(1<<8)

/* AES Manual Key */
#define		AES_MANUAL_KEY(i)			(0x0110+(4*(i)))

/* AES Decryption Tag - AES복호화를 위해 TAG를 써 줌. */
#define		AES_DEC_TAG(i)				(0x0120+(4*(i)))

/* AES Encryption Tag - AES 암호화 결과인 TAG가 저장됨 */
#define		AES_ENC_TAG(i)				(0x0130+(4*(i)))

/* AES none */
#define		AES_REG_NONCE(i)			(0x0140+(4*(i)))		// i: 0~2 (12바이트)

/* AES Initial Vector */
#define		AES_REG_IV(i)				(0x0150+(4*(i)))		// i: 0~3 (16바이트)
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/* Hash 시스템 레지스터 */
/*------------------------------------------------------------------*/
/* Command */
#define		HASH_COMMAND				(0x0200)
#define	HASH_OP_START				(1<<0)	// 해시동작 명령 -> HASH_DONE 인터럽트 발생
#define	HASH_OP_CLEAR				(1<<1)

/* Control */
#define		HASH_CONTROL				(0x0204)
#define	HASH_DATA_SHUFFLE			(1<<0)
#define REG_SKEY_USE				(1<<1)
#define	REG_SKEY_BS					(1<<8)
#define	HASH_C_BS					(1<<9)
#define	HASH_T_BS					(1<<10)
#ifdef _BYTE_SHUFFLE_
#define set_hash_control(x)			((0x7<<8) | (x))
#else
#define set_hash_control(x)			(x)
#endif

/* STatus */
#define		HASH_STATUS					(0x0208)
#define	HASH_STATUS_CLEAR			(1<<0)
#define	HASH_STATUS_LOAD			(1<<1)
#define	HASH_STATUS_NEW				(1<<2)
#define	HASH_STATUS_LAST			(1<<3)
#define	HASH_STATUS_WORKING			(1<<4)
#define	HASH_STATUS_DONE			(1<<5)
#define	HASH_STATUS_MORE			(1<<6)
#define	HASH_STATUS_CDONE			(1<<7)
#define	HASH_STATUS_EN				(1<<16)
#define	HASH_STATUS_DATA_SHUFFLE	(1<<17)

/* Hash Symm Key */
#define		HASH_REG_SKEY(i)			(0x210+(4*(i)))		// i: 0~3 (16바이트)

/* Hash C */
#define		HASH_REG_C(i)				(0x220+(4*(i)))		// i: 0~3 (16바이트)

/* HASH T */
#define		HASH_REG_T(i)				(0x230+(4*(i)))		// i: 0~4 (20바이트)
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/* ECC256 시스템 레지스터 */
/*------------------------------------------------------------------*/
/* Command */
#define		ECC256_COMMAND				(0x0300)
#define	ECC256_OP_START				(1<<0)	// ECC256 동작 명령 -> ECC256_DONE 인터럽트 발생

/* Control */
#define		ECC256_CONTROL				(0x0304)
#define	ECC256_MODE0_SIGN_VERIFY	0
#define	ECC256_MODE0_SIGN_GEN		1
#define	ECC256_MODE0_KEY_GEN		2
#define	ECC256_MODE0_KDF			3
#define	ECC256_MODE0_ECQV_VERIFY	4
#define	ECC256_MODE0_Y_RECOVER		5
#define	ECC256_MODE0_ECQV_GEN		6
#define	ECC256_MODE1_NORMAL			0
#define	ECC256_MODE1_KDF_DEC		1
#define	ECC256_MODE1_FAST			2
#define	ECC256_RND_D_USE			(1<<5)
#define	ECC256_RND_K_USE			(1<<6)
#define	ECC256_REG_E_USE			(1<<7)
#define ECC256_D_BS					(1<<8)
#define ECC256_K_BS					(1<<9)
#define ECC256_E_BS					(1<<10)
#define ECC256_R_BS					(1<<11)
#define ECC256_S_BS					(1<<12)
#define ECC256_QX_BS				(1<<13)
#define ECC256_QY_BS				(1<<14)
#define ECC256_QX2_BS				(1<<15)
#define ECC256_QY2_BS				(1<<16)
#define set_ecc_mode0(x)			((x)&7)
#define set_ecc_mode1(x)            (((x)&3)<<3)
#ifdef _BYTE_SHUFFLE_
#define set_ecc_control(x)			((0x1ff<<8) | (x))
#else
#define set_ecc_control(x)			(x)
#endif


/* Status */
#define		ECC256_STATUS				(0x0308)

/* Result */
#define		ECC256_RESULT				(0x030c)
#define	ECC256_RESULT_VALID			(1<<0)
#define	ECC256_RESULT_ERROR			(1<<0)

/* ECC256 D - ECC연산을 위한 개인키를 입력하는 레지스터 */
#define		ECC256_REG_D(i)				(0x0310+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 K */
#define		ECC256_REG_K(i)				(0x0330+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 R */
#define		ECC256_REG_R(i)				(0x0350+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 S */
#define		ECC256_REG_S(i)				(0x0370+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 Qx */
#define		ECC256_REG_QX(i)			(0x0390+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 Qy */
#define		ECC256_REG_QY(i)			(0x03B0+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 Qx2 */
#define		ECC256_REG_QX2(i)			(0x03D0+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 Qy2 */
#define		ECC256_REG_QY2(i)			(0x03F0+(4*(i)))		// i: 0~7 (32바이트)

/* ECC256 E */
#define		ECC256_REG_E(i)				(0x0410+(4*(i)))
/*------------------------------------------------------------------*/


/*------------------------------------------------------------------*/
/* ECC224 시스템 레지스터 */
/*------------------------------------------------------------------*/
/* Command */
#define		ECC224_COMMAND				(0x0500)
#define	ECC224_OP_START				(1<<0)	// ECC224 동작 명령 -> ECC224_DONE 인터럽트 발생

/* Control */
#define		ECC224_CONTROL				(0x0504)
#define	ECC224_MODE0_SIGN_VERIFY	(0<<0)
#define	ECC224_MODE0_SIGN_GEN		(1<<0)
#define	ECC224_MODE0_KEY_GEN		(2<<0)
#define	ECC224_MODE0_KDF			(3<<0)
#define	ECC224_MODE0_ECQV_VERIFY	(4<<0)
#define	ECC224_MODE0_Y_RECOVER		(5<<0)
#define	ECC224_MODE0_ECQV_GEN		(6<<0)
#define	ECC224_MODE1_NORMAL			(0<<3)
#define	ECC224_MODE1_KDF_DEC		(1<<3)
#define	ECC224_MODE1_FAST			(2<<3)
#define	ECC224_RND_D_USE			(1<<5)
#define	ECC224_RND_K_USE			(1<<6)
#define	ECC224_REG_E_USE			(1<<7)
#define ECC224_D_BS					(1<<8)
#define ECC224_K_BS					(1<<9)
#define ECC224_E_BS					(1<<10)
#define ECC224_R_BS					(1<<11)
#define ECC224_S_BS					(1<<12)
#define ECC224_QX_BS				(1<<13)
#define ECC224_QY_BS				(1<<14)
#define ECC224_QX2_BS				(1<<15)
#define ECC224_QXY_BS				(1<<16)

/* Status */
#define		ECC224_STATUS				(0x0508)

/* Result */
#define		ECC224_RESULT				(0x050c)
#define	ECC224_RESULT_VALID			(1<<0)
#define	ECC224_RESULT_ERROR			(1<<0)

/* ECC224 D */
#define		ECC224_REG_D(i)				(0x0510+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 K */
#define		ECC224_REG_K(i)				(0x0530+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 R */
#define		ECC224_REG_R(i)				(0x0550+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 S */
#define		ECC224_REG_S(i)				(0x0570+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 Qx */
#define		ECC224_REG_QX(i)			(0x0590+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 Qy */
#define		ECC224_REG_QY(i)			(0x05B0+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 Qx2 */
#define		ECC224_REG_QX2(i)			(0x05D0+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 Qy2 */
#define		ECC224_REG_QY2(i)			(0x05F0+(4*(i)))		// i: 0~6 (28바이트)

/* ECC224 E */
#define		ECC224_REG_E(i)				(0x0510+(4*(i)))
/*------------------------------------------------------------------*/


/*------------------------------------------------------------------*/
/* Random generator 레지스터 */
/*------------------------------------------------------------------*/
/* Command */
#define		SEC_RANDOM_COMMAND			(0x0700)
#define	SEC_RANDOM_CMD_PRIVATE_KEY	(1<<0)
#define	SEC_RANDOM_CMD_K_VALUE		(1<<1)
#define	SEC_RANDOM_CMD_SYMM_KEY		(1<<2)
#define	SEC_RANDOM_CMD_NONCE		(1<<3)

/* ECC Random K - ECC 서명에 사용되는 랜덤값 K value가 저장됨. */
#define		ECC_RND_K(i)				(0x0710+(4*(i)))		// i: 0~7 (32바이트)

/* Hash Random Symm Key */
#define 	HASH_RND_SKEY(i)			(0x0730+(4*(i)))		// i: 0~3 (16바이트)

/* AES random Nonce */
#define		AES_RND_NONCE(i)			(0x0740+(4*(i)))		// i: 0~2 (12바이트)
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/* 개인키 레지스터 */
/*------------------------------------------------------------------*/
/* 개인키 보호 레지스터
 * 	- 1을 쓰면 PRIVATE_KEY 레지스터에서 키 값이 읽히지 않는다.
 * 	- 0을 쓰면 PRIVATE_KEY 레지스터에서 키 값이 읽힌다.*/
#define		PRIVATE_KEY_P				(0x0f00)
#define PRIVATE_KEY_PROTECT			1
#define	PRIVATE_KEY_UNPROTECT		0

/* 개인키 저장 레지스터 */
#define		PRIVATE_KEY(i)				(0x0f10+(4*(i)))		// i: 0~7 (32바이트)
/*------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* 기타 레지스터들 */
/*----------------------------------------------------------------------------------*/
/* Random generator block test registers */
#define		RN_GEN_COMMAND				(0xe00)
#define		REG_RN_GEN_CMD				RN_GEN_COMMAND
#define		RN_GEN_VALUE				(0xe04)
#define		REG_RN_GEN_VALUE			REG_RN_GEN_CMD
/*----------------------------------------------------------------------------------*/


#endif //LIBCONDOR_CONDOR_HW_REGS_H

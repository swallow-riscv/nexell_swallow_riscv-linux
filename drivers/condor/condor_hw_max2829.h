//
// Created by gyun on 2019-03-23.
//

#ifndef LIBCONDOR_CONDOR_HW_MAX2829_H
#define LIBCONDOR_CONDOR_HW_MAX2829_H

/*----------------------------------------------------------------------------------*/
/* 개수 */
/*----------------------------------------------------------------------------------*/
/* MAX2829가 지원하는 채널의 개수
 * 	- 원래 MAX2829는 4.9~5.94GHz 등의 많은 채널을 지원하지만, WAVE 채널의 개수인 7개만 사용한다.
 * 		- OOB 채널설정 기능 지원을 위해 앞뒤로 몇개의 채널을 설정할 수 있도록 하여 총 15개를 지원하도록 한다. */
#define	MAX2829_SUPPORT_CHAN_MAXNUM		WAVE_10M_CHANNEL_NUM
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* MAX2829 레지스터 주소 */
/*----------------------------------------------------------------------------------*/
#define		MAX2829_REG_0_ADDR				0x0			// MAX2829 register 0
#define		MAX2829_REG_1_ADDR				0x1			// MAX2829 register 1
#define		MAX2829_REG_STANDBY_ADDR		0x2			// MAX2829 standby
#define		MAX2829_REG_INTDIVRATIO_ADDR	0x3        	// MAX2829 integer-divider ratio
#define		MAX2829_REG_FRADIVRATIO_ADDR	0x4         // MAX2829 fractional-divider ratio
#define		MAX2829_REG_BANDSEL_ADDR		0x5         // MAX2829 band select and PLL
#define		MAX2829_REG_CALIB_ADDR			0x6         // MAX2829 calibration
#define		MAX2829_REG_LFILTER_ADDR		0x7         // MAX2829 lowpass filter
#define		MAX2829_REG_RXCTRL_ADDR			0x8         // MAX2829 rx control/rssi
#define		MAX2829_REG_TXLINEAR_ADDR		0x9         // MAX2829 tx linearity/baseband gain
#define		MAX2829_REG_PA_ADDR				0xa         // MAX2829 PA bias DAC
#define		MAX2829_REG_RXGAIN_ADDR			0xb         // MAX2829 Rx gain
#define		MAX2829_REG_TXGAIN_ADDR			0xc         // MAX2829 Tx VGA gain
/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/
/* MAC2829 기본 설정 값 */
/*----------------------------------------------------------------------------------*/
/* 데이터시트 상의 기본값 */
#define		MAX2829_REG_0_DEFVAL			((0x0001140<<4) | MAX2829_REG_0_ADDR)
/* 데이터시트 상의 기본값 */
#define		MAX2829_REG_1_DEFVAL			((0x00000CA<<4) | MAX2829_REG_1_ADDR)
/* 13: Normal operation(Not MIMO), 11: Not use Vreference, 10: Not use PA Bias DAC */
#define		MAX2829_REG_STANDBY_DEFVAL		((((0<<13)|(0<<11)|(0<<10) | (1<<12)|(7<<0)) << 4) | MAX2829_REG_STANDBY_ADDR)
/* 초기 접속 채널을 5.89GHz(CCH)로 설정 */
#define		MAX2829_REG_INTDIVRATIO_DEFVAL	((((1<<12)|(0xeb<<0)) << 4) | MAX2829_REG_INTDIVRATIO_ADDR)
#define		MAX2829_REG_FRADIVRATIO_DEFVAL	((0x2666 << 4) | MAX2829_REG_FRADIVRATIO_ADDR)
/* 	13: Normal operation(Not MIMO), 10-9: Highest frequency band VCO sub-band, 8: bandswitch is doen by SPI
	7: auto VCO bandswitch disable, 6: 5.47~5.875GHz band, 5: PLL Charge-pump current 4mA,
	3-1: Reference-Divider Ratio - 40MHz, 0: RF frequency band - 5GHz */
#define		MAX2829_REG_BANDSEL_DEFVAL		((((0<<13)|(3<<9)|(1<<8)|(0<<7)|(1<<6)|(1<<5)|(2<<1)|(1<<0) | (3<<11)|(0<<4)) << 4) | MAX2829_REG_BANDSEL_ADDR)
/* 12-11: default value in Datasheet, 1: Tx calibration mode disabled, 0: Rx calibration mode disabled */
#define		MAX2829_REG_CALIB_DEFVAL		((((3<<11)|(0<<1)|(0<<0) | (1<<10)) << 4) | MAX2829_REG_CALIB_ADDR)
/* 11: RSSI high bandwidth disable, 6-5,4-3,2-0: default value in datasheet */
#define		MAX2829_REG_LFILTER_DEFVAL		((((0<<11)|(1<<5)|(0<<3)|(0<<0)) << 4) | MAX2829_REG_LFILTER_ADDR)
/* 12: Setting VGA gain using parallel I/O, 11: High RSSI output range, 10: RSSI enabled indepedent of RXHP, 8: outputs RSSI signal in Rx mode
   2: Rx Highpass -3dB conrner frequency is "30KHz" when RXHP=0 */
#define		MAX2829_REG_RXCTRL_DEFVAL	 	((((0<<12)|(1<<11)|(1<<10)|(0<<8)|(1<<2) | (1<<5)|(1<<0)) << 4) | MAX2829_REG_RXCTRL_ADDR)
/*	10: Tx VGA gain programmed using SPI, 9-8: Min. PA driver linearity, 7-6: Min. Tx VGA linearty,
	3-2: Min. Tx upconverter linearity, 1-0: Tx baseband gain=max baseband gain - 5dB */
#define		MAX2829_REG_TXLINEAR_DEFVAL		((((1<<10)|(0<<8)|(0<<6)|(0<<2)|(0<<0)) << 4) | MAX2829_REG_TXLINEAR_ADDR)
/* We don't use PA Bias DAC - Default value in datasheet */
#define		MAX2829_REG_PA_DEFVAL			((((0xf<<6)|(0<<0)) << 4) | MAX2829_REG_PA_ADDR)
/* b6-0: Rx gain */
#define		MAX2829_REG_RXGAIN_DEFVAL		(((0x4f<<0) << 4) | MAX2829_REG_RXGAIN_ADDR)
/* b5-0: Tx gain - 최대값으로 설정 */
#define		MAX2829_REG_TXGAIN_DEFVAL		(((0x3f<<0) << 4) | MAX2829_REG_TXGAIN_ADDR)
/*----------------------------------------------------------------------------------*/

#endif //LIBCONDOR_CONDOR_HW_MAX2829_H

/*
 * ibus.c
 *
 *  Created on: 25.01.2021
 *      Author: filip
 */
#include "stm32f4xx.h"
#include "global_constants.h"
#include "global_variables.h"
#include "global_functions.h"
#include "ibus.h"

volatile uint8_t rxBuf[32];
static uint8_t rxindex = 0;
static double current_time;
static double last_time;
static double gap_time;

static void failsafe_RX();

//for debugging only:
static int pok = 0;
static int pok1 = 0;

void DMA2_Stream5_IRQHandler(void) {
	//if channel4 transfer is completed:

	//if channel5 transfer is completed:
	if (DMA2->HISR & DMA_HISR_TCIF5) {
		DMA2->HIFCR |= DMA_HIFCR_CTCIF5;
		DMA2_Stream5->CR &= ~DMA_SxCR_EN;

		ibus_received = 1;

		if (I2C1_read_write_flag == 0) {
			EXTI->IMR |= EXTI_IMR_IM9;
		}
	}
}
//for DMA:
//void USART2_IRQHandler(void) {
//	//RDR not empty flag:
//	static double time_flag5_1;
//	if (0 != (USART_ISR_RXNE & USART2->ISR)) {
//		//check gap duration if bigger than 500 us brake
//
////tu jest co� �le przy zakomenntowanym dzia�a porawinie a jesli to odkomentuj� przerwa wynosi oko�o 0.065[s] a to czas trwania 10 ramek ibusa bez sensu
////		if ( (get_Global_Time() - time_flag5_1) > 0.0005) {
////			rxindex = 0;
////		}
////
////		time_flag5_1 = get_Global_Time();
//
//		//	read actual value of I-BUS (Interrupt flag will be automatically removed):
//		rxBuf[rxindex] = USART2->RDR;
//		if (rxindex == 1 && rxBuf[rxindex] == 0x40) {
//
//			//block USART2 interrupt until DMA reading finish and data are processed:
//			USART2->CR1 &= ~USART_CR1_RXNEIE;
//			EXTI->IMR &= ~EXTI_IMR_IM9;
//
//			//start DMA USART2 reading:
////			for (int i=0;i<300;i++){
////				;
////			}
//
////TO nie dzia�a
//
////			static int i;
////			while(USART_ISR_RXNE & USART2->ISR){
////
////			i=USART2->ISR;
////			}
//
//// to tez nie :
//
////			if(USART_ISR_RXNE&USART2->ISR){
////			USART2->RQR|=USART_RQR_RXFRQ;
////			}
//
////to dzia�a
//
////delay_micro(90);//waiting for clearing RDR register FIND BETTER WAY!!!!
//
//			DMA1_Channel5->CCR |= DMA_CCR_EN;
//
//		} else if (rxindex == 0 && rxBuf[rxindex] != 0x20) {
//			rxindex = 0;
//
//		} else if (rxindex == 1 && rxBuf[rxindex] != 0x40) {
//			rxindex = 0;
//
//		} else {
//			//if header is right increase rxindex
//			rxindex++;
//		}
//	}
////idle detection flag:
//	if (0 != (USART_ISR_IDLE & USART2->ISR)) {
//		USART2->ICR |= USART_ICR_IDLECF;
//
//		if (ibus_received == 0) {
//			USART2->CR1 |= USART_CR1_RXNEIE;
//		}
//
//	}
//}

//for Interrupt:
void USART1_IRQHandler(void) {

	int16_t status_register;
	//ODBIOR OD RX:
	status_register=USART1->SR;
	//	check if interrupt was generated by right flag:
	if (0 != (USART_SR_RXNE & USART1->SR)) {
		//	read actual value of I-BUS (flag will be automatically removed):

		current_time = get_Global_Time();
		gap_time = current_time - last_time;
		last_time = current_time;
		//check if gap is bigger than 0.5 [ms]
		if (gap_time > 0.5) {
			rxindex = 0;
		}
		rxBuf[rxindex] = USART1->DR;
		if (rxindex == 31) {
			ibus_received = 1;
			//block receiving new data until old data are processed:
			USART1->CR1 &= ~USART_CR1_RXNEIE;
			pok++;
		}	//	waiting for header 0x2040:
		else if (rxindex == 0 && rxBuf[rxindex] != 0x20) {
			rxindex = 0;
		} else if (rxindex == 1 && rxBuf[rxindex] != 0x40) {
			rxindex = 0;
		}
		else {	//if header is right increase rxindex
			rxindex++;
		}
	}
	//idle detection flag:
	status_register=USART1->SR;
	 if (0 != (USART_SR_IDLE & USART1->SR)) {
		 USART1->DR;
			if (ibus_received == 0) {
				USART1->CR1 |= USART_CR1_RXNEIE;
			}
			pok1++;
		}
	}


void Ibus_save() {
	static double time_flag3_1;
	if ((get_Global_Time() - time_flag3_1) >= MAX_NO_SIGNAL_TIME) {
		failsafe_type = 3;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;
	}

	// checking checksum and rewriting rxBuf to channels:
	if (ibus_received) {
		time_flag3_1 = get_Global_Time();
		uint16_t checksum = 0xFFFF;
		for (int8_t i = 0; i < 30; i++) {
			checksum -= rxBuf[i];
		}
		if (checksum == ((rxBuf[31] << 8) + rxBuf[30])) {
			for (int8_t i = 0; i < CHANNELS; i++) {
				channels[i] = (rxBuf[2 * (i + 1) + 1] << 8)
						+ rxBuf[2 * (i + 1)];
			}

			failsafe_RX();
			Throttle = channels[2];
		}

		//	unlock receiving new data from i-Bus:
		rxindex = 0;
		ibus_received = 0;
		USART1->CR1 |= USART_CR1_RXNEIE;
	}
}
static void failsafe_RX() {
	// Arming switch - SA
	if (channels[4] <= DISARM_VALUE) {
		failsafe_type = 1;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;
	} else if (channels[0] <= MIN_RX_SIGNAL || channels[0] >= MAX_RX_SIGNAL
			|| channels[1] <= MIN_RX_SIGNAL || channels[1] >= MAX_RX_SIGNAL
			|| channels[2] <= MIN_RX_SIGNAL || channels[2] >= MAX_RX_SIGNAL
			|| channels[3] <= MIN_RX_SIGNAL || channels[3] >= MAX_RX_SIGNAL) {

		failsafe_type = 2;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;

	} else {
		PWM_M1 = &pwm_m1;
		PWM_M2 = &pwm_m2;
		PWM_M3 = &pwm_m3;
		PWM_M4 = &pwm_m4;
	}
}

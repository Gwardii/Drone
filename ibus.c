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
#include "flash.h"
#include "ibus.h"

volatile uint8_t rxBuf[32];
static uint8_t rxindex = 0;
//static double current_time;
//static double last_time;
//static double gap_time;

static void failsafe_RX();

void DMA2_Stream5_IRQHandler(void) {
	//if channel4 transfer is completed:

	//if channel5 transfer is completed:
	if (DMA2->HISR & DMA_HISR_TCIF5) {
		DMA2->HIFCR |= DMA_HIFCR_CTCIF5;
		DMA2_Stream5->CR &= ~DMA_SxCR_EN;

		ibus_received = 1;
		if (imu_received == 0) {
			EXTI->IMR |= EXTI_IMR_IM4; //unblock IMU reading
		}
	}
}
//for DMA:
void USART1_IRQHandler(void) {
	//RDR not empty flag:
	if (0 != (USART_SR_RXNE & USART1->SR)) {
		//check gap duration if bigger than 500 us brake

//tu jest co� �le przy zakomenntowanym dzia�a porawinie a jesli to odkomentuj� przerwa wynosi oko�o 0.065[s] a to czas trwania 10 ramek ibusa bez sensu
//		if ( (get_Global_Time() - time_flag5_1) > SEC_TOUS(0.0005)) {
//			rxindex = 0;
//		}
//
//		time_flag5_1 = get_Global_Time();

		//	read actual value of I-BUS (Interrupt flag will be automatically removed):
		rxBuf[rxindex] = USART1->DR;
		if (rxindex == 1 && rxBuf[rxindex] == 0x40) {

			//block USART1 interrupt and IMU reading until DMA reading finish and data are processed:
			USART1->CR1 &= ~USART_CR1_RXNEIE;
			EXTI->IMR &= ~EXTI_IMR_IM4;
			DMA2_Stream5->CR |= DMA_SxCR_EN;

		} else if (rxindex == 0 && rxBuf[rxindex] != 0x20) {
			rxindex = 0;

		} else if (rxindex == 1 && rxBuf[rxindex] != 0x40) {
			rxindex = 0;

		} else {
			//if header is right increase rxindex
			rxindex++;
		}
	}
//idle detection flag:
	if (0 != (USART_SR_IDLE & USART1->SR)) {
		USART1->DR;
		if (ibus_received == 0) {
			USART1->CR1 |= USART_CR1_RXNEIE;
			DMA2_Stream5->CR &= ~DMA_SxCR_EN;
		}
	}
}

////for Interrupt:
//void USART1_IRQHandler(void) {
//
//	//ODBIOR OD RX:
//
//	//	check if interrupt was generated by right flag:
//	if (0 != (USART_SR_RXNE & USART1->SR)) {
//		//	read actual value of I-BUS (flag will be automatically removed):
//
//		current_time = get_Global_Time();
//		gap_time = current_time - last_time;
//		last_time = current_time;
//		//check if gap is bigger than 0.5 [ms]
//		if (gap_time > SEC_TO_US(0.5)) {
//			rxindex = 0;
//		}
//		rxBuf[rxindex] = USART1->DR;
//		if (rxindex == 31) {
//			ibus_received = 1;
//			//block receiving new data until old data are processed:
//			USART1->CR1 &= ~USART_CR1_RXNEIE;
//			pok++;
//		}	//	waiting for header 0x2040:
//		else if (rxindex == 0 && rxBuf[rxindex] != 0x20) {
//			rxindex = 0;
//		} else if (rxindex == 1 && rxBuf[rxindex] != 0x40) {
//			rxindex = 0;
//		}
//		else {	//if header is right increase rxindex
//			rxindex++;
//		}
//	}
//	//idle detection flag:
//
//	 if (0 != (USART_SR_IDLE & USART1->SR)) {
//		 USART1->DR;
//			if (ibus_received == 0) {
//				USART1->CR1 |= USART_CR1_RXNEIE;
//			}
//			pok1++;
//		}
//	}

void Ibus_save() {
	uint16_t checksum=0xFFFF;

	if ((get_Global_Time() - time_flag3_1) >= SEC_TO_US(MAX_NO_SIGNAL_TIME)) {
		FailSafe_type = RX_TIMEOUT;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;
	}

	// checking checksum and rewriting rxBuf to channels:
	if (ibus_received) {
		time_flag3_1 = get_Global_Time();
		checksum = 0xFFFF;
		for (int8_t i = 0; i < 30; i++) {
			checksum -= rxBuf[i];
		}
		if (checksum == ((rxBuf[31] << 8) + rxBuf[30])) {
			for (int8_t i = 0; i < CHANNELS; i++) {
				channels_previous_values[i]=channels[i];
				channels[i] = (rxBuf[2 * (i + 1) + 1] << 8)
						+ rxBuf[2 * (i + 1)];
			}

			failsafe_RX();
			Throttle = channels[2];

			if(channels[6] < 1400){
				flight_mode=FLIGHT_MODE_ACRO;
			}
			else if(channels[6]>1450){
				flight_mode=FLIGHT_MODE_STABLE;
			}

			if (channels[7] >= 1400 && channels[7] < 1700) {
				blackbox_command = 1;
			} else if (channels[7] >= 1700) {
				blackbox_command = 0;
			} else {
				blackbox_command = 0;
			}
		}

		//	unlock receiving new data from i-Bus:
		rxindex = 0;
		ibus_received = 0;
		USART1->CR1 |= USART_CR1_RXNEIE;
	}
}
static void failsafe_RX() {

#if defined(USE_PREARM)
	// Arming switch:
	if (channels[4] <= ARM_VALUE) {
		FailSafe_type = DISARMED;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;
		arming_status = -1;
		if (channels[8] > PREARM_VALUE ) {
			arming_status = 0;
		}
	} else if (channels[0] <= MIN_RX_SIGNAL || channels[0] >= MAX_RX_SIGNAL
			|| channels[1] <= MIN_RX_SIGNAL || channels[1] >= MAX_RX_SIGNAL
			|| channels[2] <= MIN_RX_SIGNAL || channels[2] >= MAX_RX_SIGNAL
			|| channels[3] <= MIN_RX_SIGNAL || channels[3] >= MAX_RX_SIGNAL) {

		FailSafe_type = INCORRECT_CHANNELS_VALUES;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;

	} else if (arming_status == 0 || arming_status == 1) {
		arming_status = 1;
		motor_1_value_pointer = &motor_1_value;
		motor_2_value_pointer = &motor_2_value;
		motor_3_value_pointer = &motor_3_value;
		motor_4_value_pointer = &motor_4_value;
	}
#else
	// Arming switch:
	if (channels[4] <= ARM_VALUE) {
		FailSafe_type = DISARMED;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;

		}
	 else if (channels[0] <= MIN_RX_SIGNAL || channels[0] >= MAX_RX_SIGNAL
			|| channels[1] <= MIN_RX_SIGNAL || channels[1] >= MAX_RX_SIGNAL
			|| channels[2] <= MIN_RX_SIGNAL || channels[2] >= MAX_RX_SIGNAL
			|| channels[3] <= MIN_RX_SIGNAL || channels[3] >= MAX_RX_SIGNAL) {

		 for(uint8_t i=0; i<CHANNELS;i++){
			 channels[i]=channels_previous_values[i];
		 }

		FailSafe_type = INCORRECT_CHANNELS_VALUES;
		EXTI->SWIER |= EXTI_SWIER_SWIER15;

	} else  {

		motor_1_value_pointer = &motor_1_value;
		motor_2_value_pointer = &motor_2_value;
		motor_3_value_pointer = &motor_3_value;
		motor_4_value_pointer = &motor_4_value;
	}
#endif
}


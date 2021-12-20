/*
 * conection.c
 *
 *  Created on: 17.03.2021
 *      Author: symon
 */
#include "stm32f4xx.h"
#include "global_constants.h"
#include "global_variables.h"
#include "global_functions.h"
#include "connection.h"

uint8_t table_of_bytes_to_sent[2 * ALL_ELEMENTS_TO_SEND + 4];
static volatile int32_t txTransmitted;
static volatile int32_t txSize;
static uint8_t bufor[50];

void DMA2_Stream6_IRQHandler(void) {
	//if channel6 transfer USART6 is completed:
	if (DMA2->HISR & DMA_HISR_TCIF6) {
		DMA2->HIFCR |= DMA_HIFCR_CTCIF6;
		DMA2_Stream6->CR &= ~DMA_SxCR_EN;
		transmitting_is_Done = 1;
	}
//	//if channel3 transfer I2C is completed:
//	if (DMA1->ISR & DMA_ISR_TCIF3) {
//		DMA1->IFCR |= DMA_IFCR_CTCIF3;
//		I2C1_read_write_flag = 1;
//
//		DMA1_Channel3->CCR &= ~DMA_CCR_EN;
//		if (ibus_received == 0) {
//			USART2->CR1 |= USART_CR1_RXNEIE;
//			//USART2->CR1 |= USART_CR1_IDLEIE;
//		}
//	}
}

void USART6_IRQHandler(void) {

	if (0 != (USART_SR_RXNE & USART6->SR)) {
		//	read actual value (flag will be automatically removed):
		static uint8_t i;

		bufor[i] = USART6->DR;
		switch (bufor[i]) {
			case 0:
				blackbox_command=0;
			break;
			case 1:
				blackbox_command=1;
			break;
			case 2:
				blackbox_command=2;
			break;
			default:
			break;
		}


		i++;
		if (i >= 50) {
			i = 0;
		}
	}
	if (USART6->SR & USART_SR_IDLE)
	    {
	        USART6->DR;                             //If not read usart will crush
	        DMA2_Stream6->CR &= ~DMA_SxCR_EN;       /* Disable DMA on stream 6 - trigers dma TC */
	    }


//TRANSMISJA:

//	check if interrupt was generated by right flag:
	if ((0 != (USART_CR1_TXEIE & USART6->CR1))
			&& (0 != (USART_SR_TXE & USART6->SR))) {
		//	transmit data:
		if ((0 != txSize) && (txTransmitted < txSize)) {
			USART6->DR = table_of_bytes_to_sent[txTransmitted];
			txTransmitted += 1;
		}
		// if everything is transmitted, unable transmission interrupts:
		if (txTransmitted == txSize) {
			USART6->CR1 &= ~USART_CR1_TXEIE;
			transmitting_is_Done = 1;
		}
	}
}


void USART3_IRQHandler(void) {

	if (USART6->SR & USART_SR_IDLE)
	    {
	        USART3->DR;                             //If not read usart will crush
	       // DMA1_StreamX->CR &= ~DMA_SxCR_EN;     // Disable DMA on stream X
	    }

}


void print(uint16_t x[], uint8_t data_to_send) {
	uint16_t sum = 0;

	table_of_bytes_to_sent[0] = 0x20;
	table_of_bytes_to_sent[1] = 0x40;
	for (int i = 0; i < data_to_send; i++) {
		table_of_bytes_to_sent[2 * i + 2] = x[i] >> 8;
		table_of_bytes_to_sent[2 * i + 3] = x[i];
		sum += x[i];
	}
	table_of_bytes_to_sent[2 * data_to_send + 2] = sum >> 8;
	table_of_bytes_to_sent[2 * data_to_send + 3] = sum;

	txSize = 2 * data_to_send + 4;

	txTransmitted = 0;
	transmitting_is_Done = 0;
	New_data_to_send = 0;

	DMA2_Stream6->M0AR = (uint32_t) (table_of_bytes_to_sent);
	DMA2_Stream6->NDTR = 2 * ALL_ELEMENTS_TO_SEND + 4;

	DMA2_Stream6->CR |= DMA_SxCR_EN;			//DMA option
	//USART6->CR1 |= USART_CR1_TXEIE;			//Interrupt option
}



/*
 * global_functions.c

 *
 *  Created on: 03.05.2021
 *      Author: symon
 */
#include "stm32f4xx.h"
#include "global_constants.h"
#include "global_variables.h"
#include "global_functions.h"

void delay_micro(uint16_t time) {

	TIM7->ARR = time-1;
	TIM7->EGR |= TIM_EGR_UG;
	TIM7->CR1 |= TIM_CR1_CEN;

	while (Tim_7_flag != 1) {
		; //wait for time [us]
	}
	Tim_7_flag = 0;
}
void delay_mili(uint16_t time) {
//wait for time [ms]
	for (uint16_t i = 0; i < time; i++) {
		delay_micro(1000);
	}
}

void TIM6_DAC_IRQHandler() {
	if (TIM_SR_UIF & TIM6->SR) {
		TIM6->SR &= ~TIM_SR_UIF;
		Global_Time += (TIM6->ARR+1) * 0.000001;
	}
}
void TIM7_IRQHandler() {
	if (TIM_SR_UIF & TIM7->SR) {
		TIM7->SR &= ~TIM_SR_UIF;
		TIM7->CR1 &= ~TIM_CR1_CEN;
		Tim_7_flag = 1;
	}
}

double get_Global_Time() {
	return (Global_Time + (TIM6->CNT) * 0.000001);
}

void anti_windup(ThreeF *sum_err, PIDF *R_PIDF, PIDF *P_PIDF, PIDF *Y_PIDF) {
	if (channels[4] > 1600) {

		if ((sum_err->roll * R_PIDF->I ) > MAX_I_CORRECTION) {
			sum_err->roll = MAX_I_CORRECTION / R_PIDF->I ;
		} else if ((sum_err->roll * R_PIDF->I) < -MAX_I_CORRECTION) {
			sum_err->roll = -MAX_I_CORRECTION / R_PIDF->I ;
		}
		if ((sum_err->pitch * P_PIDF->I ) > MAX_I_CORRECTION) {
			sum_err->pitch = MAX_I_CORRECTION / P_PIDF->I ;
		} else if ((sum_err->pitch * P_PIDF->I ) < -MAX_I_CORRECTION) {
			sum_err->pitch = -MAX_I_CORRECTION / P_PIDF->I ;
		}
		if ((sum_err->yaw * Y_PIDF->I) > MAX_I_CORRECTION) {
			sum_err->yaw = MAX_I_CORRECTION / Y_PIDF->I ;
		} else if ((sum_err->yaw * Y_PIDF->I ) < -MAX_I_CORRECTION) {
			sum_err->yaw = -MAX_I_CORRECTION / Y_PIDF->I ;
		}
	}

	else {			// quad is disarmed so turn off I term of corrections
		sum_err->roll = 0;
		sum_err->pitch = 0;
		sum_err->yaw = 0;
	}

}

void set_motors(ThreeF corr) {
	const uint16_t max_value = 4000;

	//	Make corrections:
	//	right back:
	motor_1_value = Throttle * 2 - corr.roll + corr.pitch - corr.yaw;
	//	right front:
	motor_2_value = Throttle * 2  - corr.roll - corr.pitch + corr.yaw;
	//	left back:
	motor_3_value = Throttle * 2  + corr.roll + corr.pitch + corr.yaw;
	//	left front:
	motor_4_value = Throttle * 2  + corr.roll - corr.pitch - corr.yaw;

	if (motor_1_value < IDLE_VALUE*2) {
		motor_1_value = IDLE_VALUE*2;
	} else if (motor_1_value > max_value)
		motor_1_value = max_value;
	if (motor_2_value < IDLE_VALUE*2) {
		motor_2_value = IDLE_VALUE*2;
	} else if (motor_2_value > max_value)
		motor_2_value = max_value;
	if (motor_3_value < IDLE_VALUE*2) {
		motor_3_value = IDLE_VALUE*2;
	} else if (motor_3_value > max_value)
		motor_3_value = max_value;
	if (motor_4_value < IDLE_VALUE*2) {
		motor_4_value = IDLE_VALUE*2;
	} else if (motor_4_value > max_value)
		motor_4_value = max_value;
}

void update_motors() {

#if defined(ESC_PROTOCOL_DSHOT)

	//	Dshot:

	fill_Dshot_buffer(prepare_Dshot_package(*motor_1_value_pointer),prepare_Dshot_package(*motor_2_value_pointer),prepare_Dshot_package(*motor_3_value_pointer),prepare_Dshot_package(*motor_4_value_pointer));

	DMA1_Stream6->PAR = (uint32_t) (&(TIM2->CCR4));
	DMA1_Stream6->M0AR = (uint32_t) (dshot_buffer_1);
	DMA1_Stream6->NDTR = DSHOT_BUFFER_LENGTH;

	DMA1_Stream7->PAR = (uint32_t) (&(TIM3->CCR3));
	DMA1_Stream7->M0AR = (uint32_t) (dshot_buffer_2);
	DMA1_Stream7->NDTR = DSHOT_BUFFER_LENGTH;

	DMA1_Stream2->PAR = (uint32_t) (&(TIM3->CCR4));
	DMA1_Stream2->M0AR = (uint32_t) (dshot_buffer_3);
	DMA1_Stream2->NDTR = DSHOT_BUFFER_LENGTH;

	DMA1_Stream1->PAR = (uint32_t) (&(TIM2->CCR3));
	DMA1_Stream1->M0AR = (uint32_t) (dshot_buffer_4);
	DMA1_Stream1->NDTR = DSHOT_BUFFER_LENGTH;

	DMA1_Stream1->CR |= DMA_SxCR_EN;
	DMA1_Stream2->CR |= DMA_SxCR_EN;
	DMA1_Stream6->CR |= DMA_SxCR_EN;
	DMA1_Stream7->CR |= DMA_SxCR_EN;

#elif defined(ESC_PROTOCOL_DSHOT_BURST)

	//	Dshot with burst transfer:

//	fill_Dshot_burst_buffer(prepare_Dshot_package(*motor_1_value_pointer),prepare_Dshot_package(*motor_2_value_pointer),prepare_Dshot_package(*motor_3_value_pointer),prepare_Dshot_package(*motor_4_value_pointer));
//	DMA1_Stream2->CR |= DMA_SxCR_EN;
//	DMA1_Stream6->CR |= DMA_SxCR_EN;

#elif defined(ESC_PROTOCOL_ONESHOT125)|| defined(ESC_PROTOCOL_ONESHOT_V1)

	//	OneShot125 or OneShot125_v2:

	prepare_OneShot_PWM();

	TIM2->EGR |= TIM_EGR_UG;
	TIM3->EGR |= TIM_EGR_UG;
	TIM2->CR1 |= TIM_CR1_CEN;
	TIM3->CR1 |= TIM_CR1_CEN;

#elif defined (ESC_PROTOCOL_PWM)

	//	PWM:

	TIM2->CCR4 = *motor_1_value_pointer; 			//value motor 1
	TIM3->CCR3 = *motor_2_value_pointer; 			//value motor 2
	TIM3->CCR4 = *motor_3_value_pointer; 			//value motor 3
	TIM2->CCR3 = *motor_4_value_pointer; 			//value motor 4

#endif
}

void turn_ON_BLUE_LED() {
	GPIOB->BSRR |= GPIO_BSRR_BR5;
}
void turn_OFF_BLUE_LED() {
	GPIOB->BSRR |= GPIO_BSRR_BS5;
}
void turn_ON_RED_LED() {
	GPIOB->BSRR |= GPIO_BSRR_BR4;
}
void turn_OFF_RED_LED() {
	GPIOB->BSRR |= GPIO_BSRR_BS4;
}

void EXTI9_5_IRQHandler() {
	if ((EXTI->PR & EXTI_PR_PR5)) {
		EXTI->PR |= EXTI_PR_PR5; // clear this bit setting it high


		USB_detected= GPIOC->IDR & GPIO_IDR_ID5;

	}
	if ((EXTI->PR & EXTI_PR_PR6)) {
		EXTI->PR |= EXTI_PR_PR6; // clear this bit setting it high
	}
	if ((EXTI->PR & EXTI_PR_PR7)) {
		EXTI->PR |= EXTI_PR_PR7; // clear this bit setting it high
	}
	if ((EXTI->PR & EXTI_PR_PR8)) {
		EXTI->PR |= EXTI_PR_PR8; // clear this bit setting it high
	}
	if ((EXTI->PR & EXTI_PR_PR9)) {
		EXTI->PR |= EXTI_PR_PR9; // clear this bit setting it high
	}
}


//FAILSAFE HANDLER:
void EXTI15_10_IRQHandler() {
	if ((EXTI->PR & EXTI_PR_PR10)) {
		EXTI->PR |= EXTI_PR_PR10; // clear this bit setting it high
	}
	if ((EXTI->PR & EXTI_PR_PR11)) {
		EXTI->PR |= EXTI_PR_PR11; // clear this bit setting it high
	}
	if ((EXTI->PR & EXTI_PR_PR12)) {
		EXTI->PR |= EXTI_PR_PR12; // clear this bit setting it high
	}
	if ((EXTI->PR & EXTI_PR_PR13)) {
		EXTI->PR |= EXTI_PR_PR13; // clear this bit setting it high
	}
	if ((EXTI->PR & EXTI_PR_PR14)) {
		EXTI->PR |= EXTI_PR_PR14; // clear this bit setting it high
	}
	//FAILSAFEs
	if ((EXTI->PR & EXTI_PR_PR15)) {
		static uint16_t err_counter[10];
		EXTI->PR |= EXTI_PR_PR15; // clear(setting 1) this bit (and at the same time bit SWIER15)

		switch (failsafe_type) {
		case 1://DISARM

			motor_1_value_pointer = &MOTOR_OFF;
			motor_2_value_pointer = &MOTOR_OFF;
			motor_3_value_pointer = &MOTOR_OFF;
			motor_4_value_pointer = &MOTOR_OFF;

			err_counter[0]++;
			failsafe_type = 0;
			break;
		case 2://BAD_CHANNELS_VALUES

			motor_1_value_pointer = &MOTOR_OFF;
			motor_2_value_pointer = &MOTOR_OFF;
			motor_3_value_pointer = &MOTOR_OFF;
			motor_4_value_pointer = &MOTOR_OFF;

			err_counter[1]++;
			failsafe_type = 0;
			break;
		case 3://RX_TIMEOUT

			motor_1_value_pointer = &MOTOR_OFF;
			motor_2_value_pointer = &MOTOR_OFF;
			motor_3_value_pointer = &MOTOR_OFF;
			motor_4_value_pointer = &MOTOR_OFF;

			err_counter[2]++;
			failsafe_type = 0;
			break;
		case 4:
			err_counter[3]++;
			failsafe_type = 0;
			break;
		case 5:
			err_counter[4]++;
			failsafe_type = 0;
			break;
		case 6:
			err_counter[5]++;
			failsafe_type = 0;
			break;
		case 7:
				err_counter[6]++;
				failsafe_type = 0;
				break;
		}
	}
}

//	motor 1
void DMA1_Stream6_IRQHandler(void) {

	if (DMA1->HISR & DMA_HISR_TCIF6) {
		DMA1->HIFCR |= DMA_HIFCR_CTCIF6;

	}
	if (DMA1->HISR & DMA_HISR_HTIF6) {
		DMA1->HIFCR |= DMA_HIFCR_CHTIF6;
	}
	if (DMA1->HISR & DMA_HISR_DMEIF6) {
		DMA1->HIFCR |= DMA_HIFCR_CDMEIF6;
	}
	if (DMA1->HISR & DMA_HISR_TEIF6) {
		DMA1->HIFCR |= DMA_HIFCR_CTEIF6;
	}
}

//	motor 2
void DMA1_Stream7_IRQHandler(void) {

	if (DMA1->HISR & DMA_HISR_TCIF7) {
		DMA1->HIFCR |= DMA_HIFCR_CTCIF7;
	}
	if (DMA1->HISR & DMA_HISR_HTIF7) {
		DMA1->HIFCR |= DMA_HIFCR_CHTIF7;
	}
	if (DMA1->HISR & DMA_HISR_DMEIF7) {
		DMA1->HIFCR |= DMA_HIFCR_CDMEIF7;
	}
	if (DMA1->HISR & DMA_HISR_TEIF7) {
		DMA1->HIFCR |= DMA_HIFCR_CTEIF7;
	}
}

//	motor 3
void DMA1_Stream2_IRQHandler(void) {

	if (DMA1->LISR & DMA_LISR_TCIF2) {
		DMA1->LIFCR |= DMA_LIFCR_CTCIF2;
	}
	if (DMA1->LISR & DMA_LISR_HTIF2) {
		DMA1->LIFCR |= DMA_LIFCR_CHTIF2;
	}
	if (DMA1->LISR & DMA_LISR_DMEIF2) {
		DMA1->LIFCR |= DMA_LIFCR_CDMEIF2;
	}
	if (DMA1->LISR & DMA_LISR_TEIF2) {
		DMA1->LIFCR |= DMA_LIFCR_CTEIF2;
	}
}

//	motor 4
void DMA1_Stream1_IRQHandler(void) {

	if (DMA1->LISR & DMA_LISR_TCIF1) {
		DMA1->LIFCR |= DMA_LIFCR_CTCIF1;
	}

	if (DMA1->LISR & DMA_LISR_HTIF1) {
		DMA1->LIFCR |= DMA_LIFCR_CHTIF1;
	}
	if (DMA1->LISR & DMA_LISR_DMEIF1) {
		DMA1->LIFCR |= DMA_LIFCR_CDMEIF1;
	}
	if (DMA1->LISR & DMA_LISR_TEIF1) {
		DMA1->LIFCR |= DMA_LIFCR_CTEIF1;
	}

}


void DMA1_Stream4_IRQHandler(void) {

	if (DMA1->HISR & DMA_HISR_TCIF4) {
		DMA1->HIFCR |= DMA_HIFCR_CTCIF4;
		DMA1_Stream4->CR &= ~DMA_SxCR_EN;
	}
	if (DMA1->HISR & DMA_HISR_HTIF4) {
		DMA1->HIFCR |= DMA_HIFCR_CHTIF4;
	}
	if (DMA1->HISR & DMA_HISR_DMEIF4) {
		DMA1->HIFCR |= DMA_HIFCR_CDMEIF4;
	}
	if (DMA1->HISR & DMA_HISR_TEIF4) {
		DMA1->HIFCR |= DMA_HIFCR_CTEIF4;
	}
}



uint16_t get_Dshot_checksum(uint16_t value) {
	value=value<<1;
	return (value ^ (value >> 4) ^ (value >> 8)) & 0x0F;
}

uint16_t prepare_Dshot_package(uint16_t value) {
	//value is in range of 2000-4000 so I need to transform it into Dshot range (48-2047)
	value -= 1953;
	if(value>0 && value<48){
		value=48;
	}
	return ((value << 5) | get_Dshot_checksum(value));

}

void fill_Dshot_buffer(uint16_t m1_value, uint16_t m2_value, uint16_t m3_value,
		uint16_t m4_value) {

	for (uint8_t i = 2; i < DSHOT_BUFFER_LENGTH; i++) {
		if ((1 << (i-2)) & m1_value) {
			dshot_buffer_1[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_1_LENGTH;
		} else {
			dshot_buffer_1[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_0_LENGTH;
		}
		if ((1 << (i-2)) & m2_value) {
			dshot_buffer_2[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_1_LENGTH;
		} else {
			dshot_buffer_2[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_0_LENGTH;
		}
		if ((1 << (i-2)) & m3_value) {
			dshot_buffer_3[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_1_LENGTH;
		} else {
			dshot_buffer_3[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_0_LENGTH;
		}
		if ((1 << (i-2)) & m4_value) {
			dshot_buffer_4[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_1_LENGTH;
		} else {
			dshot_buffer_4[DSHOT_BUFFER_LENGTH - 1 - i] = DSHOT_0_LENGTH;
		}
	}
	// make 0 pulse after Dshot frame:
	dshot_buffer_1[DSHOT_BUFFER_LENGTH - 1]=0;
	dshot_buffer_1[DSHOT_BUFFER_LENGTH - 2]=0;
	dshot_buffer_2[DSHOT_BUFFER_LENGTH - 1]=0;
	dshot_buffer_2[DSHOT_BUFFER_LENGTH - 2]=0;
	dshot_buffer_3[DSHOT_BUFFER_LENGTH - 1]=0;
	dshot_buffer_3[DSHOT_BUFFER_LENGTH - 2]=0;
	dshot_buffer_4[DSHOT_BUFFER_LENGTH - 1]=0;
	dshot_buffer_4[DSHOT_BUFFER_LENGTH - 2]=0;
}

void fill_Dshot_burst_buffer(uint16_t m1_value, uint16_t m2_value,
		uint16_t m3_value, uint16_t m4_value) {

// buffer array looks like this [first bit motor1; first bit motor2 ;...; last bit motor1;last bit motor2]

	for (int i = 2; i < DSHOT_BUFFER_LENGTH; i++) {
		if ((1 << (i-2)) & m4_value) {
			dshot_buffer_4_1[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i-1] =
			DSHOT_1_LENGTH;
		} else {
			dshot_buffer_4_1[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i-1] =
			DSHOT_0_LENGTH;
		}
		if ((1 << (i-2)) & m1_value ){
			dshot_buffer_4_1[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i ] =
			DSHOT_1_LENGTH;
		} else {
			dshot_buffer_4_1[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i] =
			DSHOT_0_LENGTH;
		}
		if ((1 << (i-2)) & m2_value) {
			dshot_buffer_2_3[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i-1] =
			DSHOT_1_LENGTH;
		} else {
			dshot_buffer_2_3[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i-1] =
			DSHOT_0_LENGTH;
		}
		if ((1 << (i-2)) & m3_value) {
			dshot_buffer_2_3[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i] =
			DSHOT_1_LENGTH;
		} else {
			dshot_buffer_2_3[DSHOT_BUFFER_LENGTH * 2 - 1 - 2 * i] =
			DSHOT_0_LENGTH;
		}

	}
	dshot_buffer_4_1[2*DSHOT_BUFFER_LENGTH - 1]=0;
	dshot_buffer_4_1[2*DSHOT_BUFFER_LENGTH - 2]=0;
	dshot_buffer_2_3[2*DSHOT_BUFFER_LENGTH - 1]=0;
	dshot_buffer_2_3[2*DSHOT_BUFFER_LENGTH - 2]=0;
}

void prepare_OneShot_PWM() {

	TIM2->CCR4 = 3500-*motor_1_value_pointer * 0.875f+1; 			//value motor 1
	TIM3->CCR3 = 3500-*motor_2_value_pointer * 0.875f+1; 			//value motor 2
	TIM3->CCR4 = 3500-*motor_3_value_pointer * 0.875f+1; 			//value motor 3
	TIM2->CCR3 = 3500-*motor_4_value_pointer * 0.875f+1; 			//value motor 4

}









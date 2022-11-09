/*
 * filters.h
 *
 *  Created on: 24.09.2021
 *      Author: symon
 */

#ifndef FILTERS_H_
#define FILTERS_H_

#include <stdint.h>
#include "global_constants.h"

typedef struct
{
	uint8_t length;			 //	how many previous samples will be taken into calculation
	float *buffer;			 //	buffer for input measurements
	uint8_t buffer_index;	 //	variable to control position of the latest measurement
	float *impulse_response; //	coefficients for inputs values
	float output;			 //	final output value

} FIR_Filter;

typedef struct
{
	uint8_t filter_order;		  //	how many previous samples will be taken into calculation
	float *buffer_input;		  //	buffer for input measurements
	float *buffer_output;		  //	buffer for output values from filter
	uint8_t buffer_index;		  //	variable to control position of the latest measurement
	float *forward_coefficients;  //	coefficients for inputs values
	float *feedback_coefficients; //	coefficients for previous outputs values
	float output;				  //	final output value

} IIR_Filter;

// "Notch filter is just a combination of low and high pass filter" - not really! This description is good for band-stop filter
// Notch filter is much more precise and narrower. It is a combination of:
// - 2nd order system (inversion of it) with damping = 0, which define center frequency,
// - 2 poles away from the center frequency which define width of a filter.
// But still notch filter, as well as a band-stop, is just an IIR filter with suitable coefficients.

// in general notch filter can be higher order but it can be also done by serial connecting a few notch filters 2nd order.

// in case of 2nd order filters circle buffer has no sense so lets create special struct for 2nd order filters:

typedef struct
{
	float frequency; // 	cut-off/center frequency
	float Q_factor;	 //	quality factor
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
	float x1; //	last input
	float x2; //	2nd last input
	float y1; //	last output
	float y2; //	2nd last output
} biquad_Filter_t;

typedef enum
{
	BIQUAD_LPF,
	BIQUAD_NOTCH,
	BIQUAD_BPF,
} biquad_Filter_type;

void FIR_filter_init(FIR_Filter *fir);
float FIR_filter_apply(FIR_Filter *fir, float input);

void IIR_filter_init(IIR_Filter *fir);
float IIR_filter_apply(IIR_Filter *fir, float input);

void biquad_filter_init(biquad_Filter_t *filter, biquad_Filter_type filter_type, float center_frequency_Hz, float quality_factor, uint16_t sampling_frequency_Hz);
float biquad_filter_apply_DF1(biquad_Filter_t *filter, float input);
float biquad_filter_apply_DF2(biquad_Filter_t *filter, float input);

void Gyro_Acc_filters_setup();
void Gyro_Acc_filtering(float *temporary);

void D_term_filters_setup();
void D_term_filtering(ThreeF *input);

#endif /* FILTERS_H_ */

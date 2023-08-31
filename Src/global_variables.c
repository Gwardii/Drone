/*
 * global_variables.c
 *
 *  Created on: 05.05.2021
 *      Author: symon
 */

#include "stm32f4xx.h"
#include "global_constants.h"
#include "global_variables.h"

 // FOR DEBUGING ONLY

double debug_variable_1;

volatile flight_mode_e flight_mode;

volatile arming_e Arming_status;

gyro_t gyro_1 = { .name = "MPU6000", .calibrated = false, .new_raw_data_flag = false,.address = 0, .rev_id = 0, .offset.roll = 0, .offset.pitch = 0,.offset.yaw = 0 };
acc_t acc_1 = { .name = "MPU6000", .calibrated = true, .new_raw_data_flag = false, .id = 0 , .offset.roll = 0, .offset.pitch = 0,.offset.yaw = 0 };


//----------TIME VARIABLES--------
volatile timeUs_t Global_Time = 0;

timeUs_t dt_global = 0;

// main:
timeUs_t time_flag0_1 = 0;
// stabilize:
timeUs_t time_flag1_1 = 0;
// acro:
timeUs_t time_flag2_2 = 0;
// ibus:
timeUs_t time_flag3_1 = 0;
// MPU:
timeUs_t time_flag4_1 = 0;
// flash:
timeUs_t time_flag5_1 = 0;
//	OSD:
timeUs_t time_flag6_1 = 0;

//---------VARIABLES-----------

//------------PIDF-------------

PIDF_t R_PIDF = { 500, 0, 25, 0 }; // 590 30 55 0
PIDF_t P_PIDF = { 700, 0, 40, 0 }; // 750 30 65 0
PIDF_t Y_PIDF = { 700, 0, 0, 0 };  // 1100 30 30 0

PIDF_t corr_PIDF[3];
threef_t corr_sum;

float MCU_temperature = 0;

bool buzzer_active = false;

rx_t receiver = { .number_of_channels = 14, .type = RX_IBUS };

threef_t global_euler_angles = { 0, 0, 0 };

threef_t global_angles = { 0, 0, 0 };

quaternion_t q_global_position = { 1, 0, 0, 0 };

threef_t desired_rotation_speed = { 0, 0, 0 };
threef_t desired_angles = { 0, 0, 0 };

// motor's values set by PID's:
uint16_t motor_value[MOTORS_COUNT];

//	motor's RPM values (from BDshot)
uint32_t motors_rpm[MOTORS_COUNT];

// used in BDshot:
float motors_error[MOTORS_COUNT];
float BDshot_invalid_response[MOTORS_COUNT];
float BDshot_no_response[MOTORS_COUNT];

// pointers for motor's values:
uint16_t* motor_1_value_pointer = &MOTOR_OFF;
uint16_t* motor_2_value_pointer = &MOTOR_OFF;
uint16_t* motor_3_value_pointer = &MOTOR_OFF;
uint16_t* motor_4_value_pointer = &MOTOR_OFF;

int16_t Gyro_Acc[GYRO_ACC_SIZE]; // table for measurements 3 gyro, 3 accelerometer, 1 temperature
int16_t Gyro_Acc_raw[GYRO_ACC_SIZE - 1]; // table for measurements 3 gyro, 3 accelerometer

uint16_t table_to_send[ALL_ELEMENTS_TO_SEND];

//----------------FAILSAFE-------------------

uint16_t failsafe_counter[FAILSAFE_COUNTER];

//----------------FILTERS--------------------
#if defined(USE_FIR_FILTERS)
const float GYRO_FILTER_X_COEF[GYRO_FILTERS_ORDER] = { 0.135250f, 0.216229f, 0.234301f, 0.234301f, 0.216229f, 0.135250f };
const float GYRO_FILTER_Y_COEF[GYRO_FILTERS_ORDER] = { 0.135250f, 0.216229f, 0.234301f, 0.234301f, 0.216229f, 0.135250f };
const float GYRO_FILTER_Z_COEF[GYRO_FILTERS_ORDER] = { 0.135250f, 0.216229f, 0.234301f, 0.234301f, 0.216229f, 0.135250f };

const float ACC_FILTER_X_COEF[ACC_FILTERS_ORDER] = { 0.135250f, 0.216229f, 0.234301f, 0.234301f, 0.216229f, 0.135250f };
const float ACC_FILTER_Y_COEF[ACC_FILTERS_ORDER] = { 0.135250f, 0.216229f, 0.234301f, 0.234301f, 0.216229f, 0.135250f };
const float ACC_FILTER_Z_COEF[ACC_FILTERS_ORDER] = { 0.135250f, 0.216229f, 0.234301f, 0.234301f, 0.216229f, 0.135250f };

const float D_TERM_FILTER_COEF[D_TERM_FILTERS_ORDER] = { 0.135250f, 0.216229f, 0.234301f, 0.234301f, 0.216229f, 0.135250f };

#elif defined(USE_IIR_FILTERS)

const float GYRO_FILTER_X_FORW_COEF[GYRO_FILTERS_ORDER + 1] = { 0.02008336f, 0.04016673f, 0.02008336f };
const float GYRO_FILTER_Y_FORW_COEF[GYRO_FILTERS_ORDER + 1] = { 0.02008336f, 0.04016673f, 0.02008336f };
const float GYRO_FILTER_Z_FORW_COEF[GYRO_FILTERS_ORDER + 1] = { 0.02008336f, 0.04016673f, 0.02008336f };

const float GYRO_FILTER_X_BACK_COEF[GYRO_FILTERS_ORDER] = { -1.56101807f, 0.6413515f };
const float GYRO_FILTER_Y_BACK_COEF[GYRO_FILTERS_ORDER] = { -1.56101807f, 0.6413515f };
const float GYRO_FILTER_Z_BACK_COEF[GYRO_FILTERS_ORDER] = { -1.56101807f, 0.6413515f };

const float ACC_FILTER_X_FORW_COEF[ACC_FILTERS_ORDER + 1] = { 0.02008336f, 0.04016673f, 0.02008336f };
const float ACC_FILTER_Y_FORW_COEF[ACC_FILTERS_ORDER + 1] = { 0.02008336f, 0.04016673f, 0.02008336f };
const float ACC_FILTER_Z_FORW_COEF[ACC_FILTERS_ORDER + 1] = { 0.02008336f, 0.04016673f, 0.02008336f };

const float ACC_FILTER_X_BACK_COEF[ACC_FILTERS_ORDER] = { -1.56101807f, 0.6413515f };
const float ACC_FILTER_Y_BACK_COEF[ACC_FILTERS_ORDER] = { -1.56101807f, 0.6413515f };
const float ACC_FILTER_Z_BACK_COEF[ACC_FILTERS_ORDER] = { -1.56101807f, 0.6413515f };

const float D_TERM_FILTER_FORW_COEF[D_TERM_FILTER_ORDER + 1] = { 0.02008336f, 0.04016673f, 0.02008336f };
const float D_TERM_FILTER_BACK_COEF[D_TERM_FILTER_ORDER] = { -1.56101807f, 0.6413515f };

#endif

//---------------I2C1---------------
uint8_t I2C1_read_write_flag = 1;
uint8_t I2C1_read_buffer[I2C1_BUFFER_SIZE];

bool transmitting_is_Done = true;
#if defined(ESC_PROTOCOL_DSHOT)
uint32_t dshot_buffer_1[DSHOT_BUFFER_LENGTH];
uint16_t dshot_buffer_2[DSHOT_BUFFER_LENGTH];
uint16_t dshot_buffer_3[DSHOT_BUFFER_LENGTH];
uint32_t dshot_buffer_4[DSHOT_BUFFER_LENGTH];
#elif defined(ESC_PROTOCOL_DSHOT_BURST)
uint16_t dshot_burst_buffer_4_1[DSHOT_PWM_FRAME_LENGTH * 2];
uint16_t dshot_burst_buffer_2_3[DSHOT_PWM_FRAME_LENGTH * 2];
#elif defined(ESC_PROTOCOL_BDSHOT)
uint32_t dshot_bb_buffer_1_4[DSHOT_BB_BUFFER_LENGTH * DSHOT_BB_FRAME_SECTIONS];
uint32_t dshot_bb_buffer_2_3[DSHOT_BB_BUFFER_LENGTH * DSHOT_BB_FRAME_SECTIONS];
// BDSHOT response is being sampled just after transmission. There is ~33 [us] break before response (additional sampling) and bitrate is increased by 5/4:
uint32_t dshot_bb_buffer_1_4_r[(int)(33 * BDSHOT_RESPONSE_BITRATE / 1000 + BDSHOT_RESPONSE_LENGTH + 1) * BDSHOT_RESPONSE_OVERSAMPLING];
uint32_t dshot_bb_buffer_2_3_r[(int)(33 * BDSHOT_RESPONSE_BITRATE / 1000 + BDSHOT_RESPONSE_LENGTH + 1) * BDSHOT_RESPONSE_OVERSAMPLING];
#endif
failsafe_e FailSafe_status;

// value of PWM to power off motors (range is 2000-4000 which is in standard PWM 1000-2000):
#if defined(ESC_PROTOCOL_DSHOT) || defined(ESC_PROTOCOL_BDSHOT)

uint16_t MOTOR_OFF = 1953;

#elif defined(ESC_PROTOCOL_PWM) || defined(ESC_PROTOCOL_ONESHOT125)

uint16_t MOTOR_OFF = 2000;

#endif

//---------FLASH-----------

uint8_t flash_write_buffer[512];
uint16_t flash_write_counter = 0;

uint8_t flash_read_buffer[512];
uint16_t flash_read_counter = 0;

volatile blackbox_e Blackbox_status = BLACKBOX_IDLE;

uint32_t flash_global_write_address = 0x0;

float global_variable_monitor[3];

//---------------------------------------------
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ##
// #### HARD.H #################################
//---------------------------------------------

#ifndef _HARD_H_
#define _HARD_H_

#include "stm32f0xx.h"


#define Update_PWM1(X)    Update_TIM1_CH1(X)
#define Update_PWM2(X)    Update_TIM1_CH2(X)
#define Update_PWM3(X)    Update_TIM3_CH1(X)
#define Update_PWM4(X)    Update_TIM3_CH2(X)
#define Update_PWM5(X)    Update_TIM3_CH3(X)
#define Update_PWM6(X)    Update_TIM3_CH4(X)

//-- Defines For Configuration -------------------
//---- Configuration for Hardware Versions -------
#define HARDWARE_VERSION_1_0


#define SOFTWARE_VERSION_1_0
// #define SOFTWARE_VERSION_1_1

//---- Features Configuration ----------------
// #define WITH_GRANDMASTER
#define WITH_BIDIRECTIONAL

//------ Configuration for Firmware-Channels -----
#define WITH_ADJUST_B

//---- End of Features Configuration ----------



//--- Hardware Welcome Code ------------------//
#ifdef HARDWARE_VERSION_1_0
#define HARD "Hardware V: 1.0"
#endif
#ifdef HARDWARE_VERSION_2_0
#define HARD "Hardware V: 2.0"
#endif

//--- Software Welcome Code ------------------//
#ifdef SOFTWARE_VERSION_1_2
#define SOFT "Software V: 1.2"
#endif
#ifdef SOFTWARE_VERSION_1_1
#define SOFT "Software V: 1.1"
#endif
#ifdef SOFTWARE_VERSION_1_0
#define SOFT "Software V: 1.0"
#endif

//-------- Configuration for Outputs-Channels -----



//---- Configuration for Firmware-Programs --------


//-------- Configuration for Outputs-Firmware ------


//-- End Of Defines For Configuration ---------------

//GPIOA pin0
//GPIOA pin1    
//GPIOA pin2
//GPIOA pin3    LCD D4-D7

//GPIOA pin4
#define LCD_E ((GPIOA->ODR & 0x0010) != 0)
#define LCD_E_ON GPIOA->BSRR = 0x00000010
#define LCD_E_OFF GPIOA->BSRR = 0x00100000

//GPIOA pin5
#define LCD_RS ((GPIOA->ODR & 0x0020) != 0)
#define LCD_RS_ON GPIOA->BSRR = 0x00000020
#define LCD_RS_OFF GPIOA->BSRR = 0x00200000

//GPIOA pin6    NC

//GPIOA pin7    
//GPIOB pin0
//GPIOB pin1    3 ADC Channels

//GPIOA pin8    NC
//GPIOA pin9    TIM1_CH2

//GPIOA pin10
#define S2_PIN ((GPIOA->IDR & 0x0400) == 0)

//GPIOA pin11    NC

//GPIOA pin12
#define S1_PIN ((GPIOA->IDR & 0x1000) == 0)

//GPIOA pin13
//GPIOA pin14    
//GPIOA pin15    NC

//GPIOB pin3
//GPIOB pin4
//GPIOB pin5
//GPIOB pin6
//GPIOB pin7    NC




#define SWITCHES_TIMER_RELOAD	10

#define SWITCHES_THRESHOLD_FULL	300		//3 segundos
#define SWITCHES_THRESHOLD_HALF	100		//1 segundo
#define SWITCHES_THRESHOLD_MIN	5		//50 ms

#define TTIMER_FOR_CAT_DISPLAY			2000	//tiempo entre que dice canal y el numero
#define TIMER_STANDBY_TIMEOUT_REDUCED	2000	//reduced 2 segs
#define TIMER_STANDBY_TIMEOUT			6000	//6 segundos
#define DMX_DISPLAY_SHOW_TIMEOUT		30000	//30 segundos

//ESTADOS DE LOS SWITCHES
typedef enum
{    
    S_NO = 0,
    S_MIN,
    S_HALF,
    S_FULL
} sw_state_t;

//ESTADOS DEL MAIN
typedef enum
{    
    MAIN_SET_ZERO_0 = 0,
    MAIN_SET_ZERO_1,
    MAIN_SET_ZERO_2,
    MAIN_GOTO_TAKE_SAMPLES,
    MAIN_TAKE_SAMPLES,
    MAIN_SHOW_SCREENS,
    MAIN_SHOW_MODULE_B,
    MAIN_SHOW_COMPONENTS_XYZ,
    MAIN_SHOW_FREQUENCY,
    MAIN_SHOW_LINES,
    MAIN_SHOW_LINES_1

    
} main_state_t;

typedef enum {
    resp_ok = 0,
    resp_continue,
    resp_selected,
    resp_change,
    resp_change_all_up,
    resp_working,
    resp_error,
    resp_need_to_save,
    resp_finish

} resp_t;

typedef enum {
    FREQ_LOOK_FOR_FIRST_LOW = 0,
    FREQ_LOOK_FOR_RISING,    
    FREQ_WAIT_LOW,
    FREQ_END_RISING,
    FREQ_ENDED

} freq_t;


/* Module Functions ------------------------------------------------------------*/
sw_state_t CheckS1 (void);
sw_state_t CheckS2 (void);
void UpdateSwitches (void);
void UpdateSamplesAndPID (void);
void PIDforProgramsCHX (unsigned char, unsigned char);
unsigned short DMXtoCurrent (unsigned char);

#endif /* HARD_H_ */

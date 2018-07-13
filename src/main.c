//---------------------------------------------
// #### PROYECTO DEXEL 6CH BIDIRECCIONAL - Custom Board ####
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ##
// #### MAIN.C ################################
//---------------------------------------------

/* Includes ------------------------------------------------------------------*/
#include "hard.h"
#include "stm32f0xx.h"
#include "gpio.h"
// #include "uart.h"

#include "core_cm0.h"
#include "adc.h"
#include "tim.h"

#include "dsp.h"
#include "lcd.h"
#include "lcd_utils.h"

#include <stdio.h>
#include <string.h>
#include <math.h>



//--- VARIABLES EXTERNAS ---//


// ------- Externals del ADC -------
volatile unsigned short adc_ch [ADC_CHANNEL_QUANTITY];
volatile unsigned char seq_ready;

// ------- Externals de los timers -------
volatile unsigned char timer_1seg = 0;
volatile unsigned char switches_timer = 0;

// // ------- Externals del USART -------
// volatile unsigned char usart1_have_data;
// volatile unsigned char usart2_have_data;



//--- VARIABLES GLOBALES ---//
// ------- de los timers -------
volatile unsigned short timer_standby;
volatile unsigned short wait_ms_var = 0;

short max_x = 0;
short min_x = 0;
short max_y = 0;
short min_y = 0;
short max_z = 0;
short min_z = 0;

volatile short x = 0;
volatile short y = 0;
volatile short z = 0;

volatile unsigned short zero_for_x = 0;
volatile unsigned short zero_for_y = 0;
volatile unsigned short zero_for_z = 0;

volatile unsigned int B_module = 0;
volatile unsigned char B_is_updated = 0;

unsigned short x_zero [32];
unsigned short y_zero [32];
unsigned short z_zero [32];

unsigned short last_max_b = 0;
#define VECTOR_LENGTH 8
unsigned short v_B [VECTOR_LENGTH];

#define SAMPLES_INDEX_LENGTH    2000
unsigned short samples_index = 0;

#define B_THRESHOLD_FOR_FREQ_UP    180    //equivale a 30 Gauss
#define B_THRESHOLD_FOR_FREQ_DWN   150    //equivale a 20 Gauss

//--- FUNCIONES DEL MODULO ---//
extern void EXTI4_15_IRQHandler(void);
void TimingDelay_Decrement(void);
void DMAConfig(void);
void DMAEnableInterrupt (void);
void DMADisableInterrupt (void);
extern void DMA1_Channel1_IRQHandler (void);
short moduleShort (short);
unsigned short moduleAdjustB (short);


// ------- para el DMA -------
#define RCC_DMA_CLK (RCC->AHBENR & RCC_AHBENR_DMAEN)
#define RCC_DMA_CLK_ON 		RCC->AHBENR |= RCC_AHBENR_DMAEN
#define RCC_DMA_CLK_OFF 	RCC->AHBENR &= ~RCC_AHBENR_DMAEN

#define sequence_ready         (DMA1->ISR & DMA_ISR_TCIF1)
#define sequence_ready_reset   (DMA1->IFCR = DMA_ISR_TCIF1)



const char s_blank_line [] = {"                "};
//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
    unsigned char i = 0;
    char s_lcd1 [20];
    char s_lcd2 [20];
    char s_lcd3 [6];    
    main_state_t main_state = MAIN_SET_ZERO_0;
    resp_t resp = resp_continue;
    freq_t freq_state = FREQ_LOOK_FOR_FIRST_LOW;

    unsigned char zero_index = 0;
    unsigned short max_b = 0;

    unsigned char screen = MAIN_SHOW_MODULE_B;
    unsigned char screen_changed = 0;
    unsigned char setting_zero = 0;

    unsigned short start_freq_sample = 0;
    unsigned short end_freq_sample = 0;

    
    //GPIO Configuration.
    GPIO_Config();

    //ACTIVAR SYSTICK TIMER
    if (SysTick_Config(48000))
    {
        while (1)	/* Capture error */
        {
            for (i = 0; i < 255; i++)
            {
                asm (	"nop \n\t"
                        "nop \n\t"
                        "nop \n\t" );
            }
        }
    }

    //-- Prueba salida PWM ----------
    TIM_3_Init();
    TIM_1_Init();

    // while (1)
    // {
    //     for (ii = 0; ii < 1000; ii++)
    //     {
    //         Update_TIM1_CH2(ii);
    //         Wait_ms(5);
    //     }
    //     for (ii = 1000; ii > 0; ii--)
    //     {
    //         Update_TIM1_CH2(ii);
    //         Wait_ms(5);
    //     }
    // }
    //-- Fin Prueba salida PWM ----------    


    //-- Prueba con LCD ----------
    LCDInit();


    //--- Welcome code ---//
    Lcd_Command(CLEAR);
    Wait_ms(100);
    Lcd_Command(CURSOR_OFF);
    Wait_ms(100);
    Lcd_Command(BLINK_OFF);
    Wait_ms(100);

    while (FuncShowBlink ((const char *) " Magnetic Flux  ",
                          (const char *) " Meter          ", 1, BLINK_NO) == resp_continue);
    //-- Fin Prueba con LCD ----------

    //--- Mensaje Bienvenida ---//
    //---- Defines from hard.h -----//
#ifdef HARD
    // while (FuncShowBlink (HARD, SOFT, 1, BLINK_CROSS) == resp_continue);
#else
#error	"No Hardware defined in hard.h file"
#endif

#ifdef SOFT
    // while (FuncShowBlink ((const char *) "Dexel   ", (const char *) "Lighting", 1, BLINK_NO) == resp_continue);
#else
#error	"No Soft Version defined in hard.h file"
#endif
    while (FuncShowBlink (HARD, SOFT, 1, BLINK_CROSS) == resp_continue);

    // while (1);
    //---- End of Defines from hard.h -----//

    //-- Prueba de Switches S1 y S2 ----------
    // unsigned char check_s1 = 0, check_s2 = 0;
    // while (1)
    // {
    //     if ((CheckS1()) && (check_s1 == 0))
    //     {
    //         check_s1 = 1;
    //         while (FuncShowBlink ("S1               ",
    //                               s_blank_line, 0, BLINK_NO) == resp_continue);
    //     }
    //     else if ((!CheckS1()) && (check_s1))
    //     {
    //         check_s1 = 0;
    //         while (FuncShowBlink ("not S1           ",
    //                               s_blank_line, 0, BLINK_NO) == resp_continue);            
    //     }
                            
    //     if ((CheckS2()) && (check_s2 == 0))
    //     {
    //         check_s2 = 1;
    //         while (FuncShowBlink ("S2               ",
    //                               s_blank_line, 0, BLINK_NO) == resp_continue);            
    //     }
    //     else if ((!CheckS2()) && (check_s2))
    //     {
    //         check_s2 = 0;
    //         while (FuncShowBlink ("not S2           ",
    //                               s_blank_line, 0, BLINK_NO) == resp_continue);            
    //     }

    //     UpdateSwitches();
    // }
    //-- Fin Prueba de Switches S1 y S2 ----------

    
    //-- Prueba con ADC & DMA ----------
    //-- ADC configuration.
#ifdef ADC_WITH_DMA
    AdcConfig();

    //-- DMA configuration.
    DMAConfig();
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART;
    
    // Prueba ADC & DMA
    while(1)
    {
        switch (main_state)
        {
        case MAIN_SET_ZERO_0:
            zero_index = 0;
            main_state = MAIN_SET_ZERO_1;
            break;
            
        case MAIN_SET_ZERO_1:
            if (sequence_ready)
            {
                // Clear DMA TC flag
                sequence_ready_reset;

                if (zero_index < 32)
                {
                    x_zero[zero_index] = X_Channel;
                    y_zero[zero_index] = Y_Channel;
                    z_zero[zero_index] = Z_Channel;
                    zero_index++;
                }
            }

            resp = FuncShowBlink ((const char *) " Setting Zero   ",
                                  (const char *) " Flux           ", 2, BLINK_DIRECT);

            if ((resp == resp_finish) && (zero_index == 32))
            {
                main_state = MAIN_SET_ZERO_2;
                zero_for_x = MAFilter32Fast(x_zero);
                zero_for_y = MAFilter32Fast(y_zero);
                zero_for_z = MAFilter32Fast(z_zero);
                sprintf(s_lcd1, "x0: %d y0: %d ", zero_for_x, zero_for_y);
                sprintf(s_lcd2, "z0: %d        ", zero_for_z);
            }
            break;

        case MAIN_SET_ZERO_2:
            resp = FuncShowBlink (s_lcd1, s_lcd2, 1, BLINK_NO);

            if (resp == resp_finish)
            {
                DMAEnableInterrupt();
                main_state = MAIN_GOTO_TAKE_SAMPLES;
            }            
            break;

        case MAIN_GOTO_TAKE_SAMPLES:
            max_x = 0;
            min_x = 0;
            max_y = 0;
            min_y = 0;
            max_z = 0;
            min_z = 0;

            max_b = 0;
            samples_index = 0;
            freq_state = FREQ_LOOK_FOR_FIRST_LOW;
            start_freq_sample = 0;
            end_freq_sample = 0;
            main_state++;
            break;

        case MAIN_TAKE_SAMPLES:
            if (B_is_updated)
            {
                B_is_updated = 0;
                
                if (samples_index < SAMPLES_INDEX_LENGTH)
                {
                    if (max_x < x)
                        max_x = x;
                    else if (min_x > x)
                        min_x = x;

                    if (max_y < y)
                        max_y = y;
                    else if (min_y > y)
                        min_y = y;

                    if (max_z < z)
                        max_z = z;
                    else if (min_z > z)
                        min_z = z;                                        

                    if (max_b < B_module)
                        max_b = B_module;

                    switch (freq_state)
                    {
                    case FREQ_LOOK_FOR_FIRST_LOW:
                        if (B_module < B_THRESHOLD_FOR_FREQ_DWN)
                            freq_state++;                            
                        
                        break;

                    case FREQ_LOOK_FOR_RISING:
                        if (B_module > B_THRESHOLD_FOR_FREQ_UP)
                        {
                            start_freq_sample = samples_index;
                            freq_state++;                            
                        }
                        break;
                        
                    case FREQ_WAIT_LOW:
                        if (B_module < B_THRESHOLD_FOR_FREQ_DWN)
                            freq_state++;                            

                        break;

                    case FREQ_END_RISING:
                        if (B_module > B_THRESHOLD_FOR_FREQ_UP)
                        {
                            end_freq_sample = samples_index;
                            freq_state++;                            
                        }                        
                        break;

                    case FREQ_ENDED:
                        break;
                    }

                    samples_index++;
                }
                else
                    main_state++;

            }
            break;

        case MAIN_SHOW_SCREENS:
            main_state = screen;            

            break;

        case MAIN_SHOW_MODULE_B:
            //reviso componentes
            if (max_x < moduleShort(min_x))
                max_x = min_x;

            if (max_y < moduleShort(min_y))
                max_y = min_y;
            
            if (max_z < moduleShort(min_z))
                max_z = min_z;

            if (moduleShort(max_x) > moduleShort(max_y))
            {
                if (moduleShort(max_x) > moduleShort(max_z))
                {
                    if (max_x < 0)    //el mayor es x
                        strcpy(s_lcd3, "-x");
                    else
                        strcpy(s_lcd3, "x");
                }
                else
                {                    
                    if (max_z < 0)    //el mayor es z
                        strcpy(s_lcd3, "-z");
                    else
                        strcpy(s_lcd3, "z");
                }
            }
            else
            {
                if (moduleShort(max_y) > moduleShort(max_z))
                {
                    if (max_y < 0)    //el mayor es y
                        strcpy(s_lcd3, "-y");
                    else
                        strcpy(s_lcd3, "y");
                }
                else
                {                    
                    if (max_z < 0)    //el mayor es z
                        strcpy(s_lcd3, "-z");
                    else
                        strcpy(s_lcd3, "z");
                }
            }                
                                    
            // v_B[0] = max_b;
            v_B[0] = moduleAdjustB(max_b);
            sprintf(s_lcd1, "|Bpeak|=%4d  %s ", moduleAdjustB(max_b), s_lcd3);
            sprintf(s_lcd2, "|Bmean|=%4d    ", MAFilter8(v_B));            
            main_state = MAIN_SHOW_LINES;
            break;

        case MAIN_SHOW_COMPONENTS_XYZ:
            //reviso componentes
            if (max_x < moduleShort(min_x))
                max_x = min_x;

            if (max_y < moduleShort(min_y))
                max_y = min_y;
            
            if (max_z < moduleShort(min_z))
                max_z = min_z;
            
            sprintf(s_lcd1, "x: %d y: %d   ", moduleAdjustB(max_x), moduleAdjustB(max_y));
            sprintf(s_lcd2, "z: %d         ", moduleAdjustB(max_z));
            main_state = MAIN_SHOW_LINES;
            break;

        case MAIN_SHOW_FREQUENCY:
            if (end_freq_sample > start_freq_sample)
            {
                unsigned short f = end_freq_sample - start_freq_sample;
                f = 2000 / f;
                sprintf(s_lcd1, "freq: %d      ", f);
                strcpy(s_lcd2, s_blank_line);
            }
            else
            {
                strcpy(s_lcd1, "No frequency    ");
                strcpy(s_lcd2, "or low flux     ");
            }
            
            main_state = MAIN_SHOW_LINES;
            break;
            
        case MAIN_SHOW_LINES:
            resp = FuncShowBlink (s_blank_line, s_blank_line, 0, BLINK_NO);

            if (resp == resp_finish)
                main_state = MAIN_SHOW_LINES_1;
            
            break;

        case MAIN_SHOW_LINES_1:
            resp = FuncShowBlink (s_lcd1, s_lcd2, 0, BLINK_NO);

            if (resp == resp_finish)
                main_state = MAIN_GOTO_TAKE_SAMPLES;
            
            break;
            
        default:
            main_state = MAIN_SET_ZERO_0;
            break;
        }

        //cosas que no tienen que ver cn las muestras
        if (CheckS2() > S_NO)
        {
            if (!screen_changed)
            {
                screen_changed = 1;
                if (screen < MAIN_SHOW_FREQUENCY)
                    screen++;
                else
                    screen = MAIN_SHOW_MODULE_B;
            }
        }
        else
            screen_changed = 0;

        if (CheckS1() > S_NO)
        {
            if (!setting_zero)
            {
                setting_zero = 1;
                DMADisableInterrupt();
                sequence_ready_reset;
                main_state = MAIN_SET_ZERO_0;
            }
        }
        else
            setting_zero = 0;
        
        UpdateSwitches();
        
    }    //fin while 1
#endif
    return 0;
}
//--- End of Main ---//
short moduleShort (short a)
{
    if (a < 0)
        return -a;
    else
        return a;
}

unsigned short moduleAdjustB (short b)
{
    b -= 56;
    if (b < 0)
        return 0;

    b = b * 0.33;
    return b;    
}

void DMA1_Channel1_IRQHandler (void)
{
    if (sequence_ready)
    {
        // Clear DMA TC flag
        sequence_ready_reset;
        
        x = X_Channel - zero_for_x;
        y = Y_Channel - zero_for_y;
        z = Z_Channel - zero_for_z;

        B_module = x * x;
        B_module += y * y;
        B_module += z * z;

        B_module = sqrt(B_module);

        Update_TIM1_CH2 ((unsigned short) (B_module >> 2));
        B_is_updated = 1;
    }
}

void DMAConfig(void)
{
    /* DMA1 clock enable */
    if (!RCC_DMA_CLK)
        RCC_DMA_CLK_ON;

    //Configuro el control del DMA CH1
    DMA1_Channel1->CCR = 0;
    //priority very high
    //memory halfword
    //peripheral halfword
    //increment memory
    DMA1_Channel1->CCR |= DMA_CCR_PL | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC;
    //DMA1_Channel1->CCR |= DMA_Mode_Circular | DMA_CCR_TCIE;
    //cicular mode
    DMA1_Channel1->CCR |= DMA_CCR_CIRC;

    //Tamaño del buffer a transmitir
    DMA1_Channel1->CNDTR = ADC_CHANNEL_QUANTITY;

    //Address del periferico
    DMA1_Channel1->CPAR = (uint32_t) &ADC1->DR;

    //Address en memoria
    DMA1_Channel1->CMAR = (uint32_t) &adc_ch[0];

    //Enable
    //DMA1_Channel1->CCR |= DMA_CCR_EN;
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    NVIC_SetPriority(DMA1_Channel1_IRQn, 3);
}

void DMAEnableInterrupt (void)
{
    DMA1_Channel1->CCR |= DMA_CCR_TCIE;
}

void DMADisableInterrupt (void)
{
    DMA1_Channel1->CCR &= ~DMA_CCR_TCIE;
}

void TimingDelay_Decrement(void)
{
    if (wait_ms_var)
        wait_ms_var--;

    if (timer_standby)
        timer_standby--;

    if (switches_timer)
        switches_timer--;

    //para lcd_utils
    UpdateTimerLCD ();

    //para main menu
    // UpdateTimerModeMenu ();
}

// void EXTI4_15_IRQHandler (void)    //nueva detecta el primer 0 en usart Consola PHILIPS
// {
//     if(EXTI->PR & 0x0100)	//Line8
//     {
//         DmxInt_Break_Handler();
//         EXTI->PR |= 0x0100;
//     }
// }

//--- end of file ---//


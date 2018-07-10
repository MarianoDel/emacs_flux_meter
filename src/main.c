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
#include "menues.h"
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
volatile unsigned char temp_sample_timer = 0;
volatile unsigned short need_to_save_timer = 0;


//--- FUNCIONES DEL MODULO ---//
extern void EXTI4_15_IRQHandler(void);
void TimingDelay_Decrement(void);
void DMAConfig(void);


// ------- para el DMA -------
#define RCC_DMA_CLK (RCC->AHBENR & RCC_AHBENR_DMAEN)
#define RCC_DMA_CLK_ON 		RCC->AHBENR |= RCC_AHBENR_DMAEN
#define RCC_DMA_CLK_OFF 	RCC->AHBENR &= ~RCC_AHBENR_DMAEN

#define sequence_ready         (DMA1->ISR & DMA_ISR_TCIF1)
#define sequence_ready_reset   (DMA1->IFCR = DMA_ISR_TCIF1)



#define PID_UNDERSAMPLING    5    //muestreo a 24KHz / pid_samples


const char s_blank_line [] = {"                "};
//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
    unsigned char i = 0;
    unsigned short ii = 0;    
    char s_to_send [100];
    main_state_t main_state = MAIN_INIT;
    resp_t resp = resp_continue;
    unsigned char undersampling = 0;
    
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
        if (sequence_ready)
        {
            // Clear DMA TC flag
            sequence_ready_reset;

            if (undersampling < (PID_UNDERSAMPLING - 1))
            {
                undersampling++;
            }
            else
            {
                undersampling = 0;

                //cargo vectores
                if (index_vector < (VECTOR_LENGTH - 1))
                {
                    x = X_Channel - zero_for_x;
                    y = Y_Channel - zero_for_y;
                    z = Z_Channel - zero_for_z;

                    B_module = x * x;
                    B_module += y * y;
                    B_module += z * z;                    

                    B_module = sqrt(B_module);

                    v_B [index_vector] = B_module;
                    Update_TIM1_CH2 (B_module);

                    if (x < 0)
                        x = -x;

                    if (max_x < x)
                        max_x = x;

                    if (y < 0)
                        y = -y;

                    if (may_y < y)
                        may_y = y;

                    if (z < 0)
                        z = -z;

                    if (maz_z < z)
                        maz_z = z;
                                        
                    index_vector++;
                }
                else
                {
                    index_vector = 0;
                    main_state = MAIN_CALCULATE_VECTOR;
                }


                
            }
        }

        //me fijo si hubo overrun
        if (ADC1->ISR & ADC_IT_OVR)
        {
            ADC1->ISR |= ADC_IT_EOC | ADC_IT_EOSEQ | ADC_IT_OVR;
        }

        if (!timer_standby)
        {
            timer_standby = 1000;
            
        }        
    }
#endif
    //-- Prueba con ADC & DMA ----------

    
    // //inicializo el hard que falta
    // AdcConfig();

    // //-- DMA configuration.
    // DMAConfig();
    // DMA1_Channel1->CCR |= DMA_CCR_EN;

    // ADC1->CR |= ADC_CR_ADSTART;


    // while (1)
    // {
    //     switch (main_state)
    //     {
    //     case MAIN_INIT:
    //         memcpy(&mem_conf, pmem, sizeof(parameters_typedef));

    //         main_state++;
    //         break;

    //     case MAIN_HARDWARE_INIT:

    //         //reseteo hardware
    //         //DMX en RX
    //         SW_RX_TX_RE_NEG;
    //         DMX_Disa();

    //         //reseteo canales
    //         Update_TIM1_CH1(0);
    //         Update_TIM1_CH2(0);    
    //         Update_TIM3_CH1(0);
    //         Update_TIM3_CH2(0);
    //         Update_TIM3_CH3(0);
    //         Update_TIM3_CH4(0);
                        
    //         //reseteo menues
    //         MasterModeMenuReset();
    //         FuncSlaveModeReset();

    //         sprintf(s_to_send, "prog type: %d\n", mem_conf.program_type);
    //         Usart2Send(s_to_send);
    //         Wait_ms(100);
    //         sprintf(s_to_send, "Max pwm channels: %d %d %d %d %d %d\n",
    //                 mem_conf.max_pwm_ch1,
    //                 mem_conf.max_pwm_ch2,
    //                 mem_conf.max_pwm_ch3,
    //                 mem_conf.max_pwm_ch4,
    //                 mem_conf.max_pwm_ch5,
    //                 mem_conf.max_pwm_ch6);
            
    //         Usart2Send(s_to_send);
    //         Wait_ms(100);
            
    //         main_state++;            
    //         break;

    //     case MAIN_GET_CONF:
    //         if (mem_conf.program_type == MASTER_MODE)
    //         {
    //             //habilito transmisiones
    //             SW_RX_TX_DE;
    //             DMX_Ena();                    
    //             main_state = MAIN_IN_MASTER_MODE;             
    //         }                

    //         if (mem_conf.program_type == SLAVE_MODE)
    //         {
    //             //variables de recepcion
    //             Packet_Detected_Flag = 0;
    //             DMX_channel_selected = mem_conf.dmx_channel;
    //             DMX_channel_quantity = mem_conf.dmx_channel_quantity;

    //             //habilito recepcion
    //             SW_RX_TX_RE_NEG;
    //             DMX_Ena();    
    //             main_state = MAIN_IN_SLAVE_MODE;
    //         }

    //         if (mem_conf.program_type == PROGRAMS_MODE)
    //         {
    //             //me aseguro no cargar la linea
    //             SW_RX_TX_RE_NEG;
    //             main_state = MAIN_IN_PROGRAMS_MODE;
    //         }

    //         //default state no debiera estar nunca aca!
    //         if (main_state == MAIN_GET_CONF)
    //         {
    //             mem_conf.program_type = SLAVE_MODE;
    //             main_state = MAIN_IN_SLAVE_MODE;
    //         }                
    //         break;

    //     case MAIN_IN_MASTER_MODE:    //por ahora programs mode
    //         Func_PX(mem_conf.last_program_in_flash, mem_conf.last_program_deep_in_flash);
    //         UpdateSamplesAndPID();
    //         if (CheckS2() > S_HALF)
    //             main_state = MAIN_ENTERING_MAIN_MENU;

    //         MasterModeMenu();
    //         if (!timer_standby)
    //         {
    //             timer_standby = 40;
    //             SendDMXPacket (PCKT_INIT);
    //         }
    //         break;
            
    //     case MAIN_IN_SLAVE_MODE:
    //         FuncSlaveMode();
    //         if (!timer_standby)
    //         {
    //             timer_standby = 1000;
    //             //envio corrientes
    //             sprintf(s_to_send, "i1: %d, i2: %d, i3: %d, i4: %d, i5: %d, i6: %d, t: %d\n",
    //                     I_Channel_1,
    //                     I_Channel_2,
    //                     I_Channel_3,
    //                     I_Channel_4,
    //                     I_Channel_5,
    //                     I_Channel_6,
    //                     Temp_Channel);

    //             Usart2Send(s_to_send);

    //             //envio canales dmx
    //             sprintf(s_to_send, "c0: %d, c1: %d, c2: %d, c3: %d, c4: %d, c5: %d, c6: %d\n",
    //                     data7[0],
    //                     data7[1],
    //                     data7[2],
    //                     data7[3],
    //                     data7[4],
    //                     data7[5],
    //                     data7[6]);
                
    //             Usart2Send(s_to_send);            
    //         }

    //         if (CheckS2() > S_HALF)
    //             main_state = MAIN_ENTERING_MAIN_MENU;

    //         break;

    //     case MAIN_IN_PROGRAMS_MODE:
    //         Func_PX(mem_conf.last_program_in_flash, mem_conf.last_program_deep_in_flash);
    //         UpdateSamplesAndPID();

    //         if (CheckS2() > S_HALF)
    //             main_state = MAIN_ENTERING_MAIN_MENU;

    //         ProgramsModeMenu();
            
    //         break;

    //     case MAIN_IN_OVERTEMP:
    //         CTRL_FAN_ON;
    //         Update_PWM1(0);
    //         Update_PWM2(0);
    //         Update_PWM3(0);
    //         Update_PWM4(0);
    //         Update_PWM5(0);
    //         Update_PWM6(0);

    //         LCD_1ER_RENGLON;
    //         LCDTransmitStr("OVERTEMP");
    //         LCD_2DO_RENGLON;
    //         LCDTransmitStr(s_blank_line);

    //         sprintf(s_to_send, "overtemp: %d\n", Temp_Channel);
    //         Usart2Send(s_to_send);

    //         main_state = MAIN_IN_OVERTEMP_B;
    //         break;

    //     case MAIN_IN_OVERTEMP_B:
    //         if (Temp_Channel < TEMP_IN_50)
    //         {
    //             //reconecto
    //             main_state = MAIN_HARDWARE_INIT;
    //         }
            
    //         break;
            
    //     case MAIN_ENTERING_MAIN_MENU:
    //         //deshabilitar salidas hardware
    //         SW_RX_TX_RE_NEG;
    //         DMX_Disa();

    //         //reseteo canales
    //         Update_TIM1_CH1(0);
    //         Update_TIM1_CH2(0);    
    //         Update_TIM3_CH1(0);
    //         Update_TIM3_CH2(0);
    //         Update_TIM3_CH3(0);
    //         Update_TIM3_CH4(0);

    //         MainMenuReset();
    //         main_state++;

    //         break;

    //     case MAIN_IN_MAIN_MENU:
    //         resp = MainMenu();

    //         if (resp == resp_need_to_save)
    //         {
    //             need_to_save = 1;
    //             need_to_save_timer = 10000;
    //             main_state = MAIN_HARDWARE_INIT;
    //         }
            
    //         if (resp == resp_finish)
    //             main_state = MAIN_HARDWARE_INIT;

    //         break;
            
    //     default:
    //         main_state = MAIN_INIT;
    //         break;
    //     }

    //     //cuestiones generales
        
    //     UpdateSwitches();
        
    // }    //end of while 1
    
    return 0;
}
//--- End of Main ---//

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


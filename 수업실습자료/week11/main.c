#include "stm32f10x.h"
#include "core_cm3.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"

#include "lcd.h"
#include "touch.h"

#define PWM_MIN 500
#define PWM_MAX 2500


int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,RED,GRAY};
uint16_t counter = 0;
uint16_t ledStatus = 1;
uint16_t PWM = 500;

TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
TIM_OCInitTypeDef TIM_OCInitStructure;

void RCC_Configure(void);
void GPIO_Configure(void);
void NVIC_Configure(void);
void TIM2_Configure(void);
void TIM3_Configure(void);
void LED1_ON(void);
void LED1_OFF(void);
void LED2_ON(void);
void LED2_OFF(void);
void motor_right(void);
void motor_left(void);


void RCC_Configure() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE); // LED 1, LED 2 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // SubMotor
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); 
}

void GPIO_Configure() {
    GPIO_InitTypeDef GPIO_InitStructure;
    // LED 1
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    // LED 2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // SubMotor
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// Per 1s
void TIM2_Configure() {
    uint16_t prescalar = 7200;
    
    // Timer 2 Configuration
    TIM_TimeBaseStructure.TIM_Prescaler = prescalar;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 10000;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;    
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    // initailize timer 2
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM2, ENABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

// for SubMotor (PWM)
void TIM3_Configure() {
    uint16_t prescale = (uint16_t) (SystemCoreClock / 1000000);
    
    // Timer 3 Configuration
    TIM_TimeBaseStructure.TIM_Period = 20000; // 20ms �ֱ�
    TIM_TimeBaseStructure.TIM_Prescaler = prescale;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
    
    
    // PWM Configuration
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = PWM;
    
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

void NVIC_Configure() {
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}



void TIM2_IRQHandler(void) {
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        if (ledStatus) {
            // per 1s
            if (counter % 2 == 0) {
                LED1_ON();
                motor_right();
            } else {
                LED1_OFF();
                motor_right();
            }

            // per 5s
            if (counter == 0) {
                LED2_ON();
            }
            if (counter == 5) {
                LED2_OFF();
            }
            counter++;
            counter = counter % 10;
        } else {
            motor_left();
            LED1_OFF();
            LED2_OFF();
        }
    }

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}


void LED1_OFF(void){
    GPIO_SetBits(GPIOD, GPIO_Pin_2);
}

void LED1_ON(void){
    GPIO_ResetBits(GPIOD, GPIO_Pin_2);
}

void LED2_OFF(void){
    GPIO_SetBits(GPIOD, GPIO_Pin_3);
}

void LED2_ON(void){
    GPIO_ResetBits(GPIOD, GPIO_Pin_3);
}

void motor_right(void){
    PWM = PWM + 100;
    if(PWM > PWM_MAX) PWM = PWM_MIN;
    
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = PWM;

    TIM_OC3Init(TIM3, &TIM_OCInitStructure);
}

void motor_left(void){
    PWM = PWM - 100;
    if(PWM < PWM_MIN) PWM = PWM_MAX;
    
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = PWM;

    TIM_OC3Init(TIM3, &TIM_OCInitStructure);
}


void Print_LED_Status() {
    if (ledStatus) {
        LCD_ShowString(10, 40, "        ", WHITE, WHITE);
        LCD_ShowString(10, 40, "ON", RED, WHITE);
    } else {
        LCD_ShowString(10, 40, "        ", WHITE, WHITE);
        LCD_ShowString(10, 40, "OFF", RED, WHITE);
    }

}


  int main() {
    SystemInit();

    RCC_Configure();
    GPIO_Configure();
    NVIC_Configure();
    
    // Timer Configuration
    TIM2_Configure();
    TIM3_Configure();

    // LCD Configuration
    LCD_Init();

    // Touch Configuration
    Touch_Configuration();
    Touch_Adjust();


    LCD_Clear(WHITE);
    LCD_ShowString(10, 10, "Wed_Team08", BLUE, WHITE);
    LCD_DrawRectangle(70, 70, 130, 130);
    LCD_ShowString(80, 80, "BTN", RED, WHITE);
    Print_LED_Status();

    uint16_t x = 0, y = 0;
    while(1){
        Touch_GetXY(&x, &y, 1);
        Convert_Pos(x, y ,&x, &y);    
    
        if (x >= 70 && x <= 130 && y >= 70 && y <= 130) {
            ledStatus = (ledStatus + 1) % 2;
            Print_LED_Status();
            x = 0;
            y = 0;
        }
    }
    return 0;
}

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f10x.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_exti.h"
#include "lcd.h"
#include "touch.h"
#include "core_cm3.h"


/*
===================================================================================
                        사용자 정의 상수 및 구조체
===================================================================================
*/
#define MAIN_DELAY 100000 // 메인 루프에서 사용할 딜레이
#define PUMP_DELAY 20000 // water 함수 호출 시 물을 줄 시간을 설정
#define USART2_DELAY 20000
#define USART_DELAY 90000 // usart 데이터 송신 시 인가할 딜레이
#define BUFFER_SIZE 200 // 모든 버퍼의 표준 크기 설정 

#define SERVO_LEFT 500 // 서보 모터 왼쪽으로 돌리기
#define SERVO_RIGHT 2000 // 서보 모터 오른쪽으로 돌리기
#define WET_LIMIT 500 // 토양 습도 센서 값이 이 값보다 작으면 충분한 것으로 판단
#define BRIGHT_LIMIT 1250 // 밝기 센서 값이 이 값보다 크면 밝은 것으로 판단

// 명령어 설정
#define SET_MODE_AUTO "mode 1"
#define SET_MODE_MANUAL "mode 0"
#define SET_LED_OFF "led 0"
#define SET_LED_ON "led 1"
#define SET_DOOR_CLOSE "door 0"
#define SET_DOOR_OPEN "door 1"
#define WATERING "water"


// 구조체 정의

// 센서의 상태를 임계값을 기준으로 나누고 진리값을 저장하는 구조체
// isClose : 거리 센서가 가까운지 여부
// isEnough : 수위 센서가 충분한지 여부
// isWet : 토양 습도 센서가 충분한지 여부
typedef struct _SensorState {
    bool isClose; bool isEnough; bool isWet; bool isBright;
} SensorState;

// 장치에 연결된 모듈의 정보나 상태를 저장하는 구조체
// soilMoisture : 토양 습도 센서 값
// light : 조도 센서 값
// mode : 자동 모드인지 수동 모드인지
// led : led가 켜져있는지
// door : 문이 열려있는지
typedef struct _InnerState {
    uint16_t soilMoisture; uint16_t light; bool mode; bool led; bool door;
} DeviceState;


// 버퍼 구조체 - usart, json 데이터를 저장하는 버퍼
typedef struct _Buffer {
    char mem[BUFFER_SIZE]; uint32_t index;
} Buffer;


// USART 방향 설정에 flag 값을 지정하기 위한 열거형
typedef enum _USART_DIR { USART1_DIR = 0, USART2_DIR = 1} USART_DIR;
// pump 상태를 저장하기 위한 열거형
typedef enum _PUMP_STATE { PUMP_OFF = 0, PUMP_ON = 1} PUMP_STATE;
/*
-----------------------------------------------------------------------------------
*/

/*
===================================================================================
                        전역 변수 설정
===================================================================================
*/

// 사용자 정의 전역 변수
SensorState sensorState;    
DeviceState deviceState;
PUMP_STATE pumpState;
Buffer usart1Buffer; // usart1 버퍼
Buffer usart2Buffer; // usart2 버퍼
Buffer jsonBuffer; // json 데이터를 저장할 버퍼

// 시스템에서 사용할 구조체
GPIO_InitTypeDef GPIO_InitStructure;
TIM_OCInitTypeDef TIM_OCInitStructure;
USART_InitTypeDef USART_InitStructure;
ADC_InitTypeDef ADC_InitStructure;
TIM_TimeBaseInitTypeDef TIM_InitStructure;
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
NVIC_InitTypeDef NVIC_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
/*
-----------------------------------------------------------------------------------
*/
/*
===================================================================================
                        함수 선언
===================================================================================
*/

void NVIC_Config();
void usart1_config();
void usart2_config();
void adc_module_config();
void servo_config();
void eps32_config();
void pump_config();
void led_config();
void readBuffer(Buffer* buffer, char data);
void resetIndex(Buffer* buffer);
void excuteCommand(USART_DIR which_usart);
void controlServo(uint16_t pulse);
void controlLED(bool on);
void controlPump(bool on);
void water();
void myDelay(uint32_t count);
void sendJsonToUART();
void initialModules();

/*
===================================================================================
                        설정 관련 함수 선언
===================================================================================
*/

// NVIC 설정
// NVIC를 설정하고 각 인터럽트 우선순위를 설정
void NVIC_Config() {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
    /* 모듈화 되어 다른 모듈에 구현되어 있음 */

    // usart1 인터럽트 설정 - 우선순위 1
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;    
    NVIC_Init(&NVIC_InitStructure);
    
    // usart2 인터럽트 설정 - 우선순위 2
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // EXTI 인터럽트 설정
    // 거리 센서 인터럽트 설정 - pd1 - 우선순위 3
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 수위 센서 인터럽트 설정 - pd2 - 우선순위 4
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // TIM2 인터럽트 설정(물 펌프 딜레이) - 우선순위 5
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // ADC 인터럽트 설정(토양 수분 값, 조도 값) - 우선순위 6
    // 명령 수행 도중 값이 바뀌는 것을 방지하기 위해 우선순위를 낮게 설정
    NVIC_EnableIRQ(ADC1_2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;  
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// USART1 설정
// PA9 - TX - esp32 d16
// PA10 - RX = esp32 d17

void usart1_config() {
    // 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // GPIOA 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); //  AFIO 클럭 설정(USART1의 TX, RX 핀을 사용하기 위해)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); // USART1 클럭 설정

    // GPIO 설정
    // TX - PA9        
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // RX - PA10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOA, &GPIO_InitStructure);    
    // USART 설정
    USART_Cmd(USART1, ENABLE);
    USART_InitStructure.USART_BaudRate = 9600; // 통신 속도 설정
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;  // 데이터 비트 설정
    USART_InitStructure.USART_StopBits = USART_StopBits_1; // 스탑 비트 설정
    USART_InitStructure.USART_Parity = USART_Parity_No;// 패리티 설정
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 송수신 설정
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 하드웨어 흐름 제어 설정
    USART_Init(USART1, &USART_InitStructure); 
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 수신 인터럽트 설정
}

// usart2 설정
// pd5 - tx
// pd6 - rx
void usart2_config() {
    // 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE); // GPIOA 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // AFIO 클럭 설정(USART1의 TX, RX 핀을 사용하기 위해)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // USART2 클럭 설정
    GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE); // PD5, PD6 핀을 uart2로 사용하기 위해 리맵핑 
    // TX - PD5
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    // RX - PD6
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    // USART 설정
    USART_Cmd(USART2, ENABLE);
    USART_InitStructure.USART_BaudRate = 9600; // 통신 속도 설정
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; // 데이터 비트 설정
    USART_InitStructure.USART_StopBits = USART_StopBits_1; // 스탑 비트 설정
    USART_InitStructure.USART_Parity = USART_Parity_No; // 패리티 설정
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 송수신 설정
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 하드웨어 흐름 제어 설정
    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // 수신 인터럽트 설정
}

// adc 활용이 필요한 모듈 관련 설정
// PA1 - 토양 수분 센서 값 읽어오기
// PC1 - 조도 센서 값 읽어오기
void adc_module_config() {
    // RCC 설정

    // ADC1 - 토양 수분 센서 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // GPIOA 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); // ADC1 클럭 설정
    // ADC2 - 조도 센서 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // GPIOC 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE); // ADC2 클럭 설정


    // GPIO 설정
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    
    
    // ADC 설정
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; // 독립 모드 설정
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 스캔 모드 설정
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 연속 변환 모드 설정(연속적으로 값을 읽어오기 위해 설정)
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 외부 트리거 설정
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // 데이터 정렬 설정
    ADC_InitStructure.ADC_NbrOfChannel = 1; // 채널 수 설정
    ADC_Init(ADC1, &ADC_InitStructure); 
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5); // 채널 설정
    
    // ADC 설정
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; // 독립 모드 설정
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 스캔 모드 설정
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 연속 변환 모드 설정(연속적으로 값을 읽어오기 위해 설정)
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 외부 트리거 설정
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // 데이터 정렬 설정
    ADC_InitStructure.ADC_NbrOfChannel = 1; // 채널 수 설정
    ADC_Init(ADC2, &ADC_InitStructure);
    ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 1, ADC_SampleTime_239Cycles5); // 채널 설정


    
    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE); // ADC1 인터럽트 설정
    ADC_Cmd(ADC1, ENABLE); 
    ADC_ResetCalibration(ADC1);

    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);

    while(ADC_GetCalibrationStatus(ADC1)) ;
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);


    // 인터럽트 설정
    ADC_ITConfig(ADC2, ADC_IT_EOC, ENABLE); // ADC2 인터럽트 설정
    ADC_Cmd(ADC2, ENABLE);
    ADC_ResetCalibration(ADC2);

    while(ADC_GetResetCalibrationStatus(ADC2)) ;
    ADC_StartCalibration(ADC2);

    while(ADC_GetCalibrationStatus(ADC2)) ;
    ADC_SoftwareStartConvCmd(ADC2, ENABLE);
}


// 문(서보 모터) 관련 설정 - TIM3_CH1 - PA6(PWM) 
void servo_config() {
    // RCC 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // GPIO 설정
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // TIM 설정
    TIM_TimeBaseStructure.TIM_Period = 20000 - 1; // 20ms 주기 설정
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; // 72MHz / 72 = 1MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = 0; // 클럭 분할 설정
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 업 카운터 모드 설정
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); // TIM3 설정

    // PWM 설정
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // PWM 모드 설정
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 출력 상태 설정
    TIM_OCInitStructure.TIM_Pulse = SERVO_RIGHT; // 펄스 길이 설정
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 출력 극성 설정 
    TIM_OC1Init(TIM3, &TIM_OCInitStructure); // TIM3_CH1 설정
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable); // TIM3_CH1 설정

    // TIM 설정
    TIM_Cmd(TIM3, ENABLE);
}



// esp32와 인터페이스로 작용하는 핀들의 설정
// PD01 - EXTI 1 인터럽트로 거리값 읽어오기
// PD02 - EXTI 2 인터럽트로 수위 값 읽어오기
// PB12 - auto 모드인지 아닌지 읽어오기
// PB13 - 토양 습도 센서 값 읽어오기
void eps32_config() {
    // RCC 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // GPIOB 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE); // GPIOD 클럭 설정

    // GPIO 설정
    // 입력 설정(거리 센서, 수위 센서)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // 출력 설정(auto 모드, 토양 습도 센서) 
    // esp32 에게 정보를 전달하기 위해 설정
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // EXTI1 인터럽트 설정 
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource1);
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // 두 상태 모두 인터럽트 발생(변화된 값을 읽어오기 위해)
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // EXTI2 인터럽트 설정
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource2);
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // 두 상태 모두 인터럽트 발생(변화된 값을 읽어오기 위해)
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
}

/*
물 펌프 관련 설정
물 제어를 위한 타이머 설정 - TIM2 : PA2
물 제어를 위한 GPIO 설정 - PC4(디지털 출력)
물 제어를 위한 GPIO 설정 - PA4(디지털 출력)
// set set
// reset set
*/
void pump_config() {
    // RCC 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // GPIOA 클럭 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // GPIOC 클럭 설정
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // TIM2 클럭 설정
    
    // GPIO 설정
    // PC4 - 물 펌프 제어
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // PC 7 always on
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // TIM 설정 - 2 초동안 물을 주기 위해 설정
    uint16_t prescaler = 7200;
    TIM_InitStructure.TIM_Period = PUMP_DELAY;
    TIM_InitStructure.TIM_Prescaler = prescaler;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); // 끝날 때 인터럽트로 펌프를 끄기 위해 설정

    // 인터럽트 등록
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 기본 상태 - 물 펌프 끄기
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
    GPIO_SetBits(GPIOC, GPIO_Pin_4);
}


// LED 제어를 위한 GPIO 설정 - PC5(디지털 출력)
// PC5와 연결된 릴레이 모듈을 제어하기 위해 설정
void led_config() {
    // RCC 설정
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    // GPIO 설정
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/*
-----------------------------------------------------------------------------------
*/

/*
===================================================================================
                        인터럽트 함수 선언
===================================================================================
*/

// USART1 인터럽트 함수
// 명령어를 읽어 usart1Buffer에 저장하고, 줄바꿈 문자 입력 시 명령어를 실행하는 함수
void USART1_IRQHandler() {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char data = USART_ReceiveData(USART1); // 데이터 읽어오기
        readBuffer(&usart1Buffer, data); // 버퍼에 데이터 저장
        if (data == '\r'|| data == '\n') { // 줄바꿈 문자 "/r/n" 입력 시 명령어 실행
            excuteCommand(USART1_DIR); // 명령어 실행 함수 
            resetIndex(&usart1Buffer); // 버퍼 초기화
        } 
    }
    USART_ClearITPendingBit(USART1, USART_IT_RXNE); // 인터럽트 플래그 초기화
}

// USART2 인터럽트 함수
// esp32로부터 명령어를 읽어 usart2Buffer에 저장하고, 줄바꿈 문자 입력 시 명령어를 실행하는 함수
void USART2_IRQHandler() {
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        char data = USART_ReceiveData(USART2); // 데이터 읽어오기
        readBuffer(&usart2Buffer, data); // 버퍼에 데이터 저장
        if (data == '\r' || data == '\n') {
            excuteCommand(USART2_DIR); // 명령어 실행 함수
            resetIndex(&usart2Buffer); // 버퍼 초기화
        }
    }
    USART_ClearITPendingBit(USART2, USART_IT_RXNE); // 인터럽트 플래그 초기화
}

/* ADC 인터럽트 함수 - 토양 습도 센서 값 읽어오기 */
// PB13 포트 조작하여 esp32로 토양 임계정보 전달
void ADC1_2_IRQHandler() {

    // 토양 습도 센서(ADC1, PA1)에서 값을 읽어오기 위한 인터럽트 처리
    if (ADC_GetITStatus(ADC1, ADC_IT_EOC) != RESET) { 
        uint16_t adcValue = ADC_GetConversionValue(ADC1);
        deviceState.soilMoisture = adcValue; // 토양 습도 값 저장
        sensorState.isWet = (adcValue > WET_LIMIT) ? true : false; // 토양 센서 상태 저장
        
        // 토양습도가 충분하고 자동모드일 때 펌프를 끈다.
        // 토양습도 상태를 esp32로 전달하기 위해 핀을 조작한다.
        if (sensorState.isWet) {
            if (deviceState.mode) { 
                controlPump(false);
            }
            GPIO_SetBits(GPIOB, GPIO_Pin_13);

        // 토양습도가 충분하지 않고 자동모드일 때 물을 준다.
        // 토양습도 상태를 esp32로 전달하기 위해 핀을 조작한다.
        } else if (!sensorState.isWet) {
            if (deviceState.mode) {
                controlPump(true);
            }
            GPIO_ResetBits(GPIOB, GPIO_Pin_13);
        }
        ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
    }

    // 조도 센서(ADC2, PC1)에서 값을 읽어오기 위한 인터럽트 처리
    if (ADC_GetITStatus(ADC2, ADC_IT_EOC) != RESET) {
        uint16_t adcValue = ADC_GetConversionValue(ADC2); // 조도 센서 값 읽어오기
        deviceState.light = adcValue; // 조도 값 저장
        sensorState.isBright = (adcValue > BRIGHT_LIMIT) ? true : false; // 밝기 센서 상태 저장
        // 모드가 자동일 경우 led를 제어한다.
        if (deviceState.mode) {
            // led가 켜져있고 밝은 상태일 때 led를 끈다.
            if (sensorState.isBright && deviceState.led) {
                controlLED(false);
            // led가 꺼져있고 어두운 상태일 때 led를 켠다.
            } else if (!sensorState.isBright && !deviceState.led){
                controlLED(true);
            }
        }
        ADC_ClearITPendingBit(ADC2, ADC_IT_EOC);
    }
}

/* TIM2 인터럽트 함수 - 물 펌프 제어 */
// water 함수 호출 시 2초 후 펌프 모터를 끄기위한 인터럽트 처리
void TIM2_IRQHandler() {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        controlPump(false); // 펌프 끄기
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        TIM_Cmd(TIM2, DISABLE);
    }
}

/* EXTI1 인터럽트 함수 - 거리 센터 상태 읽어오기 */
// rising, falling edge 인터럽트로 핀 상태 변화를 읽어온다.
// esp32에서 부터 읽어온 거리 상태 정보를 저장하고 모드에 따라 문(서보 모터)를 제어한다.
void EXTI1_IRQHandler() {
    if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
        sensorState.isClose = GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_1); // 거리 센서 상태 읽어오기
        // "자동"모드일 때 거리 센서 상태에 따라 문을 제어한다.
        if (deviceState.mode) {
            // 문이 열려있고 거리 센서가 가까운 상태일 때 문을 닫는다.
            if (sensorState.isClose && deviceState.door){
              controlServo(SERVO_LEFT);
            // 문이 닫혀있고 거리 센서가 멀리 떨어진 상태일 때 문을 열린다.
            } else if (!sensorState.isClose && !deviceState.door){
              controlServo(SERVO_RIGHT);
            }
        }
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

/* EXTI2 인터럽트 함수 - 수위가 충분한지 읽어오기 */
// rising, falling edge 인터럽트로 핀 상태 변화를 읽어온다.
// esp32에서 부터 읽어온 수위 상태 정보를 저장한다.
void EXTI2_IRQHandler() {
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        sensorState.isEnough = (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2)); // 수위 센서 상태 읽어오기
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
}
/*
-----------------------------------------------------------------------------------
*/


/*
===================================================================================
                        사용자 정의 함수 선언
===================================================================================
*/

/* USART 통신 관련 함수 - 읽어온 값 버퍼에 저장 & 오버플로우 방지 */
// 버퍼에 데이터를 저장하고, 버퍼의 크기를 넘어가지 않도록 설정한다
// 줄바꿈 문자('\r')를 문자 끝으로 설정한다.
void readBuffer(Buffer* buffer, char data) {
    if (buffer->index < BUFFER_SIZE - 1) {
        if (data == '\r')
            buffer->mem[buffer->index] = '\0';
        else if (data == '\n') return;
        else 
            buffer->mem[buffer->index++] = data;
    } else {
        buffer->mem[BUFFER_SIZE - 1] = '\0';
    }
}

/*USART 통신 관련 함수 -  버퍼 인덱스 초기화 */
// 버퍼의 인덱스를 초기화한다.
void resetIndex(Buffer* buffer) {
    buffer->index = 0;
}

/* USART 통신 관련 함수 - 명령어 실행 파싱 & 실행 함수*/
// 명령어를 처리하고, 해당 명령어에 따라 동작을 수행한다. 
// 매개변수로 부터 어떤 USART에서 명령어를 읽어왔는지 구분한다.
void excuteCommand(USART_DIR which_usart) {
    // 어디서 명령어를 읽어왔는지에 처리할 버퍼를 설정한다.
    char* command = (which_usart == USART1_DIR) ? usart1Buffer.mem : usart2Buffer.mem;
    // 명령어 처리
    // "set mode 1" : auto 모드로 변경
    if (strcmp(command, SET_MODE_AUTO) == 0) {
        if (deviceState.mode) return;
        initialModules(); // 모든 모듈 초기화
        GPIO_SetBits(GPIOB, GPIO_Pin_12); // esp32에게 auto 모드로 변경되었음을 알림
        deviceState.mode = true; // 구조체에 mode 상태 저장
        return;
    // "set mode 0" : manual 모드로 변경
    } else if (strcmp(command, SET_MODE_MANUAL) == 0) {
        if (!deviceState.mode) return;
        initialModules(); // 모든 모듈 초기화
        GPIO_ResetBits(GPIOB, GPIO_Pin_12); // esp32에게 manual 모드로 변경되었음을 알림
        deviceState.mode = false; // 구조체에 mode 상태 저장
        return;
    } 

    // 모듈 제어 관련 명령어 셋은 수동 모드일 때만 동작한다.
    if (!deviceState.mode) {
        // "set led 1" : led 켜기
        if (strcmp(command, SET_LED_ON) == 0) {
            controlLED(true); // led 켜기
        // "set led 0" : led 끄기
        } else if (strcmp(command, SET_LED_OFF) == 0) {
            controlLED(false); // led 끄기
        // "set door 1" : 문 열기
        } else if (strcmp(command, SET_DOOR_OPEN) == 0) {
            controlServo(SERVO_LEFT); // 문 열기
        // "set door 0" : 문 닫기
        } else if (strcmp(command, SET_DOOR_CLOSE) == 0) {
            controlServo(SERVO_RIGHT); // 문 닫기
        // "water" : 물 주기
        } else if (strcmp(command, WATERING) == 0) {
            water(); // 물 주기
        }
    }
}

/* 문(서보 모터)를 제어하는 함수 */
void controlServo(uint16_t pulse) {
    // 변경된 문의 상태를 구조체에 저장
    if (pulse == SERVO_LEFT) deviceState.door = false;
    else deviceState.door = true;
    
    
    TIM_OCInitStructure.TIM_Pulse = pulse; // 펄스 폭을 조절해 문 제어
    TIM_OC1Init(TIM3, &TIM_OCInitStructure); // 변경된 설정 적용
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
}

/* LED 제어 함수(릴레이 모듈 활용) */
void controlLED(bool on) {
  if (on) {
    GPIO_SetBits(GPIOC, GPIO_Pin_5); // led 켜기
    deviceState.led = true; // led 상태 구조체에 저장
  }
  else {
    GPIO_ResetBits(GPIOC, GPIO_Pin_5); // led 끄기
    deviceState.led = false; // led 상태 구조체에 저장
  }
}

/* 물 펌프 제어함수 */
void water() {
    if (pumpState == PUMP_ON) return; // 이미 물을 주고 있는 상태일 때 함수 종료
    controlPump(true); // 물 펌프 켜기
    pumpState = PUMP_ON; // 물 펌프 상태 저장
    // 2초 후 물 펌프 끄기
    TIM_Cmd(TIM2, ENABLE); // 타이머의 인터럽트가 물 펌프를 끄기 위해 설정
}

// 자동 모드에서 물을 주기 위한 함수
// 타이머를 활용하지 않고 직접적으로 펌프 제어
void controlPump(bool on) { 
    if (on) {
        // 물 펌프 켜기
        GPIO_SetBits(GPIOA, GPIO_Pin_4); 
        GPIO_ResetBits(GPIOC, GPIO_Pin_4);
        pumpState = PUMP_ON; // 물 펌프 상태를 구조체에 저장
    } else {
        // 물 펌프 끄기
        GPIO_SetBits(GPIOA, GPIO_Pin_4);
        GPIO_SetBits(GPIOC, GPIO_Pin_4);
        pumpState = PUMP_OFF; // 물 펌프 상태를 구조체에 저장
    }
}

// 딜레이 함수(반복문 활용)
void myDelay(uint32_t count) {
    for (int i=0; i<count; ++i);
}

// usart2에 json 형태의 데이터를 전송하기 위한 함수
void sendJsonToUART() {
    sprintf(jsonBuffer.mem, 
        /*
            json 구조 : {"mode":0, "led":0, "door":0, "soil":0, "light":0, "level":0}
        */

        "{\"mode\":%d, \"led\":%d, \"door\":%d, \"soil\":%d, \"light\":%d, \"level\":%d}\r\n",
         deviceState.mode, deviceState.led, deviceState.door, deviceState.soilMoisture, deviceState.light, sensorState.isEnough);
    for (int i=0; jsonBuffer.mem[i] != '\0'; ++i) {
        USART_SendData(USART2,jsonBuffer.mem[i]);
        myDelay(USART2_DELAY);
    }
}

// 모듈 상태를 초기화하는 함수
void initialModules() {
    controlServo(SERVO_RIGHT); // 문 닫기
    controlLED(false); // led 끄기
    controlPump(false); // 물 펌프 끄기
}

/*
-----------------------------------------------------------------------------------
*/

/*
===================================================================================
                        메인 함수
===================================================================================
*/


 
int main() {
    char pBuffer[400];

    adc_module_config(); // 아날로그 모듈 설정(토양 수분, 조도)
    usart1_config(); // usart1 설정
    usart2_config();
    eps32_config(); // esp32와 인터페이스로 작용하는 핀의 초기화
    servo_config(); // 문(서보 모터) 관련 설정
    pump_config(); // 물 펌프 관련 설정
    led_config(); // led(릴레이 모듈) 관련 설정
    NVIC_Config(); // 인터럽트 설정
    LCD_Init(); // LCD 초기화
    LCD_Clear(WHITE);  // 화면을 흰색으로 초기화
    initialModules(); // 모듈 초기화
    
    while(1) {
        // 상태 출력
        // 물통에 물이 부족할 때(수위 센서가 임계값보다 낮을 때 - esp32)
        if (!sensorState.isEnough) {
            LCD_ShowString(10, 10, "Need to supply water", BLACK, WHITE);
        // 토양 습도가 부족할 때(토양 수분 센서가 임계값보다 낮을 때)
        } else if (!sensorState.isWet) {
            LCD_ShowString(10, 10, "give water to plant       ", BLACK, WHITE);
        // 모든 상태가 양호할 때
        } else {
            LCD_ShowString(10, 10, "Good condition            ", BLACK, WHITE);
        }

        // 모드 출력 - 자동모드일 때 자동모드, 수동모드일 때 수동모드 출력
        if (deviceState.mode) {
            LCD_ShowString(10, 30, "mode : auto   ", BLACK, WHITE);
        } else {
            LCD_ShowString(10, 30, "mode : manual ", BLACK, WHITE);
        }

        // 센서값 출력
        sprintf(pBuffer, "Bright : %d   ", deviceState.light); // 조도 센서 값 출력
        LCD_ShowString(10, 70, pBuffer, BLACK, WHITE);
        sprintf(pBuffer, "Soil moisture : %d   ", deviceState.soilMoisture); // 토양 수분 센서 값 출력
        LCD_ShowString(10, 90, pBuffer, BLACK, WHITE);
        
        // esp32 출력
        sprintf(pBuffer, "isClose : %d   ", sensorState.isClose); // 거리 센서 값 출력(esp32로 부터 읽어온 값)
        LCD_ShowString(10, 110, pBuffer, BLACK, WHITE);
        sprintf(pBuffer, "isEnough : %d   ", sensorState.isEnough); // 수위 센서 값 출력(esp32로 부터 읽어온 값)
        LCD_ShowString(10, 130, pBuffer, BLACK, WHITE);
        
        
        // 명령어 출력(블루투스 모듈로 부터 읽어온 문자열)
        sprintf(pBuffer, "Command : %s                                 ", usart1Buffer.mem);
        LCD_ShowString(10, 170, pBuffer, BLACK, WHITE);
        
        // usart2로 json 데이터 전송
        sendJsonToUART();
        // 딜레이

    }
}
/*
-----------------------------------------------------------------------------------
*/
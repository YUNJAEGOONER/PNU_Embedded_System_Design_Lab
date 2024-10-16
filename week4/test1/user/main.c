
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

#include "misc.h"

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void EXTI_Configure(void);
void USART1_Init(void);
void NVIC_Configure(void);

void EXTI15_10_IRQHandler(void);

void Delay(void);

void sendDataUART1(uint16_t data);

int mode = 0; // default 정방향
int idx = 0; 
char msg[] = "Team08\r\n";

//---------------------------------------------------------------------------------------------------

void RCC_Configure(void)
{
	// TODO: Enable the APB2 peripheral clock using the function 'RCC_APB2PeriphClockCmd'
	
	/* UART TX/RX port clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); //rx, tx가 PA를 씀
	/* Button S1, S2, S3 port clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // S1, S2, S3가 PB, PC를 씀
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	/* LED port clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	/* USART1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	/* Alternate Function IO clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // Button S1, S2, S3 port KEY 1 : PC4 /  KEY 2 : PB10 /  KEY 1 : PC 13
    // rx : PA10, tx : PA9
	// TODO: Initialize the GPIO pins using the structure 'GPIO_InitTypeDef' and the function 'GPIO_Init'
	
    /* Button S1, S2, S3 pin setting */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU | GPIO_Mode_IPD; // pull up과 down으로 설정
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU | GPIO_Mode_IPD; // pull up과 down으로 설정
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    /* LED pin setting*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
	
    /* UART pin setting */
    //TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // tx를 Alternate function output Push-pull로 설정
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 혹시 모르니 안되면 주석시키기!!
	//RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU | GPIO_Mode_IPD; // rx를 Input with pull-up / pull-down으로 설정
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
}

void EXTI_Configure(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

	// TODO: Select the GPIO pin (Joystick, button) used as EXTI Line using function 'GPIO_EXTILineConfig'
	// TODO: Initialize the EXTI using the structure 'EXTI_InitTypeDef' and the function 'EXTI_Init'
	// Button S1, S2, S3 port KEY 1 : PC4 /  KEY 2 : PB10 /  KEY 1 : PC 13
    /* Button S1 */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Button S2 */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);
    EXTI_InitStructure.EXTI_Line = EXTI_Line10;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

	/* Button S3 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource13);
    EXTI_InitStructure.EXTI_Line = EXTI_Line13;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    //안되면 Line확인하기!!
	// NOTE: do not select the UART GPIO pin used as EXTI Line here
}

void USART1_Init(void)
{
	USART_InitTypeDef USART1_InitStructure;

	// Enable the USART1 peripheral
	USART_Cmd(USART1, ENABLE);
	
	// TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
	USART1_InitStructure.USART_BaudRate = 9600;
    USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART1_InitStructure.USART_StopBits = USART_StopBits_1;
    USART1_InitStructure.USART_Parity = USART_Parity_No;
    //USART1_InitStructure.USART_Mode = 
    USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART1_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
     // usart 구조체 초기화 설정

    USART_Init(USART1,&USART1_InitStructure);
	
	// TODO: Enable the USART1 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    // RXNE = rx not empty kya~
}

void NVIC_Configure(void) {

    NVIC_InitTypeDef NVIC_InitStructure;
    
    // TODO: fill the arg you want
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 

	// TODO: Initialize the NVIC using the structure 'NVIC_InitTypeDef' and the function 'NVIC_Init'
	//일단 넘어가;;
    // Button S1
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Button S2, S3
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // UART1
	// 'NVIC_EnableIRQ' is only required for USART setting
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void USART1_IRQHandler() {
	uint16_t word;
    if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET){ // GetITStatus로 인터럽트 발생 여부 파악가능
    	// the most recent received data by the USART1 peripheral
        word = USART_ReceiveData(USART1);

        // TODO implement
        if (word == 'a') {
            mode = 0;
        }

        else if (word == 'b') {
            mode = 1;
        }
        
        // clear 'Read data register not empty' flag
    	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}

void EXTI15_10_IRQHandler(void) { // when the button is pressed

    if (EXTI_GetITStatus(EXTI_Line10) != RESET) { // key2
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10) == Bit_RESET) {
			// TODO implement
            mode = 1;
		}
		EXTI_ClearITPendingBit(EXTI_Line10);
	}
    if (EXTI_GetITStatus(EXTI_Line13) != RESET) { // key3
		if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == Bit_RESET) {
			// TODO implement
            int i = 0;
            for(i=0; i<14; i++){
                sendDataUART1(msg[i]);
            }     
		}
        EXTI_ClearITPendingBit(EXTI_Line13);
	}
}

// TODO: Create Joystick interrupt handler functions
void EXTI4_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line4) != RESET) { // key1
		if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == Bit_RESET) {
			// TODO implement
            mode = 0;
		}
		EXTI_ClearITPendingBit(EXTI_Line4);
	}
}

void Delay(void) {
	int i;

	for (i = 0; i < 2000000; i++) {}
}

void sendDataUART1(uint16_t data) {
	/* Wait till TC is set */
	while ((USART1->SR & USART_SR_TC) == 0);
	USART_SendData(USART1, data);
}

int main(void)
{

    SystemInit();

    RCC_Configure();

    GPIO_Configure();

    EXTI_Configure();

    USART1_Init();

    NVIC_Configure();
    
    unsigned led_array[4] = {
      GPIO_Pin_2,
      GPIO_Pin_3,
      GPIO_Pin_4,
      GPIO_Pin_7,
    };

    while (1) {
    	// TODO: implement 
        idx = (idx + 4) % 4;
        for (int i = 0; i < 4; ++i) {
          if (i == idx) {
             GPIO_ResetBits(GPIOD, led_array[i]);
          } else {
            GPIO_SetBits(GPIOD, led_array[i]);
          }
        }

        if(mode) {
            idx--;
        }
        else {
            idx++;
        }
    	// Delay
    	Delay();
        


    }
    return 0;
}

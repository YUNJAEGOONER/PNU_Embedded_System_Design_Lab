
#include "stm32f10x_adc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "misc.h"
#include "lcd.h"
#include "touch.h"

void Init(void);
void RccInit(void);
void GpioInit(void);
void AdcInit(void);
void DMA_Configure(void);

#define BUFFER_SIZE 1
volatile uint32_t buffer[1];

const int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,BRRED,GRAY};



int main(){
    
    
    Init();

    LCD_Clear(WHITE);
    
    //TODO: display
    while(1){
        if (buffer[0] > 3500) {
            LCD_Clear(GRAY);
        } else {
            LCD_Clear(WHITE);
        }
    }
}

void Init(void) {
	SystemInit();
	RccInit();
	GpioInit();
	AdcInit();
        DMA_Configure();

	LCD_Init();
	Touch_Configuration();
	//Touch_Adjust();
}

void RccInit(void) {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
}

void GpioInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void AdcInit(void) {
    ADC_InitTypeDef ADC_InitStructure;

    //TODO: ADC1 Configuration
    
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;  
    ADC_Init(ADC1, &ADC_InitStructure);

    //Config
    ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_239Cycles5);
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);

    while(ADC_GetResetCalibrationStatus(ADC1));

    ADC_StartCalibration(ADC1);

    while(ADC_GetCalibrationStatus(ADC1));

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

//TODO
void DMA_Configure(void) {
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;  
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)buffer;   
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;    
    DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE;   
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;  
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;   
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;   
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1, ENABLE);
}

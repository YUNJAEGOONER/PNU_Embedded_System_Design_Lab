#include "stm32f10x.h"
#include "core_cm3.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_adc.h"
#include "lcd.h"
#include "touch.h"

 //���� �迭 ����
int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,RED,GRAY};
uint16_t value;
uint16_t x1 = 0;
uint16_t y1 = 0;

void RCC_Configure(void);
void GPIO_Configure(void);
void ADC_Configure(void);
void NVIC_Configure(void);
void GPIO_Configure(void);

void RCC_Configure() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

void GPIO_Configure() {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// ADC ����ü ���� �ڵ�
// ADC �� �б�� ���ͷ�Ʈ�̿�
void ADC_Configure() {
    // ADC ���� �Լ� �ȿ��������Լ�(���۷�������)
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // �̻��ϸ� Ȯ��
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    
    
    ADC_Init(ADC1, &ADC_InitStructure);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 1, ADC_SampleTime_239Cycles5); // ���� �ʹ� ũ�� �ٲٱ�
    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    // set �ɶ����� ���?

    // Calibration�̶�? :  �Ƴ��α� �Է°��� �����ϴ� ����
   
    ADC_ResetCalibration(ADC1);
    // Calibration �ʱ�ȭ
    while(ADC_GetResetCalibrationStatus(ADC1));
    // Calibration ����
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    
    // ADC ����
}

void NVIC_Configure() {
    NVIC_InitTypeDef NVIC_InitStructure;

    // TODO: fill the arg you want
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); 
    NVIC_EnableIRQ(ADC1_2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


void ADC1_2_IRQHandler() {
  if(ADC_GetITStatus(ADC1, ADC_IT_EOC) != RESET){
    value = ADC_GetConversionValue(ADC1);
    ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
  }
}
int main() {
    // LCD ���� ������ LCD_Init�� �����Ǿ� �����Ƿ� ���⼭ �� �ʿ����
    SystemInit();
    RCC_Configure();
    GPIO_Configure();
    ADC_Configure();
    NVIC_Configure();
    // -----------------------------------  
    LCD_Init();
    Touch_Configuration();
    Touch_Adjust();
    LCD_Clear(WHITE);
     LCD_ShowString(3, 3, "THUR_Team08", BLACK, WHITE);
    while(1){
    // TODO : LCD �� ��� �� ��ġ ��ǥ �б�
        Touch_GetXY(&x1, &y1, 1);
        Convert_Pos(x1, y1, &x1, &y1);

        LCD_ShowNum(50,50, x1, 4, BLACK, WHITE);
        LCD_ShowNum(50, 70, y1, 4, BLACK, WHITE);

        ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);
        LCD_ShowNum(60,100, value, 4, BLACK, WHITE);
        ADC_ITConfig(ADC1,ADC_IT_EOC,DISABLE);
        LCD_DrawCircle(x1, y1, 4);    
    }
    return 0;
}

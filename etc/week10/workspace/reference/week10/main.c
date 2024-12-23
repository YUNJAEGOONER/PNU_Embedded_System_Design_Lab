//main.c
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "lcd.h"
#include "touch.h"
#include "misc.h"

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void NVIC_Configure(void);
void ADC_Configure(void);
void Delay(void);

int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,BRRED,GRAY};
//---------------------------------------------------------------------------------------------------

uint16_t lightSenser = 0;
uint16_t x1 = 0;
uint16_t y1 = 0;
void RCC_Configure(void)
{
  // TODO: Enable the APB2 peripheral clock using the function
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOB, ENABLE); // ADC1, port C RCC ENABLE

  /* Alternate Function IO clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

void GPIO_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /* ADC1 */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  //@ Set Pin Mode Input Pull-up/down UP
  //@ Set Pin Speed Max : 50MHz
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  //@ Set Pin Mode General Output Push-Pull
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}
//ADC init 함수
void ADC_Configure(void) {
  ADC_InitTypeDef ADC_InitStructure;
  // ADC1 Configuration
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1,
                           ADC_SampleTime_239Cycles5);
  ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE); // interrupt enable
  ADC_Cmd(ADC1, ENABLE); // ADC1 enable
  ADC_ResetCalibration(ADC1);

  while(ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1));
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void NVIC_Configure(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  // TODO: fill the arg you want
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  // ADC1
  //@ NVIC Line ADC1
  NVIC_EnableIRQ(ADC1_2_IRQn);
  NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // TODO
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
// ADC는 인터럽트 베이스이므로 핸들러 작성 필요
void ADC1_2_IRQHandler() {
  if(ADC_GetITStatus(ADC1, ADC_IT_EOC) != RESET){
    lightSenser = ADC_GetConversionValue(ADC1);
    ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
  }
}

int main(void)
{

  SystemInit();
  RCC_Configure();
  GPIO_Configure();
  ADC_Configure();
  NVIC_Configure();

  LCD_Init();
  Touch_Configuration();
  Touch_Adjust();

  LCD_Clear(WHITE);
  LCD_ShowString(10, 10, "THU_Team07", BLACK, WHITE);

  while (1) {
    // TODO: implement

    Touch_GetXY(&x1, &y1, 1);
    Convert_Pos(x1, y1, &x1, &y1);

    LCD_ShowNum(50,50, x1, 4, BLACK, WHITE);
    LCD_ShowNum(50, 70, y1, 4, BLACK, WHITE);

    ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);
    LCD_ShowNum(60,100, lightSenser, 4, BLACK, WHITE);
    ADC_ITConfig(ADC1,ADC_IT_EOC,DISABLE);
    LCD_DrawCircle(x1, y1, 4);
  }
  return 0;
}

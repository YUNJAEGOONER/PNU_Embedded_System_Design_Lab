#include "stm32f10x.h"

// RCC ���� �ּ�
// Reset and Clock Control
#define RCC_APB2ENR (*(volatile unsigned int *)0x40021018)

// Configuration register
// ����� Port : PA3, PA4, PB10, PC4, PC13
// Port_A HIGH & LOW (PA3, PA4) : Port_A�� ���� �ּҿ� offset ���ϱ�
#define GPIOA_CRL (*(volatile unsigned int *)0x40010800)
#define GPIOA_CRH (*(volatile unsigned int *)0x40010804)
//Port_B HIGH (PA10) : Port_B�� ���� �ּҿ� offset ���ϱ�
#define GPIOB_CRH (*(volatile unsigned int *)0x40010C04)
// Port_C HIGH & LOW (PC4, PC13) : Port_C�� ���� �ּҿ� offset ���ϱ�
#define GPIOC_CRL (*(volatile unsigned int *)0x40011000)
#define GPIOC_CRH (*(volatile unsigned int *)0x40011004)

//IDR(Port input data register)
//Port�� ���� �ּҿ� + IDR offset(0x08)
#define GPIOA_IDR (*(volatile unsigned int *) 0x40010808) // key�� B,C�� ->�ʿ��������
#define GPIOC_IDR (*(volatile unsigned int *)0x40011008)
#define GPIOB_IDR (*(volatile unsigned int *)0x40010C08)


// set-reset ������ ���� brr, bsrr
// Port_A�� ���� �ּҿ� brr, bsrr offset�� ���� ���Ѵ�.
#define GPIOA_BRR (*(volatile unsigned int *)0x40010814)
#define GPIOA_BSRR (*(volatile unsigned int *)0x40010810)
#define GPIOC_BRR (*(volatile unsigned int *) 0x4001114)

//PortD�� ������� ����
#define GPIOD_CRL (*(volatile unsigned int *)0x40011400)
#define GPIOD_BSRR (*(volatile unsigned int *)0x40011410)
#define GPIOD_BRR (*(volatile unsigned int *)0x40011414)

void delay() {
  int i;
  for (i=0; i<10000000; i++) {}
}


int main(void)
{
  // clock enable : PORT A, B, C, D ON
  RCC_APB2ENR |= 0x3C;

  // �������� �ʱ�ȭ = Key 1 PC4
  GPIOC_CRL &= 0xFFF0FFFF; // PC4�� ����
  GPIOC_CRL |= 0x00080000; // mode : input (10 00)

  // Key 2 PB10
  GPIOB_CRH &= 0xFFFFF0FF; // PB10�� ����
  GPIOB_CRH |= 0x00000800; // mode : input (10 00)

  // Key 3 PC13
  GPIOC_CRH &= 0xFF0FFFFF; // PC13�� ����
  GPIOC_CRH |= 0x00800000; // mode : input (10 00)

  //PA3, PA4 -> ������ ��� ��� ���� ���
  //GPIOA_CRH &= 0xFF0FF0FF; //PA10, PA13�� ����
  //GPIOA_CRH |= 0x00100100; // ����� ����ϴϱ� outmode?
  
  GPIOA_CRH &= 0x00000000; //PA10, PA13�� ����
  GPIOA_CRH |= 0x11111111; // ����� ����ϴϱ� outmode?
  

  //���� �ڵ�� �ּ�ó�� �Ͽ����ϴ�.
  //GPIOA_BRR &= 0x00000000;
  //GPIOA_BSRR |= (0x8 | 0x10);
  
  GPIOD_CRL &= 0x0FF000FF;
  GPIOD_CRL |= 0x30033300;
  GPIOD_BSRR |= 0x9C; // PD2, PD3, PD4, PD7 OF
  
  GPIOA_BSRR |= (0x2000 | 0x400);
  GPIOD_BSRR |= 0xFF;
  GPIOA_BRR &= 0x00000000;
    // 4. input data register



  while(1){
    if(~(GPIOC_IDR) & (0x1 << 4)) { //Key1(pc4)
    
    //GPIOD_BRR |= 0x84; // PD2, PD7 ON LED1, LED4
      
    GPIOD_BRR |= 0x8;
    GPIOD_BRR |= 0x10;

    delay();
    
    //GPIOD_BSRR |= 0x84; // PD2, PD7 OFF LED1, LED4

    GPIOD_BSRR |= 0x8;
    GPIOD_BSRR |= 0x10;
  }
  else if(~(GPIOB_IDR) & (0x1 << 10)){ // Key2(PB10)
    GPIOD_BRR |= 0x8;

    delay();

    GPIOD_BSRR |= 0x8;

  }
  else if(~(GPIOC_IDR) & (0x1 << 13)){ //Key3(PC13)
    GPIOD_BRR |= 0x10;

    delay();

    GPIOD_BSRR |= 0x10;
  }

  
  }
  return 0;
}


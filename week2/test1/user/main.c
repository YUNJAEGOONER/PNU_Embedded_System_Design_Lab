#include "stm32f10x.h"

#define RCC_APB2ENR (*(volatile unsigned int *)0x40021018)

#define GPIOA_CRL (*(volatile unsigned int *)0x40010800)
#define GPIOA_CRH (*(volatile unsigned int *)0x40010804)
#define GPIOA_IDR (*(volatile unsigned int *) 0x40010808)
#define GPIOA_BRR (*(volatile unsigned int *)0x40010814)
#define GPIOA_BSRR (*(volatile unsigned int *)0x40010810)

#define GPIOB_CRH (*(volatile unsigned int *)0x40010C04)
#define GPIOB_IDR (*(volatile unsigned int *)0x40010C08)

#define GPIOC_CRL (*(volatile unsigned int *)0x40011000)
#define GPIOC_CRH (*(volatile unsigned int *)0x40011004)
#define GPIOC_IDR (*(volatile unsigned int *)0x40011008)
#define GPIOC_BRR (*(volatile unsigned int *) 0x4001114)

#define GPIOD_CRL (*(volatile unsigned int *)0x40011400)
#define GPIOD_BSRR (*(volatile unsigned int *)0x40011410)
#define GPIOD_BRR (*(volatile unsigned int *)0x40011414)

void delay() {
  int i;
  for (i=0; i<10000000; i++) {}
}

int main(void)
{

// Key 1 PC4
GPIOC_CRL &= 0xFFF0FFFF;
GPIOC_CRL |= 0x00080000;
// Key 2 PB10
GPIOB_CRH &= 0xFFFFF0FF;
GPIOB_CRH |= 0x00000800;
// Key 3 PC13(input)
GPIOC_CRH &= 0xFF0FFFFF;
GPIOC_CRH |= 0x00800000;      
//PA13 (�����ȣ) output
GPIOA_CRL &= 0xFFF00FFF;
GPIOA_CRL |= 0x00011000;

GPIOA_BRR &= 0x00000000;
GPIOA_BSRR |= (0x8 | 0x10);

GPIOC_IDR &= (1 << 4);
GPIOB_IDR &= (1 << 10);
GPIOC_IDR &= (1 << 13);



// clock
// PORT A, B, C, D ON
RCC_APB2ENR |= 0x3C;

while(1){
  if(!(GPIOC_IDR & (1 << 4))){ //Key1
    
    GPIOA_BRR |= 0x8;
    GPIOA_BRR |= 0x10;

    delay();

    GPIOA_BSRR |= 0x8;
    GPIOA_BSRR |= 0x10;

    //GPIOD_BRR |= 0x84; // PD2, PD7 ON LED1, LED4
  }
  else if((GPIOB_IDR & 0x0400) == 0){ // Key2
    GPIOD_BSRR |= 0x84; // PD2, PD7 OFF LED1, LED4
  }
  else if((GPIOC_IDR & 0x2000) == 0){ //key 3
    GPIOD_BRR |= 0x18; // PD3, PD4 ON LED2, LED3
  }
  else if((GPIOA_IDR & 0x01) == 0){ //key 4
    GPIOD_BSRR |= 0x18; // PD3, PD4 OFF LED2, LED3
  }
}
  return 0;
}


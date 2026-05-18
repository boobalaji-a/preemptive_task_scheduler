/*
 * kernel.c
 *
 *  STM32F103C8T6 Version
 */
#include "kernel.h"

#define SYSPRI3    (*((volatile uint32_t *)0xE000ED20))
#define ICSR       (*((volatile uint32_t *)0xE000ED04))

void schedulerLaunch(void);

#define NUM_OF_THREADS   3
#define STACKSIZE        100

#define BUS_FREQ   64000000

struct tcb{
    int32_t *stackPt;
    struct tcb *nextPt;
};

typedef struct tcb tcbType;

tcbType tcbs[NUM_OF_THREADS];
tcbType *currentPt;

int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];

void kernelStackInit(int i)
{
    tcbs[i].stackPt = &TCB_STACK[i][STACKSIZE-16];

    /*
     * xPSR
     * Thumb bit must be 1
     */
    TCB_STACK[i][STACKSIZE-1] = 0x01000000;
}

uint8_t kernelAddThreads( void(*task0)(void), void(*task1)(void), void(*task2)(void))
{
    __disable_irq();

    tcbs[0].nextPt = &tcbs[1];
    tcbs[1].nextPt = &tcbs[2];
    tcbs[2].nextPt = &tcbs[0];

    kernelStackInit(0);
    TCB_STACK[0][STACKSIZE-2] = (int32_t)(task0);

    kernelStackInit(1);
    TCB_STACK[1][STACKSIZE-2] = (int32_t)(task1);

    kernelStackInit(2);
    TCB_STACK[2][STACKSIZE-2] = (int32_t)(task2);

    currentPt = &tcbs[0];

    __enable_irq();

    return 1;
}

void kernelLaunch(uint32_t quanta)
{
    SysTick->CTRL = 0;
    SysTick->VAL  = 0;

    SysTick->LOAD = (quanta * BUS_FREQ) - 1;

    NVIC_SetPriority(SysTick_IRQn, 15); // SysTick set to default priority
    SysTick->CTRL = 0x07;

    schedulerLaunch();
}

__attribute__((naked)) void SysTick_Handler(void)
{
    __asm volatile
    (
        "CPSID   I               \n"

        "PUSH    {R4-R11}        \n"

        "LDR     R0, =currentPt  \n"
        "LDR     R1, [R0]        \n"

        "STR     SP, [R1]        \n"

        "LDR     R1, [R1, #4]    \n"
        "STR     R1, [R0]        \n"

        "LDR     R1, [R0]        \n"
        "LDR     SP, [R1]        \n"

        "POP     {R4-R11}        \n"

        "CPSIE   I               \n"

        "BX      LR              \n"
    );
}

__attribute__((naked)) void schedulerLaunch(void)
{
    __asm volatile
    (
        "LDR     R0, =currentPt  \n"
        "LDR     R2, [R0]        \n"

        "LDR     SP, [R2]        \n"

        "POP     {R4-R11}        \n"

        "POP     {R0-R3}         \n"
        "POP     {R12}           \n"

        "ADD     SP, SP, #4      \n"

        "POP     {LR}            \n"

        "ADD     SP, SP, #4      \n"

        "CPSIE   I               \n"

        "BX      LR              \n"
    );
}

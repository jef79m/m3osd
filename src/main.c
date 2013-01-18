#include "board.h"
#include "osdcore.h"
#include "sensors.h"
#include "uart.h"
#include "gps.h"
#include "multiwii.h"
#include "usb.h"
#include "fonts.h"
#include "osdComponents.h"

#define BLINK_STACK_SIZE 48
OS_STK blinkStack[BLINK_STACK_SIZE];
volatile uint32_t minCycles, idleCounter, totalCycles;
uint32_t oldIdleCounter;
volatile float idlePercent;

void blinkTask(void *unused)
{
    while (1) {
        CoTickDelay(250);
        LED0_TOGGLE;
        idlePercent = 100.0f * (idleCounter - oldIdleCounter) * minCycles / totalCycles;
        oldIdleCounter = idleCounter;
        totalCycles = 0;
        // gpsData.mode = MODE_PASSTHROUGH;
    }
}

void CoIdleTask(void *pdata)
{
    volatile unsigned long cycles;
    volatile unsigned int *DWT_CYCCNT = (volatile unsigned int *) 0xE0001004;
    volatile unsigned int *DWT_CONTROL = (volatile unsigned int *) 0xE0001000;
    volatile unsigned int *SCB_DEMCR = (volatile unsigned int *) 0xE000EDFC;

    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CONTROL = *DWT_CONTROL | 1;    // enable the counter

    minCycles = 99999999;
    while (1) {
        idleCounter++;

        __nop();
        __nop();
        __nop();
        __nop();

        cycles = *DWT_CYCCNT;
        *DWT_CYCCNT = 0;        // reset the counter

        // record shortest number of instructions for loop
        totalCycles += cycles;
        if (cycles < minCycles)
            minCycles = cycles;
    }
}

void CoStkOverflowHook(OS_TID taskID)
{
    // Process stack overflow here
    while (1);
}

void setup(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    // turn on peripherals needed by all
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // all ain
    gpio.GPIO_Pin = GPIO_Pin_All;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);
    GPIO_Init(GPIOB, &gpio);
    GPIO_Init(GPIOC, &gpio);

    // Debug LED
    gpio.GPIO_Pin = LED0_PIN | LED1_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED0_GPIO, &gpio);
    LED0_OFF;
    LED1_OFF;

    // delay/timing timer configuration (TIM4)
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM4, ENABLE);

}

// microsecond delay using TIM4
void timerDelay(uint16_t us)
{
    uint16_t cnt = TIM4->CNT;
    uint16_t targetTimerVal = cnt + us;

    if (targetTimerVal < cnt)
        // wait till timer rolls over
        while (TIM4->CNT > targetTimerVal);

    while (TIM4->CNT < targetTimerVal);
}


/*
  	 ! 	" 	# 	$ 	 % 	& 	' 	( 	) 	* 	+ 	, 	— 	. 	/
3. 	0 	1 	2 	3 	4 	5 	6 	7 	8 	9 	 : 	 ; 	< 	= 	> 	 ?
4. 	@ 	A 	B 	C 	D 	E 	F 	G 	H 	I 	J 	K 	L 	M 	N 	O
5. 	P 	Q 	R 	S 	T 	U 	V 	W 	X 	Y 	Z 	[ 	\ 	] 	^ 	_
6. 	` 	a 	b 	c 	d 	e 	f 	g 	h 	i 	j 	k 	l 	m 	n 	o
7. 	p 	q 	r 	s 	t 	u 	v 	w 	x 	y 	z 	{ 	| 	} 	~ 	DEL
*/

#define MAIN_STACK_SIZE 512
OS_STK mainStack[MAIN_STACK_SIZE];

void mainTask(void *unused)
{
    int i;
    int offset, head, alt;

    while (1) {
        CoWaitForSingleFlag(osdData.osdUpdateFlag, 0);
        CoClearFlag(osdData.osdUpdateFlag);
        LED1_TOGGLE;
        
       // osdDrawRectangle(0, 0, OSD_WIDTH, osdData.Height, 1);

        //osdHorizon();
        newHorizon();
        osdHeading();
        

        // altitude
        osdDrawVerticalLine(OSD_WIDTH - 100 + 20 + 12, osdData.Height / 2 - 140 / 2 - 3, 160, 1);
        osdSetCursor(OSD_WIDTH - 100 - 1 + 24, osdData.Height / 2 - 10);
        // osdDrawCharacter(239+32,4);
        osdDrawCharacter(249 + 32, FONT_16PX_FIXED);
        for (i = 50; i < 210; i += 10) {
            alt = multiwiiData.altitude / 100;
            //alt = multiwiiData.GPS_altitude;
            offset = alt % 10;
            osdDrawHorizontalLine(OSD_WIDTH - 100 + 20 + 12, osdData.Height / 2 - 50 - 140 / 2 + i + offset - 3, 5, 1);
            if (!((alt - offset + 120 - i) % 20) && (alt - offset + 120 - i + 10 < alt || alt - offset + 120 - i - 10 > alt)) {
                osdSetCursor(OSD_WIDTH - 90 + 24, osdData.Height / 2 - 50 - 140 / 2 + i + offset - 6);
                osdDrawDecimal(FONT_8PX_FIXED, alt - offset + 120 - i, 5, 0, 5);
            }

        }
        osdSetCursor(OSD_WIDTH - 98 + 20 + 12, osdData.Height / 2 - 10);
        //osdDrawDecimal(4, multiwiiData.GPS_altitude, 4, 0, 4);
        osdDrawDecimal(4, multiwiiData.altitude, 5, 0, 2);


        /*
           osdDrawCircle(50, 150, 32, 1, 0xFF);
           osdDrawFilledCircle(50, 150, 16, 1, 0xFF);
           osdDrawFilledCircle(50, 150, 8, 0, GFX_QUADRANT0 | GFX_QUADRANT2);
         */

        if (osdData.PAL) {
            osdSetCursor(OSD_WIDTH - 8 * 8 - 1, 0);
            osdDrawCharacter('P', FONT_8PX_FIXED);
            osdDrawCharacter('A', FONT_8PX_FIXED);
            osdDrawCharacter('L', FONT_8PX_FIXED);
        } else {
            osdSetCursor(OSD_WIDTH - 8 * 8 - 1, 0);
            osdDrawCharacter('N', FONT_8PX_FIXED);
            osdDrawCharacter('T', FONT_8PX_FIXED);
            osdDrawCharacter('S', FONT_8PX_FIXED);
            osdDrawCharacter('C', FONT_8PX_FIXED);
        }
        osdDrawDecimal2(FONT_8PX_FIXED, osdData.maxScanLine, 3, 0, 3);

        // GPS
        osdSetCursor(3, 50);
        osdDrawCharacter('F', FONT_16PX_FIXED);
        osdDrawCharacter('i', FONT_16PX_FIXED);
        osdDrawCharacter('x', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_FIX, 1, 0, 1);

        osdSetCursor(3, 50 + 16);
        osdDrawCharacter('S', FONT_16PX_FIXED);
        osdDrawCharacter('a', FONT_16PX_FIXED);
        osdDrawCharacter('t', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_numSat, 2, 0, 2);

        // int32_t GPS_LAT;
        osdSetCursor(0, 0);
        //osdDrawCharacter(multiwiiData.GPS_LAT < 0 ? 'S' : 'N', FONT_8PX_FIXED);      // S
        osdDrawDecimal2(FONT_8PX_FIXED, abs(multiwiiData.GPS_LAT), 9, 1, 7);

        // int32_t GPS_LON;
        osdSetCursor(0, 0 + 10);
        //osdDrawCharacter(multiwiiData.GPS_LON < 0 ? 'W' : 'E', FONT_8PX_FIXED);      // W
        osdDrawDecimal2(FONT_8PX_FIXED, abs(multiwiiData.GPS_LON), 9, 1, 7);

        // int32_t GPS_homeLAT;
        osdSetCursor(0, +20);
        //osdDrawCharacter(multiwiiData.GPS_homeLAT < 0 ? 'S' : 'N', FONT_8PX_FIXED);  // S
        osdDrawDecimal2(FONT_8PX_FIXED, abs(multiwiiData.GPS_homeLAT), 9, 1, 7);

        // int32_t GPS_homeLON;
        osdSetCursor(0, 0 + 30);
        //osdDrawCharacter(multiwiiData.GPS_homeLON < 0 ? 'W' : 'E', FONT_8PX_FIXED);  // W
        osdDrawDecimal2(FONT_8PX_FIXED, abs(multiwiiData.GPS_homeLON), 9, 1, 7);




        osdSetCursor(3, 50 + 16 * 2);
        osdDrawCharacter('A', FONT_16PX_FIXED);
        osdDrawCharacter('l', FONT_16PX_FIXED);
        osdDrawCharacter('G', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_altitude, 5, 0, 5);  // altitude in 0.1m

        osdSetCursor(3, 50 + 16 * 3);
        osdDrawCharacter('S', FONT_16PX_FIXED);
        osdDrawCharacter('p', FONT_16PX_FIXED);
        osdDrawCharacter('G', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_speed /**360/100000*/ , 5, 0, 5);    // speed in sm/sec

        osdSetCursor(3, 50 + 16 * 4);
        osdDrawCharacter('u', FONT_16PX_FIXED);
        //osdDrawDecimal(1, multiwiiData.GPS_update, 3, 3, 3);
        //osdDrawDecimal(2, multiwiiData.GPS_update, 3, 3, 3);
        //osdDrawDecimal(3, multiwiiData.GPS_update, 3, 3, 3);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_update, 3, 3, 3);
        //osdDrawDecimal(FONT_8PX_FIXED, multiwiiData.GPS_update, 3, 3, 3);

        osdSetCursor(3, 50 + 16 * 5);
        osdDrawCharacter('L', FONT_16PX_FIXED);
        osdDrawCharacter('O', FONT_16PX_FIXED);
        osdDrawCharacter('S', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_distanceToHome, 5, 0, 5);    // meters?


        osdSetCursor(3, 50 + 16 * 6);
        osdDrawCharacter('H', FONT_16PX_FIXED);
        osdDrawCharacter('O', FONT_16PX_FIXED);
        osdDrawCharacter('D', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_directionToHome, 5, 0, 5);   // ??


/*
        osdSetCursor(0, osdData.Height - 2 * 16 - 1);
        osdDrawDecimal2(FONT_16PX_FIXED, sensorData.volts * 100, 5, 0, 2);
        osdDrawCharacter('V', FONT_16PX_FIXED);
        osdDrawDecimal2(FONT_16PX_FIXED, sensorData.amps, 5, 0, 1);
        osdDrawCharacter('A', FONT_16PX_FIXED);
*/

        osdSetCursor(0, osdData.Height - 1 * 16 - 1);
        osdDrawDecimal2(FONT_16PX_FIXED, 1234, 5, 0, 5);
        osdDrawCharacter('M', FONT_16PX_FIXED);
        osdDrawCharacter('a', FONT_16PX_FIXED);
        osdDrawCharacter('h', FONT_16PX_FIXED);

/*
        osdSetCursor(50, 30);
        osdDrawDecimal(2, sensorData.batteryData.voltage[0] * 100, 3, 1, 2);
        osdDrawCharacter('V', 2);
        osdSetCursor(50, 56);
        osdDrawDecimal(2, sensorData.volts * 100, 4, 1, 2);
        osdDrawCharacter('V', 2);
*/
    
        CoTickDelay(5);
    }
}

int main(void)
{
    int i = 0, j = 0, q = 0;
    int x = 0, y = 0, xdir = 1, ydir = 1;

    setup();
    CoInitOS();

    USB_Renumerate();
    USB_Interrupts_Config();
    USB_Init();

    osdInit();
    sensorsInit();
    gpsInit();
    multiwiiInit();

    CoCreateTask(blinkTask, 0, 30, &blinkStack[BLINK_STACK_SIZE - 1], BLINK_STACK_SIZE);
    CoCreateTask(mainTask, 0, 10, &mainStack[MAIN_STACK_SIZE - 1], MAIN_STACK_SIZE);
    // minCycles = 99999999;
    CoStartOS();

}

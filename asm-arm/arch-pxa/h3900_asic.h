/*
 *
 * Definitions for H3900 Handheld Computer
 *
 * Copyright 2001 Compaq Computer Corporation.
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Author: Andrew Christian
 *
 */

#ifndef _INCLUDE_H3900_ASIC_H_ 
#define _INCLUDE_H3900_ASIC_H_

#include "h3900.h"



#define GPIO_H3900_ASIC         	GPIO_GPIO (1)
#define GPIO_H3900_AC_IN                GPIO_GPIO (12) 
#define GPIO_H3900_COM_DSR              GPIO_GPIO (13)
#define GPIO_H3900_MMC_INT              GPIO_GPIO (18)
#define GPIO_H3900_NOPT_IND             GPIO_GPIO (20)   
#define GPIO_H3900_OPT_BAT_FAULT        GPIO_GPIO (22)
#define GPIO_H3900_CLK_OUT              GPIO_GPIO (27)

/****************************************************/

#define IRQ_GPIO_H3900_ASIC             IRQ_GPIO1
#define IRQ_GPIO_H3900_AC_IN            IRQ_GPIO12
#define IRQ_GPIO_H3900_MMC_INT          IRQ_GPIO18
#define IRQ_GPIO_H3900_NOPT_IND         IRQ_GPIO20 /* almost same as OPT_DET */

/********************* H3900, ASIC #3 ********************/

#define _H3900_ASIC2_Base            (H3900_EGPIO_VIRT)
#define H3900_ASIC2_OFFSET(s,x,y)    \
    (*((volatile s *) (_H3900_ASIC2_Base + _H3900_ASIC2_ ## x ## _Base + _H3900_ASIC2_ ## x ## _ ## y)))
#define H3900_ASIC2_NOFFSET(s,x,n,y) \
    (*((volatile s *) (_H3900_ASIC2_Base + _H3900_ASIC2_ ## x ## _ ## n ## _Base + _H3900_ASIC2_ ## x ## _ ## y)))

#define _H3900_ASIC2_GPIO_Base                 0x0000
#define _H3900_ASIC2_GPIO_Direction            0x0000    /* R/W, 16 bits 1:input, 0:output */
#define _H3900_ASIC2_GPIO_InterruptType        0x0004    /* R/W, 12 bits 1:edge, 0:level          */
#define _H3900_ASIC2_GPIO_InterruptEdgeType    0x0008    /* R/W, 12 bits 1:rising, 0:falling */
#define _H3900_ASIC2_GPIO_InterruptLevelType   0x000C    /* R/W, 12 bits 1:high, 0:low  */
#define _H3900_ASIC2_GPIO_InterruptClear       0x0010    /* W,   12 bits */
#define _H3900_ASIC2_GPIO_InterruptFlag        0x0010    /* R,   12 bits - reads int status */
#define _H3900_ASIC2_GPIO_Data                 0x0014    /* R/W, 16 bits */
#define _H3900_ASIC2_GPIO_BattFaultOut         0x0018    /* R/W, 16 bit - sets level on batt fault */
#define _H3900_ASIC2_GPIO_InterruptEnable      0x001c    /* R/W, 12 bits 1:enable interrupt */
#define _H3900_ASIC2_GPIO_Alternate            0x003c    /* R/W, 12+1 bits - set alternate functions */

#define H3900_ASIC2_GPIODIR       H3900_ASIC2_OFFSET( u16, GPIO, Direction )
#define H3900_ASIC2_GPIINTTYPE    H3900_ASIC2_OFFSET( u16, GPIO, InterruptType )
#define H3900_ASIC2_GPIINTESEL    H3900_ASIC2_OFFSET( u16, GPIO, InterruptEdgeType )
#define H3900_ASIC2_GPIINTALSEL   H3900_ASIC2_OFFSET( u16, GPIO, InterruptLevelType )
#define H3900_ASIC2_GPIINTCLR     H3900_ASIC2_OFFSET( u16, GPIO, InterruptClear )
#define H3900_ASIC2_GPIINTFLAG    H3900_ASIC2_OFFSET( u16, GPIO, InterruptFlag )
#define H3900_ASIC2_GPIOPIOD      H3900_ASIC2_OFFSET( u16, GPIO, Data )
#define H3900_ASIC2_GPOBFSTAT     H3900_ASIC2_OFFSET( u16, GPIO, BattFaultOut )
#define H3900_ASIC2_GPIINTSTAT    H3900_ASIC2_OFFSET( u16, GPIO, InterruptEnable )
#define H3900_ASIC2_GPIOALT       H3900_ASIC2_OFFSET( u16, GPIO, Alternate )

#define GPIO2_IN_Y1_N                       (1 << 0)   /* Output: Touchscreen Y1 */
#define GPIO2_IN_X0                         (1 << 1)   /* Output: Touchscreen X0 */
#define GPIO2_IN_Y0                         (1 << 2)   /* Output: Touchscreen Y0 */
#define GPIO2_IN_X1_N                       (1 << 3)   /* Output: Touchscreen X1 */
#define GPIO2_BT_RST                        (1 << 4)   /* Output: Bluetooth reset */
#define GPIO2_PEN_IRQ                       (1 << 5)   /* Input : Pen down        */
#define GPIO2_SD_DETECT                     (1 << 6)   /* Input : SD detect */
#define GPIO2_EAR_IN_N                      (1 << 7)   /* Input : Audio jack plug inserted */
#define GPIO2_OPT_PCM_RESET                 (1 << 8)   /* Output: Card reset (pin 2 on expansion) */
#define GPIO2_OPT_RESET                     (1 << 9)   /* Output: Option pack reset (pin 8 on expansion) */
#define GPIO2_USB_DETECT_N                  (1 << 10)  /* Input : */
#define GPIO2_SD_CON_SLT                    (1 << 11)  /* Input : */
#define GPIO2_OPT_ON                        (1 << 12)  /* Output: Option jacket power */
#define GPIO2_OPT_ON_NVRAM                  (1 << 13)  /* Output: Option jacket NVRAM power */

#define _H3900_ASIC2_KPIO_Base                 0x0200
#define _H3900_ASIC2_KPIO_Direction            0x0000    /* R/W, 12 bits 1:input, 0:output */
#define _H3900_ASIC2_KPIO_InterruptType        0x0004    /* R/W, 12 bits 1:edge, 0:level          */
#define _H3900_ASIC2_KPIO_InterruptEdgeType    0x0008    /* R/W, 12 bits 1:rising, 0:falling */
#define _H3900_ASIC2_KPIO_InterruptLevelType   0x000C    /* R/W, 12 bits 1:high, 0:low  */
#define _H3900_ASIC2_KPIO_InterruptClear       0x0010    /* W,   20 bits - 8 special */
#define _H3900_ASIC2_KPIO_InterruptFlag        0x0010    /* R,   20 bits - 8 special - reads int status */
#define _H3900_ASIC2_KPIO_Data                 0x0014    /* R/W, 16 bits */
#define _H3900_ASIC2_KPIO_BattFaultOut         0x0018    /* R/W, 16 bit - sets level on batt fault */
#define _H3900_ASIC2_KPIO_InterruptEnable      0x001c    /* R/W, 20 bits - 8 special (DON'T TRY TO READ!) */
#define _H3900_ASIC2_KPIO_Alternate            0x003c    /* R/W, 6 bits */

#define H3900_ASIC2_KPIODIR       H3900_ASIC2_OFFSET( u16, KPIO, Direction )
#define H3900_ASIC2_KPIINTTYPE    H3900_ASIC2_OFFSET( u16, KPIO, InterruptType )
#define H3900_ASIC2_KPIINTESEL    H3900_ASIC2_OFFSET( u16, KPIO, InterruptEdgeType )
#define H3900_ASIC2_KPIINTALSEL   H3900_ASIC2_OFFSET( u16, KPIO, InterruptLevelType )
#define H3900_ASIC2_KPIINTCLR     H3900_ASIC2_OFFSET( u32, KPIO, InterruptClear )
#define H3900_ASIC2_KPIINTFLAG    H3900_ASIC2_OFFSET( u32, KPIO, InterruptFlag )
#define H3900_ASIC2_KPIOPIOD      H3900_ASIC2_OFFSET( u16, KPIO, Data )
#define H3900_ASIC2_KPOBFSTAT     H3900_ASIC2_OFFSET( u16, KPIO, BattFaultOut )
#define H3900_ASIC2_KPIINTSTAT    H3900_ASIC2_OFFSET( u32, KPIO, InterruptEnable )
#define H3900_ASIC2_KPIOALT       H3900_ASIC2_OFFSET( u16, KPIO, Alternate )

#define KPIO_SPI_INT          (1 << 16)
#define KPIO_OWM_INT          (1 << 17)
#define KPIO_ADC_INT          (1 << 18)
#define KPIO_UART_0_INT       (1 << 19)
#define KPIO_UART_1_INT       (1 << 20)
#define KPIO_TIMER_0_INT      (1 << 21)
#define KPIO_TIMER_1_INT      (1 << 22)
#define KPIO_TIMER_2_INT      (1 << 23)

#define KPIO_RECORD_BTN_N     (1 << 0)   /* Record button */
#define KPIO_KEY_5W1_N        (1 << 1)   /* Keypad */
#define KPIO_KEY_5W2_N        (1 << 2)   /* */
#define KPIO_KEY_5W3_N        (1 << 3)   /* */
#define KPIO_KEY_5W4_N        (1 << 4)   /* */
#define KPIO_KEY_5W5_N        (1 << 5)   /* */
#define KPIO_KEY_LEFT_N       (1 << 6)   /* */
#define KPIO_KEY_RIGHT_N      (1 << 7)   /* */
#define KPIO_KEY_AP1_N        (1 << 8)   /* Old "Calendar" */
#define KPIO_KEY_AP2_N        (1 << 9)   /* Old "Schedule" */
#define KPIO_KEY_AP3_N        (1 << 10)  /* Old "Q"        */
#define KPIO_KEY_AP4_N        (1 << 11)  /* Old "Undo"     */
#define KPIO_KEY_ALL          0x0fff

/* Alternate KPIO functions (set by default) */
#define KPIO_ALT_KEY_5W1_N        (1 << 1)   /* Action key */
#define KPIO_ALT_KEY_5W2_N        (1 << 2)   /* J1 of keypad input */
#define KPIO_ALT_KEY_5W3_N        (1 << 3)   /* J2 of keypad input */
#define KPIO_ALT_KEY_5W4_N        (1 << 4)   /* J3 of keypad input */
#define KPIO_ALT_KEY_5W5_N        (1 << 5)   /* J4 of keypad input */
#define KPIO_ALT_KEY_ALL           0x003e

#define _H3900_ASIC2_SPI_Base                  0x0400
#define _H3900_ASIC2_SPI_Control               0x0000    /* R/W 8 bits */
#define _H3900_ASIC2_SPI_Data                  0x0004    /* R/W 8 bits */
#define _H3900_ASIC2_SPI_ChipSelectDisabled    0x0008    /* W   8 bits */

#define H3900_ASIC2_SPI_Control             H3900_ASIC2_OFFSET( u8, SPI, Control )
#define H3900_ASIC2_SPI_Data                H3900_ASIC2_OFFSET( u8, SPI, Data )
#define H3900_ASIC2_SPI_ChipSelectDisabled  H3900_ASIC2_OFFSET( u8, SPI, ChipSelectDisabled )

#define SPI_CONTROL_SPR(clk)      ((clk) & 0x0f)  /* Clock rate: valid from 0000 (8kHz) to 1000 (2.048 MHz) */
#define SPI_CONTROL_SPE           (1 << 4)   /* SPI Enable (1:enable, 0:disable) */
#define SPI_CONTROL_SPIE          (1 << 5)   /* SPI Interrupt enable (1:enable, 0:disable) */
#define SPI_CONTROL_SEL           (1 << 6)   /* Chip select: 1:SPI_CS1 enable, 0:SPI_CS0 enable */
#define SPI_CONTROL_SEL_CS0       (0 << 6)   /* Set CS0 low */
#define SPI_CONTROL_SEL_CS1       (1 << 6)   /* Set CS0 high */
#define SPI_CONTROL_CPOL          (1 << 7)   /* Clock polarity, 1:SCK high when idle */

#define _H3900_ASIC2_PWM_0_Base                0x0600
#define _H3900_ASIC2_PWM_1_Base                0x0700
#define _H3900_ASIC2_PWM_TimeBase              0x0000    /* R/W 6 bits */
#define _H3900_ASIC2_PWM_PeriodTime            0x0004    /* R/W 12 bits */
#define _H3900_ASIC2_PWM_DutyTime              0x0008    /* R/W 12 bits */

#define H3900_ASIC2_PWM_0_TimeBase          H3900_ASIC2_NOFFSET(  u8, PWM, 0, TimeBase )
#define H3900_ASIC2_PWM_0_PeriodTime        H3900_ASIC2_NOFFSET( u16, PWM, 0, PeriodTime )
#define H3900_ASIC2_PWM_0_DutyTime          H3900_ASIC2_NOFFSET( u16, PWM, 0, DutyTime )

#define H3900_ASIC2_PWM_1_TimeBase          H3900_ASIC2_NOFFSET(  u8, PWM, 1, TimeBase )
#define H3900_ASIC2_PWM_1_PeriodTime        H3900_ASIC2_NOFFSET( u16, PWM, 1, PeriodTime )
#define H3900_ASIC2_PWM_1_DutyTime          H3900_ASIC2_NOFFSET( u16, PWM, 1, DutyTime )

#define PWM_TIMEBASE_VALUE(x)    ((x)&0xf)   /* Low 4 bits sets time base, max = 8 */
#define PWM_TIMEBASE_ENABLE     ( 1 << 4 )   /* Enable clock */
#define PWM_TIMEBASE_CLEAR      ( 1 << 5 )   /* Clear the PWM */

#define _H3900_ASIC2_LED_0_Base                0x0800
#define _H3900_ASIC2_LED_1_Base                0x0880
#define _H3900_ASIC2_LED_2_Base                0x0900
#define _H3900_ASIC2_LED_TimeBase              0x0000    /* R/W  7 bits */
#define _H3900_ASIC2_LED_PeriodTime            0x0004    /* R/W 12 bits */
#define _H3900_ASIC2_LED_DutyTime              0x0008    /* R/W 12 bits */
#define _H3900_ASIC2_LED_AutoStopCount         0x000c    /* R/W 16 bits */

#define H3900_ASIC2_LED_0_TimeBase          H3900_ASIC2_NOFFSET(  u8, LED, 0, TimeBase )
#define H3900_ASIC2_LED_0_PeriodTime        H3900_ASIC2_NOFFSET( u16, LED, 0, PeriodTime )
#define H3900_ASIC2_LED_0_DutyTime          H3900_ASIC2_NOFFSET( u16, LED, 0, DutyTime )
#define H3900_ASIC2_LED_0_AutoStopClock     H3900_ASIC2_NOFFSET( u16, LED, 0, AutoStopClock )

#define H3900_ASIC2_LED_1_TimeBase          H3900_ASIC2_NOFFSET(  u8, LED, 1, TimeBase )
#define H3900_ASIC2_LED_1_PeriodTime        H3900_ASIC2_NOFFSET( u16, LED, 1, PeriodTime )
#define H3900_ASIC2_LED_1_DutyTime          H3900_ASIC2_NOFFSET( u16, LED, 1, DutyTime )
#define H3900_ASIC2_LED_1_AutoStopClock     H3900_ASIC2_NOFFSET( u16, LED, 1, AutoStopClock )

#define H3900_ASIC2_LED_2_TimeBase          H3900_ASIC2_NOFFSET(  u8, LED, 2, TimeBase )
#define H3900_ASIC2_LED_2_PeriodTime        H3900_ASIC2_NOFFSET( u16, LED, 2, PeriodTime )
#define H3900_ASIC2_LED_2_DutyTime          H3900_ASIC2_NOFFSET( u16, LED, 2, DutyTime )
#define H3900_ASIC2_LED_2_AutoStopClock     H3900_ASIC2_NOFFSET( u16, LED, 2, AutoStopClock )

#define LEDTBS_MASK            0x0f    /* Low 4 bits sets time base, max = 13 */
#define LEDTBS_BLINK     ( 1 << 4 )    /* Enable blinking */
#define LEDTBS_AUTOSTOP  ( 1 << 5 ) 
#define LEDTBS_ALWAYS    ( 1 << 6 )    /* Enable blink always */

#define _H3900_ASIC2_UART_0_Base               0x0A00
#define _H3900_ASIC2_UART_1_Base               0x0C00
#define _H3900_ASIC2_UART_Receive              0x0000    /* R    8 bits */
#define _H3900_ASIC2_UART_Transmit             0x0000    /*   W  8 bits */
#define _H3900_ASIC2_UART_IntEnable            0x0004    /* R/W  8 bits */
#define _H3900_ASIC2_UART_IntVerify            0x0008    /* R/W  8 bits */
#define _H3900_ASIC2_UART_FIFOControl          0x000c    /* R/W  8 bits */
#define _H3900_ASIC2_UART_LineControl          0x0010    /* R/W  8 bits */
#define _H3900_ASIC2_UART_ModemStatus          0x0014    /* R/W  8 bits */
#define _H3900_ASIC2_UART_LineStatus           0x0018    /* R/W  8 bits */
#define _H3900_ASIC2_UART_ScratchPad           0x001c    /* R/W  8 bits */
#define _H3900_ASIC2_UART_DivisorLatchL        0x0020    /* R/W  8 bits */
#define _H3900_ASIC2_UART_DivisorLatchH        0x0024    /* R/W  8 bits */

#define H3900_ASIC2_UART_0_Receive          H3900_ASIC2_NOFFSET(  u8, UART, 0, Receive )
#define H3900_ASIC2_UART_0_Transmit         H3900_ASIC2_NOFFSET(  u8, UART, 0, Transmit )
#define H3900_ASIC2_UART_0_IntEnable        H3900_ASIC2_NOFFSET(  u8, UART, 0, IntEnable )
#define H3900_ASIC2_UART_0_IntVerify        H3900_ASIC2_NOFFSET(  u8, UART, 0, IntVerify )
#define H3900_ASIC2_UART_0_FIFOControl      H3900_ASIC2_NOFFSET(  u8, UART, 0, FIFOControl )
#define H3900_ASIC2_UART_0_LineControl      H3900_ASIC2_NOFFSET(  u8, UART, 0, LineControl )
#define H3900_ASIC2_UART_0_ModemStatus      H3900_ASIC2_NOFFSET(  u8, UART, 0, ModemStatus )
#define H3900_ASIC2_UART_0_LineStatus       H3900_ASIC2_NOFFSET(  u8, UART, 0, LineStatus )
#define H3900_ASIC2_UART_0_ScratchPad       H3900_ASIC2_NOFFSET(  u8, UART, 0, ScratchPad )
#define H3900_ASIC2_UART_0_DivisorLatchL    H3900_ASIC2_NOFFSET(  u8, UART, 0, DivisorLatchL )
#define H3900_ASIC2_UART_0_DivisorLatchH    H3900_ASIC2_NOFFSET(  u8, UART, 0, DivisorLatchH )

#define H3900_ASIC2_UART_1_Receive          H3900_ASIC2_NOFFSET(  u8, UART, 1, Receive )
#define H3900_ASIC2_UART_1_Transmit         H3900_ASIC2_NOFFSET(  u8, UART, 1, Transmit )
#define H3900_ASIC2_UART_1_IntEnable        H3900_ASIC2_NOFFSET(  u8, UART, 1, IntEnable )
#define H3900_ASIC2_UART_1_IntVerify        H3900_ASIC2_NOFFSET(  u8, UART, 1, IntVerify )
#define H3900_ASIC2_UART_1_FIFOControl      H3900_ASIC2_NOFFSET(  u8, UART, 1, FIFOControl )
#define H3900_ASIC2_UART_1_LineControl      H3900_ASIC2_NOFFSET(  u8, UART, 1, LineControl )
#define H3900_ASIC2_UART_1_ModemStatus      H3900_ASIC2_NOFFSET(  u8, UART, 1, ModemStatus )
#define H3900_ASIC2_UART_1_LineStatus       H3900_ASIC2_NOFFSET(  u8, UART, 1, LineStatus )
#define H3900_ASIC2_UART_1_ScratchPad       H3900_ASIC2_NOFFSET(  u8, UART, 1, ScratchPad )
#define H3900_ASIC2_UART_1_DivisorLatchL    H3900_ASIC2_NOFFSET(  u8, UART, 1, DivisorLatchL )
#define H3900_ASIC2_UART_1_DivisorLatchH    H3900_ASIC2_NOFFSET(  u8, UART, 1, DivisorLatchH )

#define _H3900_ASIC2_TIMER_Base                0x0E00    /* 8254-compatible timers */
#define _H3900_ASIC2_TIMER_Counter0            0x0000    /* R/W  8 bits */
#define _H3900_ASIC2_TIMER_Counter1            0x0004    /* R/W  8 bits */
#define _H3900_ASIC2_TIMER_Counter2            0x0008    /* R/W  8 bits */
#define _H3900_ASIC2_TIMER_Control             0x000a    /*   W  8 bits */
#define _H3900_ASIC2_TIMER_Command             0x0010    /* R/W  8 bits */

#define H3900_ASIC2_TIMER_Counter0          H3900_ASIC2_OFFSET( u8, TIMER, Counter0 )
#define H3900_ASIC2_TIMER_Counter1          H3900_ASIC2_OFFSET( u8, TIMER, Counter1 )
#define H3900_ASIC2_TIMER_Counter2          H3900_ASIC2_OFFSET( u8, TIMER, Counter2 )
#define H3900_ASIC2_TIMER_Control           H3900_ASIC2_OFFSET( u8, TIMER, Control )
#define H3900_ASIC2_TIMER_Command           H3900_ASIC2_OFFSET( u8, TIMER, Command )

/* These defines are likely incorrect - in particular, TIMER_CNTL_MODE_4 might
   need to be 0x04 */
#define TIMER_CNTL_SELECT(x)               (((x)&0x3)<<6)   /* Select counter */
#define TIMER_CNTL_RW(x)                   (((x)&0x3)<<4)   /* Read/write mode */
#define TIMER_CNTL_RW_LATCH                TIMER_CNTL_RW(0)
#define TIMER_CNTL_RW_LSB_MSB              TIMER_CNTL_RW(3) /* LSB first, then MSB */
#define TIMER_CNTL_MODE(x)                 (((x)&0x7)<<1)   /* Mode */
#define TIMER_CNTL_MODE_0                  TIMER_CNTL_MODE(0)  /* Supported for 0 & 1 */
#define TIMER_CNTL_MODE_2                  TIMER_CNTL_MODE(2)  /* Supported for all timers */
#define TIMER_CNTL_MODE_4                  TIMER_CNTL_MODE(4)  /* Supported for all timers */
#define TIMER_CNTL_BCD                     ( 1 << 0 )       /* 1=Use BCD counter, 4 decades */

#define TIMER_CMD_GAT_0                    ( 1 << 0 )    /* Gate enable, counter 0 */
#define TIMER_CMD_GAT_1                    ( 1 << 1 )    /* Gate enable, counter 1 */
#define TIMER_CMD_GAT_2                    ( 1 << 2 )    /* Gate enable, counter 2 */
#define TIMER_CMD_CLK_0                    ( 1 << 3 )    /* Clock enable, counter 0 */
#define TIMER_CMD_CLK_1                    ( 1 << 4 )    /* Clock enable, counter 1 */
#define TIMER_CMD_CLK_2                    ( 1 << 5 )    /* Clock enable, counter 2 */
#define TIMER_CMD_MODE_0                   ( 1 << 6 )    /* Mode 0 enable, counter 0 */
#define TIMER_CMD_MODE_1                   ( 1 << 7 )    /* Mode 0 enable, counter 1 */

#define _H3900_ASIC2_CLOCK_Base                0x1000
#define _H3900_ASIC2_CLOCK_Enable              0x0000    /* R/W  18 bits */

#define H3900_ASIC2_CLOCK_Enable            H3900_ASIC2_OFFSET( u32, CLOCK, Enable )

#define ASIC2_CLOCK_AUDIO_1                0x01    /* Enable 4.1 MHz clock for 8Khz and 4khz sample rate */
#define ASIC2_CLOCK_AUDIO_2                0x02    /* Enable 12.3 MHz clock for 48Khz and 32khz sample rate */
#define ASIC2_CLOCK_AUDIO_3                0x04    /* Enable 5.6 MHz clock for 11 kHZ sample rate */
#define ASIC2_CLOCK_AUDIO_4                0x08    /* Enable 11.289 MHz clock for 44 and 22 kHz sample rate */
#define ASIC2_CLOCK_AUDIO_MASK             0x0f    /* Bottom four bits are for audio */
#define ASIC2_CLOCK_ADC              ( 1 << 4 )    /* 1.024 MHz clock to ADC (CX4) */
#define ASIC2_CLOCK_SPI              ( 1 << 5 )    /* 4.096 MHz clock to SPI (CX5) */
#define ASIC2_CLOCK_OWM              ( 1 << 6 )    /* 4.096 MHz clock to OWM (CX6) */
#define ASIC2_CLOCK_PWM              ( 1 << 7 )    /* 2.048 MHz clock to PWM (CX7) */
#define ASIC2_CLOCK_UART_1           ( 1 << 8 )    /* 24.576 MHz clock to UART1 (turn off bit 16) (CX8) */
#define ASIC2_CLOCK_UART_0           ( 1 << 9 )    /* 24.576 MHz clock to UART0 (turn off bit 17) (CX9) */
#define ASIC2_CLOCK_SD_1             ( 1 << 10 )   /* 16.934 MHz to SD */
#define ASIC2_CLOCK_SD_2             ( 2 << 10 )   /* 24.576 MHz to SD */
#define ASIC2_CLOCK_SD_3             ( 3 << 10 )   /* 33.869 MHz to SD */
#define ASIC2_CLOCK_SD_4             ( 4 << 10 )   /* 49.152 MHz to SD */
#define ASIC2_CLOCK_SD_MASK              0x1c00    /* Bits 10 through 12 are for SD */
#define ASIC2_CLOCK_EX0              ( 1 << 13 )   /* Enable 32.768 kHz (LED,Timer,Interrupt) */
#define ASIC2_CLOCK_EX1              ( 1 << 14 )   /* Enable 24.576 MHz (ADC,PCM,SPI,PWM,UART,SD,Audio) */
#define ASIC2_CLOCK_EX2              ( 1 << 15 )   /* Enable 33.869 MHz (SD,Audio) */
#define ASIC2_CLOCK_SLOW_UART_1      ( 1 << 16 )   /* Enable 3.686 MHz to UART1 (turn off bit 8) */
#define ASIC2_CLOCK_SLOW_UART_0      ( 1 << 17 )   /* Enable 3.686 MHz to UART0 (turn off bit 9) */

#define _H3900_ASIC2_ADC_Base                  0x1200
#define _H3900_ASIC2_ADC_Multiplexer           0x0000    /* R/W 4 bits - low 3 bits set channel */
#define _H3900_ASIC2_ADC_ControlStatus         0x0004    /* R/W 8 bits */
#define _H3900_ASIC2_ADC_Data                  0x0008    /* R   10 bits */

#define H3900_ASIC2_ADMUX        H3900_ASIC2_OFFSET( u32, ADC, Multiplexer )
#define H3900_ASIC2_ADCSR        H3900_ASIC2_OFFSET(  u8, ADC, ControlStatus )
#define H3900_ASIC2_ADCDR        H3900_ASIC2_OFFSET( u16, ADC, Data )

#define ASIC2_ADMUX(x)                     ((x)&0x07)    /* Low 3 bits sets channel.  max = 4 */
#define ASIC2_ADMUX_MASK                         0x07
#define ASIC2_ADMUX_0_LIGHTSENSOR      ASIC2_ADMUX(0)   
#define ASIC2_ADMUX_1_IMIN             ASIC2_ADMUX(1)   
#define ASIC2_ADMUX_2_VS_MBAT          ASIC2_ADMUX(2)   
#define ASIC2_ADMUX_3_TP_X0            ASIC2_ADMUX(3)    /* Touchpanel X0 */
#define ASIC2_ADMUX_4_TP_Y1            ASIC2_ADMUX(4)    /* Touchpanel Y1 */
#define ASIC2_ADMUX_CLKEN                  ( 1 << 3 )    /* Enable clock */
 
#define ASIC2_ADCSR_ADPS(x)                ((x)&0x0f)    /* Low 4 bits sets prescale, max = 8 */
#define ASIC2_ADCSR_FREE_RUN               ( 1 << 4 )
#define ASIC2_ADCSR_INT_ENABLE             ( 1 << 5 )
#define ASIC2_ADCSR_START                  ( 1 << 6 )    /* Set to start conversion.  Goes to 0 when done */
#define ASIC2_ADCSR_ENABLE                 ( 1 << 7 )    /* 1:power up ADC, 0:power down */


#define _H3900_ASIC2_INTR_Base                 0x1600
#define _H3900_ASIC2_INTR_MaskAndFlag          0x0000    /* R/(W) 8bits */
#define _H3900_ASIC2_INTR_ClockPrescale        0x0004    /* R/(W) 5bits */
#define _H3900_ASIC2_INTR_TimerSet             0x0008    /* R/(W) 8bits */

#define H3900_ASIC2_INTR_MaskAndFlag      H3900_ASIC2_OFFSET(  u8, INTR, MaskAndFlag )
#define H3900_ASIC2_INTR_ClockPrescale    H3900_ASIC2_OFFSET(  u8, INTR, ClockPrescale )
#define H3900_ASIC2_INTR_TimerSet         H3900_ASIC2_OFFSET( u16, INTR, TimerSet )

#define ASIC2_INTMASK_GLOBAL               ( 1 << 0 )    /* Global interrupt mask */
//#define ASIC2_INTR_POWER_ON_RESET        ( 1 << 1 )    /* 01: Power on reset (bits 1 & 2 ) */
//#define ASIC2_INTR_EXTERNAL_RESET        ( 2 << 1 )    /* 10: External reset (bits 1 & 2 ) */
#define ASIC2_INTMASK_UART_0               ( 1 << 4 )    
#define ASIC2_INTMASK_UART_1               ( 1 << 5 )    
#define ASIC2_INTMASK_TIMER                ( 1 << 6 )    
#define ASIC2_INTMASK_OWM                  ( 1 << 7 )    

#define ASIC2_INTCPS_CPS(x)                ((x)&0x0f)    /* 4 bits, max 14 */
#define ASIC2_INTCPS_SET                   ( 1 << 4 )    /* Time base enable */

#define _H3900_ASIC2_OWM_Base                  0x1800
#define _H3900_ASIC2_OWM_Command               0x0000    /* R/W 4 bits command register */
#define _H3900_ASIC2_OWM_Data                  0x0004    /* R/W 8 bits, transmit / receive buffer */
#define _H3900_ASIC2_OWM_Interrupt             0x0008    /* R/W Command register */
#define _H3900_ASIC2_OWM_InterruptEnable       0x000c    /* R/W Command register */
#define _H3900_ASIC2_OWM_ClockDivisor          0x0010    /* R/W 5 bits of divisor and pre-scale */

#define H3900_ASIC2_OWM_Command            H3900_ASIC2_OFFSET( u8, OWM, Command )
#define H3900_ASIC2_OWM_Data               H3900_ASIC2_OFFSET( u8, OWM, Data )
#define H3900_ASIC2_OWM_Interrupt          H3900_ASIC2_OFFSET( u8, OWM, Interrupt )
#define H3900_ASIC2_OWM_InterruptEnable    H3900_ASIC2_OFFSET( u8, OWM, InterruptEnable )
#define H3900_ASIC2_OWM_ClockDivisor       H3900_ASIC2_OFFSET( u8, OWM, ClockDivisor )

#define OWM_CMD_ONE_WIRE_RESET ( 1 << 0 )    /* Set to force reset on 1-wire bus */
#define OWM_CMD_SRA            ( 1 << 1 )    /* Set to switch to Search ROM accelerator mode */
#define OWM_CMD_DQ_OUTPUT      ( 1 << 2 )    /* Write only - forces bus low */
#define OWM_CMD_DQ_INPUT       ( 1 << 3 )    /* Read only - reflects state of bus */

#define OWM_INT_PD             ( 1 << 0 )    /* Presence detect */
#define OWM_INT_PDR            ( 1 << 1 )    /* Presence detect result */
#define OWM_INT_TBE            ( 1 << 2 )    /* Transmit buffer empty */
#define OWM_INT_TEMT           ( 1 << 3 )    /* Transmit shift register empty */
#define OWM_INT_RBF            ( 1 << 4 )    /* Receive buffer full */

#define OWM_INTEN_EPD          ( 1 << 0 )    /* Enable presence detect interrupt */
#define OWM_INTEN_IAS          ( 1 << 1 )    /* INTR active state */
#define OWM_INTEN_ETBE         ( 1 << 2 )    /* Enable transmit buffer empty interrupt */
#define OWM_INTEN_ETMT         ( 1 << 3 )    /* Enable transmit shift register empty interrupt */
#define OWM_INTEN_ERBF         ( 1 << 4 )    /* Enable receive buffer full interrupt */

#define _H3900_ASIC2_FlashCtl_Base             0x1A00

/****************************************************/
/* H3900, ASIC #3 -- SDIO and GPIO functions
 */

/********* GPIO **********/

#define _H3900_ASIC3_Base	       0x14800000
#define _H3900_ASIC3_GPIOA_Base        0x0000
#define _H3900_ASIC3_GPIOB_Base        0x0100

#define _H3900_ASIC3_GPIO_Mask          0x00    /* R/W 0:don't mask, 1:mask interrupt */
#define _H3900_ASIC3_GPIO_Direction     0x04    /* R/W 0:input, 1:output              */
#define _H3900_ASIC3_GPIO_Out           0x08    /* R/W 0:output low, 1:output high    */
#define _H3900_ASIC3_GPIO_TriggerType   0x0c    /* R/W 0:level, 1:edge                */
#if 0
#define _H3900_ASIC3_GPIO_EdgeTrigger   0x70    /* R/W 0:falling, 1:rising            */
#define _H3900_ASIC3_GPIO_LevelTrigger  0x74    /* R/W 0:low, 1:high level detect     */
#define _H3900_ASIC3_GPIO_LevelStatus   0x78    /* R/W 0:none, 1:detect               */
#define _H3900_ASIC3_GPIO_EdgeStatus    0x7c    /* R/W 0:none, 1:detect               */
#define _H3900_ASIC3_GPIO_State         0x80    /* R   See masks below  (default 0)         */
#define _H3900_ASIC3_GPIO_Reset         0x84    /* R/W See masks below  (default 0x04)      */
#define _H3900_ASIC3_GPIO_SleepMask     0x88    /* R/W 0:don't mask, 1:mask trigger in sleep mode  */
#define _H3900_ASIC3_GPIO_SleepDir      0x8c    /* R/W direction 0:input, 1:ouput in sleep mode    */
#define _H3900_ASIC3_GPIO_SleepOut      0x90    /* R/W level 0:low, 1:high in sleep mode           */
#define _H3900_ASIC3_GPIO_Status        0x94    /* R   Pin status                                  */
#define _H3900_ASIC3_GPIO_BattFaultDir  0x98    /* R/W direction 0:input, 1:output in batt_fault   */
#define _H3900_ASIC3_GPIO_BattFaultOut  0x9c    /* R/W level 0:low, 1:high in batt_fault           */
#endif

#define H3900_ASIC3_GPIO_STATE_MASK            (1 << 0)
#define H3900_ASIC3_GPIO_STATE_DIRECTION       (1 << 1)
#define H3900_ASIC3_GPIO_STATE_OUT             (1 << 2)
#define H3900_ASIC3_GPIO_STATE_TRIGGER_TYPE    (1 << 3)
#define H3900_ASIC3_GPIO_STATE_EDGE_TRIGGER    (1 << 4)
#define H3900_ASIC3_GPIO_STATE_LEVEL_TRIGGER   (1 << 5)

#define H3900_ASIC3_GPIO_RESET_SOFTWARE        (1 << 0)
#define H3900_ASIC3_GPIO_RESET_AUTO_SLEEP      (1 << 1)
#define H3900_ASIC3_GPIO_RESET_FIRST_PWR_ON    (1 << 2)

/* These are all outputs */
#define GPIOB_IR_ON_N          (1 << 0)   /* Apply power to the IR Module */
#define GPIOB_LCD_9V_ON        (1 << 1)   /* Enables LCD_9V */
#define GPIOB_RS232_ON         (1 << 2)   /* Turn on power to the RS232 chip ? */
#define GPIOB_LCD_NV_ON        (1 << 3)   
#define GPIOB_CH_TIMER         (1 << 4)   /* Charger */
#define GPIOB_LCD_5V_ON        (1 << 5)   /* Enables LCD_5V */
#define GPIOB_LCD_ON           (1 << 6)   /* Enables LCD_3V */
#define GPIOB_LCD_PCI          (1 << 7)   /* Connects to PDWN on LCD controller */
#define GPIOB_TP_170           (1 << 8)   /* TP170 */
#define GPIOB_CIR_CTL_PWR_ON   (1 << 9)   
#define GPIOB_AUD_RESET        (1 << 10)  
#define GPIOB_BT_PWR_ON        (1 << 11)  /* Bluetooth power on */
#define GPIOB_SPK_ON           (1 << 12)  /* Built-in speaker on */
#define GPIOB_FL_ON            (1 << 13)  
#define GPIOB_AUD_PWR_ON       (1 << 14)  /* All audio power */
#define GPIOB_SOFT_RESET       (1 << 15)  



#endif /* _INCLUDE_H3900_GPIO_H_ */

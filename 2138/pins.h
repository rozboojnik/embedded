/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    pins.h
 *
 * Description:
 *    Defines for used hardware pins.
 *
 *****************************************************************************/
#ifndef _PINS_H_
#define _PINS_H_

#define BT_RST        0x04000000
#define LCD_RST       0x02000000
#define BUZZER_PIN    0x00002000
#define BACKLIGHT_PIN 0x00001000
#define LED_GREEN_PIN 0x10000000
#define LED_RED_PIN   0x40000000

#define LCD_CS_V1_0   0x01000000
#define LCD_CS_V1_1   0x00008000
#define LCD_CLK       0x00000010
#define LCD_MOSI      0x00000040


#define KEYPIN_CENTER 0x00004000
#define KEYPIN_UP     0x00010000
#define KEYPIN_DOWN   0x00800000
#define KEYPIN_LEFT   0x00008000 
#define KEYPIN_RIGHT  0x00400000

#endif

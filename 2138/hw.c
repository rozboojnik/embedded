/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    hw.c
 *
 * Description:
 *    Implements hardware specific routines
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include <lpc2xxx.h>

#include "hw.h"
#include "key.h"
#include "pins.h"
#include "eeprom.h"

/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/


/*****************************************************************************
 * Global variables
 ****************************************************************************/
tBool ver1_0;
tBool ver1_1;

/*****************************************************************************
 * Local variables
 ****************************************************************************/
static tU8 greenLedShadow;
static tU8 btResetShadow;

/*****************************************************************************
 * Local prototypes
 ****************************************************************************/


/*****************************************************************************
 *
 * Description:
 *    Initialize the io-pins and find out if HW is ver 1.0 or ver 1.1
 *
 ****************************************************************************/
void
immediateIoInit(void)
{
  tU8 initCommand[] = {0x12, 0x97, 0x80, 0x00, 0x40, 0x00, 0x14, 0x00, 0x00};
  //                                                         04 = LCD_RST# low
  //                                                         10 = BT_RST# low

  //make all key signals as inputs
  IODIR &= ~(KEYPIN_CENTER | KEYPIN_UP | KEYPIN_DOWN | KEYPIN_LEFT | KEYPIN_RIGHT);

  IODIR |= BUZZER_PIN;
  IOSET  = BUZZER_PIN;

  IODIR |= BACKLIGHT_PIN;
  IOSET  = BACKLIGHT_PIN;

  //initialize PCA9532
  i2cInit();
  if (I2C_CODE_OK == pca9532(initCommand, sizeof(initCommand), NULL, 0))
  {
    ver1_0 = FALSE;
    ver1_1 = TRUE;
    greenLedShadow = FALSE;
    btResetShadow  = TRUE;
  }

  else
  {
    ver1_0 = TRUE;
    ver1_1 = FALSE;
    
    IODIR |= LCD_RST;
    IOCLR  = LCD_RST;
    
    IODIR |= BT_RST;
    IOCLR  = BT_RST;
    
    IODIR |= (LED_GREEN_PIN | LED_RED_PIN);
    IOSET  = (LED_GREEN_PIN | LED_RED_PIN);
  }
}


/*****************************************************************************
 *
 * Description:
 *    Reset the LCD (by strobing the LCD reset pin)
 *
 ****************************************************************************/
void
resetLCD(void)
{
  tU8 command[] = {0x07, 0x00};
  //                       04 = LCD_RST# low
  //                       00 = LCD_RST# high

  //check if ver 1.0 of HW
  if (TRUE == ver1_0)
  {
    IOCLR  = LCD_RST;
    osSleep(2);
    IOSET  = LCD_RST;
    osSleep(5);
  }
  
  //HW is ver 1.1
  else
  {
    command[1] = 0x04;
    pca9532(command, sizeof(command), NULL, 0);
    osSleep(2);
    command[1] = 0x00;
    pca9532(command, sizeof(command), NULL, 0);
    osSleep(5);
  }
}


/*****************************************************************************
 *
 * Description:
 *    Controls the reset signal to the BGB203-S06 Bluetooth moduls
 *
 ****************************************************************************/
void
resetBT(tBool resetFlag)
{
  tU8 command[] = {0x07, 0x00};
  //                       10 = BT_RST# low
  //                       00 = BT_RST# high
  //                       40 = Green LED on  (must handle this also)

  //check if ver 1.0 of HW
  if (TRUE == ver1_0)
  {
    if (TRUE == resetFlag)
      IOCLR  = BT_RST;
    else
      IOSET  = BT_RST;
  }
  
  //HW is ver 1.1
  else
  {
    btResetShadow = resetFlag;
    if (TRUE == resetFlag)
      command[1] = 0x10;
    if (TRUE == greenLedShadow)
      command[1] |= 0x40;
    pca9532(command, sizeof(command), NULL, 0);
  }
}


/*****************************************************************************
 *
 * Description:
 *    Controls the buzzer (on or off)
 *
 ****************************************************************************/
void
setBuzzer(tBool on)
{
  if (TRUE == on)
    IOCLR = BUZZER_PIN;
  else
    IOSET = BUZZER_PIN;
}


/*****************************************************************************
 *
 * Description:
 *    Controls the two LEDs
 *
 ****************************************************************************/
void
setLED(tU8 ledSelect, tBool ledState)
{
  tU8 commandLedGreen[] = {0x07, 0x00};
  //                               40 = LED on
  //                               00 = LED off
  //                               10 = BT_RST# low   (must handle this also)

  tU8 commandLedRed[] =   {0x08, 0x00};
  //                               01 = LED on
  //                               00 = LED off

  //check if ver 1.0 of HW
  if (TRUE == ver1_0)
  {
    if (LED_GREEN == ledSelect)
    {
      if (TRUE == ledState)
        IOCLR = LED_GREEN_PIN;
      else
        IOSET = LED_GREEN_PIN;
    }
    else if (LED_RED == ledSelect)
    {
      if (TRUE == ledState)
        IOCLR = LED_RED_PIN;
      else
        IOSET = LED_RED_PIN;
    }
  }

  //HW is ver 1.1
  else
  {
    if (LED_GREEN == ledSelect)
    {
      greenLedShadow = ledState;
      if (TRUE == ledState)
        commandLedGreen[1] = 0x40;
      if(TRUE == btResetShadow)
        commandLedGreen[1] |= 0x10;
      pca9532(commandLedGreen, sizeof(commandLedGreen), NULL, 0);
    }
    else if (LED_RED == ledSelect)
    {
      if (TRUE == ledState)
        commandLedRed[1] = 0x01;
      pca9532(commandLedRed, sizeof(commandLedRed), NULL, 0);
    }
  }
}


/*****************************************************************************
 *
 * Description:
 *    Get current state of joystick switch
 *
 ****************************************************************************/
tU8
getKeys(void)
{
  tU8 commandReadKeys[] = {0x00};
  tU8 readKeys = KEY_NOTHING;
  tU8 keySample;

  //check if ver 1.0 of HW
  if (TRUE == ver1_0)
  {
    if ((IOPIN & KEYPIN_CENTER) == 0) readKeys |= KEY_CENTER;
    if ((IOPIN & KEYPIN_UP) == 0)     readKeys |= KEY_UP;
    if ((IOPIN & KEYPIN_DOWN) == 0)   readKeys |= KEY_DOWN;
    if ((IOPIN & KEYPIN_LEFT) == 0)   readKeys |= KEY_LEFT;
    if ((IOPIN & KEYPIN_RIGHT) == 0)  readKeys |= KEY_RIGHT;
  }

  //HW is ver 1.1
  else
  {
    pca9532(commandReadKeys, sizeof(commandReadKeys), &keySample, 1);
    if ((keySample & 0x01) == 0) readKeys |= KEY_CENTER;
    if ((keySample & 0x04) == 0) readKeys |= KEY_UP;
    if ((keySample & 0x10) == 0) readKeys |= KEY_DOWN;
    if ((keySample & 0x02) == 0) readKeys |= KEY_LEFT;
    if ((keySample & 0x08) == 0) readKeys |= KEY_RIGHT;
  }

  return readKeys;
}


/*****************************************************************************
 *
 * Description:
 *    Select/deselect LCD controller (by controlling chip select signal)
 *
 ****************************************************************************/
void
selectLCD(tBool select)
{
  //check if ver 1.0 of HW
  if (TRUE == ver1_0)
  {
    if (TRUE == select)
      IOCLR = LCD_CS_V1_0;
    else
      IOSET = LCD_CS_V1_0;
  }

  //HW is ver 1.1
  else
  {
    if (TRUE == select)
      IOCLR = LCD_CS_V1_1;
    else
      IOSET = LCD_CS_V1_1;
  }
}


/*****************************************************************************
 *
 * Description:
 *    Send 9-bit data to LCD controller
 *
 ****************************************************************************/
void
sendToLCD(tU8 firstBit, tU8 data)
{
  //disable SPI
  IOCLR = LCD_CLK;
  PINSEL0 &= 0xffff00ff;
  
  if (1 == firstBit)
    IOSET = LCD_MOSI;   //set MOSI
  else
    IOCLR = LCD_MOSI;   //reset MOSI
  
  //Set clock high
  IOSET = LCD_CLK;
  
  //Set clock low
  IOCLR = LCD_CLK;
  
  /*
   * Enable SPI again
   */
  //initialize SPI interface
  SPI_SPCCR = 0x08;    
  SPI_SPCR  = 0x20;

  //connect SPI bus to IO-pins
  PINSEL0 |= 0x00005500;
  
  //send byte
  SPI_SPDR = data;
  while((SPI_SPSR & 0x80) == 0)
    ;
}


/*****************************************************************************
 *
 * Description:
 *    Initialize the SPI interface for the LCD controller
 *
 ****************************************************************************/
void
initSpiForLcd(void)
{
  //make SPI slave chip select an output and set signal high
  //check if ver 1.0 of HW
  if (TRUE == ver1_0)
  {
    IODIR |= (LCD_CS_V1_0 | LCD_CLK | LCD_MOSI);
  }

  //HW is ver 1.1
  else
  {
    IODIR |= (LCD_CS_V1_1 | LCD_CLK | LCD_MOSI);
  }
  
  //deselect controller
  selectLCD(FALSE);

  //connect SPI bus to IO-pins
  PINSEL0 |= 0x00005500;
  
  //initialize SPI interface
  SPI_SPCCR = 0x08;    
  SPI_SPCR  = 0x20;
}


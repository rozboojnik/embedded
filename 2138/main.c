/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    main.c
 *
 * Description:
 *    Main function of the "LPC2104 Color LCD Game Board with Bluetooth"
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include <printf_P.h>
#include <ea_init.h>
#include <stdlib.h>

#include "lcd.h"
#include "key.h"
#include "uart.h"
#include "exampleGame.h"
#include "snake.h"
#include "pong.h"
#include "bt.h"
#include "hw.h"
#include "chess/chess.h"
#include "startupDisplay.h"
#include "fire_0_100x40c.h"
#include "fire_1_100x40c.h"
#include "fire_2_100x40c.h"
#include "fire_3_100x40c.h"
#include "fire_4_100x40c.h"
#include "fire_5_100x40c.h"
#include "fire_6_100x40c.h"

/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/
#define PROC1_STACK_SIZE 800
#define INIT_STACK_SIZE  600


/*****************************************************************************
 * Global variables
 ****************************************************************************/
volatile tU32 ms;


/*****************************************************************************
 * Local variables
 ****************************************************************************/
static tU8 proc1Stack[PROC1_STACK_SIZE];
static tU8 initStack[INIT_STACK_SIZE];
static tU8 pid1;

static tU8 contrast = 56;
static tU8 cursor   = 0;


/*****************************************************************************
 * Local prototypes
 ****************************************************************************/
static void proc1(void* arg);
static void initProc(void* arg);


/*****************************************************************************
 *
 * Description:
 *    The first function to execute
 *
 ****************************************************************************/
int
main(void)
{
  tU8 error;
  tU8 pid;

  //immediate initilaizeation of hardware I/O pins
  immediateIoInit();

  osInit();
  osCreateProcess(initProc, initStack, INIT_STACK_SIZE, &pid, 1, NULL, &error);
  osStartProcess(pid, &error);

  osStart();
  return 0;
}


/*****************************************************************************
 *
 * Description:
 *    Draw cursor in main menu
 *
 * Params:
 *    [in] cursor - Cursor position
 *
 ****************************************************************************/
static void
drawMenuCursor(tU8 cursor)
{
  tU32 row;

  for(row=0; row<5; row++)
  {
    lcdGotoxy(18,20+(14*row));
    if(row == cursor)
      lcdColor(0x00,0xe0);
    else
      lcdColor(0x00,0xfd);

    switch(row)
    {
      case 0: lcdPuts("Play Example"); break;
      case 1: lcdPuts("Play Snake"); break;
      case 2: lcdPuts("Play P-Pong"); break;
      case 3: lcdPuts("Play Chess"); break;
      case 4: lcdPuts("Bluetooth"); break;
      default: break;
    }
  }
}


/*****************************************************************************
 *
 * Description:
 *    Draw main menu
 *
 ****************************************************************************/
static void
drawMenu(void)
{
  lcdColor(0,0);
  lcdClrscr();

  lcdRect(14, 0, 102, 128, 0x6d);
  lcdRect(15, 17, 100, 110, 0);

  lcdGotoxy(48,1);
  lcdColor(0x6d,0);
  lcdPuts("MENU");
  drawMenuCursor(cursor);
}


/*****************************************************************************
 *
 * Description:
 *    A process entry function
 *
 * Params:
 *    [in] arg - This parameter is not used in this application.
 *
 ****************************************************************************/
static void
proc1(void* arg)
{
  //shortly bleep with the buzzer and flash with the LEDs
  tU8 i,j;

  for(i=0; i<3; i++)
  {
    for(j=0; j<3; j++)
    {
      setBuzzer(TRUE);
      setLED(LED_GREEN, FALSE);
      setLED(LED_RED,   TRUE);
      osSleep(1);

      setBuzzer(FALSE);
      setLED(LED_GREEN, TRUE);
      setLED(LED_RED,   FALSE);
      osSleep(1);
    }
    osSleep(10);
  }
  setLED(LED_GREEN, FALSE);
  setLED(LED_RED,   FALSE);

  //display startup message
  resetLCD();
  lcdInit();
  displayStartupSequence();

  //print menu
  drawMenu();

  for(;;)
  {
    tU8 anyKey;
    static tU8 i = 0;

    anyKey = checkKey();
    if (anyKey != KEY_NOTHING)
    {
      //select specific function
      if (anyKey == KEY_CENTER)
      {
        switch(cursor)
        {
          case 0: playExample(); break;
          case 1: playSnake(); break;
          case 2: playPong(); break;
          case 3: playChess(); break;
          case 4: handleBt(); break;
          default: break;
        }
        drawMenu();
      }

      //move cursor up
      else if (anyKey == KEY_UP)
      {
        if (cursor > 0)
          cursor--;
        else
          cursor = 4;
        drawMenuCursor(cursor);
      }

      //move cursor down
      else if (anyKey == KEY_DOWN)
      {
        if (cursor < 4)
          cursor++;
        else
          cursor = 0;
        drawMenuCursor(cursor);
      }

      //adjust contrast
      else if (anyKey == KEY_RIGHT)
      {
        contrast++;
        if (contrast > 127)
          contrast = 127;
        lcdContrast(contrast);
      }
      else if (anyKey == KEY_LEFT)
      {
        if (contrast > 0)
          contrast--;
        lcdContrast(contrast);
      }
    }

    switch(i)
    {
      case 0: lcdIcon(15, 88, 100, 40, _fire_0_100x40c[2], _fire_0_100x40c[3], &_fire_0_100x40c[4]); i++; break;
      case 1: lcdIcon(15, 88, 100, 40, _fire_1_100x40c[2], _fire_1_100x40c[3], &_fire_1_100x40c[4]); i++; break;
      case 2: lcdIcon(15, 88, 100, 40, _fire_2_100x40c[2], _fire_2_100x40c[3], &_fire_2_100x40c[4]); i++; break;
      case 3: lcdIcon(15, 88, 100, 40, _fire_3_100x40c[2], _fire_3_100x40c[3], &_fire_3_100x40c[4]); i++; break;
      case 4: lcdIcon(15, 88, 100, 40, _fire_4_100x40c[2], _fire_4_100x40c[3], &_fire_4_100x40c[4]); i++; break;
      case 5: lcdIcon(15, 88, 100, 40, _fire_5_100x40c[2], _fire_5_100x40c[3], &_fire_5_100x40c[4]); i++; break;
      case 6: lcdIcon(15, 88, 100, 40, _fire_6_100x40c[2], _fire_6_100x40c[3], &_fire_6_100x40c[4]); i = 0; break;
      default: i = 0; break;
    }
    osSleep(20);
  }
}


/*****************************************************************************
 *
 * Description:
 *    The entry function for the initialization process.
 *
 * Params:
 *    [in] arg - This parameter is not used in this application.
 *
 ****************************************************************************/
static void
initProc(void* arg)
{
  tU8 error;

  eaInit();
  printf("\n*********************************************************");
  printf("\n*                                                       *");
  printf("\n* Welcome to Embedded Artists' summer promotion board;  *");
  printf("\n*   'LPC2104 Color LCD Game Board with Bluetooth'       *");
  printf("\n* in cooperation with Future Electronics and Philips.   *");
  printf("\n* Boards with embedded JTAG includes J-link(tm)         *");
  printf("\n* technology from Segger.                               *");
  printf("\n*                                                       *");
  printf("\n* Program version:  1.8                                 *");
  printf("\n* Program date:     2006-07-27                          *");
  if (TRUE == ver1_0)
    printf("\n* Hardware version: 1.0                                 *");
  else if (TRUE == ver1_1)
    printf("\n* Hardware version: 1.1                                 *");
  printf("\n*                                                       *");
  printf("\n* (C) Embedded Artists AB, 2006                         *");
  printf("\n*                                                       *");
  printf("\n*********************************************************\n");

  osCreateProcess(proc1, proc1Stack, PROC1_STACK_SIZE, &pid1, 3, NULL, &error);
  osStartProcess(pid1, &error);

  initBtProc();

  osDeleteProcess();
}

/*****************************************************************************
 *
 * Description:
 *    The timer tick entry function that is called once every timer tick
 *    interrupt in the RTOS. Observe that any processing in this
 *    function must be kept as short as possible since this function
 *    execute in interrupt context.
 *
 * Params:
 *    [in] elapsedTime - The number of elapsed milliseconds since last call.
 *
 ****************************************************************************/
void
appTick(tU32 elapsedTime)
{
  ms += elapsedTime;

  if((ms % 50) == 0)
    sampleKey();
}






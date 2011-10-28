/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    key.c
 *
 * Description:
 *    Implements sampling and handling of joystick key.
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include <printf_P.h>
#include "key.h"
#include "hw.h"


/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/
#define FIRST_REPEAT  4
#define SECOND_REPEAT 3


/*****************************************************************************
 * Local variables
 ****************************************************************************/
static tU8 centerReleased = TRUE;
static tU8 keyUpReleased = TRUE;
static tU8 keyDownReleased = TRUE;
static tU8 keyLeftReleased = TRUE;
static tU8 keyRightReleased = TRUE;
static tU8 centerKeyCnt;
static tU8 upKeyCnt;
static tU8 downKeyCnt;
static tU8 leftKeyCnt;
static tU8 rightKeyCnt;

static volatile tU8 activeKey = KEY_NOTHING;
static volatile tU8 activeKey2 = KEY_NOTHING;


/*****************************************************************************
 *
 * Description:
 *    Function to check if any key press has been detected
 *
 ****************************************************************************/
tU8
checkKey(void)
{
  tU8 retVal = activeKey;
  activeKey = KEY_NOTHING;
  return retVal;
}

/*****************************************************************************
 *
 * Description:
 *    Function to check current (instantaneous) key state
 *
 ****************************************************************************/
tU8
checkKey2(void)
{
  return activeKey2;
}

/*****************************************************************************
 *
 * Description:
 *    Sample key states
 *
 ****************************************************************************/
void
sampleKey(void)
{
  tBool nothing = TRUE;
  tU8   readKeys;
  
  //get sample
  readKeys = getKeys();
  

  //check center key
  if (readKeys & KEY_CENTER)
  {
    nothing = FALSE;
  	if (centerReleased == TRUE)
  	{
  		centerReleased = FALSE;
  		centerKeyCnt = 0;
  		activeKey = KEY_CENTER;
  		activeKey2 = KEY_CENTER;
  	}
  	else
  	{
  	  centerKeyCnt++;
  	  if (centerKeyCnt == FIRST_REPEAT)
  	  {
  		  activeKey = KEY_CENTER;
  		  activeKey2 = KEY_CENTER;
  	  }
  	  else if (centerKeyCnt >= FIRST_REPEAT + SECOND_REPEAT)
  	  {
  		  centerKeyCnt = FIRST_REPEAT;
  		  activeKey = KEY_CENTER;
  		  activeKey2 = KEY_CENTER;
  	  }
  	}
  }
  else
  	centerReleased = TRUE;

  //check up key
  if (readKeys & KEY_UP)
  {
    nothing = FALSE;
  	if (keyUpReleased == TRUE)
  	{
  		keyUpReleased = FALSE;
  		upKeyCnt = 0;
  		activeKey = KEY_UP;
  		activeKey2 = KEY_UP;
  	}
  	else
  	{
  	  upKeyCnt++;
  	  if (upKeyCnt == FIRST_REPEAT)
  	  {
  		  activeKey = KEY_UP;
  		  activeKey2 = KEY_UP;
  	  }
  	  else if (upKeyCnt >= FIRST_REPEAT + SECOND_REPEAT)
  	  {
  		  upKeyCnt = FIRST_REPEAT;
  		  activeKey = KEY_UP;
  		  activeKey2 = KEY_UP;
  	  }
  	}
  }
  else
  	keyUpReleased = TRUE;

  //check down key
  if (readKeys & KEY_DOWN)
  {
    nothing = FALSE;
  	if (keyDownReleased == TRUE)
  	{
  		keyDownReleased = FALSE;
  		downKeyCnt = 0;
  		activeKey = KEY_DOWN;
  		activeKey2 = KEY_DOWN;
  	}
  	else
  	{
  	  downKeyCnt++;
  	  if (downKeyCnt == FIRST_REPEAT)
  	  {
  		  activeKey = KEY_DOWN;
  		  activeKey2 = KEY_DOWN;
  	  }
  	  else if (downKeyCnt >= FIRST_REPEAT + SECOND_REPEAT)
  	  {
  		  downKeyCnt = FIRST_REPEAT;
  		  activeKey = KEY_DOWN;
  		  activeKey2 = KEY_DOWN;
  	  }
  	}
  }
  else
  	keyDownReleased = TRUE;

  //check left key
  if (readKeys & KEY_LEFT)
  {
    nothing = FALSE;
  	if (keyLeftReleased == TRUE)
  	{
  		keyLeftReleased = FALSE;
  		leftKeyCnt = 0;
  		activeKey = KEY_LEFT;
  		activeKey2 = KEY_LEFT;
  	}
  	else
  	{
  	  leftKeyCnt++;
  	  if (leftKeyCnt == FIRST_REPEAT)
  	  {
  		  activeKey = KEY_LEFT;
  		  activeKey2 = KEY_LEFT;
  	  }
  	  else if (leftKeyCnt >= FIRST_REPEAT + SECOND_REPEAT)
  	  {
  		  leftKeyCnt = FIRST_REPEAT;
  		  activeKey = KEY_LEFT;
  		  activeKey2 = KEY_LEFT;
  	  }
  	}
  }
  else
  	keyLeftReleased = TRUE;

  //check right key
  if (readKeys & KEY_RIGHT)
  {
    nothing = FALSE;
  	if (keyRightReleased == TRUE)
  	{
  		keyRightReleased = FALSE;
  		rightKeyCnt = 0;
  		activeKey = KEY_RIGHT;
  		activeKey2 = KEY_RIGHT;
  	}
  	else
  	{
  	  rightKeyCnt++;
  	  if (rightKeyCnt == FIRST_REPEAT)
  	  {
  		  activeKey = KEY_RIGHT;
  		  activeKey2 = KEY_RIGHT;
  	  }
  	  else if (rightKeyCnt >= FIRST_REPEAT + SECOND_REPEAT)
  	  {
  		  rightKeyCnt = FIRST_REPEAT;
  		  activeKey = KEY_RIGHT;
  		  activeKey2 = KEY_RIGHT;
  	  }
  	}
  }
  else
  	keyRightReleased = TRUE;
  
  if (nothing == TRUE)
    activeKey2 = KEY_NOTHING;
}



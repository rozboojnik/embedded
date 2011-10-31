/******************************************************************************
 *
 * Copyright:
 *    (C) 2011
 *
 * File:
 *    Arrow.c
 *
 * Description:
 *    Implementation of arrows.
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include <printf_P.h>
#include <ea_init.h>
#include "lcd.h"
#include "ArrowLeft.h"
#include "ArrowRight.h"
#include "ArrowUp.h"
#include "ArrowDown.h"
#include "Arrow.h"

/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/


/*****************************************************************************
 * Local variables
 ****************************************************************************/


/*****************************************************************************
 * External variables
 ****************************************************************************/


/*****************************************************************************
 *
 * Description:
 *    Opis tutaj
 *
 ****************************************************************************/
void getUpArrow(void){
	lcdIcon(5, 5, 90, 93, _ArrowUp[2], _ArrowUp[3], &_ArrowUp[4]);
}

void getDownArrow(void){
	lcdIcon(5,5,90,88,_ArrowDown[2], _ArrowDown[3], &_ArrowDown[4]);
}

void getRightArrow(void){
	lcdIcon(5,5,90,92,_ArrowRight[2], _ArrowRight[3], &_ArrowRight[4]);
}

void getLeftArrow(void){
	lcdIcon(5,5,90,93,_ArrowLeft[2], _ArrowLeft[3], &_ArrowLeft[4]);
}

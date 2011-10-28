/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    bt.h
 *
 * Description:
 *    Expose public functions related to communication with the Philips
 *    Bluetooth module, BGB203-S06
 *
 *****************************************************************************/
#ifndef _BT_H_
#define _BT_H_

void initBtProc(void);
void handleBt(void);
void blockBtProc(void);
void activateBtProc(void);

#endif

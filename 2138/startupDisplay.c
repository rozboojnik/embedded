/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    startupDisplay.c
 *
 * Description:
 *    Implements a startup sequence displaying different pictures and texts.
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
#include "ea_97x60c.h"
#include "future_128x39c.h"
#include "philips_122x25c.h"
#include "segger_85x40c.h"
#include "fun_0_130x90c.h"
#include "fun_1_130x90c.h"


/*****************************************************************************
 *
 * Description:
 *    A process entry function 
 *
 ****************************************************************************/
void
displayStartupSequence(void)
{
  tU32 step = 0;
  tU8 anyKey = KEY_NOTHING;

  for(step=0; step<=48; step++)
  {
    anyKey = checkKey();
    if (anyKey != KEY_NOTHING)
      break;

    switch(step)
    {
      case 0: lcdColor(0xfd,0x00); lcdClrscr(); break;
      case 1: lcdIcon(0, 0, 130, 90, _fun_0_130x90c[2], _fun_0_130x90c[3], &_fun_0_130x90c[4]); break;

      case 2: lcdGotoxy(8,100); lcdPutchar('H'); break;
      case 3: lcdPutchar('A'); break;
      case 4: lcdPutchar('V'); break;
      case 5: lcdPutchar('E'); lcdIcon(0, 0, 130, 90, _fun_1_130x90c[2], _fun_1_130x90c[3], &_fun_1_130x90c[4]); break;

      case 6: lcdGotoxy(8+(8*4),100); lcdPutchar(' '); break;
      case 7: lcdPutchar('S'); break;
      case 8: lcdPutchar('O'); break;
      case 9: lcdPutchar('M'); lcdIcon(0, 0, 130, 90, _fun_0_130x90c[2], _fun_0_130x90c[3], &_fun_0_130x90c[4]); break;

      case 10: lcdGotoxy(8+(8*8),100); lcdPutchar('E'); break;
      case 11: lcdPutchar(' '); break;
      case 12: lcdPutchar('F'); break;
      case 13: lcdPutchar('U'); lcdIcon(0, 0, 130, 90, _fun_1_130x90c[2], _fun_1_130x90c[3], &_fun_1_130x90c[4]); break;
      
      case 14: lcdGotoxy(8+(8*12),100); lcdPutchar('N'); break;
      case 15: lcdPutchar('!'); break;
      case 17:
      case 25:
      case 33:
      case 41: lcdIcon(0, 0, 130, 90, _fun_0_130x90c[2], _fun_0_130x90c[3], &_fun_0_130x90c[4]); break;

      case 21:
      case 29:
      case 37:
      case 45: lcdIcon(0, 0, 130, 90, _fun_1_130x90c[2], _fun_1_130x90c[3], &_fun_1_130x90c[4]); break;

      default: break;
    }
    osSleep(15);
  }
  
  if (anyKey == KEY_NOTHING)
  for(step=0; step<=257; step++)
  {
    anyKey = checkKey();
    if (anyKey != KEY_NOTHING)
      break;

    switch(step)
    {
      case 0: lcdColor(0xff,0x00); lcdClrscr(); break;
      case 1: lcdIcon(16, 0, 97, 60, _ea_97x60c[2], _ea_97x60c[3], &_ea_97x60c[4]); break;
      case 2: lcdGotoxy(16,66); lcdPutchar('D'); break;
      case 3: lcdPutchar('e'); break;
      case 4: lcdPutchar('s'); break;
      case 5: lcdPutchar('i'); break;
      case 6: lcdPutchar('g'); break;
      case 7: lcdPutchar('n'); break;
      case 8: lcdPutchar('e'); break;
      case 9: lcdPutchar('d'); break;
      case 10: lcdPutchar(' '); break;
      case 11: lcdPutchar('a'); break;
      case 12: lcdPutchar('n'); break;
      case 13: lcdPutchar('d'); break;
      case 14: lcdGotoxy(20,80); lcdPutchar('p'); break;
      case 15: lcdPutchar('r'); break;
      case 16: lcdPutchar('o'); break;
      case 17: lcdPutchar('d'); break;
      case 18: lcdPutchar('u'); break;
      case 19: lcdPutchar('c'); break;
      case 20: lcdPutchar('e'); break;
      case 21: lcdPutchar('d'); break;
      case 22: lcdPutchar(' '); break;
      case 23: lcdPutchar('b'); break;
      case 24: lcdPutchar('y'); break;
      case 25: lcdGotoxy(0,96); lcdPutchar('E'); break;
      case 26: lcdPutchar('m'); break;
      case 27: lcdPutchar('b'); break;
      case 28: lcdPutchar('e'); break;
      case 29: lcdPutchar('d'); break;
      case 30: lcdPutchar('d'); break;
      case 31: lcdPutchar('e'); break;
      case 32: lcdPutchar('d'); break;
      case 33: lcdPutchar(' '); break;
      case 34: lcdPutchar('A'); break;
      case 35: lcdPutchar('r'); break;
      case 36: lcdPutchar('t'); break;
      case 37: lcdPutchar('i'); break;
      case 38: lcdPutchar('s'); break;
      case 39: lcdPutchar('t'); break;
      case 40: lcdPutchar('s'); break;
      case 41: lcdGotoxy(32,112); lcdPutchar('('); break;
      case 42: lcdPutchar('C'); break;
      case 43: lcdPutchar(')'); break;
      case 44: lcdPutchar(' '); break;
      case 45: lcdPutchar('2'); break;
      case 46: lcdPutchar('0'); break;
      case 47: lcdPutchar('0'); break;
      case 48: lcdPutchar('6'); break;

      case 60: lcdClrscr(); lcdIcon(0, 0, 128, 39, _future_128x39c[2], _future_128x39c[3], &_future_128x39c[4]); break;
      case 61: lcdGotoxy(8,44); lcdPutchar('i'); break;
      case 62: lcdPutchar('n'); break;
      case 63: lcdPutchar(' '); break;
      case 64: lcdPutchar('c'); break;
      case 65: lcdPutchar('o'); break;
      case 66: lcdPutchar('o'); break;
      case 67: lcdPutchar('p'); break;
      case 68: lcdPutchar('e'); break;
      case 69: lcdPutchar('r'); break;
      case 70: lcdPutchar('a'); break;
      case 71: lcdPutchar('t'); break;
      case 72: lcdPutchar('i'); break;
      case 73: lcdPutchar('o'); break;
      case 74: lcdPutchar('n'); break;
      case 75: lcdGotoxy(20,60); lcdPutchar('w'); break;
      case 76: lcdPutchar('i'); break;
      case 77: lcdPutchar('t'); break;
      case 78: lcdPutchar('h'); break;
      case 79: lcdPutchar(' '); break;
      case 80: lcdPutchar('F'); break;
      case 81: lcdPutchar('u'); break;
      case 82: lcdPutchar('t'); break;
      case 83: lcdPutchar('u'); break;
      case 84: lcdPutchar('r'); break;
      case 85: lcdPutchar('e'); break;
      case 86: lcdGotoxy(4,76); lcdPutchar('E'); break;
      case 87: lcdPutchar('l'); break;
      case 88: lcdPutchar('e'); break;
      case 89: lcdPutchar('c'); break;
      case 90: lcdPutchar('t'); break;
      case 91: lcdPutchar('r'); break;
      case 92: lcdPutchar('o'); break;
      case 93: lcdPutchar('n'); break;
      case 94: lcdPutchar('i'); break;
      case 95: lcdPutchar('c'); break;
      case 96: lcdPutchar('s'); break;
      case 97: lcdPutchar(' '); break;
      case 98: lcdPutchar('a'); break;
      case 99: lcdPutchar('n'); break;
      case 100: lcdPutchar('d'); break;
      case 105: lcdIcon(3, 98, 122, 25, _philips_122x25c[2], _philips_122x25c[3], &_philips_122x25c[4]); break;
      
      case 120: lcdClrscr(); lcdIcon(22, 3, 85, 40, _segger_85x40c[2], _segger_85x40c[3], &_segger_85x40c[4]); break;
      case 121: lcdGotoxy(12,48); lcdPutchar('E'); break;
      case 122: lcdPutchar('m'); break;
      case 123: lcdPutchar('b'); break;
      case 124: lcdPutchar('e'); break;
      case 125: lcdPutchar('d'); break;
      case 126: lcdPutchar('d'); break;
      case 127: lcdPutchar('e'); break;
      case 128: lcdPutchar('d'); break;
      case 129: lcdPutchar(' '); break;
      case 130: lcdPutchar('J'); break;
      case 131: lcdPutchar('T'); break;
      case 132: lcdPutchar('A'); break;
      case 133: lcdPutchar('G'); break;
      case 134: lcdGotoxy(40,64); lcdPutchar('J'); break;
      case 135: lcdPutchar('-'); break;
      case 136: lcdPutchar('l'); break;
      case 137: lcdPutchar('i'); break;
      case 138: lcdPutchar('n'); break;
      case 139: lcdPutchar('k'); break;
      case 140: lcdGotoxy(20,80); lcdPutchar('t'); break;
      case 141: lcdPutchar('e'); break;
      case 142: lcdPutchar('c'); break;
      case 143: lcdPutchar('h'); break;
      case 144: lcdPutchar('n'); break;
      case 145: lcdPutchar('o'); break;
      case 146: lcdPutchar('l'); break;
      case 147: lcdPutchar('o'); break;
      case 148: lcdPutchar('g'); break;
      case 149: lcdPutchar('y'); break;
      case 150: lcdGotoxy(16,94); lcdPutchar('l'); break;
      case 151: lcdPutchar('i'); break;
      case 152: lcdPutchar('c'); break;
      case 153: lcdPutchar('e'); break;
      case 154: lcdPutchar('n'); break;
      case 155: lcdPutchar('s'); break;
      case 156: lcdPutchar('e'); break;
      case 157: lcdPutchar('d'); break;
      case 158: lcdPutchar(' '); break;
      case 159: lcdPutchar('b'); break;
      case 160: lcdPutchar('y'); break;
      case 161: lcdGotoxy(8,110); lcdPutchar('w'); break;
      case 162: lcdPutchar('w'); break;
      case 163: lcdPutchar('w'); break;
      case 164: lcdPutchar('.'); break;
      case 165: lcdPutchar('s'); break;
      case 166: lcdPutchar('e'); break;
      case 167: lcdPutchar('g'); break;
      case 168: lcdPutchar('g'); break;
      case 169: lcdPutchar('e'); break;
      case 170: lcdPutchar('r'); break;
      case 171: lcdPutchar('.'); break;
      case 172: lcdPutchar('c'); break;
      case 173: lcdPutchar('o'); break;
      case 174: lcdPutchar('m'); break;

      case 190: lcdColor(0xff,0x00); lcdClrscr(); break;
      case 191: lcdIcon(16, 0, 97, 60, _ea_97x60c[2], _ea_97x60c[3], &_ea_97x60c[4]); break;
      case 192: lcdGotoxy(0,66); lcdPutchar('P'); break;
      case 193: lcdPutchar('r'); break;
      case 194: lcdPutchar('o'); break;
      case 195: lcdPutchar('g'); break;
      case 196: lcdPutchar('r'); break;
      case 197: lcdPutchar('a'); break;
      case 198: lcdPutchar('m'); break;
      case 199: lcdPutchar(' '); break;
      case 200: lcdPutchar('v'); break;
      case 201: lcdPutchar('e'); break;
      case 202: lcdPutchar('r'); break;
      case 203: lcdPutchar(':'); break;
      case 204: lcdPutchar('1'); break;
      case 205: lcdPutchar('.'); break;
      case 206: lcdPutchar('8'); break;
      case 207: lcdGotoxy(20,80); lcdPutchar('C'); break;
      case 208: lcdPutchar('h'); break;
      case 209: lcdPutchar('e'); break;
      case 210: lcdPutchar('c'); break;
      case 211: lcdPutchar('k'); break;
      case 212: lcdPutchar(' '); break;
      case 213: lcdPutchar('f'); break;
      case 214: lcdPutchar('o'); break;
      case 215: lcdPutchar('r'); break;
      case 216: lcdGotoxy(8,96); lcdPutchar('u'); break;
      case 217: lcdPutchar('p'); break;
      case 218: lcdPutchar('d'); break;
      case 219: lcdPutchar('a'); break;
      case 220: lcdPutchar('t'); break;
      case 221: lcdPutchar('e'); break;
      case 222: lcdPutchar('s'); break;
      case 223: lcdPutchar(' '); break;
      case 224: lcdPutchar('a'); break;
      case 225: lcdPutchar('t'); break;
      case 226: lcdPutchar(' '); break;
      case 227: lcdPutchar('t'); break;
      case 228: lcdPutchar('h'); break;
      case 229: lcdPutchar('e'); break;
      case 230: lcdGotoxy(8,112); lcdPutchar('s'); break;
      case 231: lcdPutchar('u'); break;
      case 232: lcdPutchar('p'); break;
      case 233: lcdPutchar('p'); break;
      case 234: lcdPutchar('o'); break;
      case 235: lcdPutchar('r'); break;
      case 236: lcdPutchar('t'); break;
      case 237: lcdPutchar(' '); break;
      case 238: lcdPutchar('p'); break;
      case 239: lcdPutchar('a'); break;
      case 240: lcdPutchar('g'); break;
      case 241: lcdPutchar('e'); break;
      case 242: lcdPutchar('.'); break;

      default: break;
    }
    osSleep(10);
  }

  lcdColor(0x00,0x00);
  lcdClrscr();
}

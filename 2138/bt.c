/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    bt.c
 *
 * Description:
 *    Demonstrate communication with the Philips Bluetooth module, BGB203-S06
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
#include <string.h>
#include "lcd.h"
#include "key.h"
#include "uart.h"
#include "hw.h"
#include "select.h"

/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/
#define RECV_BUF_LEN 32
#define PROC_BT_STACK_SIZE 800

#define BT_BACKGROUND_COLOR     0x01
#define BT_BACKBACKGROUND_COLOR 0x03

#define MAX_BT_UNITS 5

typedef struct
{
  tU8 active;
  tU8 btAddress[13];
  tU8 btName[17];
} tBtRecord;


/*****************************************************************************
 * Local variables
 ****************************************************************************/
static tU8 procBtStack[PROC_BT_STACK_SIZE];
static tU8 pidBt;

static volatile tBool stopRecvProc = FALSE;
static tCntSem recvSem;

static tU8 btCursor = 0;

static tBool btSleepState = FALSE;
static tBool btCommandMode = FALSE;

static tU8 localName[17];
static tU8 btAddress[13];

static tBtRecord foundBtUnits[MAX_BT_UNITS];


/*****************************************************************************
 * External variables
 ****************************************************************************/
extern volatile tU32 ms;


/*****************************************************************************
 * Local function prototype
 ****************************************************************************/
static void procBt(void* arg);
static void btSetMode(tBool commandMode);


/*****************************************************************************
 *
 * Description:
 *    Initialize and start a process handling the BGB203-S06 communication
 *    (over UART channel #1).
 *
 ****************************************************************************/
void
initBtProc(void)
{
  tU8 error;
  
  osSemInit(&recvSem, 1);

  osCreateProcess(procBt, procBtStack, PROC_BT_STACK_SIZE, &pidBt, 4, NULL, &error);
  osStartProcess(pidBt, &error);
}


/*****************************************************************************
 *
 * Description:
 *    A process entry function - the BGb203-S06 handling process
 *
 * Params:
 *    [in] arg - This parameter is not used in this application. 
 *
 ****************************************************************************/
static void
procBt(void* arg)
{
  tU8 error;

  //reset BGB203 modules
  resetBT(TRUE);
  osSleep(2);
  resetBT(FALSE);
  osSleep(5);

  //extra sleep
  osSleep(50);
  
  //initialize uart #1: 115200 kbps, 8N1, FIFO
  initUart1(B115200((CORE_FREQ) / PBSD), UART_8N1, UART_FIFO_16);

  /***************************************************************************
   * Startup command sequence
   *   This command sequence is just used for production testing
   **************************************************************************/
  osSleep(50);
  uart1SendString("+++");
  osSleep(50);
  uart1SendString("ATI\r");
  osSleep(50);
  uart1SendString("AT+BTLNM\r");
  
  osSleep(100);
  uart1SendString("AT+BTBDA\r");
  osSleep(100);
  uart1SendString("AT+BTINQ=5\r");
  osSleep(600);

  //Switch to server mode where other BT devices can detect this unit
  //during an 'inquiry'
  uart1SendString("AT+BTSRV=1\r");

  /***************************************************************************
   * Loop forever and create a terminal directly between the
   * USB serial port and the BT module
   **************************************************************************/
  while(1)
  {
    osSemTake(&recvSem, 0, &error);

    while (stopRecvProc == FALSE)
    {
      tU8 rxChar;
      tU8 nothing;


      nothing = TRUE;

      //check if any character has been received from terminal
      if (consolGetChar(&rxChar) == TRUE)
      {
        uart1SendChar(rxChar);
        nothing = FALSE;
      }

      //check if any character has been received from BT
      if (uart1GetChar(&rxChar) == TRUE)
      {
        printf("%c", rxChar);
        nothing = FALSE;
      }
        
      if (nothing == TRUE)
        osSleep(1);
    }
    stopRecvProc = FALSE;
  }
}


/*****************************************************************************
 *
 * Description:
 *    Prints a text string with a delay between each character, much like
 *    a typewriter.
 *
 ****************************************************************************/
static void
displayMessage(tU8 *pMessage, tU8 speed)
{
  while(*pMessage != '\0')
  {
    lcdPutchar(*pMessage++);
    osSleep(speed);
  }
}


/*****************************************************************************
 *
 * Description:
 *    Prints the list of 'found' Bluetooth units
 *
 ****************************************************************************/
static void
drawBtsFound(tBool drawCursor, tU8 cursorPos)
{
  tU32 row;

  for(row=0; row<MAX_BT_UNITS; row++)
  {
    lcdGotoxy(2,30+(14*row));
    if(foundBtUnits[row].active == FALSE)
    {
      if ((drawCursor == TRUE) && (cursorPos == row))
        lcdColor(BT_BACKGROUND_COLOR+1,0xe0);
      else
        lcdColor(BT_BACKGROUND_COLOR,0xfd);
      lcdPuts("-");
    }
    else
    {
      if ((drawCursor == TRUE) && (cursorPos == row))
        lcdColor(BT_BACKGROUND_COLOR+1,0xe0);
      else
        lcdColor(BT_BACKGROUND_COLOR,0xfd);
      lcdPuts(foundBtUnits[row].btAddress);
    }
  }
}


/*****************************************************************************
 *
 * Description:
 *    Perform a Bluetooth 'inquiry' and displays the result, i.e., found
 *    units in the proximity of this unit.
 *
 ****************************************************************************/
static void
btInquiry(void)
{
  tU8  rxChar;
  tU8  done;
  tU8  anyKey;
  tU8  cursorPos;
  volatile tU32 timeStamp;
  tU8  recvPos;
  tU8  error;
  tU8  recvBuf[RECV_BUF_LEN];
  tU8  foundBt;
  tU32 cnt;
  tU8  numServices;

  for(foundBt=0; foundBt<MAX_BT_UNITS; foundBt++)
  {
    foundBtUnits[foundBt].active = FALSE;
    foundBtUnits[foundBt].btName[0] = '\0';
  }

  //clear menu screen
  lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);
  lcdGotoxy(18,16);
  lcdColor(BT_BACKGROUND_COLOR,0xfd);
  lcdPuts("Inquiry");
  drawBtsFound(FALSE,0);

  //stop the BT handling process
  stopRecvProc = TRUE;

  //*************************************************************
  //* Start inquiry (during 6 seconds)
  //*************************************************************
  osSleep(100);
  uart1SendString("+++");
  osSleep(100);
  uart1SendString("AT+BTINQ=6\r");

  //*************************************************************
  //* Get (receive and interpret) discovered units
  //*************************************************************
  timeStamp = ms;
  done = FALSE;
  recvPos = 0;
  foundBt = 0;
  cnt = 0;
  while((done == FALSE) && ((ms - timeStamp) < 6500))
  {
    //check if any character has been received from BT
    if (uart1GetChar(&rxChar) == TRUE)
    {
      printf("%c", rxChar);
      
      if (rxChar == 0x0a)
      {
        if (recvPos > 0)
          recvBuf[recvPos-1] = '\0';

        //evaluate received bytes
        if ((memcmp(recvBuf, "+BTINQ: ", 8) == 0) && (recvPos == 29))
        {
          if (foundBt < MAX_BT_UNITS)
          {
            for(recvPos=0; recvPos<12; recvPos++)
              foundBtUnits[foundBt].btAddress[recvPos] = recvBuf[recvPos + 8];
            foundBtUnits[foundBt].btAddress[12] = '\0';
            foundBtUnits[foundBt].active = TRUE;
            foundBt++;

            //display newly found bt unit
            drawBtsFound(FALSE, 0);
          }
        }
        else if (memcmp(recvBuf, "+BTINQ: COMPLETE", 16) == 0)
        {
          done = TRUE;
        }
        recvPos = 0;
      }
      else if (recvPos < RECV_BUF_LEN)
        recvBuf[recvPos++] = rxChar;
    }
    osSleep(1);

    //print activity indicator
    lcdGotoxy(74,16);
    lcdColor(BT_BACKGROUND_COLOR,0xfd);
    switch(cnt % 150)
    {
      case   0: lcdPuts("   ");break;
      case  25: lcdPuts(".  ");break;
      case  50: lcdPuts(".. ");break;
      case  75: lcdPuts("...");break;
      case 100: lcdPuts(" ..");break;
      case 125: lcdPuts("  .");break;
      default: break;
    }
    cnt ++;
  }

  lcdGotoxy(74,16);
  lcdColor(BT_BACKGROUND_COLOR,0xfd);
  lcdPuts("-done");

  //*************************************************************
  //* Handle user key inputs (move between discovered units)
  //*************************************************************
  done = FALSE;
  cursorPos = 0;
  drawBtsFound(TRUE, cursorPos);
  while(done == FALSE)
  {
    anyKey = checkKey();
    if (anyKey != KEY_NOTHING)
    {
      //exit and save new name
      if (anyKey == KEY_CENTER)
      {
        done = TRUE;
      }

      //
      else if (anyKey == KEY_UP)
      {
        if (cursorPos > 0)
          cursorPos--;
        else
          cursorPos = MAX_BT_UNITS-1;
        drawBtsFound(TRUE, cursorPos);
      }
      
      //
      else if (anyKey == KEY_DOWN)
      {
        if (cursorPos < MAX_BT_UNITS-1)
          cursorPos++;
        else
          cursorPos = 0;
          drawBtsFound(TRUE, cursorPos);
      }
    }
    osSleep(1);
  }

  //*************************************************************
  //* Get name of selected (discovered) bt unit
  //*************************************************************
  cnt = 0;
  numServices = 0;
  if (foundBtUnits[cursorPos].active == TRUE)
  {
    tU8   serviceName[17];
    tBool longName = FALSE;

    uart1SendString("AT+BTSDP=");
    uart1SendString(foundBtUnits[cursorPos].btAddress);
    uart1SendString("\r");

    timeStamp = ms;
    done = FALSE;
    recvPos = 0;

    while((done == FALSE) && ((ms - timeStamp) < 6000))
    {
      //check if any character has been received from BT
      if (uart1GetChar(&rxChar) == TRUE)
      {
        printf("%c", rxChar);
      
        if (rxChar == 0x0a)
        {
          if (recvPos > 0)
            recvBuf[recvPos-1] = '\0';

          //evaluate received bytes
          if (memcmp(recvBuf, "+BTSDP: COMPLETE", 16) == 0)
            done = TRUE;

          else if (memcmp(recvBuf, "+BTSDP: ", 8) == 0)
          {
            numServices++;
            
            //save first services found
            if (numServices == 1)
            {
              tU8 *pStr;
              
              //skip to first '"'
              pStr = strchr(&recvBuf[10], '\"');
              if (pStr != NULL)
              {
                tU8 i;

                //get string (to '"')
                pStr++;
                i = 0;
                while((*pStr != '\"') && (i < 16))
                  serviceName[i++] = *pStr++;
                serviceName[i] = '\0';
                if (*pStr != '\"')
                  longName = TRUE;
                else
                  longName = FALSE;
              }
              else
                serviceName[0] = '\0';
            }
          }
          recvPos = 0;
        }
        else if (recvPos < RECV_BUF_LEN)
          recvBuf[recvPos++] = rxChar;
      }
      osSleep(1);

      //print activity indicator
      lcdGotoxy(74,16);
      lcdColor(BT_BACKGROUND_COLOR,0xfd);
      switch(cnt % 150)
      {
        case   0: lcdPuts("     ");break;
        case  25: lcdPuts("-  ");break;
        case  50: lcdPuts("-- ");break;
        case  75: lcdPuts("---");break;
        case 100: lcdPuts(" --");break;
        case 125: lcdPuts("  -");break;
        default: break;
      }
      cnt++;
    }
    
    //display result
    lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);
    lcdColor(BT_BACKGROUND_COLOR,0xfd);
    lcdGotoxy(2,16+(14*0));
    lcdPuts("Found ");
    {
      tU8 str[3];
      
      str[1] = (numServices % 10) + '0';
      if (numServices >= 10)
        str[0] = (numServices / 10) + '0';
      else
        str[0] = ' ';
      str[2] = '\0';
      lcdPuts(str);
    }
    lcdGotoxy(2,16+(14*1));
    lcdPuts(" services");
    
    if (numServices > 0)
    {
      lcdGotoxy(2,16+(14*2));
      lcdPuts("First service");
      lcdGotoxy(2,16+(14*3));
      lcdPuts(" name is:");
      lcdGotoxy(2,16+(14*4));
      lcdPuts(serviceName);
    
      if(longName == TRUE)
      {
        lcdGotoxy(2,16+(14*5));
        lcdPuts("(name trunc:ed)");
      }
    }

    //wait for key press
    done = FALSE;
    while(done == FALSE)
    {
      anyKey = checkKey();
      if (anyKey != KEY_NOTHING)
        done = TRUE;
      osSleep(1);
    }
  }
  
  //erase screen
  lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);

  //start BT handling process again
  stopRecvProc = FALSE;
  osSemGive(&recvSem, &error);


  //*************************************************************
  //* Exit
  //*************************************************************
  btSetMode(btCommandMode);
}


/*****************************************************************************
 *
 * Description:
 *    Prints the Bluetooth address and the current cursor position.
 *
 ****************************************************************************/
static void
drawBtAddress(tU8 cursor)
{
  tU32 col;

  lcdGotoxy(16,85);
  for(col=0; col<12; col++)
  {
    if(col == cursor)
      lcdColor(BT_BACKGROUND_COLOR,0xe0);
    else
      lcdColor(BT_BACKGROUND_COLOR,0x1c);
    lcdPutchar(btAddress[col]);
  }
}


/*****************************************************************************
 *
 * Description:
 *    Function to handle setting of Bluetooth address. It first reads
 *    the current address from the BGB203-S06 unit.
 *
 ****************************************************************************/
static void
btSetAddress(void)
{
  tU8 rxChar;
  tU8 done;
  tU8 anyKey;
  tU8 cursorPos;
  tU32 timeStamp;
  tU8 recvBuf[RECV_BUF_LEN];
  tU8 recvPos;
  tU8 error;

  //clear menu screen
  lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);
  lcdGotoxy(18,20);
  lcdColor(BT_BACKGROUND_COLOR,0xfd);
  lcdPuts("Set address:");
  lcdGotoxy(6,34);
  lcdPuts("Move cursor and");
  lcdGotoxy(6,48);
  lcdPuts("set each char.");
  lcdGotoxy(6,62);
  lcdPuts("Exit with c-key");

  //stop the BT handling process
  stopRecvProc = TRUE;

  //*************************************************************
  //* Get current address
  //*************************************************************
  osSleep(100);
  uart1SendString("+++");
  osSleep(100);
  uart1SendString("AT+BTBDA\r");

  //*************************************************************
  //* Get (receive and interpret) current address
  //*************************************************************
  timeStamp = ms;
  done = FALSE;
  recvPos = 0;
  while((done == FALSE) && ((ms - timeStamp) < 200))
  {
    //check if any character has been received from BT
    if (uart1GetChar(&rxChar) == TRUE)
    {
      printf("%c", rxChar);
      
      if (rxChar == 0x0a)
      {
        if (recvPos > 0)
          recvBuf[recvPos-1] = '\0';

        //evaluate received bytes
        if ((memcmp(recvBuf, "+BTBDA: ", 8) == 0) && (recvPos == 21))
        {
          recvPos = 8;
          for(recvPos=8; recvPos<20; recvPos++)
            btAddress[recvPos-8] = recvBuf[recvPos];
          btAddress[12] = '\0';
          done = TRUE;
        }
        recvPos = 0;
      }
      else if (recvPos < RECV_BUF_LEN)
        recvBuf[recvPos++] = rxChar;
    }
  }

  //start BT handling process again
  stopRecvProc = FALSE;
  osSemGive(&recvSem, &error);

  //*************************************************************
  //* Handle user key inputs
  //*************************************************************
  done = FALSE;
  cursorPos = 0;
  drawBtAddress(cursorPos);
  while(done == FALSE)
  {
    anyKey = checkKey();
    if (anyKey != KEY_NOTHING)
    {
      //exit and save new address
      if (anyKey == KEY_CENTER)
      {
        tMenu menu;

        menu.xPos = 5;
        menu.yPos = 35;
        menu.xLen = 6+(14*8);
        menu.yLen = 3*14+5;
        menu.noOfChoices = 2;
        menu.initialChoice = 0;
        menu.pHeaderText = "Exit";
        menu.headerTextXpos = 50;
        menu.pChoice[0] = "Cancel";
        menu.pChoice[1] = "Store new addr";
        menu.bgColor       = 0;
        menu.borderColor   = 0x6d;
        menu.headerColor   = 0;
        menu.choicesColor  = 0xfd;
        menu.selectedColor = 0xe0;
        
        switch(drawMenu(menu))
        {
          case 0: break;  //Ignore changes
          case 1:         //Store new address
            //*************************************************************
            //* Set new local address (and automatically save in FLASH)
            //*************************************************************
            uart1SendString("AT+BTSET=1,");
            uart1SendString(btAddress);     //12 digits
            uart1SendString("\r");
            break;
          default: break;
        }

        done = TRUE;
      }

      //
      else if (anyKey == KEY_UP)
      {
        btAddress[cursorPos]++;
        if (btAddress[cursorPos] > 'F')
          btAddress[cursorPos] = '0';
        else if ((btAddress[cursorPos] > '9') && (btAddress[cursorPos] < 'A'))
          btAddress[cursorPos] = 'A';
        drawBtAddress(cursorPos);
      }
      
      //
      else if (anyKey == KEY_DOWN)
      {
        btAddress[cursorPos]--;
        if (btAddress[cursorPos] < '0')
          btAddress[cursorPos] = 'F';
        else if ((btAddress[cursorPos] < 'A') && (btAddress[cursorPos] > '9'))
          btAddress[cursorPos] = '9';
        drawBtAddress(cursorPos);
      }

      //
      else if (anyKey == KEY_LEFT)
      {
        if (cursorPos > 0)
          cursorPos--;
        drawBtAddress(cursorPos);
      }

      //
      else if (anyKey == KEY_RIGHT)
      {
        if (cursorPos < 11)
          cursorPos++;
        drawBtAddress(cursorPos);
      }
    }
    osSleep(1);
  }

  osSleep(100);
  btSetMode(btCommandMode);
}


/*****************************************************************************
 *
 * Description:
 *    Prints the Bluetooth name and the current cursor position.
 *
 ****************************************************************************/
static void
drawBtName(tU8 cursor)
{
  tU32 col;

  lcdGotoxy(0,85);
  for(col=0; col<=15; col++)
  {
    if(col == cursor)
      lcdColor(BT_BACKGROUND_COLOR+1,0xe0);
    else
      lcdColor(BT_BACKGROUND_COLOR,0xfd);
    lcdPutchar(localName[col]);
  }
}


/*****************************************************************************
 *
 * Description:
 *    Function to handle setting of Bluetooth name. It first reads
 *    the current (Bluetooth) name from the BGB203-S06 unit.
 *
 ****************************************************************************/
static void
btSetName(void)
{
  tU8  rxChar;
  tU8  done;
  tU8  i;
  tU8  anyKey;
  tU8  cursorPos;
  tU32 timeStamp;
  volatile tU8  recvPos;
  tU8  error;
  tU8  recvBuf[RECV_BUF_LEN];

  //clear menu screen
  lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);
  lcdGotoxy(34,20);
  lcdColor(BT_BACKGROUND_COLOR,0xfd);
  lcdPuts("Set name:");
  lcdGotoxy(6,34);
  lcdPuts("Move cursor and");
  lcdGotoxy(6,48);
  lcdPuts("set each char.");
  lcdGotoxy(6,62);
  lcdPuts("Exit with c-key");

  //stop the BT handling process
  stopRecvProc = TRUE;

  //*************************************************************
  //* Get current name
  //*************************************************************
  osSleep(100);
  uart1SendString("+++");
  osSleep(100);
  uart1SendString("AT+BTLNM\r");

  //*************************************************************
  //* Get (receive and interpret) current name
  //*************************************************************
  timeStamp = ms;
  done = FALSE;
  recvPos = 0;
  while((done == FALSE) && ((ms - timeStamp) < 200))
  {
    //check if any character has been received from BT
    if (uart1GetChar(&rxChar) == TRUE)
    {
      printf("%c", rxChar);
      
      if (rxChar == 0x0a)
      {
        if (recvPos > 0)
          recvBuf[recvPos-1] = '\0';

        //evaluate received bytes
        if ((memcmp(recvBuf, "+BTLNM: \"", 9) == 0) && (recvPos < 27))
        {
          for(recvPos=0; recvPos<16; recvPos++)
            localName[recvPos] = ' ';
          localName[16] = '\0';

          recvPos = 9;
          while(recvBuf[recvPos] != '\"')
          {
            localName[recvPos-9] = recvBuf[recvPos];
            recvPos++;
          }
          done = TRUE;
        }
        recvPos = 0;
      }
      else if (recvPos < RECV_BUF_LEN)
        recvBuf[recvPos++] = rxChar;
    }
  }

  //start BT handling process again
  stopRecvProc = FALSE;
  osSemGive(&recvSem, &error);

  //*************************************************************
  //* Handle user key inputs
  //*************************************************************
  done = FALSE;
  cursorPos = 0;
  drawBtName(cursorPos);
  while(done == FALSE)
  {
    anyKey = checkKey();
    if (anyKey != KEY_NOTHING)
    {
      //exit and save new name
      if (anyKey == KEY_CENTER)
      {
        tMenu menu;

        menu.xPos = 5;
        menu.yPos = 35;
        menu.xLen = 6+(14*8);
        menu.yLen = 3*14+5;
        menu.noOfChoices = 2;
        menu.initialChoice = 0;
        menu.pHeaderText = "Exit";
        menu.headerTextXpos = 42;
        menu.pChoice[0] = "Cancel";
        menu.pChoice[1] = "Store new name";
        menu.bgColor       = 0;
        menu.borderColor   = 0x6d;
        menu.headerColor   = 0;
        menu.choicesColor  = 0xfd;
        menu.selectedColor = 0xe0;
        
        switch(drawMenu(menu))
        {
          case 0: break;  //Ignore changes
          case 1:         //Store new address
            //*************************************************************
            //* Trim ending spaces in new name
            //*************************************************************
            for(i=15; i>0; i--)
            {
              if (localName[i] == ' ')
                localName[i] = '\0';
              else
                break;
            }

            //*************************************************************
            //* Set new local name
            //*************************************************************
            uart1SendString("AT+BTLNM=\"");
            uart1SendString(localName);
            uart1SendString("\"\r");
            osSleep(100);

            //*************************************************************
            //* Save new settings in FLASH
            //*************************************************************
            uart1SendString("AT+BTFLS\r");
            break;
          default: break;
        }
        done = TRUE;
      }

      //
      else if (anyKey == KEY_UP)
      {
        localName[cursorPos]++;
        if (localName[cursorPos] > 127)
          localName[cursorPos] = ' ';
        drawBtName(cursorPos);
      }
      
      //
      else if (anyKey == KEY_DOWN)
      {
        localName[cursorPos]--;
        if (localName[cursorPos] < ' ')
          localName[cursorPos] = 127;
        drawBtName(cursorPos);
      }

      //
      else if (anyKey == KEY_LEFT)
      {
        if (cursorPos > 0)
          cursorPos--;
        drawBtName(cursorPos);
      }

      //
      else if (anyKey == KEY_RIGHT)
      {
        if (cursorPos < 15)
          cursorPos++;
        drawBtName(cursorPos);
      }
    }
    osSleep(1);
  }

  osSleep(100);
  btSetMode(btCommandMode);
}


/*****************************************************************************
 *
 * Description:
 *    Activates or deactivates the BGB203-S06 unit, by controlling the
 *    unit's reset signal.
 *    Switch between modes every time this function is called.
 *
 ****************************************************************************/
static void
btActiveSleep(void)
{
  //clear menu screen
  lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);
  lcdGotoxy(18,34);
  lcdColor(BT_BACKGROUND_COLOR,0xfd);

  if (btSleepState == FALSE)
  {
    tU8 messagePart1[] = "Deactivate";
    tU8 messagePart2[] = "BT module...";
    
    resetBT(TRUE);
    displayMessage(messagePart1, 3);
    lcdGotoxy(18,48);
    displayMessage(messagePart2, 3);
    
    btSleepState = TRUE;
  }
  else
  {
    tU8 messagePart1[] = "Activate BT";
    tU8 messagePart2[] = "module...";
    
    resetBT(FALSE);
    btSetMode(btCommandMode);

    displayMessage(messagePart1, 3);
    lcdGotoxy(18,48);
    displayMessage(messagePart2, 3);

    btSleepState = FALSE;
  }
  osSleep(200);
}


/*****************************************************************************
 *
 * Description:
 *    Set the BGB203-S06 unit in command or data mode.
 *
 ****************************************************************************/
static void
btSetMode(tBool commandMode)
{
  //check if set BGB203-S06 in command mode
  if (commandMode == TRUE)
  {
    osSleep(100);
    uart1SendString("+++");
    osSleep(150);
  }

  //set BGB203-S06 in data mode (server mode)
  else
  {
    osSleep(100);
    uart1SendString("+++");
    osSleep(100);
    uart1SendString("AT+BTSRV=1\r");
  }
}


/*****************************************************************************
 *
 * Description:
 *    Set the BGB203-S06 unit in command or data mode.
 *    Switch between modes every time this function is called.
 *
 ****************************************************************************/
static void
btMode(void)
{
  //clear menu screen
  lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);
  lcdGotoxy(14,26);
  lcdColor(BT_BACKGROUND_COLOR,0xfd);

  if (btCommandMode == FALSE)
  {
    tU8 messagePart1[] = "Activate";
    tU8 messagePart2[] = "command mode";
    tU8 messagePart3[] = "(unit cannot";
    tU8 messagePart4[] = "be discovered";
    tU8 messagePart5[] = "in this mode)";
    
    displayMessage(messagePart1, 3);
    lcdGotoxy(14,40);
    displayMessage(messagePart2, 3);
    lcdGotoxy(14,54);
    displayMessage(messagePart3, 3);
    lcdGotoxy(14,68);
    displayMessage(messagePart4, 3);
    lcdGotoxy(14,84);
    displayMessage(messagePart5, 3);

    btSetMode(TRUE);
    btCommandMode = TRUE;
  }
  else
  {
    tU8 messagePart1[] = "Activate";
    tU8 messagePart2[] = "data mode...";
    tU8 messagePart3[] = "(unit can now";
    tU8 messagePart4[] = "be discovered)";
    
    displayMessage(messagePart1, 3);
    lcdGotoxy(14,40);
    displayMessage(messagePart2, 3);
    lcdGotoxy(14,54);
    displayMessage(messagePart3, 3);
    lcdGotoxy(14,68);
    displayMessage(messagePart4, 3);

    btSetMode(FALSE);
    btCommandMode = FALSE;
  }
  osSleep(100);
}


/*****************************************************************************
 *
 * Description:
 *    Draw cursor in main menu
 *
 * Params:
 *    [in] cursor - Cursor positions
 *
 ****************************************************************************/
static void
drawBtMenuCursor(tU8 cursor)
{
  tU32 row;

  for(row=0; row<=5; row++)
  {
    lcdGotoxy(7,16+(14*row));
    if(row == btCursor)
      lcdColor(BT_BACKGROUND_COLOR,0xe0);
    else
      lcdColor(BT_BACKGROUND_COLOR,0xfd);
    
    switch(row)
    {
      case 0: lcdPuts("Inquiry"); break;
      case 1: lcdPuts("Set name"); break;
      case 2: lcdPuts("Set address"); break;
      case 3: if (btCommandMode == FALSE) lcdPuts("Set comm.mode"); else lcdPuts("Set data mode"); break;
      case 4: if (btSleepState == FALSE) lcdPuts("Deactivate BT"); else lcdPuts("Activate BT"); break;
      case 5: lcdPuts("Main menu"); break;
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
drawBtMenu(void)
{
  lcdColor(0,0);
  lcdClrscr();

  lcdRect(255, 255, 130, 130, BT_BACKBACKGROUND_COLOR);
  lcdRect(1, 16, 126, 84, BT_BACKGROUND_COLOR);

  lcdGotoxy(8,0);
  lcdColor(BT_BACKBACKGROUND_COLOR,0);
  lcdPuts("BLUETOOTH MENU");
  drawBtMenuCursor(btCursor);
  
  //display sleep status
  lcdGotoxy(0,101);
  lcdColor(BT_BACKBACKGROUND_COLOR,0x00);
  lcdPuts("BT stat:");
  lcdColor(BT_BACKBACKGROUND_COLOR,0xfd);
  if (btSleepState == FALSE)
    lcdPuts("Active");
  else
    lcdPuts("Inactive");

  lcdGotoxy(32,115);
  if (btCommandMode == FALSE)
    lcdPuts("Data mode");
  else
    lcdPuts("Command mode");
}


/*****************************************************************************
 *
 * Description:
 *    Implements the Bluetooth function menu
 *
 ****************************************************************************/
void
handleBt(void)
{
  tU8 exit = FALSE;

  //print menu
  drawBtMenu();
  
  while(exit == FALSE)
  {
    tU8 anyKey;

    anyKey = checkKey();
    if (anyKey != KEY_NOTHING)
    {
      //select specific function
      if (anyKey == KEY_CENTER)
      {
        switch(btCursor)
        {
          case 0: btInquiry(); break;
          case 1: btSetName(); break;
          case 2: btSetAddress(); break;
          case 3: btMode(); break;
          case 4: btActiveSleep(); break;
          case 5: exit = TRUE; break;
          default: break;
        }
        drawBtMenu();
      }
      
      //move cursor up
      else if (anyKey == KEY_UP)
      {
        if (btCursor > 0)
          btCursor--;
        else
          btCursor = 5;
        drawBtMenuCursor(btCursor);
      }
      
      //move cursor down
      else if (anyKey == KEY_DOWN)
      {
        if (btCursor < 5)
          btCursor++;
        else
          btCursor = 0;
        drawBtMenuCursor(btCursor);
      }
    }
    osSleep(1);
  }
}


/*****************************************************************************
 *
 * Description:
 *    Blocks the Bluetooth handling process to receive/send any characters
 *    from/to the BGB203-S06 unit.
 *
 ****************************************************************************/
void
blockBtProc(void)
{
  //stop the BT handling process
  stopRecvProc = TRUE;
  osSleep(10);
}


/*****************************************************************************
 *
 * Description:
 *    Release any blocking of the Bluetooth handling process
 *    (that receives/sends characters from/to the BGB203-S06 unit).
 *
 ****************************************************************************/
void
activateBtProc(void)
{
  tU8 error;

  //start BT handling process again
  stopRecvProc = FALSE;
  osSemGive(&recvSem, &error);

  //set to previous mode
  osSleep(100);
  btSetMode(btCommandMode);
}

/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * Description:
 *    Implements the pong game.
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include "lcd.h"
#include "key.h"
#include "bt.h"
#include "math.h"
#include "select.h"
#include "stdio.h"
#include <printf_P.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"


/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/
#define COLOR_BLACK 0x00
#define COLOR_WHITE 0xff
#define COLOR_RED   0xe0
#define COLOR_GREEN 0x1c
#define COLOR_BLUE  0x03

#define BACKGROUND_COLOR    COLOR_BLACK
#define DEFAULT_TEXT_COLOR  0xe0

#define SCREEN_HEIGHT 130
#define SCREEN_WIDTH  130

#define SCORE_HEIGHT  15

#define BOARD_WIDTH    SCREEN_WIDTH
#define BOARD_HEIGHT   (SCREEN_HEIGHT-SCORE_HEIGHT)
#define BOARD_TOP_Y    SCORE_HEIGHT+1
#define BOARD_BOTTOM_Y SCREEN_HEIGHT-1

#define BALL_MOVE_TIME 100
#define PLAYER_MOVE_TIME 30
#define STATUS_UPDATE_TIME (PLAYER_MOVE_TIME/2)

#define SPEED_INCR 1.3
#define SPEED_DECR 0.8

#define WIN_SCORE 3

#define PLAYER_WIDTH 3
#define PLAYER_SIZE  16
#define PLAYER_STEP  1

#define PLAYER_CLIENT 0
#define PLAYER_HOST   1

typedef struct _PongPlayer
{
  tU8 xPos;
  tU8 yPos;
  tU8 size;
  tU8 score;
  tU8 color;
  tU8 type;
  tU32 lastMove;
  tU8 key;
} tPongPlayer;

typedef struct _PongBall
{
  double xPos;
  double yPos;
  tU8 size;
  tU8 color;
  double xSpeed;
  double ySpeed;
  double sXPos;
  double sYPos;
} tPongBall;

#define GAME_TYPE_SINGLE 0
#define GAME_TYPE_DUAL_C 1
#define GAME_TYPE_DUAL_S 2

#define MAX_BT_UNITS 5

typedef struct
{
  tU8 active;
  tU8 btAddress[13];
  tU8 btName[17];
} tBtRecord;


/******************************************************************************
 * Local function prototypes
 *****************************************************************************/
static void activateServer(void);
static tBool checkIfClinetConnected(tU8 *pBtAddr);
static tBool searchServers(tU8 *pBtAddr);
static tBool connectToServer(tU8 *pBtAddr);
static tBool handleComm(void);
static tBool btSendAndRecvStatus(tU8 key, tBool force);


/******************************************************************************
 * Local (file global) variables
 *****************************************************************************/
static tBtRecord foundBtUnits[MAX_BT_UNITS];

static tU8 gameType;

static tPongPlayer player1 = 
{
  0,
  BOARD_TOP_Y+BOARD_HEIGHT/2-PLAYER_SIZE/2, 
  PLAYER_SIZE, 
  0, 
  COLOR_RED,   
  PLAYER_CLIENT
};

static tPongPlayer player2 = 
{
  BOARD_WIDTH-1-PLAYER_WIDTH, 
  BOARD_TOP_Y+BOARD_HEIGHT/2-PLAYER_SIZE/2, 
  PLAYER_SIZE, 
  0, 
  COLOR_GREEN, 
  PLAYER_HOST
};

static tPongBall ball = {0.0, 0.0, 2, COLOR_WHITE, 2.6, 1.0};

static tPongPlayer* pActivePlayer  = &player1;

static tPongPlayer* pServingPlayer = &player1;
static tBool serving = TRUE;

static tU32 lastMove = 0;
static tU32 lastStatus = 0;

static tU8 remoteClientKey = KEY_NOTHING;


/******************************************************************************
 * External variables
 *****************************************************************************/
extern tU32 ms;


/******************************************************************************
 * Implementation of local functions
 *****************************************************************************/

/*****************************************************************************
 *
 * Description:
 *    Draw current paddle position of specified player
 *
 ****************************************************************************/
static void 
paintPlayer(tPongPlayer* pPlayer) 
{
  lcdRect(pPlayer->xPos, pPlayer->yPos, PLAYER_WIDTH, pPlayer->size, pPlayer->color);
}


/*****************************************************************************
 *
 * Description:
 *    Draw the current score setting
 *
 ****************************************************************************/
static void 
paintScore(void)
{
  tU8 scoreStr[4];


  // left player
  scoreStr[0] = player1.score/100     + '0';
  scoreStr[1] = (player1.score/10)%10 + '0';
  scoreStr[2] = player1.score%10      + '0';
  scoreStr[3] = '\0';  

  lcdColor(BACKGROUND_COLOR, player1.color); 
  lcdGotoxy(1,0);
  lcdPuts(scoreStr);

  // name of the game
  lcdColor(BACKGROUND_COLOR, COLOR_BLUE); 
  lcdGotoxy(46, 0);
  lcdPuts("Pong");


  // right player
  scoreStr[0] = player2.score/100     + '0';
  scoreStr[1] = (player2.score/10)%10 + '0';
  scoreStr[2] = player2.score%10      + '0';
  scoreStr[3] = '\0';  

  lcdColor(BACKGROUND_COLOR, player2.color); 
  lcdGotoxy(102,0);
  lcdPuts(scoreStr);
}


/*****************************************************************************
 *
 * Description:
 *    Draw the entire game board including player paddles
 *
 ****************************************************************************/
static void 
paintGame(void)
{
  //clear screen
  lcdColor(BACKGROUND_COLOR, DEFAULT_TEXT_COLOR);
  lcdClrscr();

  paintScore();

  lcdRect(0, SCORE_HEIGHT, BOARD_WIDTH, 1, 0xff);

  // paint players
  paintPlayer(&player1);
  paintPlayer(&player2);
}


/*****************************************************************************
 *
 * Description:
 *    
 *
 ****************************************************************************/
static void
startNewServ(tPongPlayer* pPlayer)
{
  pServingPlayer = pPlayer;

  if (pPlayer == &player1)
    ball.xSpeed = 2.6;
  else
    ball.xSpeed = -2.6;

  ball.ySpeed = 1;

  paintScore();

  serving = TRUE;

  // temp
  pActivePlayer = pPlayer;    
}


/*****************************************************************************
 *
 * Description:
 *    
 *
 ****************************************************************************/
static void
moveBall(tU8 key)
{
  tPongPlayer* pDefending = NULL;
  tBool riskForHit = FALSE;
  tS8 dir = 1;

  if (serving &&
      (gameType == GAME_TYPE_SINGLE || gameType == GAME_TYPE_DUAL_S))
  {
    tU8 midPos = pServingPlayer->yPos + pServingPlayer->size/2 - ball.size/2;

    ball.xPos = pServingPlayer->xPos;
    if (pServingPlayer == &player1)
      ball.xPos+=PLAYER_WIDTH;
    else
      ball.xPos-=ball.size;

    if (ball.yPos != midPos)
    {
      // erase last position
      lcdRect(ball.xPos, ball.yPos, ball.size, ball.size, BACKGROUND_COLOR);

      ball.yPos = midPos;

      // paint ball
      lcdRect(ball.xPos, ball.yPos, ball.size, ball.size, ball.color);
    }

    if (/*key*/pServingPlayer->key == KEY_CENTER)
    {
      serving = FALSE;
    }

    return;
  }

  if (lastMove + BALL_MOVE_TIME > ms)
    return;

  // erase last position
  lcdRect((tU8)round(ball.xPos), (tU8)round(ball.yPos), ball.size, ball.size, BACKGROUND_COLOR);

  if (gameType == GAME_TYPE_SINGLE 
      || gameType == GAME_TYPE_DUAL_S)
  {
    // check if there is a risk hitting one of the pads
    if (ball.xSpeed > 0)
    {
      dir = -1;
      pDefending = &player2;
      riskForHit = (ball.xPos+ball.xSpeed+ball.size >= player2.xPos);
    }
    else
    {
      dir = 1;
      pDefending = &player1;
      riskForHit = (ball.xPos+ball.xSpeed < player1.xPos+PLAYER_WIDTH);
    }

    // check collision with wall
    if (ball.yPos+ball.ySpeed < BOARD_TOP_Y 
        || ball.yPos+ball.size+ball.ySpeed > BOARD_BOTTOM_Y)
    {
      ball.ySpeed = -ball.ySpeed;
    }


    // see if the pad is actually hit
    if (riskForHit)
    {
      double percIntoPad = 0;
      double angle = 0;
      double absSpeed = 0;

      tU8 yPos = (ball.ySpeed/ball.xSpeed)
                 * (pDefending->xPos - ball.xPos) + ball.yPos;

      if (yPos+ball.size >= pDefending->yPos && yPos <= pDefending->yPos+pDefending->size)
      {
        // calculate new angle
        percIntoPad = (ball.yPos+(ball.size/2)-pDefending->yPos)
                      / pDefending->size;

        angle = (-5*M_PI/12) + (10*percIntoPad*M_PI/12);
        absSpeed = sqrt(ball.xSpeed*ball.xSpeed+ball.ySpeed*ball.ySpeed);
        ball.xSpeed = dir*absSpeed*cos(angle);
        ball.ySpeed = absSpeed*sin(angle);

        // increase speed
        if (pDefending->key == KEY_CENTER)
        {
          ball.xSpeed *= SPEED_INCR;
          ball.ySpeed *= SPEED_INCR;
        }
        // decrease speed
        else
        {
          if (ball.xSpeed >= 2.0)
            ball.xSpeed *= SPEED_DECR;
          if (ball.ySpeed >= 2.0)
            ball.ySpeed *= SPEED_DECR;
        }
      }
    }

    ball.xPos += ball.xSpeed;
    ball.yPos += ball.ySpeed;

    // check if ball is outside board
    if (ball.xPos <= 0)
    {
      player2.score++;
      startNewServ(&player2);          
    }
    else if (ball.xPos >= BOARD_WIDTH-1)
    {
      player1.score++;
      startNewServ(&player1);          
    }
    else
    {
      // paint ball
      lcdRect(round(ball.xPos), round(ball.yPos), ball.size, ball.size, ball.color);

    }
  }

  else
  {
    // x and y position have been retreived from server
    ball.xPos = ball.sXPos;
    ball.yPos = ball.sYPos;
    
    lcdRect(round(ball.xPos), round(ball.yPos), ball.size, ball.size, ball.color);
  }

  lastMove = ms;
}


/*****************************************************************************
 *
 * Description:
 *    
 *
 ****************************************************************************/
static void
movePlayer(tPongPlayer* pPlayer, tU8 key)
{
  if (key == KEY_NOTHING && (gameType != GAME_TYPE_DUAL_C))
    return;

  if (pPlayer->lastMove + PLAYER_MOVE_TIME > ms)
    return;

  pPlayer->lastMove = ms;

  if (gameType == GAME_TYPE_SINGLE || gameType == GAME_TYPE_DUAL_S)
  {
    switch (key)
    {
    case KEY_UP:
      if (pPlayer->yPos-PLAYER_STEP <= BOARD_TOP_Y)
        return;

      pPlayer->yPos -= PLAYER_STEP;
      break;

    case KEY_RIGHT:
      pActivePlayer = &player2;
      return;

    case KEY_DOWN:
      if (pPlayer->yPos+PLAYER_STEP+pPlayer->size > BOARD_BOTTOM_Y)
        return;

      pPlayer->yPos += PLAYER_STEP;
      break;

    case KEY_LEFT:
      pActivePlayer = &player1;
      return;
    }
  }

  if (pPlayer->yPos > BOARD_TOP_Y)
  {
    lcdRect(pPlayer->xPos, BOARD_TOP_Y, PLAYER_WIDTH, 
            pPlayer->yPos-BOARD_TOP_Y, BACKGROUND_COLOR);
  }

  paintPlayer(pPlayer);

  if (pPlayer->yPos+pPlayer->size < BOARD_BOTTOM_Y)
  {
    lcdRect(pPlayer->xPos, pPlayer->yPos+pPlayer->size,PLAYER_WIDTH, 
            BOARD_BOTTOM_Y, BACKGROUND_COLOR);
  }

  return;
}


/*****************************************************************************
 *
 * Description:
 *    Display string one character at a time
 *
 ****************************************************************************/
static void
displayMessage(tU8 *pMessage, tU8 speed)
{
  while (*pMessage != '\0')
  {
    lcdPutchar(*pMessage++);
    osSleep(speed);
  }
}


/******************************************************************************
 * Implementation of public functions
 *****************************************************************************/

/*****************************************************************************
 *
 * Description:
 *    Implement Pong game
 *
 ****************************************************************************/
void
playPong(void)
{
  tBool gameOver = FALSE;
  tBool done = FALSE;
  tBool connected;
  tMenu menu;
  tU8   btAddress[13];

  //block BT process to communicate with BGB203
  blockBtProc();

  player1.score = 0;
  player2.score = 0;
  paintGame();

  //ask player which type of game (single/dual over Bluetooth)
  menu.xPos = 10;
  menu.yPos = 40;
  menu.xLen = 6+(13*8);
  menu.yLen = 5*14;
  menu.noOfChoices = 3;
  menu.initialChoice = 0;
  menu.pHeaderText = "Type of Game?";
  menu.headerTextXpos = 13;
  menu.pChoice[0] = "Single player";
  menu.pChoice[1] = "Dual - Client";
  menu.pChoice[2] = "Dual - Server";
  menu.bgColor       = 0;
  menu.borderColor   = 0x6d;
  menu.headerColor   = 0;
  menu.choicesColor  = 0xfd;
  menu.selectedColor = 0xe0;

  switch (drawMenu(menu))
  {
  case 0: gameType = GAME_TYPE_SINGLE; break;  //Single player
  case 1:  //Dual player client
    gameType = GAME_TYPE_DUAL_C;

    lcdRect(2, 16, 125, 84, 0x6d);
    lcdRect(4, 18, 121, 80, 0x00);
    lcdColor(0x00,0xfd);

    //search for available servers...
    if (TRUE == searchServers(btAddress))
    {
      //print "trying to connect" message...
      lcdRect(2, 16, 125, 84, 0x6d);
      lcdRect(4, 18, 121, 80, 0x00);
      lcdColor(0x00,0xfd);
      lcdGotoxy(8,18);
      lcdPuts("Connecting");

      //connect
      if (TRUE == connectToServer(btAddress))
        done = FALSE; //start playing as client
      else
      {
        lcdGotoxy(8,32);
        displayMessage("Failed to", 3);
        lcdGotoxy(8,46);
        displayMessage("connect...", 3);
        osSleep(150);

        //failed to connect
        done = TRUE;
      }
    }
    else
      done = TRUE;
    break;

  case 2:  //Dual player server
    {
      tU32 cnt = 0;
      tU8  key;

      //print "waiting for connection" message...
      lcdRect(2, 16, 125, 84, 0x6d);
      lcdRect(4, 18, 121, 80, 0x00);
      lcdColor(0x00,0xfd);
      lcdGotoxy(8,18);
      lcdPuts("Waiting for");
      lcdGotoxy(8,32);
      lcdPuts("client conn");

      gameType = GAME_TYPE_DUAL_S;
      activateServer();

      connected = FALSE;
      while (connected == FALSE)
      {
        key = checkKey();

        if (TRUE == checkIfClinetConnected(btAddress))
        {
          //print from which BT address
          lcdGotoxy(8,18);
          lcdPuts("Request from");
          lcdGotoxy(8,32);
          lcdPuts(btAddress);
          lcdPuts("  ");

          //ask player if start playing with connected client
          menu.xPos = 10;
          menu.yPos = 50;
          menu.xLen = 6+(13*8);
          menu.yLen = 5*14;
          menu.noOfChoices = 2;
          menu.initialChoice = 0;
          menu.pHeaderText = "Accept conn?";
          menu.headerTextXpos = 17;
          menu.pChoice[0] = "Start playing";
          menu.pChoice[1] = "Refuse";
          menu.bgColor       = 0;
          menu.borderColor   = 0x6d;
          menu.headerColor   = 0;
          menu.choicesColor  = 0xfd;
          menu.selectedColor = 0xe0;

          switch (drawMenu(menu))
          {
          case 0: uart1SendString("\nLETS START PLAYING\n"); done = FALSE; break;  //start playing as server
          case 1: uart1SendString("+++"); done = TRUE; break;                      //refuse connection attempt and cancel game
          default: break;
          }
          connected = TRUE;
        }

        //check if any key pressed to cancel waiting
        else if (key != KEY_NOTHING)
        {
          //cancel server and exit game
          done = TRUE;
          break;
        }

        else
        {
          //print activity indicator
          lcdGotoxy(96,30);
          lcdColor(0x00,0xfd);
          switch (cnt % 150)
          {
          case   0: lcdPuts("   ");break;
          case  25: lcdPuts(".  ");break;
          case  50: lcdPuts(".. ");break;
          case  75: lcdPuts("...");break;
          case 100: lcdPuts(" ..");break;
          case 125: lcdPuts("  .");break;
          default: break;
          }
          cnt++;
          osSleep(1);
        }
      }
    }
    break;
  default: break;
  }

  while (done == FALSE)
  {
    player1.score = 0;
    player2.score = 0;
    gameOver = FALSE;

    startNewServ(&player1);
    paintGame();

    while (!gameOver)
    {
      volatile tU8 key = checkKey2();

      if (gameType == GAME_TYPE_SINGLE)
      {
        player1.key = key;
        player2.key = key;
        movePlayer(pActivePlayer, key);
      }
      else
      {
        // get data from server 
        if (FALSE == btSendAndRecvStatus(key, FALSE))
        {
          gameOver = TRUE;

          menu.xPos = 2;
          menu.yPos = 40;
          menu.xLen = 6+(15*8);
          menu.yLen = 3*14;
          menu.noOfChoices = 1;
          menu.initialChoice = 0;
          menu.pHeaderText = "Lost connection";
          menu.headerTextXpos = 4;
          menu.pChoice[0] = "End game";
          menu.bgColor       = 0;
          menu.borderColor   = 0x6d;
          menu.headerColor   = 0;
          menu.choicesColor  = 0xfd;
          menu.selectedColor = 0xe0;

          switch (drawMenu(menu)) {
          case 0: done = TRUE; break;   //End game
          default: break;
          }
        }

//        if(gameType == GAME_TYPE_DUAL_S)
        {
          player2.key = key;
        }

        player1.key = remoteClientKey;

        if (gameType == GAME_TYPE_DUAL_C 
            || remoteClientKey == KEY_UP || remoteClientKey == KEY_DOWN)
        {
          movePlayer(&player1, remoteClientKey);
        }

        movePlayer(&player2, key);
      }

      moveBall(key);

      if (player1.score == WIN_SCORE || player2.score == WIN_SCORE)
      {
        // make sure that client is updated
        btSendAndRecvStatus(0, TRUE);

        gameOver = TRUE;

        menu.xPos = 10;
        menu.yPos = 40;
        menu.xLen = 6+(12*8);
        menu.yLen = 4*14;
        menu.noOfChoices = 2;
        menu.initialChoice = 0;
        menu.pHeaderText = "Game over!";
        menu.headerTextXpos = 20;
        menu.pChoice[0] = "Restart game";
        menu.pChoice[1] = "End game";
        menu.bgColor       = 0;
        menu.borderColor   = 0x6d;
        menu.headerColor   = 0;
        menu.choicesColor  = 0xfd;
        menu.selectedColor = 0xe0;

        switch (drawMenu(menu))
        {
        case 0: done = FALSE; break;  //Restart game
        case 1: done = TRUE; break;   //End game
        default: break;
        }
      }
    }
  }
  
  lcdRect(2, 16, 125, 84, 0x6d);
  lcdRect(4, 18, 121, 80, 0x00);
  lcdColor(0x00,0xfd);
  lcdGotoxy(8,18);
  lcdPuts("Exiting...");

  activateBtProc();
}



/******************************************************************************
 ******************************************************************************
 * BLUETOOTH HANDLING PARTS
 ******************************************************************************
 *****************************************************************************/
#define RECV_BUF_LEN 40
static tU8 recvPos;
static tU8 recvBuf[RECV_BUF_LEN];


/******************************************************************************
 * Activate server functionality
 *****************************************************************************/
static void
activateServer(void)
{
  osSleep(100);
  uart1SendString("+++");
  osSleep(100);
  uart1SendString("AT+BTCAN\r");
  osSleep(50);
  uart1SendString("AT+BTSRV=20,\"PingPongServer\"\r");
  recvPos = 0;
}

/******************************************************************************
 * Return: TRUE if connection established
 *         FALSE if no connection established
 *****************************************************************************/
static tBool
checkIfClinetConnected(tU8 *pBtAddr)
{
  tU8 rxChar;

  //check if any character has been received from BT
  if (uart1GetChar(&rxChar) == TRUE)
  {
    printf("%c", rxChar);

    if (rxChar == 0x0a)
    {
      if (recvPos > 0)
        recvBuf[recvPos-1] = '\0';

      //evaluate received bytes
      if ((memcmp(recvBuf, "CONNECT ", 8) == 0) && (recvPos == 21))
      {
        for (recvPos=0; recvPos<12; recvPos++)
        {
          *pBtAddr = recvBuf[recvPos + 8];
          pBtAddr++;
        }
        *pBtAddr = '\0';

        return TRUE;
      }
      recvPos = 0;
    }
    else if (recvPos < RECV_BUF_LEN)
      recvBuf[recvPos++] = rxChar;
  }

  //no connection established
  return FALSE;
}


/******************************************************************************
 * Return: TRUE if connection succeeded
 *         FALSE if connection failed
 *****************************************************************************/
static tBool
connectToServer(tU8 *pBtAddr)
{
  volatile tU32 timeStamp;
  tU8 connected;
  tU8 rxChar;
  tU32 cnt = 0;

  osSleep(100);
  uart1SendString("+++");
  osSleep(100);
  uart1SendString("AT+BTCAN\r");
  osSleep(50);
  uart1SendString("AT+BTCLT=\"");
  uart1SendString(pBtAddr);
  uart1SendString("\",20,3\r");
  osSleep(100);

  //wait for response "CONNECT <BTADDR>" 
  timeStamp = ms;
  connected = FALSE;
  recvPos = 0;
  while ((connected == FALSE) && ((ms - timeStamp) < 10000))
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
        if ((memcmp(recvBuf, "CONNECT ", 8) == 0) && (recvPos == 21))
        {
          connected = TRUE;
        }
        else if ((memcmp(recvBuf, "NO CARRIER", 10) == 0))
        {
          return FALSE;
        }
        recvPos = 0;
      }
      else if (recvPos < RECV_BUF_LEN)
        recvBuf[recvPos++] = rxChar;
    }
    osSleep(1);

    //print activity indicator
    lcdGotoxy(88,18);
    lcdColor(0x00,0xfd);
    switch (cnt % 150)
    {
    case   0: lcdPuts("   ");break;
    case  25: lcdPuts(".  ");break;
    case  50: lcdPuts(".. ");break;
    case  75: lcdPuts("...");break;
    case 100: lcdPuts(" ..");break;
    case 125: lcdPuts("  .");break;
    default: break;
    }
    cnt++;
  }

  //wait for accpet from server
  if (connected == TRUE)
  {
    //wait for response "LETS START PLAYING" 
    timeStamp = ms;
    connected = FALSE;
    recvPos = 0;
    while ((connected == FALSE) && ((ms - timeStamp) < 10000))
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
          if (memcmp(recvBuf, "LETS START PLAYING", 18) == 0)
          {
            return TRUE;
          }
          else if ((memcmp(recvBuf, "NO CARRIER", 10) == 0))
          {
            return FALSE;
          }
          recvPos = 0;
        }
        else if (recvPos < RECV_BUF_LEN)
          recvBuf[recvPos++] = rxChar;
      }
      osSleep(1);

      //print activity indicator
      lcdGotoxy(88,18);
      lcdColor(0x00,0xfd);
      switch (cnt % 150)
      {
      case   0: lcdPuts("   ");break;
      case  25: lcdPuts("*  ");break;
      case  50: lcdPuts("** ");break;
      case  75: lcdPuts("***");break;
      case 100: lcdPuts(" **");break;
      case 125: lcdPuts("  *");break;
      default: break;
      }
      cnt++;
    }
  }

  return FALSE;
}


static void
convertToDigits(tU8 *pBuf, tU8 value)
{
  static const tU8 toHex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  *pBuf++ = toHex[value >> 4];
  *pBuf   = toHex[value & 0x0f];
}

static tBool
decodeFromDigits(tU8 *pBuf, tU8 *pValue)
{
  tU8   result = 99;
  tBool valid  = TRUE;
  
  if ((*pBuf >= '0') && (*pBuf <= '9'))
  {
    result = *pBuf - '0';
  }
  else if ((*pBuf >= 'A') && (*pBuf <= 'F'))
  {
    result = *pBuf - 'A' + 10;
  }
  else
    valid = FALSE;

  result <<= 4;
  pBuf++;
  
  if ((*pBuf >= '0') && (*pBuf <= '9'))
  {
    result |= *pBuf - '0';
  }
  else if ((*pBuf >= 'A') && (*pBuf <= 'F'))
  {
    result |= *pBuf - 'A' + 10;
  }
  else
    valid = FALSE;
  
  *pValue = result;
  return valid;
}

/******************************************************************************
 * Return: TRUE if connection established
 *         FALSE if no connection established
 *****************************************************************************/
static tBool
handleComm(void)
{
  tU8 rxChar;

  //check if any character has been received from BT
  if (uart1GetChar(&rxChar) == TRUE)
  {
    if (rxChar == 0x0a)
    {
      //evaluate received bytes
      if ((recvBuf[0] == 'S') && (recvPos == 13))
      {
        if (gameType == GAME_TYPE_DUAL_C)
        {
          tU8 score1, score2, tmp;
          
          decodeFromDigits(&recvBuf[1],  &player1.yPos);
          decodeFromDigits(&recvBuf[3],  &player2.yPos);
          decodeFromDigits(&recvBuf[5],  &tmp);
          ball.sXPos = tmp;
          decodeFromDigits(&recvBuf[7],  &tmp);
          ball.sYPos = tmp;
          decodeFromDigits(&recvBuf[9],  &score1);
          decodeFromDigits(&recvBuf[11], &score2);
        
          if(player1.score != score1 || player2.score != score2)
          {
            player1.score = score1;
            player2.score = score2;
            paintScore();
          }
        }
        recvPos = 0;

        return TRUE;
      }

      if ((recvBuf[0] == 'C') && (recvPos == 3))
      {
        if (gameType == GAME_TYPE_DUAL_S)
        {
          decodeFromDigits(&recvBuf[1],  &remoteClientKey);
        }
        recvPos = 0;

        return TRUE;
      }

      else if ((memcmp(recvBuf, "NO CARRIER", 10) == 0))
      {
        return FALSE;
      }
      recvPos = 0;
    }
    else if (recvPos < RECV_BUF_LEN)
      recvBuf[recvPos++] = rxChar;
  }

  return TRUE;
}


static tBool 
btSendAndRecvStatus(tU8 key, tBool force)
{
  tU8 buf[14];
  
  if (FALSE == handleComm())
    return FALSE;

  if (lastStatus + STATUS_UPDATE_TIME > ms && !force)
  {
    return TRUE;
  }

  if (gameType == GAME_TYPE_DUAL_S)
  {
    // send status info
    // - player positions and ball position
    buf[0] = 'S';
    convertToDigits(&buf[1],  (tU8)player1.yPos);
    convertToDigits(&buf[3],  (tU8)player2.yPos);
    convertToDigits(&buf[5],  (tU8)round(ball.xPos));
    convertToDigits(&buf[7],  (tU8)round(ball.yPos));
    convertToDigits(&buf[9],  (tU8)player1.score);
    convertToDigits(&buf[11], (tU8)player2.score);
    buf[13] = 0x0a;
    uart1SendChars(buf, 14);

  }
  else
  {
    // send key state
    buf[0] = 'C';
    convertToDigits(&buf[1],  key);
    buf[3] = 0x0a;
    uart1SendChars(buf, 4);
  }

  lastStatus = ms;
  
  return TRUE;
}



/*****************************************************************************
 *
 * Description:
 *    Prints the list of 'found' Bluetooth units
 *
 ****************************************************************************/
static void
drawBtsFound(tU8 cursorPos)
{
  tU32 row;

  for(row=0; row<MAX_BT_UNITS; row++)
  {
    lcdGotoxy(2,30+(14*row));
    if(foundBtUnits[row].active == FALSE)
    {
      if (cursorPos == row)
        lcdColor(0x01,0xe0);
      else
        lcdColor(0x00,0xfd);
      lcdPuts("-");
    }
    else
    {
      if (cursorPos == row)
        lcdColor(0x01,0xe0);
      else
        lcdColor(0x00,0xfd);
      lcdPuts(foundBtUnits[row].btAddress);
    }
  }
}


/******************************************************************************
 * Perform a BT inquiry after PingPong servers
 *****************************************************************************/
static tBool
searchServers(tU8 *pBtAddr)
{
  tU8  rxChar;
  tU8  done;
  tU8  i;
  tU8  anyKey;
  tU8  cursorPos;
  volatile tU32 timeStamp;
  tU8  recvPos;
  tU8  recvBuf[RECV_BUF_LEN];
  tU8  foundBt;
  tU32 cnt;

  for(foundBt=0; foundBt<MAX_BT_UNITS; foundBt++)
  {
    foundBtUnits[foundBt].active = FALSE;
    foundBtUnits[foundBt].btName[0] = '\0';
  }

  //clear menu screen
  lcdRect(1, 16, 126, 84, 0x00);
  lcdGotoxy(18,16);
  lcdColor(0x00,0xfd);
  lcdPuts("Inquiry");

  //*************************************************************
  //* Start inquiry (during 6 seconds)
  //*************************************************************
  osSleep(100);
  uart1SendString("+++");
  osSleep(100);
  uart1SendString("AT+BTCAN\r");
  osSleep(50);
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
//            foundBtUnits[foundBt].active = TRUE;
            foundBt++;

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
    lcdColor(0x00,0xfd);
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
    cnt++;
  }

  //*************************************************************
  //* Get name of discovered bt units
  //*************************************************************
  for(i=0; i<foundBt; i++)
  {
    uart1SendString("AT+BTSDP=");
    uart1SendString(foundBtUnits[i].btAddress);
    uart1SendString("\r");

    timeStamp = ms;
    done = FALSE;
    recvPos = 0;

    while((done == FALSE) && ((ms - timeStamp) < 100000))
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
            tU8 *pStr;
            
            //seach for service name
            pStr = strchr(&recvBuf[10], '\"');
            if (pStr != NULL)
            {
              pStr++;

              //get string (to '"')
              if (0 == strncmp("PingPongServer", pStr, 14))
                foundBtUnits[i].active = TRUE;
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
      lcdColor(0x00,0xfd);
      switch(cnt % 150)
      {
        case   0: lcdPuts("   ");break;
        case  25: lcdPuts("*  ");break;
        case  50: lcdPuts("** ");break;
        case  75: lcdPuts("***");break;
        case 100: lcdPuts(" **");break;
        case 125: lcdPuts("  *");break;
        default: break;
      }
      cnt++;
    }
  }

  lcdGotoxy(74,16);
  lcdColor(0x00,0xfd);
  lcdPuts("-done");

  //*************************************************************
  //* Handle user key inputs (move between discovered units)
  //*************************************************************
  done = FALSE;
  cursorPos = 0;
  drawBtsFound(cursorPos);
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
        drawBtsFound(cursorPos);
      }
      
      //
      else if (anyKey == KEY_DOWN)
      {
        if (cursorPos < MAX_BT_UNITS-1)
          cursorPos++;
        else
          cursorPos = 0;
          drawBtsFound(cursorPos);
      }
    }
    osSleep(1);
  }
  
  if (foundBtUnits[cursorPos].active == TRUE)
  {
    strcpy(pBtAddr, foundBtUnits[cursorPos].btAddress);
    return TRUE;
  }
  else
    return FALSE;
}


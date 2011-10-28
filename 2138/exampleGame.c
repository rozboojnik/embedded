/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    exampleGame.c
 *
 * Description:
 *    Implementation of example game.
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
#include "select.h"
#include "brickWall_130x130c.h"


/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/
#define GAME_NOT_STARTED 0
#define GAME_RUNNING     1
#define GAME_OVER        2
#define GAME_END         3

#define BOARD_WIDTH  10
#define BOARD_HEIGHT 16

#define FIGURE_SIZE 4
#define NUM_OF_FIGURE 19


/*****************************************************************************
 * Local variables
 ****************************************************************************/
static tU8 gameStatus;
static tU8 score;
static tU8 cursor;

static tS32 shift[NUM_OF_FIGURE]={1,0,3,4,5,2,7,8,9,6,11,12,13,10,15,14,17,16,18};
static tS32 board[BOARD_WIDTH][BOARD_HEIGHT];
static tS32 currFigure;
static tS32 currXpos;
static tS32 currYpos;
static volatile tU32 lastUpdate;
static volatile tU32 delayTimeMs;


/*****************************************************************************
 * External variables
 ****************************************************************************/
extern tU32 ms;


char figures[NUM_OF_FIGURE][FIGURE_SIZE][FIGURE_SIZE]=
{{".X..",
  ".X..",
  ".X..",
  ".X.."},
 {"....",
  "XXXX",
  "....",
  "...."},

 {"XXX.",
  "X...",
  "....",
  "...."},
 {".XX.",
  "..X.",
  "..X.",
  "...."},
 {"..X.",
  "XXX.",
  "....",
  "...."},
 {".X..",
  ".X..",
  ".XX.",
  "...."},

 {"XXX.",
  "..X.",
  "....",
  "...."},
 {"..X.",
  "..X.",
  ".XX.",
  "...."},
 {"X...",
  "XXX.",
  "....",
  "...."},
 {".XX.",
  ".X..",
  ".X..",
  "...."},

 {".X..",
  "XXX.",
  "....",
  "...."},
 {".X..",
  ".XX.",
  ".X..",
  "...."},
 {"XXX.",
  ".X..",
  "....",
  "...."},
 {".X..",
  "XX..",
  ".X..",
  "...."},

 {"X...",
  "XX..",
  ".X..",
  "...."},
 {".XX.",
  "XX..",
  "....",
  "...."},

 {".X..",
  "XX..",
  "X...",
  "...."},
 {"XX..",
  ".XX.",
  "....",
  "...."},
 {"XX..",
  "XX..",
  "....",
  "...."}};


/*****************************************************************************
 *
 * Description:
 *    Check if current place for figure is valid
 *
 ****************************************************************************/
static tS8
isValid(tS32 x, tS32 y, tS32 shape)
{
  tS32 i,j;

  for (i=0; i<FIGURE_SIZE; i++)
    for (j=0; j<FIGURE_SIZE; j++)
      if((figures[shape][i][j] == 'X') &&
         ((i+x >= BOARD_WIDTH) || (j+y >= BOARD_HEIGHT) || (i+x < 0)))
        return 0;

  for (i=0; i<FIGURE_SIZE; i++)
    for (j=0; j<FIGURE_SIZE; j++)
      if((i+x < BOARD_WIDTH) &&
         (j+y < BOARD_HEIGHT) &&
         (figures[shape][i][j] == 'X') &&
         (board[i+x][j+y] != 0))
        return 0;

  return 1;
}


/*****************************************************************************
 *
 * Description:
 *    Draw/refresh game board
 *
 ****************************************************************************/
static void
drawGame(void)
{
  tS32 i,j;

  for (i=1; i<=BOARD_WIDTH; i++)
    for (j=1; j<=BOARD_HEIGHT; j++)
      if(board[i-1][j-1] != 0)
        lcdRectBrd(30+(i*6), 10+(j*6), 6, 6, 209,50,5);
      else
        lcdRect(30+(i*6), 10+(j*6), 6, 6, 0);
}


/*****************************************************************************
 *
 * Description:
 *    Insert one figure at a specific x,y position on the game board
 *
 ****************************************************************************/
static void
insertFigure(tS32 shape, tS32 x, tS32 y)
{
  tS32 i,j;
  
  for (i=0; i<FIGURE_SIZE; i++)
    for (j=0; j<FIGURE_SIZE; j++)
      if(figures[shape][i][j] == 'X')
        board[x+i][y+j] = 1;
}


/*****************************************************************************
 *
 * Description:
 *    Delete one figure at a specific x,y position on the game board
 *
 ****************************************************************************/
static void
deleteFigure(tS32 shape, tS32 x, tS32 y)
{
  tS32 i,j;
  
  for (i=0; i<FIGURE_SIZE; i++)
    for (j=0; j<FIGURE_SIZE; j++)
      if(figures[shape][i][j] == 'X')
        board[x+i][y+j] = 0;
}


/*****************************************************************************
 *
 * Description:
 *    Returns TRUE if one row of the board is full
 *
 ****************************************************************************/
static char
full(tS32 j)
{
  tS32 i;
  
  for(i=0; i<BOARD_WIDTH; i++)
    if(board[i][j] != 1)
      return 0;

  return 1;
}


/*****************************************************************************
 *
 * Description:
 *    Draw the current score
 *
 ****************************************************************************/
static void
drawScore(void)
{
  tU8 str[4];
  
  str[0] = score/100 + '0';
  str[1] = (score/10)%10 + '0';
  str[2] = score%10 + '0';
  str[3] = 0;
  if (str[0] == '0')
  {
    str[0] = ' ';
    if (str[1] == '0')
      str[1] = ' ';
  }
  lcdGotoxy(80,116);
  lcdPuts(str);
}


/*****************************************************************************
 *
 * Description:
 *    Check if time to update score
 *
 ****************************************************************************/
static void
checkScore(void)
{
  tS32 i,j,k;
  
  for (j=0; j<BOARD_HEIGHT; j++)
  {
    //check if one row is full
    if(full(j))
    {
      //if so, erase that row...
      for(k=j; k>0; k--)
        for(i=0; i<BOARD_WIDTH; i++)
          board[i][k] = board[i][k-1];
          
      //and update score
      score++;
      drawScore();
    }
  }
}


/*****************************************************************************
 *
 * Description:
 *    Returns TRUE if game is over
 *
 ****************************************************************************/
static char
gameOver(void)
{
  tS32 i;

  for(i=0; i<NUM_OF_FIGURE; i++)
    if(!isValid(3,0,i))
      return 1;

  return 0;
}


/*****************************************************************************
 *
 * Description:
 *    Calculate new update/advance time, based in current score
 *    The higher score, the shorter update time
 *
 ****************************************************************************/
static void
updateDelayTime(void)
{
  if (score >= 75)
    delayTimeMs = 200;
  else if (score >= 50)
    delayTimeMs = 300;
  else if (score >= 25)
    delayTimeMs = 350;
  else if (score >= 10)
    delayTimeMs = 400;
  else
    delayTimeMs = 500;
}


/*****************************************************************************
 *
 * Description:
 *    Advance the game, check if game is over
 *
 ****************************************************************************/
static void
advanceGame(void)
{
  insertFigure(currFigure, currXpos, currYpos);
  drawGame();
  deleteFigure(currFigure, currXpos, currYpos);

  /*******************************************************
   * Check if time to update figure position
   *******************************************************/
  updateDelayTime();
  if ((ms - lastUpdate) > delayTimeMs)
  {
    lastUpdate = ms;
    
    if (isValid(currXpos, currYpos+1, currFigure))
      currYpos++;

    //figure has hit bottom
    //insert new, random figure on the game board
    else
    {
      insertFigure(currFigure, currXpos, currYpos);
      currYpos  = 0;
      currXpos  = 3;
      currFigure = rand() % NUM_OF_FIGURE;

      //check if game is over
      if (gameOver())
        gameStatus = GAME_OVER;
    }

    //check score, i.e., if any rows are full
    checkScore();
  }
}


/*****************************************************************************
 *
 * Description:
 *    Draw game background and game board, initialize all variables
 *
 ****************************************************************************/
static void
setupGame (tBool drawBoard)
{
  tS32 i,j;

  //initialize random generator and reset score
  srand(ms);
  score = 0;

  //draw background picture
  lcdIcon(0, 0, 130, 130, _brickWall_130x130c[2], _brickWall_130x130c[3], &_brickWall_130x130c[4]);

  //draw game board
  if (drawBoard == TRUE)
  {
    lcdColor(1, 0xe0);
    lcdGotoxy(16, 116);
    lcdPuts(" Score:   0 ");

    lcdRect(34, 14, 64, 100, 3);
    lcdRect(36, 16, 60, 96, 0);
  }

  //initialise game board
  for (i=0; i<BOARD_WIDTH; i++)
    for (j=0; j<BOARD_HEIGHT; j++)
      board[i][j] = 0;

  currFigure = rand() % NUM_OF_FIGURE;
  currXpos  = 3;
  currYpos  = 0;
  insertFigure(currFigure, currXpos, currYpos);

  lastUpdate = ms;
}


/*****************************************************************************
 *
 * Description:
 *    Implements example game
 *    
 ****************************************************************************/
void
playExample(void)
{
  gameStatus = GAME_NOT_STARTED;
  cursor     = 0;

  setupGame(FALSE);
  lcdGotoxy(5,40);
  lcdColor(0,0xe0);
  lcdPuts("Press to start");

  while(gameStatus != GAME_END)
  {
    tU8 anyKey;

    anyKey = checkKey();
    switch(gameStatus)
    {
      case GAME_NOT_STARTED:
        if (anyKey != KEY_NOTHING)
        {
          gameStatus = GAME_RUNNING;
          setupGame(TRUE);
        }
        break;
        
      case GAME_RUNNING:
        if (anyKey != KEY_NOTHING)
        {
          if (anyKey == KEY_UP)
          {
            if(isValid(currXpos, currYpos, shift[currFigure]))
              currFigure = shift[currFigure];
          }
          else if (anyKey == KEY_DOWN)
          {
            if(isValid(currXpos, currYpos+1, currFigure))
              currYpos++;
          }
          else if (anyKey == KEY_RIGHT)
          {
            if(isValid(currXpos+1, currYpos, currFigure))
              currXpos++;
          }
          else if (anyKey == KEY_LEFT)
          {
            if(isValid(currXpos-1, currYpos, currFigure))
              currXpos--;
          }
          else if (anyKey == KEY_CENTER)
            gameStatus = GAME_OVER;

          advanceGame();
          osSleep(1);
        }
        else
        {
          advanceGame();
          osSleep(1);
        }
        break;
        
      case GAME_OVER:
      {
        tMenu menu;
        
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
        
        switch(drawMenu(menu))
        {
          case 0: gameStatus = GAME_RUNNING; setupGame(TRUE); break;  //Restart game
          case 1: gameStatus = GAME_END; break;                       //End game
          default: break;
        }
      }
        break;
        
      default:
        gameStatus = GAME_END;
        break;
    }
  }
}


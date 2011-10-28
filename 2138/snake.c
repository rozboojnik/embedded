/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Embedded Artists AB
 *
 * File:
 *    snake.c
 *
 * Description:
 *    Implements the snake game.
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


/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/
#define MAXROW 20
#define MAXCOL 31

#define SNAKE_START_COL 15
#define SNAKE_START_ROW  7
#define PAUSE_LENGTH     2


/*****************************************************************************
 * Local prototypes
 ****************************************************************************/
static void showScore();
static void addSegment();
static void setupLevel();
static void gotoxy(tU8 x, tU8 y, tU8 color);


/*****************************************************************************
 * Local variables
 ****************************************************************************/
static tS32  score;
static tS32  snakeLength;
static tS32  speed;
static tS32  obstacles;
static tS32  level;
static tBool firstPress;
static tS32  high_score = 0;
static tS8   screenGrid[MAXROW][MAXCOL];
static tS8   direction = KEY_RIGHT;

struct snakeSegment
{
  tS32 row;
  tS32 col;
} snake[100];


/*****************************************************************************
 * External variables
 ****************************************************************************/
extern tU32 ms;


/*****************************************************************************
 *
 * Description:
 *    Implement Snake game
 *
 ****************************************************************************/
void playSnake(void)
{
  tU8 keypress;
  tU8 done = FALSE;

  //game loop
  do
  {
    obstacles = 4;
    level     = 1;
    score     = 0;
    speed     = 14;
    srand(ms);        //Ensure random seed initiated
    setupLevel();

    //main loop
    do
    {
      tS32 i;
      
      //delay between snake moves
      osSleep(speed * PAUSE_LENGTH);
      
      //check if key press
      keypress = checkKey();
      if (keypress != KEY_NOTHING)
      {
        if ((keypress == KEY_UP)    ||
            (keypress == KEY_RIGHT) ||
            (keypress == KEY_DOWN)  ||
            (keypress == KEY_LEFT))
          direction = keypress;
      }

      //add a segment to the end of the snake
      addSegment();

      //removed last segment of snake
      gotoxy(snake[0].col, snake[0].row, 0);

      //remove last segment from the array
      for(i=1; i<=snakeLength; i++)
        snake[i-1] = snake[i];

      //display snake in yellow
      for (i=0; i<=snakeLength; i++)
        gotoxy(snake[i].col, snake[i].row, 0xfc);

      //if first press on each level, pause until a key is pressed
      if (firstPress == TRUE)
      {
        while(KEY_NOTHING == checkKey())
          ;
        firstPress = FALSE;
      }

      /* collision detection - walls (bad!) */
      if ((snake[snakeLength-1].row >= MAXROW) || (snake[snakeLength-1].row < 0) ||
          (snake[snakeLength-1].col >= MAXCOL) || (snake[snakeLength-1].col < 0) ||

      /* collision detection - obstacles (bad!) */
          (screenGrid[snake[snakeLength-1].row][snake[snakeLength-1].col] == 'x'))
        keypress = KEY_CENTER;

      //collision detection - snake (bad!)
      for (i=0; i<snakeLength-1; i++)
        if ((snake[snakeLength-1].row) == (snake[i].row) &&
            (snake[snakeLength-1].col) == (snake[i].col))
        {
          keypress = KEY_CENTER;   //exit loop - game over
          break;
        }

      //collision detection - food (good!)
      if (screenGrid[snake[snakeLength-1].row][snake[snakeLength-1].col] == '.')
      {
        //increase score and length of snake
        score += snakeLength * obstacles;
        showScore();
        snakeLength++;
        addSegment();

        //if length of snake reaches certain size, onto next level
        if (snakeLength == (level + 3) * 2)
        {
          score += level * 1000;
          obstacles += 2;          //add obstacles
          level++;
          
          //check if time to inclrease speed (every 5 levels)
          if ((level % 5 == 0) && (speed > 1))
            speed--;

          //draw next level
          setupLevel();
        }
      }
    } while (keypress != KEY_CENTER);
    
    //game over message
    if (score > high_score)
      high_score = score;
    showScore();

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
        case 0: done = FALSE; break;  //Restart game
        case 1: done = TRUE; break;   //End game
        default: break;
      }
    }

  } while (done == FALSE);
}


/*****************************************************************************
 *
 * Description:
 *    Initialize one level of the game. Draw game board.
 *
 ****************************************************************************/
void setupLevel()
{
  tS32 row, col, i;

  //clear screen
  lcdColor(0,0xe0);
  lcdClrscr();

  //draw frame
  lcdGotoxy(42,0);
  lcdPuts("Snake");

  //draw game board rectangle
  lcdRect(0, 14, (4*MAXCOL)+4, (4*MAXROW)+4, 3);
  lcdRect(2, 16, 4*MAXCOL,     4*MAXROW,     1);

  //set up global variables for new level
  snakeLength = level + 4;
  direction   = KEY_RIGHT;
  firstPress  = TRUE;

  //fill grid with blanks
  for(row=0; row<MAXROW; row++)
    for(col=0; col<MAXCOL; col++)
      screenGrid[row][col] = ' ';

  //fill grid with Xs and food
  for(i=0; i<obstacles*2; i++)
  {
    row = rand() % MAXROW;
    col = rand() % MAXCOL;
    if (i < obstacles)
      screenGrid[row][col] = 'x';  //= obstacle
    else
      screenGrid[row][col] = '.';  //= food
  }

  //create snake array of length snakeLength
  for(i=0; i<snakeLength; i++)
  {
    snake[i].row = SNAKE_START_ROW;
    snake[i].col = SNAKE_START_COL + i;
  }

  //draw game board
  for(row=0; row<MAXROW; row++)
  {
    for(col=0; col<MAXCOL; col++)
    {
      switch(screenGrid[row][col])
      {
        case ' ': gotoxy(col,row,0); break;
        case '.': gotoxy(col,row,0x1c); break;
        case 'x': gotoxy(col,row,0xe0); break;
        default: break;
      }
    }
  }

  showScore();
}


/*****************************************************************************
 *
 * Description:
 *    Draw current score
 *
 ****************************************************************************/
void showScore()
{
  tU8 str[13];
  
  str[0] = 'L';
  str[1] = ':';
  str[2] = level + '0';
  str[3] = ' ';
  str[4] = 'S';
  str[5] = ':';
  str[6] = score/100000 + '0';
  str[7] = (score/10000)%10 + '0';
  str[8] = (score/1000)%10 + '0';
  str[9] = (score/100)%10 + '0';
  str[10] = (score/10)%10 + '0';
  str[11] = score%10 + '0';
  str[12] = 0;
  
  //remove leading zeroes
  if (str[6] == '0')
  {
    str[6] = ' ';
    if (str[7] == '0')
    {
      str[7] = ' ';
      if (str[8] == '0')
      {
        str[8] = ' ';
        if (str[9] == '0')
        {
          str[9] = ' ';
          if (str[10] == '0')
          {
            str[10] = ' ';
          }
        }
      }
    }
  }
  lcdGotoxy(0,114);
  lcdPuts(str);
}


/*****************************************************************************
 *
 * Description:
 *    Add one snake segment
 *
 ****************************************************************************/
void addSegment()
{
  switch(direction)
  {
    case(KEY_RIGHT): snake[snakeLength].row = snake[snakeLength-1].row;
                     snake[snakeLength].col = snake[snakeLength-1].col+1;
                     break;
    case(KEY_LEFT) : snake[snakeLength].row = snake[snakeLength-1].row;
                     snake[snakeLength].col = snake[snakeLength-1].col-1;
                     break;
    case(KEY_UP)   : snake[snakeLength].row = snake[snakeLength-1].row-1;
                     snake[snakeLength].col = snake[snakeLength-1].col;
                     break;
    case(KEY_DOWN) : snake[snakeLength].row = snake[snakeLength-1].row+1;
                     snake[snakeLength].col = snake[snakeLength-1].col;
  }
}


/*****************************************************************************
 *
 * Description:
 *    Goto a specifc xy position and draw a 4x4 pixel rectangle in
 *    specified color
 *
 ****************************************************************************/
void gotoxy(tU8 x, tU8 y, tU8 color)
{
  lcdRect(2+(x*4), 16+(y*4), 4, 4, color);
}



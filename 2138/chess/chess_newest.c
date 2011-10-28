/******************************************************************************
 *
 * Copyright:
 *    (C) 2006 Rob Jansen
 *
 * File:
 *    chess.c
 *
 * Description:
 *    Chess program for the LPC2104 Color LCD Game Board.
 *
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
#include <lpc2xxx.h>
#include "../lcd.h"
#include "../key.h"
#include "../select.h"
#include "chess.h"
#include "pieces.h"
#include "M6502.h"
#include "usurpator.h"


/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/

/*
 * The memory for the M6502 simulator
 * was originaly one bit array (0x0000..0x1700) that contains both code
 * and data, but this had to be split such that the usurpator code is
 * used from Flash.
 * Having separate arrays is slower due to complexity of the Rd6502() and
 * Wr6502() functions but is not possible in full environment due to memory
 * constraints.
 * undefine COMPRESSED_MEM if more memory is available to speed up simulation
 */
#define COMPRESSED_MEM

#define FIELD_WIDTH     16           // Size of one field, must be bigger
#define FIELD_HEIGHT    16           // than the size of a piece
#define FIELD_SIZE		(FIELD_WIDTH * FIELD_HEIGHT)

#define FIGURE_WIDTH    12           // Size of the chess pieces
#define FIGURE_HEIGHT   12
#define FIGURE_SIZE     (FIGURE_WIDTH * FIGURE_HEIGHT)

#define WIDTH_OFFS       2           // Where the chess pieces are in a field
#define HEIGHT_OFFS      2

#define BLACK   ( (6<<5)+(4<<2)+2 )  // LCD colors used (RGB332)
#define WHITE   ( (7<<5)+(6<<2)+3 )

#define PIECE_1 ( (4<<5)+(5<<2)+2 )
#define PIECE_2 ( (0<<5)+(2<<2)+1 )
#define PIECE_3 ( (7<<5)+(7<<2)+3 )
#define PIECE_4 ( (6<<5)+(7<<2)+2 )

#if defined COMPRESSED_MEM
	#define M6502_MEM_START_HIGH 0x1400
	#define M6502_MEM_SIZE (0x1700-0x1200)
	#define M6502_MEM_TOP  0x1700
#else
	#define M6502_MEM_SIZE 0x1700        // Size of the M6502 memory
#endif
/*****************************************************************************
 * Local variables
 ****************************************************************************/

static tU8 bitmap[FIELD_SIZE];

#if defined COMPRESSED_MEM
	static tU8 ram[0x0200];
	static tU8 ramHigh[M6502_MEM_SIZE];
#else
	static tU8 ram[M6502_MEM_SIZE];
#endif

static tU8 board[64];         // Copy of the chess board from 6502 memory
                              // Used to check for fields that need a redraw
static M6502 cpuRegs;
static M6502 *R;

static tU8 halt;              // Flag to identify if simulation should be stopped
static tU8 border;            // Border (background) color used as color
static tU8 oldTo, oldFrom;

static tU8 cursor;

static char move[5];

static const char translate[] =
{
	'.','p','n','b','r','q','k','.','P','.','N','B','R','Q','K','.'
};

/*****************************************************************************
 * External variables
 ****************************************************************************/

/*****************************************************************************
 *
 * Description:
 *    Draw the given piece on the field on the LCD.
 *    Redraws also the background color of the field and draws an empty
 *    field if there is no (valid) piece on that field.
 *
 *    See pieces.h for the encoding used for each pixel.
 *
 * Params:
 *    [in] i     - The column, where 0 represents column A and 7 column H
 *    [in] j     - The row. Reversed so 0 represents row 8 and 7 row 1
 *                 (i,j) = (0,0) is A8
 *                         (0,7) is A1
 *                         (7,7) is H1
 *    [in] piece - The single character representing the piece
 *                 P, R, N, B, Q, K
 *
 * ToDo:
 *    + Change the assignment of y such that it also allows the user to play
 *      the black pieces (use 7-j to revert all rows)
 *    + Add selectable border or (background) color
 *      to allow drawing a 'cursor' for user input or computer feedback.
 ****************************************************************************/
static void
drawPiece(tU8 i, tU8 j, char piece)
{
	const tU8 *ptr;
	tU8 *bptr;
	tU16 x, y;
	tU8 background;

	if(border)
		background = border;
	else
		background = ((i+j)%2) ? BLACK : WHITE;

	switch(piece | 0x20)
	{
		case 'p': ptr=Pawn;   break;
		case 'r': ptr=Rook;   break;
		case 'n': ptr=Knight; break;
		case 'b': ptr=Bishop; break;
		case 'q': ptr=Queen;  break;
		case 'k': ptr=King;   break;
		default:  ptr=NULL;   break;
	}

	/*
	 * Create the bitmap of a complete field on the board.
	 * this includes the background.
	 */
	bptr = bitmap;
	for(x=0; x<((FIELD_WIDTH*HEIGHT_OFFS)+WIDTH_OFFS); x++)
	{
		*bptr = background;
		bptr++;
	}

	for(y=0; y<FIGURE_HEIGHT; y++)
	{
		if(ptr == NULL)
		{
			/*
			 * Empty square, show only the background
			 */
			for(x=0; x<FIGURE_WIDTH; x++)
			{
				*bptr = ((i+j)%2) ? BLACK : WHITE;
				bptr++;
			}
		}
		else
		{
			for(x=0; x<FIGURE_WIDTH; x++)
			{
				tU8 code;

				if(piece & 0x20)
					code = (*ptr)%5;
				else
					code = (*ptr)/5;

				switch(code)
				{
					case 0: *bptr=((i+j)%2) ? BLACK : WHITE; break;
					case 1: *bptr=PIECE_1;                   break;
					case 2: *bptr=PIECE_2;                   break;
					case 3: *bptr=PIECE_3;                   break;
					case 4: *bptr=PIECE_4;                   break;
				}
				ptr++;
				bptr++;
			}
		}
		for(x=0; x<(WIDTH_OFFS*2); x++)
		{
			*bptr = background;
			bptr++;
		}
	}
	for(x=0; x<((FIELD_WIDTH*HEIGHT_OFFS)+WIDTH_OFFS); x++)
	{
		*bptr = background;
		bptr++;
	}

	/*
	 * Place the field on the LCD
	 */
	x = i*FIELD_WIDTH;
	y = j*FIELD_HEIGHT; // Change to allow user to play black

	lcdIcon(x, y, FIELD_WIDTH, FIELD_HEIGHT, 0, 0, bitmap);
}

/*****************************************************************************
 *
 * Description:
 *    Draw the given piece on the field on the LCD.
 *    Redraws also the background color of the field and draws an empty
 *    field if there is no (valid) piece on that field.
 *
 *    See pieces.h for the encoding used for each pixel.
 *
 * Params:
 *    [in] i     - The column, where 0 represents column A and 7 column H
 *    [in] j     - The row. Reversed so 0 represents row 8 and 7 row 1
 *                 (i,j) = (0,0) is A8
 *                         (0,7) is A1
 *                         (7,7) is H1
 *    [in] piece - the piece in usurpator standard
 *    [in] color - The background color to be used
 *
 ****************************************************************************/
static void
drawBorderPiece(tU8 i, tU8 j, tU8 piece,tU8 color)
{
	border = color;
	drawPiece(i, j, translate[piece&0xf]);
	border = 0;
}

/*****************************************************************************
 *
 * Description:
 *    show the chess board on the LCD
 ****************************************************************************/
static void showBoard(void)
{
	int i;

	for(i=0;i<64;i++)
	{
		if(ram[i] != board[i])
		{
			board[i] = ram[i];
			switch(ram[i] & 0xf)
			{

				case 0xc: // White Rook
					drawPiece((i%8),i/8,'R'); break;
				case 0xa: // White Knight
					drawPiece((i%8),i/8,'N'); break;
				case 0xb: // White Bishop
					drawPiece((i%8),i/8,'B'); break;
				case 0xd: // White Queen
					drawPiece((i%8),i/8,'Q'); break;
				case 0xe: // White King
					drawPiece((i%8),i/8,'K'); break;
				case 0x8: // White Pawn
					drawPiece((i%8),i/8,'P'); break;
				case 0x4: // r
					drawPiece((i%8),i/8,'r'); break;
				case 0x2: // n
					drawPiece((i%8),i/8,'n'); break;
				case 0x3: // b
					drawPiece((i%8),i/8,'b'); break;
				case 0x5: // q
					drawPiece((i%8),i/8,'q'); break;
				case 0x6: // k
					drawPiece((i%8),i/8,'k'); break;
				case 0x1: // p
					drawPiece((i%8),i/8,'p'); break;
				case 0x0: // .
					drawPiece((i%8),i/8,'.'); break;
			}
		}
	}
}

/*****************************************************************************
 *
 * Description:
 *    Show the move made by drawing a border around both the fields.
 *    If an old move was present, remove the borders around the fields
 *
 * Params:
 *    [in] from  - First field in usurpator format
 *    [in] to    - Second field in usurpator format
 *    [in] color - The color of the border (RGB332)
 *
 ****************************************************************************/
void showMove(tU8 from, tU8 to, tU8 color)
{
	char move[5];

	if(!((from<64)&&(to<64))) return;

	move[0] = 'A'+(from & 7);
	move[1] = '0'+8-(from >> 3);
	move[2] = 'A'+(to & 7);
	move[3] = '0'+8-(to >> 3);
	move[4] = '\0';

	printf("[%s]", move);

	if(oldFrom <64)
	{
		drawBorderPiece(oldFrom & 7, oldFrom>>3, ram[oldFrom], 0);
		drawBorderPiece(oldTo & 7, oldTo>>3,ram[oldTo], 0);
	}
	oldFrom = from;
	oldTo   = to;

	drawBorderPiece(from & 7, from>>3, ram[from], color);
	drawBorderPiece(to & 7, to>>3, ram[to], color);
}

/*****************************************************************************
 *
 * Description:
 *    Get a move from the user.
 *    Draws a border around the current cursor position (where we were at
 *    the last move from the user) and use the joystick to move.
 *    When the center key is pressed, the cursor (border around a square)
 *    is changed from dark blue into light blue and the second "to" field
 *    can be selected.
 *
 * ToDo:
 *    sometimes a keypress is not detected.
 *    Figure out why and fix ...
 *
 ****************************************************************************/

void getUserMove()
{
	tU8 key; tU8 from=0; tU8 to=0;
	unsigned found = 0;

	drawBorderPiece(cursor & 7, cursor >> 3, ram[cursor], 3);
	/*
	 * Get piece to move
	 */
	do
	{
		key = checkKey();
		if(key != KEY_NOTHING)
		{
			drawBorderPiece(cursor & 7, cursor >> 3, ram[cursor], 0);
			switch(key)
			{
				case KEY_UP:
					if(cursor >= 8) cursor -= 8;
					break;
				case KEY_DOWN:
					if(cursor <56) cursor += 8;
					break;
				case KEY_LEFT:
					if(cursor & 7) cursor -= 1;
					break;
				case KEY_RIGHT:
					if((cursor&7)<7) cursor += 1;
					break;
				case KEY_CENTER:
					from = cursor;
					drawBorderPiece(cursor & 7, cursor>>3,ram[cursor], 3);
					found= 1;
					break;
			}
			printf("Cursor = %d\n", cursor);
			drawBorderPiece(cursor & 7, cursor >> 3, ram[cursor], 1);
		}
	} while (!found);
	/*
	 * Get position to move to
	 */
	found = 0;
	do
	{
		key = checkKey();
		if(key != KEY_NOTHING)
		{
			if(cursor != from)
				drawBorderPiece(cursor & 7, cursor >> 3, ram[cursor], 0);
			else
				drawBorderPiece(cursor & 7, cursor >> 3, ram[cursor], 3);
			switch(key)
			{
				case KEY_UP:
					if(cursor >= 8) cursor -= 8;
					break;
				case KEY_DOWN:
					if(cursor <56) cursor += 8;
					break;
				case KEY_LEFT:
					if(cursor & 7) cursor -= 1;
					break;
				case KEY_RIGHT:
					if((cursor&7)<7) cursor += 1;
					break;
				case KEY_CENTER:
					if(cursor != from)
					{
						// we are still at the from field.
						// ignore keypress (rumble on input line ?)
						to = cursor;
						found= 1;
					}
					break;
			}
			drawBorderPiece(cursor & 7, cursor >> 3, ram[cursor], 1);
		}
	} while(!found);

	/*
	 * Store move in text format
	 */
	move[0] = 'A'+(from & 7);
	move[1] = '0'+8-(from >> 3);
	move[2] = 'A'+(to & 7);
	move[3] = '0'+8-(to >> 3);
	move[4] = '\0';

	printf("[%s]", move);

}

/*****************************************************************************
 *
 * Description:
 *    Interface function for the M6502 simulator to write 6502 memory.
 *    6502 memory is defined starting at 0x0000 in RAM.
 *
 ****************************************************************************/
#if defined COMPRESSED_MEM
void
Wr6502(register word Addr, register byte Value)
{
	if(Addr < 0x200)
	{
		ram[Addr] = Value;
	}
	else if((Addr >=  M6502_MEM_START_HIGH) && (Addr < M6502_MEM_TOP))
	{
		ramHigh[Addr-M6502_MEM_START_HIGH] = Value;
	}
	else
	{
		printf("Invalid write to address 0x%x\n", Addr);
		halt=1;
	}
}
#else
void
Wr6502(register word Addr, register byte Value)
{
	if(Addr < M6502_MEM_SIZE)
	{
		ram[Addr] = Value;
	}
	else
	{
		printf("Invalid write to address 0x%x\n", Addr);
		halt=1;
	}
}
#endif
/*****************************************************************************
 *
 * Description:
 *    Interface function for the M6502 simulator to read 6502 memory.
 *    6502 memory is defined starting at 0x0000 in RAM.
 *
 ****************************************************************************/
#if defined COMPRESSED_MEM
byte
Rd6502(register word Addr)
{
	if(Addr < 0x200)
	{
		return ram[Addr];
	}
	else if (Addr < M6502_MEM_START_HIGH)
	{
		return usurpator[Addr-0x200];
	}
	else if(Addr < M6502_MEM_TOP)
	{
		return ramHigh[Addr-M6502_MEM_START_HIGH];
	}
	else
	{
		printf("Invalid read from address 0x%x\n", Addr);
		halt=1;
		return 0xEA; // return NOP
	}
}
#else
byte
Rd6502(register word Addr)
{
	if(Addr < M6502_MEM_SIZE)
	{
		return ram[Addr];
	}
	else
	{
		printf("Invalid read from address 0x%x\n", Addr);
		halt=1;
		return 0xEA; // return NOP
	}
}
#endif
/*****************************************************************************
 *
 * Description:
 *    This is the function that is called when the 6502 emulator reaches
 *    an invalid opcode.
 *    Some of the invalid opcodes are defined here to create a virtual com
 *    channel between the application and the chess engine.
 *    Currently it implements the AIM65 functions used by the original
 *    usurpator with  extra opcodes to draw the board and read a random
 *    value from the timer.
 *
 *    At the point where usurpator asks for the first character, the 6502
 *    code now first uses opcode 0x12 to redraw the board.
 *
 * Return value:
 *    0 - opcode was indeed invalid
 *    1 - opcode is handled by the patch function.
 *
 * ToDo:
 *    + To speed up drawing of the board and reduce memory, change this
 *      function and drawPiece() to accept the native usurpator chess piece
 *      codings instead of the ascii versions.
 *
 ****************************************************************************/
byte Patch6502(register byte Op,register M6502 *R)
{
	static int idx;

	switch(Op)
	{
		case 0x02: // instead of JSR INALL return char in A
#if 0
			/*
			 * The old text based input
			 *
			 * Kept this here for reference
			 */
			R->A = consolGetCh();
			if((R->A >= 'a') && (R->A <='z'))
			{
				R->A &= 0x5f;
			}
			printf("%c",R->A);
			if(R->A == 'Q') halt++;
#else
			if(idx == 0)
			{
				if(ram[0x88] == 0) // If color == black then play computer move
				{
					R->A = 0xd;
				} else
				{
					getUserMove();
					R->A = move[0];
					idx++;
				}
			} else              // Simulated serial input
			{
				if(idx<4)
				{
					R->A = move[idx];
					idx++;
				}
			}
#endif
			break;
		case 0x03: // instead of JSR CRLF
			printf("\n");
			break;
		case 0x04: // instead of JSR NUMA
			printf("%x%x", (R->A)>>4, (R->A)&0x0f);
			break;
		case 0x0B: // instead of JSR BLANK
			printf(" ");
			break;
		case 0x0C: // JSR WRAX
			printf("%x%x", (R->A)>>4, (R->A)&0x0f);
			printf("%x%x", (R->X)>>4, (R->X)&0x0f);
			break;
		case 0x0F: // JSR OUTALL
			printf("%c", R->A);
			break;
		case 0x12: // Print board, is placed right before the first JSR INALL
			showBoard();
			idx=0;
			if(ram[0x88] == 8)
			{
				// If it's now the user's turn, show usurpators move
				showMove(ram[0x96], ram[0x97], 0xe0);
			}
			break;
		case 0x13:
			R->A = T0TC;
			break;
		case 0x14:
			showMove(R->A, R->X, 0x40);
			break;
		default:
			return 0;
	}
	return 1;
}

void
playChess(void)
{
	unsigned a;

	printf("\n6502 Emulator with Usurpator II Chess program\n");

	/*
	 * Initialize the M6502 memory (copy program into memory)
	 * and initialize the copy of the chess board to contain
	 * invalid pieces (to force a redraw of the complete board)
	 */
#ifndef COMPRESSED_MEM
	for(a=0x200; a<0x1400; a++)
	{
		ram[a] = usurpator[a-0x0200];
	}
#endif
	for(a=0; a<64; a++)
	{
		board[a] = 0xff;
	}
	border  = 0;
	oldFrom = 64;
	oldTo   = 64;
	cursor  = 51;

	/*
	 * Initialize the M6502 simulator
	 * but specify our own start address since there is no memory
	 * at the location of the reset vector.
	 */

	R = &cpuRegs;

	Reset6502(R);
	R->PC.W = 0x0200;
	halt=0;

	printf("\nEnter a move as two fields (i.e. B1C3),");
	printf("\nUse <enter> to let the computer make a move");
	printf("\nor press Q to quit");

	/*
	 * Run the Usurpator chess program until we decide
	 * it is time to halt the simulator and exit the program.
	 * At exit print the stack pointer, PC and content of the
	 * stack for debugging purposes
	 */

	while(!halt)
	{
		Exec6502(R);
	}

	printf("S, PC =%x, %x\n",R->S, R->PC.W);
	for(a=R->S + 0x100; a<0x200; a++)
		printf("%x%x ", ram[a]>>4, ram[a]&0x0f);

	return;
}


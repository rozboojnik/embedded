
	Chess for the 'LPC2104 Color LCD Game with Bluetooth' board
	
	Copyright (C) 2006, Rob Jansen
	
	The usurpator and portable 6502 simulator are (C) by the respective
	owners and the code may only be used for non-commercial applications.
	
	
	
	
This chess program is the smallest chess engine, and the only one that will fit
in an LPC2104 that is known at this time.
The program is built around a modified version of the original usurpator II chess
engine and runs inside a portable 6502 simulator.

When the program is started, the chess board is shown and the user (playing white)
has to perform a move.

Both fields for the move have to be selected with the joystick, press the center
key to mark the first and second field. If the user makes an invalid move, the first
field is marked with a red border and a new move has to be selected.

Moves that the computer thinks about are shown in dark red, as soon as the computer
decided which move to make, the move is done and both fields are highlighted with
a light red border (and the blue cursor for the user is shown to ask for the next
user's move).

Since the usurpator II engine does not use any books for opening or end situations,
this program will not play as good as modern chess computers but it plays fairly
well and it is just fun to play chess on this small board.


The program uses the gaming environment from the LCD board published by
Embedded Artists at the support pages for this board, it has been tested with
both versions 1.7 and 1.8 of the program. To add the program to your own (EA/game)
environment, add the chess subdir to your environment and add 'chess' to the SUBDIRS
section in the makefile and 'chess/chess.a' to the LIBS.

Have fun!

	Rob
-------------------

Things still to do:

* smaller chess board to allow status line for printing text.

* currently the user always plays white and the computer black.
  add a selection to allow user to select to play black or white.
  
* translate the code into native ARM (Thumb) code to speed up the chess engine.
  
* add the option to exit and return to the main program loop.

* the user interface sometimes misses key pressed.
  Find problem and fix it.
  
* the user interface does not detect win/loose situations.
  you'll have to watch the board to detect this yourself.
This is the original usurpator II code for the chess program
in both assembly (usurpator.s65) and as motorola S record format (usurpator.hex).

Note that the usurpator.h file contains some changes not seen in the original code
It involves the calls to the original AIM65 monitor routines which are replaced
by some (invalid) opcodes which are handled by the Patch6502() function.

The 6502 macro assembler from Michal Kowalski (http://home.pacbell.net/michal_k/)
was used to create the motorola S records.

The M6502 simulator is from Michal Kowalski (http://home.pacbell.net/michal_k/)

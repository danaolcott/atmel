///////////////////////////////////////////
/*
Character offset table for writing fonts
with constant spacing.  Offset is the number
of pixels from trailing edge of character
to the overall width of each character.

As of 1/2/18, not sure how accurate these are,
need to check

 */
///////////////////////////////////////////


#include <stdint.h>
#include <stddef.h>

#include "offset.h"

//assume first entry is char 32, or space
const uint8_t offset[] =
{
	4, 		// ' '   char 32
	3,	    // '!'	  char 33
	2,	    // '"'	  char 34
	1,	    // '#'   char 35
	2,	    // '$'
	1,	    // '%'
	1,	    // '&'
	5,	    // '''
	3,	    // '('
	3,	    // ')'
	0,	    // '*'
	2,	    // '+'
	4,	    // ','
	2,	    // '-'
	4,	    // '.'
	1,	    // '/'
	1,	    // '0'
	2,	    // '1'
	2,	    // '2'
	2,	    // '3'
	1,	    // '4'
	2,	    // '5'
	2,	    // '6'
	2,	    // '7'
	2,	    // '8'
	2,	    // '9'
	4,	    // ':'
	4,	    // ';'
	3,	    // '<'
	2,	    // '='
	2,	    // '>'
	2,	    // '?'
	1,	    // '@'
	2,	    // 'A'
	1,	    // 'B'
	1,	    // 'C'
	1,	    // 'D'
	1,	    // 'E'
	1,	    // 'F'
	1,	    // 'G'
	2,	    // 'H'
	3,	    // 'I'
	1,	    // 'J'
	1,	    // 'K'
	1,	    // 'L'
	1,	    // 'M'
	1,	    // 'N'
	1,	    // 'O'
	1,	    // 'P'
	2,	    // 'Q'
	1,	    // 'R'
	2,	    // 'S'
	2,	    // 'T'
	2,	    // 'U'
	2,	    // 'V'
	1,	    // 'W'
	1,	    // 'X'
	2,	    // 'Y'
	1,	    // 'Z'
	3,	    // '['
	1,	    // backslash
	3,	    // ']'
	1,	    // '^'
	0,	    // '_'
	3,	    // '`'
	1,	    // 'a'
	1,	    // 'b'
	2,	    // 'c'
	1,	    // 'd'
	2,	    // 'e'
	2,	    // 'f'
	1,	    // 'g'
	1,	    // 'h'
	3,	    // 'i'
	2,	    // 'j'
	1,	    // 'k'
	3,	    // 'l'
	1,	    // 'm'
	2,	    // 'n'
	2,	    // 'o'
	1,	    // 'p'
	1,	    // 'q'
	1,	    // 'r'
	2,	    // 's'
	2,	    // 't'
	1,	    // 'u'
	2,	    // 'v'
	1,	    // 'w'
	1,	    // 'x'
	2,	    // 'y'
	2,	    // 'z'
	2,	    // '{'
	3,	    // '|'
	2,	    // '}'
	1,	    // '~'
};

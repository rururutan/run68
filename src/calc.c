/* $Id: calc.c,v 1.1.1.1 2001-05-23 11:22:05 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.3  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.2  1999/12/07  12:39:26  yfujii
 * *** empty log message ***
 *
 * Revision 1.2  1999/10/21  12:17:46  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include "run68.h"

// long add_long(long src, long dest, int size);
// long sub_long(long src, long dest, int size);

/*
 　機能：destにsrcをsizeサイズで加算する
 戻り値：答え
*/
long add_long(long src, long dest, int size) {

	long result;

	switch(size) {
		case S_BYTE:
			result = (dest & 0xffffff00) | (((dest & 0xff) + (src & 0xff)) & 0xff);
			break;
		case S_WORD:
			result = (dest & 0xffff0000) | (((dest & 0xffff) + (src & 0xffff)) & 0xffff);
			break;
		case S_LONG:
			result = dest + src;
			break;
	}

	return(result);
}

/*
 　機能：destからsrcをsizeサイズで減算する
 戻り値：答え
*/
long sub_long(long src, long dest, int size) {

	long result;

	switch(size) {
		case S_BYTE:
			result = (dest & 0xffffff00) | (((dest & 0xff) - (src & 0xff)) & 0xff);
			break;
		case S_WORD:
			result = (dest & 0xffff0000) | (((dest & 0xffff) - (src & 0xffff)) & 0xffff);
			break;
		case S_LONG:
			result = dest - src;
			break;
	}

	return(result);
}

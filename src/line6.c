/* $Id: line6.c,v 1.1.1.1 2001-05-23 11:22:07 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.6  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.5  1999/12/07  12:44:37  yfujii
 * *** empty log message ***
 *
 * Revision 1.5  1999/11/22  03:57:08  yfujii
 * Condition code calculations are rewriten.
 *
 * Revision 1.3  1999/10/28  06:34:08  masamichi
 * Modified trace behavior
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include "run68.h"

/*
 　機能：６ライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	line6( char *pc_ptr )
{
        char	code;
	char	cond ;
	char	disp ;
	short	disp_w ;

	code = *pc_ptr;
	cond = (*pc_ptr & 0x0F) ;
	disp = *(pc_ptr + 1) ;
	pc += 2 ;

	if ( cond == 0x01 ) {	/* bsr */
		ra [ 7 ] -= 4 ;
		if ( disp == 0 ) {
			disp_w = (short)imi_get( S_WORD ) ;
			mem_set( ra [ 7 ], pc, S_LONG ) ;
			pc += (disp_w - 2) ;
		} else {
			mem_set( ra [ 7 ], pc, S_LONG ) ;
			pc += disp ;
		}
		return( FALSE ) ;
	}

	if ( get_cond( cond ) == TRUE ) {
		if ( disp == 0 ) {
			disp_w = (short)imi_get( S_WORD ) ;
			pc += (disp_w - 2) ;
		} else {
			pc += disp ;
		}
	} else {
		if ( disp == 0 )
			pc += 2 ;
	}

	return( FALSE ) ;
}

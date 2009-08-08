/* $Id: line7.c,v 1.2 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.4  1999/12/07  12:44:50  yfujii
 * *** empty log message ***
 *
 * Revision 1.4  1999/11/22  03:57:08  yfujii
 * Condition code calculations are rewriten.
 *
 * Revision 1.3  1999/10/20  04:00:59  masamichi
 * Added showing more information about errors.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include "run68.h"

/*
 　機能：７ライン命令(moveq)を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	line7( char *pc_ptr )
{
	char	code ;
	char	reg ;
	char	data ;

	code = *(pc_ptr++) ;
	pc += 2 ;
	if ( (code & 0x01) != 0 ) {
		err68a( "おかしな命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}
	reg = ((code >> 1) & 0x07) ;
	data = *pc_ptr ;
	if ( data < 0 ) {
		rd [ reg ] = (0xFFFFFF00 | data) ;
	} else {
		rd [ reg ] = data ;
	}

	/* フラグの変化 */
	general_conditions(rd[reg], S_LONG);

#ifdef	TRACE
	printf( "trace: moveq    src=%d PC=%06lX\n", data, pc ) ;
#endif

	return( FALSE ) ;
}

/* $Id: lined.c,v 1.1.1.1 2001-05-23 11:22:08 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.5  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.4  1999/12/07  12:45:54  yfujii
 * *** empty log message ***
 *
 * Revision 1.4  1999/11/22  03:57:08  yfujii
 * Condition code calculations are rewriten.
 *
 * Revision 1.3  1999/10/20  04:14:48  masamichi
 * Added showing more information about errors.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include "run68.h"

static	int	Adda( char, char ) ;
static	int	Addx( char, char ) ;
static	int	Add1( char, char ) ;
static	int	Add2( char, char ) ;

/*
 　機能：Ｄライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	lined( char *pc_ptr )
{
	char	code1, code2 ;

	code1 = *(pc_ptr++) ;
	code2 = *pc_ptr ;
	pc += 2 ;

	if ( (code2 & 0xC0) == 0xC0 ) {
		return( Adda( code1, code2 ) ) ;
	} else {
		if ( (code1 & 0x01) == 1 ) {
			if ( (code2 & 0x30) == 0x00 )
				return( Addx( code1, code2 ) ) ;
			else
				return( Add1( code1, code2 ) ) ;
		} else {
			return( Add2( code1, code2 ) ) ;
		}
	}
}

static	int	Adda( char code1, char code2 )
{
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	char	size ;
	long	src_data ;
	long	save_pc ;

	save_pc = pc ;
	dst_reg  = ((code1 & 0x0E) >> 1) ;
	if ( (code1 & 0x01) == 0x01 )
		size = S_LONG ;
	else
		size = S_WORD ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;


	/* ソースのアドレッシングモードに応じた処理 */
	if (size == S_BYTE) {
		err68a( "不正な命令: adda.b <ea>, An を実行しようとしました。", __FILE__, __LINE__ ) ;
		return(TRUE);
	} else if (get_data_at_ea(EA_All, mode, src_reg, size, &src_data)) {
		return(TRUE);
	}

	if ( size == S_WORD ) {
		if ( (src_data & 0x8000) != 0 ) {
			src_data |= 0xFFFF0000 ;
		} else {
			src_data &= 0x0000FFFF ;
		}
	}

	ra [ dst_reg ] += src_data ;

#ifdef	TRACE
	printf( "trace: adda.%c   src=%d PC=%06lX\n",
		size_char [ size ], src_data, save_pc ) ;
#endif

	return( FALSE ) ;
}

static	int	Addx( char code1, char code2 )
{
	char	size ;
	char	src_reg ;
	char	dst_reg ;
	short	save_z ;
	short	save_x ;
	long	dest_data;

	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;

	if ( (code2 & 0x08) != 0 ) {
		/* -(An), -(An) */
		err68a( "未定義命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

	dest_data = rd [ dst_reg ];

	save_z = CCR_Z_REF() != 0 ? 1 : 0;
	save_x = CCR_X_REF() != 0 ? 1 : 0;
	rd [ dst_reg ] = add_long(rd [ src_reg ] + save_x, dest_data , size ) ;

	/* フラグの変化 */
	add_conditions(rd[src_reg], dest_data, rd[dst_reg], size, save_z);

#ifdef	TRACE
	switch( size ) {
		case S_BYTE:
			rd [ 8 ] = ( rd [ src_reg ] & 0xFF ) ;
			break ;
		case S_WORD:
			rd [ 8 ] = ( rd [ src_reg ] & 0xFFFF) ;
			break ;
		default:	/* S_LONG */
			rd [ 8 ] = rd [ src_reg ] ;
			break ;
	}
	printf( "trace: addx.%c   src=%d PC=%06lX\n",
		size_char [ size ], rd [ 8 ], pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：add Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Add1( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	short	disp = 0 ;
	long	save_pc ;
	int	work_mode;
	long	src_data;
	long	dest_data;

	save_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = ((code1 & 0x0E) >> 1) ;
	dst_reg = (code2 & 0x07) ;
	size = ((code2 >> 6) & 0x03) ;

	if (get_data_at_ea(EA_All, EA_DD, src_reg, size, &src_data)) {
		return(TRUE);
	}

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, dst_reg, size, &dest_data)) {
		return(TRUE);
	}

	/* Sub演算 */
	rd [ 8 ] = add_long(src_data, dest_data, size ) ;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableMemory, work_mode, dst_reg, size, rd[8])) {
		return(TRUE);
	}

	/* フラグの変化 */
	sub_conditions(src_data, dest_data, rd[ 8 ], size, 1);

#ifdef	TRACE
	switch( size ) {
		case S_BYTE:
			rd [ 8 ] = ( rd [ src_reg ] & 0xFF ) ;
			break ;
		case S_WORD:
			rd [ 8 ] = ( rd [ src_reg ] & 0xFFFF) ;
			break ;
		default:	/* S_LONG */
			rd [ 8 ] = rd [ src_reg ] ;
			break ;
	}
	printf( "trace: add.%c    src=%d PC=%06lX\n",
		size_char [ size ], rd [ 8 ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：add <ea>,Dn命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Add2( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	src_data ;
	long	save_pc ;
	long	dest_data;

	save_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;


	if (mode == EA_AD && size == S_BYTE) {
		err68a( "不正な命令: sub.b An, Dn を実行しようとしました。", __FILE__, __LINE__ ) ;
		return(TRUE);
	} else if (get_data_at_ea(EA_All, mode, src_reg, size, &src_data)) {
		return(TRUE);
	}

	if (get_data_at_ea(EA_All, EA_DD, dst_reg, size, &dest_data)) {
		return(TRUE);
	}
    switch(size)
    {
    case 0:
    	rd[dst_reg] = (rd[dst_reg] & 0xffffff00) | (add_long(src_data, dest_data, size) & 0xff);
        break;
    case 1:
    	rd[dst_reg] = (rd[dst_reg] & 0xffff0000) | (add_long(src_data, dest_data, size) & 0xffff);
        break;
    case 2:
    	rd[dst_reg] = add_long(src_data, dest_data, size ) ;
        break;
    default:
        return TRUE;
    }
	/* フラグの変化 */
	add_conditions(src_data, dest_data, rd[ dst_reg ], size, 1);

#ifdef	TRACE
	printf( "trace: add.%c    src=%d PC=%06lX\n",
		size_char [ size ], src_data, save_pc ) ;
#endif

	return( FALSE ) ;
}

/* $Id: linec.c,v 1.2 2009-08-05 14:44:33 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:08  masamic
 * First imported source code and docs
 *
 * Revision 1.7  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.6  1999/12/07  12:45:42  yfujii
 * *** empty log message ***
 *
 * Revision 1.6  1999/11/22  03:57:08  yfujii
 * Condition code calculations are rewriten.
 *
 * Revision 1.4  1999/11/01  12:10:21  masamichi
 * Maybe correct error at code: $C073
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

static	int	And1( char, char ) ;
static	int	And2( char, char ) ;
static	int	Exg( char, char ) ;
static	int	Mulu( char, char ) ;
static	int	Muls( char, char ) ;

/*
 　機能：Ｃライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	linec( char *pc_ptr )
{
	char	code1, code2 ;

	code1 = *(pc_ptr++) ;
	code2 = *pc_ptr ;
	pc += 2 ;
	if ( (code1 & 0x01) == 0 ) {
		if ( (code2 & 0xC0) == 0xC0 )
			return( Mulu( code1, code2 ) ) ;
		return( And2( code1, code2 ) ) ;
	} else {
		if ( (code2 & 0xC0) == 0xC0 )
			return( Muls( code1, code2 ) ) ;
		if ( (code2 & 0xF0) == 0x00 ) {
			/* abcd */
			char	src_reg = (code2 & 0x7);
			char	dst_reg = ((code1 & 0xE) >> 1);
			char	size = 0;	/* S_BYTE 固定 */
			long	src_data;
			long	dst_data;
			long	low;
			long	high;
			long	kekka;
			long	X;

			if ( (code2 & 0x8) != 0 ) {
				/* -(am),-(an); */
				if ( get_data_at_ea(EA_All, EA_AIPD, src_reg, size, &src_data) ) {
					return( TRUE );
				}
				if ( get_data_at_ea(EA_All, EA_AIPD, dst_reg, size, &dst_data) ) {
					return( TRUE );
				}
			}else{
				/* dm,dn; */
				if ( get_data_at_ea(EA_All, EA_DD, src_reg, size, &src_data) ) {
					return( TRUE );
				}
				if ( get_data_at_ea(EA_All, EA_DD, dst_reg, size, &dst_data) ) {
					return( TRUE );
				}
			}

			X = (CCR_X_REF() != 0) ? 1 : 0;

			low = (src_data & 0x0f) + (dst_data & 0x0f) + X;
			if ( low >= 0x0a ) {
				low += 0x06;
			}

			high = (src_data & 0xf0) + (dst_data & 0xf0) + (low & 0xf0);
			if ( high >= 0xa0 ) {
				high += 0x60;
			}

			if ( high >= 0x100 ) {
				CCR_X_ON();
				CCR_C_ON();
			}else{
				CCR_X_OFF();
				CCR_C_OFF();
			}

			kekka = (high & 0xf0) | (low & 0x0f);

			/* 0 以外の値になった時のみ、Z フラグをリセットする */
			if ( kekka != 0 ) {
				CCR_Z_OFF();
			}

			/* Nフラグは結果に応じて立てる */
			if ( kekka & 0x80 ) {
				CCR_N_ON();
			}else{
				CCR_N_OFF();
			}

			/* Vフラグ */
			if ( dst_data < 0x80 ) {
				if ( 5 <= (dst_data & 0x0f) ) {
					if ( (0x80 <= kekka) && (kekka <= 0x85) ) {
						CCR_V_ON();
					}else{
						CCR_V_OFF();
					}
				}else{
					if ( (0x80 <= kekka) && (kekka <= (0x80 + (dst_data & 0x0f))) ) {
						CCR_V_ON();
					}else{
						CCR_V_OFF();
					}
				}
			}else{
				if ( (0x80 <= kekka) && (kekka <= dst_data) ) {
					CCR_V_ON();
				}else{
					CCR_V_OFF();
				}
			}

			dst_data = kekka;

			if ( (code2 & 0x8) != 0 ) {
				/* -(am),-(an); */
				if ( set_data_at_ea(EA_All, EA_AI, dst_reg, size, dst_data) ) {
					return( TRUE );
				}
			}else{
				/* dm,dn; */
				if ( set_data_at_ea(EA_All, EA_DD, dst_reg, size, dst_data) ) {
					return( TRUE );
				}
			}

			return( FALSE );
/*
			err68a( "未定義命令(abcd)を実行しました", __FILE__, __LINE__ ) ;
			return( TRUE ) ;	 abcd 
*/
		}
		if ( (code2 & 0x30) == 0x00 ) {
			return( Exg( code1, code2 ) ) ;
		}
		return( And1( code1, code2 ) ) ;
	}
}

/*
 　機能：and Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	And1( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	short	disp = 0 ;
	long	data ;
	long	save_pc ;
	int	work_mode;
	long	src_data;

	save_pc = pc ;
	size = ((code2 >> 6) & 0x03) ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = ((code1 & 0x0E) >> 1) ;
	dst_reg = (code2 & 0x07) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_All, EA_DD, src_reg, size, &src_data)) {
		return(TRUE);
	}

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, dst_reg, size, &data)) {
		return(TRUE);
	}

	/* AND演算 */
	data &= src_data;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableMemory, work_mode, dst_reg, size, data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	general_conditions(data, size);

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
	printf( "trace: and.%c    src=0x%08X PC=%06lX\n",
		size_char [ size ], rd [ 8 ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：and <ea>,Dn命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	And2( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	src_data ;
	long	save_pc ;
    int     work_pc;
	long	data;

	save_pc = pc ;
	work_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;


	/* ソースのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_Data, mode, src_reg, size, &src_data)) {
		return(TRUE);
	}

	/* デスティネーションのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_All, EA_DD, dst_reg, size, &data)) {
		return(TRUE);
	}

	/* AND演算 */
	data &= src_data;

	if (set_data_at_ea(EA_All, EA_DD, dst_reg, size, data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	general_conditions(data, size);

	return( FALSE ) ;
}

/*
 　機能：exg命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Exg( char code1, char code2 )
{
	char	src_reg ;
	char	dst_reg ;
	char	mode ;
	long	tmp ;

	mode = ((code2 & 0xF8) >> 3) ;
	src_reg = ((code1 & 0x0E) >> 1) ;
	dst_reg = (code2 & 0x07) ;

	switch( mode ) {
		case 0x08:
			tmp = rd [ src_reg ] ;
			rd [ src_reg ] = rd [ dst_reg ] ;
			rd [ dst_reg ] = tmp ;
			break ;
		case 0x09:
			tmp = ra [ src_reg ] ;
			ra [ src_reg ] = ra [ dst_reg ] ;
			ra [ dst_reg ] = tmp ;
			break ;
		case 0x11:
			tmp = rd [ src_reg ] ;
			rd [ src_reg ] = ra [ dst_reg ] ;
			ra [ dst_reg ] = tmp ;
			break ;
		default:
			err68a( "EXG: 不正なOPモードです。", __FILE__, __LINE__ ) ;
			return( TRUE ) ;
	}

#ifdef	TRACE
	printf( "trace: exg      PC=%06lX\n", pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：mulu命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Mulu( char code1, char code2 )
{
	char	src_reg ;
	char	dst_reg ;
	char	mode ;
	UShort	src_data ;
	UShort	dst_data ;
	ULong	ans ;
	long	save_pc ;
	long	src_data_l;

	save_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;

	dst_data = (rd [ dst_reg ] & 0xFFFF) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_Data, mode, src_reg, S_WORD, &src_data_l)) {
		return(TRUE);
	}
	src_data = (UShort)src_data_l;

	ans = src_data * dst_data ;
	rd [ dst_reg ] = ans ;
#ifdef	TRACE
	printf( "trace: mulu     src=%u PC=%06lX\n", src_data, save_pc ) ;
#endif

	/* フラグの変化 */
	general_conditions(ans, S_LONG);

	return( FALSE ) ;
}

/*
 　機能：muls命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Muls( char code1, char code2 )
{
	char	src_reg ;
	char	dst_reg ;
	char	mode ;
	short	src_data ;
	short	dst_data ;
	long	ans ;
	long	save_pc ;
	long	src_data_l;

	save_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;

	dst_data = (rd [ dst_reg ] & 0xFFFF) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_Data, mode, src_reg, S_WORD, &src_data_l)) {
		return(TRUE);
	}
	src_data = (UShort)src_data_l;

	ans = src_data * dst_data ;
	rd [ dst_reg ] = ans ;

#ifdef	TRACE
	printf( "trace: muls     src=%d PC=%06lX\n", src_data, save_pc ) ;
#endif

	/* フラグの変化 */
	general_conditions(ans, S_LONG);

	return( FALSE ) ;
}

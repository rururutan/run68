/* $Id: line4.c,v 1.1.1.1 2001-05-23 11:22:07 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.8  2000/01/09  06:48:03  yfujii
 * PC relative addressing for movem instruction is fixed.
 *
 * Revision 1.6  1999/12/07  12:44:14  yfujii
 * *** empty log message ***
 *
 * Revision 1.6  1999/11/30  13:27:07  yfujii
 * Wrong operation for 'NOT' instruction is fixed.
 *
 * Revision 1.5  1999/11/22  03:57:08  yfujii
 * Condition code calculations are rewriten.
 *
 * Revision 1.4  1999/10/28  06:34:08  masamichi
 * Modified trace behavior and added abs.w addressing for jump
 *
 * Revision 1.3  1999/10/20  03:55:03  masamichi
 * Added showing more information about errors.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include <string.h>
#include "run68.h"

static	int	Lea( char, char ) ;
static	int	Link( char ) ;
static	int	Unlk( char ) ;
static	int	Tas( char ) ;
static	int	Tst( char ) ;
static	int	Pea( char ) ;
static	int	Movem_f( char ) ;
static	int	Movem_t( char ) ;
static	int	Move_f_sr( char ) ;
static	int	Move_t_sr( char ) ;
static	int	Move_f_usp( char ) ;
static	int	Move_t_usp( char ) ;
static	int	Move_t_ccr( char ) ;
static	int	Swap( char ) ;
static	int	Clr( char ) ;
static	int	Ext( char ) ;
static	int	Neg( char ) ;
static	int	Negx( char ) ;
static	int	Not( char ) ;
static	int	Jmp( char, char ) ;
static	int	Jsr( char ) ;
static	int	Trap( char ) ;
static	int	Rte( void ) ;
static	int	Rts( void ) ;

#if defined(DEBUG_JSR)
static int sub_level = 0;
static int sub_num = 1;
#endif

/*
 　機能：４ライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	line4( char *pc_ptr )
{
	char	code1, code2 ;
	code1 = *(pc_ptr++) ;
	code2 = *pc_ptr ;
	pc += 2 ;

	/* lea */
	if ( (code1 & 0x01) == 0x01 && (code2 & 0xC0) == 0xC0 )
		return( Lea( code1, code2 ) ) ;

	switch( code1 ) {
		case 0x40:
			if ( (code2 & 0xC0) != 0xC0 )
				return( Negx( code2 ) ) ;
			else
				return( Move_f_sr( code2 ) ) ;
			break ;
		case 0x42:
			if ( (code2 & 0xC0) != 0xC0 )
				return( Clr( code2 ) ) ;
			break ;
		case 0x44:
			if ( (code2 & 0xC0) != 0xC0 )
				return( Neg( code2 ) ) ;
			else
				return( Move_t_ccr( code2 ) ) ;
			break ;
		case 0x46:
			if ( (code2 & 0xC0) == 0xC0 )
				return( Move_t_sr( code2 ) ) ;
			else
				return( Not( code2 ) ) ;
			break ;
		case 0x48:	/* movem_f_reg / swap / pea / ext / nbcd */
			if ((code2 & 0xC0) == 0x40 ) {
				if ( (code2 & 0xF8) == 0x40 )
					return( Swap( code2 ) ) ;
				else
					return( Pea( code2 ) ) ;
			} else {
				if ((code2 & 0xC0) == 0 )
					;	/* nbcd */
				if ((code2 & 0x38) == 0 )
					return( Ext( code2 ) ) ;
				if ((code2 & 0x80) != 0 )
					return( Movem_f( code2 ) ) ;
			}
			break ;
		case 0x4A:	/* tas / tst */
			if ( (code2 & 0xC0) == 0xC0 )
				return( Tas( code2 ) ) ;
			else
				return( Tst( code2 ) ) ;
		case 0x4C:	/* movem_t_reg */
			if ( (code2 & 0x80) != 0 )
				return( Movem_t( code2 ) ) ;
			break ;
		case 0x4E:
			if ( (code2 & 0xF0) == 0x40 )
				return( Trap( code2 ) ) ;
			if ( code2 == 0x71 )	/* nop */
				return( FALSE ) ;
			if ( code2 == 0x73 )
				return( Rte() ) ;
			if ( code2 == 0x75 )
				return( Rts() ) ;
			if ( code2 == 0x76 ) {
				err68a( "TRAPV命令を実行しました", __FILE__, __LINE__ ) ;
				return( TRUE ) ;
			}
			if ( code2 == 0x77 )
				;	/* rtr */
			if ( (code2 & 0xF8) == 0x50 )
				return( Link( code2 ) ) ;
			if ( (code2 & 0xF8) == 0x58 )
				return( Unlk( code2 ) ) ;
			if ( (code2 & 0xF8) == 0x60 )
				return( Move_t_usp( code2 ) ) ;
			if ( (code2 & 0xF8) == 0x68 )
				return( Move_f_usp( code2 ) ) ;
			if ( (code2 & 0xC0) == 0xC0 )
				return( Jmp( code1, code2 ) ) ;
			if ( (code2 & 0xC0) == 0x80 )
				return( Jsr( code2 ) ) ;
			break ;
	}

	err68a( "未定義命令を実行しました", __FILE__, __LINE__ ) ;
	return( TRUE ) ;
}

/*
 　機能：lea命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Lea( char code1, char code2 )
{
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	save_pc ;

	save_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (get_ea(save_pc, EA_Control, mode, src_reg, &(ra[dst_reg]))) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lea      src=%d PC=%06lX\n", ra [ dst_reg ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：link命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Link( char code )
{
	char	reg ;
	short	len ;

	reg = (code & 0x07) ;
	len = (short)imi_get( S_WORD ) ;

	ra [ 7 ] -= 4 ;
	mem_set( ra [ 7 ], ra [ reg ], S_LONG ) ;
	ra [ reg ] = ra [ 7 ] ;
	ra [ 7 ] += len ;

#ifdef	TRACE
	printf( "trace: link     len=%d PC=%06lX\n", len, pc - 2 ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：unlk命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Unlk( char code )
{
	char	reg ;

	reg = (code & 0x07) ;

	ra [ 7 ] = ra [ reg ] ;
	ra [ reg ] = mem_get( ra [ 7 ], S_LONG ) ;
	ra [ 7 ] += 4 ;

#ifdef	TRACE
	printf( "trace: unlk     PC=%06lX\n", pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：tas命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Tas( char code )
{
	char	mode ;
	char	reg ;
	long	data ;
	int	work_mode;

	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, S_BYTE, &data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	general_conditions(data, S_BYTE);

	/* OR演算 */
	data |= 0x80;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, S_BYTE, data)) {
		return(TRUE);
	}

	return( FALSE ) ;
}

/*
 　機能：tst命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Tst( char code )
{
	char	size ;
	char	mode ;
	char	reg ;
	long	data ;
	long	save_pc ;

	save_pc = pc ;
	size = ((code >> 6) & 0x03) ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	if (get_data_at_ea(EA_VariableData, mode, reg, size, &data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	general_conditions(data, size);

#ifdef	TRACE
	printf( "trace: tst.%c    dst=%d PC=%06lX\n",
		size_char [ size ], data, save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：pea命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Pea( char code )
{
	char	mode ;
	char	reg ;
	long	data ;
	long	save_pc ;

	save_pc = pc ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	if (get_ea(save_pc, EA_Control, mode, reg, &data)) {
		return(TRUE);
	}

	ra [ 7 ] -= 4 ;
	mem_set( ra [ 7 ], data, S_LONG ) ;

#ifdef	TRACE
	printf( "trace: pea      src=%d PC=%06lX\n", data, save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：movem from reg命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Movem_f( char code )
{
	long	mem_adr ;
	char	mode ;
	char	reg ;
	char	size ;
	char	size2 ;
	short	rlist ;
	short	mask = 1 ;
	short	disp = 0 ;
	long	save_pc ;
	int		i ;
	int		work_mode ;

	save_pc = pc ;
	if ( (code & 0x40) != 0 ) {
		size = S_LONG ;
		size2 = 4 ;
	} else {
		size = S_WORD ;
		size2 = 2 ;
	}
	mode = (code & 0x38) >> 3 ;
	reg = (code & 0x07) ;
	rlist = (short)imi_get( S_WORD ) ;

	// アドレッシングモードが MD_AIPD の場合は、
	// MD_AIとして実効アドレスを取得する。
	if (mode == MD_AIPD) {
		work_mode = MD_AI;
	} else {
		work_mode = mode;
	}

	/* アドレッシングモードに応じた処理 */
	if (get_ea(save_pc, EA_PreDecriment, work_mode, reg, &mem_adr)) {
		return(TRUE);
	}

	if (mode == MD_AIPD) {

		// アドレスレジスタの退避
		for ( i = 7 ; i >= 0 ; i--, mask <<= 1 ) {
			if ( (rlist & mask) != 0 ) {
				ra [ reg ] -= size2 ;
				mem_adr -= size2 ;
				mem_set( mem_adr, ra [ i ] , size ) ;
			}
		}

		// データレジスタの退避
		for ( i = 7 ; i >= 0 ; i--, mask <<= 1 ) {
			if ( (rlist & mask) != 0 ) {
				ra [ reg ] -= size2 ;
				mem_adr -= size2 ;
				mem_set( mem_adr, rd [ i ] , size ) ;
			}
		}

	} else {

		// データレジスタの退避
		for ( i = 0 ; i <= 7 ; i++, mask <<= 1 ) {
			if ( (rlist & mask) != 0 ) {
				mem_set( mem_adr, rd [ i ] , size ) ;
				mem_adr += size2 ;
			}
		}

		// アドレスレジスタの退避
		for ( i = 0 ; i <= 7 ; i++, mask <<= 1 ) {
			if ( (rlist & mask) != 0 ) {
				mem_set( mem_adr, ra [ i ] , size ) ;
				mem_adr += size2 ;
			}
		}

	}

#ifdef	TRACE
	printf( "trace: movemf.%c PC=%06lX\n", size_char [ size ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：movem to reg命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Movem_t( char code )
{
	long	mem_adr ;
	char	mode ;
	char	reg ;
	char	size ;
	char	size2 ;
	short	rlist ;
	short	mask = 1 ;
	long	save_pc ;
	int		i ;
	int		work_mode ;

	//save_pc = pc ;
	if ( (code & 0x40) != 0 ) {
		size = S_LONG ;
		size2 = 4 ;
	} else {
		size = S_WORD ;
		size2 = 2 ;
	}
	mode = (code & 0x38) >> 3 ;
	reg = (code & 0x07) ;
	rlist = (short)imi_get( S_WORD ) ;

	// PC相対実行アドレス用PCセーブ
	save_pc = pc ;

	// アドレッシングモードが MD_AIPI の場合は、
	// MD_AIとして実効アドレスを取得する。
	if (mode == MD_AIPI) {
		work_mode = MD_AI;
	} else {
		work_mode = mode;
	}

	/* アドレッシングモードに応じた処理 */
	if (get_ea(save_pc, EA_PostIncrement, work_mode, reg, &mem_adr)) {
		return(TRUE);
	}

	// データレジスタの復帰
	for ( i = 0 ; i <= 7 ; i++, mask <<= 1 ) {
		if ( (rlist & mask) != 0 ) {
			if (size == S_WORD) {
				rd [ i ] = mem_get( mem_adr, S_WORD ) ;
				if (rd[i] & 0x8000) {
					rd [ i ] |= 0xFFFF0000;
				} else {
					rd [ i ] &= 0xFFFF;
				}
			} else {
				rd [ i ] = mem_get( mem_adr, S_LONG ) ;
			}
			if ( mode == MD_AIPI )
				ra [ reg ] += size2 ;
			mem_adr += size2 ;
		}
	}

	// アドレスレジスタの復帰
	for ( i = 0 ; i <= 7 ; i++, mask <<= 1 ) {
		if ( (rlist & mask) != 0 ) {
			if (size == S_WORD) {
				ra [ i ] = mem_get( mem_adr, S_WORD ) ;
				if (ra[i] & 0x8000) {
					ra [ i ] |= 0xFFFF0000;
				} else {
					ra [ i ] &= 0xFFFF;
				}
			} else {
				ra [ i ] = mem_get( mem_adr, S_LONG ) ;
			}
			if ( mode == MD_AIPI )
				ra [ reg ] += size2 ;
			mem_adr += size2 ;
		}
	}

#ifdef	TRACE
	printf( "trace: movemt.%c PC=%06lX\n", size_char [ size ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：move from SR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Move_f_sr( char code )
{
	char	mode ;
	char	reg ;
	long	save_pc ;

	save_pc = pc ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	/* ディスティネーションのアドレッシングモードに応じた処理 */
	// ※アクセス権限がEA_ALLになっているが、これは後でチェックの必要がある
	if (set_data_at_ea(EA_All, mode, reg, S_WORD, (long)sr)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: move_f_sr PC=%06lX\n", save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：move to SR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Move_t_sr( char code )
{
	char	mode ;
	char	reg ;
	long	save_pc ;
	long	data;

	save_pc = pc ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	if ( SR_S_REF() == 0 ) {
		err68a( "特権命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

	/* ソースのアドレッシングモードに応じた処理 */
	// ※アクセス権限がEA_ALLになっているが、これは後でチェックの必要がある
	if (get_data_at_ea(EA_All, mode, reg, S_WORD, &data)) {
		return(TRUE);
	}

	sr = (short)data ;

#ifdef	TRACE
	printf( "trace: move_t_sr PC=%06lX\n", save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：move from USP命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Move_f_usp( char code )
{
	char	reg ;

	if ( SR_S_REF() == 0 ) {
		err68a( "特権命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

	reg = (code & 0x07) ;

#ifdef	TRACE
	printf( "trace: move_f_usp PC=%06lX\n", pc ) ;
#endif

	if ( usp == 0 ) {
		err68( "MOVE FROM USP命令を実行しました" ) ;
		return( TRUE ) ;
	}

	ra [ reg ] = usp ;

	return( FALSE ) ;
}

/*
 　機能：move to USP命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Move_t_usp( char code )
{
	char	reg ;

	if ( SR_S_REF() == 0 ) {
		err68a( "特権命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

	reg = (code & 0x07) ;

#ifdef	TRACE
	printf( "trace: move_t_usp PC=%06lX\n", pc ) ;
#endif

	err68( "MOVE TO USP命令を実行しました" ) ;
	return( TRUE ) ;
}

/*
 　機能：move to CCR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Move_t_ccr( char code )
{
	char	mode ;
	char	reg ;
	long	save_pc ;
	long	data ;

	save_pc = pc ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	/* ソースのアドレッシングモードに応じた処理 */
	// ※アクセス権限がEA_ALLになっているが、これは後でチェックの必要がある
	if (get_data_at_ea(EA_All, mode, reg, S_WORD, &data)) {
		return(TRUE);
	}

	sr = (( sr & 0xFF00 ) | ( (short)data & 0xFF )) ;

#ifdef	TRACE
	printf( "trace: move_t_ccr PC=%06lX\n", save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：swap命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Swap( char code )
{
	char	reg ;
	long	data ;
	long	data2 ;

	reg = (code & 0x07) ;
	data = ((rd [ reg ] >> 16) & 0xFFFF) ;
	data2 = ((rd [ reg ] & 0xFFFF) << 16) ;
	data |= data2 ;
	rd [ reg ] = data ;

#ifdef	TRACE
	printf( "trace: swap     PC=%06lX\n", pc ) ;
#endif

	/* フラグの変化 */
	general_conditions(data, S_LONG);

	return( FALSE ) ;
}

/*
 　機能：clr命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Clr( char code )
{
	char	size ;
	char	mode ;
	char	reg ;
	long	save_pc ;
	long	data;
	int	work_mode;

	save_pc = pc ;
	size = ((code >> 6) & 0x03) ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	/* ここでわざわざ使いもしない値をリードしているのは */
	/* 68000の仕様がそうなっているため。 */

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* CLEAR */
	data = 0;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	general_conditions(data, size);

#ifdef	TRACE
	printf( "trace: clr.%c    PC=%06lX\n", size_char [ size ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：ext命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Ext( char code )
{
	char	size ;
	char	reg ;

	reg = (code & 0x07) ;
	if ( (code & 0x40) != 0 )
		size = S_LONG ;
	else
		size = S_WORD ;

	if ( size == S_WORD ) {
		if ( (rd [ reg ] & 0x80) != 0 )
			rd [ reg ] |= 0xFF00 ;
		else
			rd [ reg ] &= 0xFFFF00FF ;
	} else {
		if ( (rd [ reg ] & 0x8000) != 0 )
			rd [ reg ] |= 0xFFFF0000 ;
		else
			rd [ reg ] &= 0x0000FFFF ;
	}

	/* フラグの変化 */
	general_conditions(rd[reg], size);

#ifdef	TRACE
	printf( "trace: ext.%c    PC=%06lX\n", size_char [ size ], pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：neg命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Neg( char code )
{
	char	size ;
	char	mode ;
	char	reg ;
	long	data ;
	long	save_pc ;
	long	dest_data;
	int	work_mode;

	save_pc = pc ;
	size = ((code >> 6) & 0x03) ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* NEG演算 */
	dest_data = sub_long(data, 0, size);

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, dest_data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	neg_conditions(data, dest_data,  size, 1);

#ifdef	TRACE
	printf( "trace: neg.%c    PC=%06lX\n", size_char [ size ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：negx命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Negx( char code )
{
	char	size ;
	char	mode ;
	char	reg ;
	long	data ;
	long	save_pc ;
	short	save_z ;
	short	save_x;
	long	dest_data;
	int	work_mode;

	save_pc = pc ;
	size = ((code >> 6) & 0x03) ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	save_x = CCR_X_REF() != 0 ? 1 : 0;
	save_z = CCR_Z_REF() != 0 ? 1 : 0;

	/* NEG演算 */
	dest_data = sub_long(data + save_x, 0, size);

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, dest_data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	neg_conditions(data, dest_data,  size, save_z);

#ifdef	TRACE
	printf( "trace: negx.%c   PC=%06lX\n", size_char [ size ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：not命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Not( char code )
{
	char	size ;
	char	mode ;
	char	reg ;
	long	data ;
	long	save_pc ;
	int	work_mode;

	save_pc = pc ;
	size = ((code >> 6) & 0x03) ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* NOT演算 */
    data = data ^ 0xffffffff;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	general_conditions(data,  size);

#ifdef	TRACE
	printf( "trace: not.%c    PC=%06lX\n", size_char [ size ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：jmp命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Jmp( char code1, char code2 )
{
	char	mode ;
	char	reg ;
	long	save_pc ;

	save_pc = pc;

	mode = ((code2 & 0x38) >> 3) ;
	reg = (code2 & 0x07) ;

#ifdef TRACE
    /* ニーモニックのトレース出力 */
    fprintf(stderr, "0x%08x: %s\n", pc, mnemonic);
#endif

	/* アドレッシングモードに応じた処理 */
	// ※アクセス権限がEA_ALLになっているが、これは後でチェックの必要がある
	if (get_ea(save_pc, EA_All, mode, reg, &pc)) {
		return(TRUE);
	}

	return( FALSE ) ;
}

/*
 　機能：jsr命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Jsr( char code )
{
	char	mode ;
	char	reg ;
	long	data ;
	long	save_pc ;

	save_pc = pc ;
	mode = ((code & 0x38) >> 3) ;
	reg = (code & 0x07) ;

#ifdef	TRACE
	printf( "trace: jsr      PC=%06lX\n", pc ) ;
#endif

	/* アドレッシングモードに応じた処理 */
	// ※アクセス権限がEA_ALLになっているが、これは後でチェックの必要がある
	if (get_ea(save_pc, EA_All, mode, reg, &data)) {
		return(TRUE);
	}

	ra [ 7 ] -= 4 ;
	mem_set( ra [ 7 ], pc, S_LONG ) ;
	pc = data;

#if defined(DEBUG_JSR)
			printf("%8d: %8d: $%06x JSR    TO $%06x, TOS = $%06x\n", sub_num++, sub_level++, save_pc - 2, pc, mem_get(ra[7], S_LONG));
#endif

	return( FALSE ) ;
}

/*
 　機能：trap命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Trap( char code )
{
	if ( (code & 0x0F) == 15 ) {
		return( iocs_call() ) ;
	} else {
		err68a( "未定義の例外処理を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}
}

/*
 　機能：rte命令を実行する
 戻り値： TRUE = 実行終了
 戻り値：FALSE = 実行継続
*/
static	int	Rte()
{
#ifdef	TRACE
	printf( "trace: rte      PC=%06lX\n", pc ) ;
#endif

	if ( SR_S_REF() == 0 ) {
		err68a( "特権命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}
	sr = (short)mem_get( ra [ 7 ], S_WORD ) ;
	ra [ 7 ] += 2 ;
	pc = mem_get( ra [ 7 ], S_LONG ) ;
	ra [ 7 ] += 4 ;
	trap_count = RAS_INTERVAL ;

	return( FALSE ) ;
}

/*
 　機能：rts命令を実行する
 戻り値：FALSE = 実行継続
*/
static	int	Rts()
{
#if defined(DEBUG_JSR)
	long	save_pc ;
	save_pc = pc - 2;
#endif

#ifdef	TRACE
	printf( "trace: rts      PC=%06lX\n", pc ) ;
#endif

	pc = mem_get( ra [ 7 ], S_LONG ) ;
	ra [ 7 ] += 4 ;

#if defined(DEBUG_JSR)
			printf("%8d: %8d: $%06x RETURN TO $%06x\n", sub_num++, --sub_level, save_pc, pc - 2);
#endif

	return( FALSE ) ;
}

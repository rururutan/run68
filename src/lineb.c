/* $Id: lineb.c,v 1.1.1.1 2001-05-23 11:22:08 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.7  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.6  1999/12/07  12:45:26  yfujii
 * *** empty log message ***
 *
 * Revision 1.6  1999/11/22  03:57:08  yfujii
 * Condition code calculations are rewriten.
 *
 * Revision 1.4  1999/10/25  04:22:27  masamichi
 * Full implements EOR instruction.
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

static	int	Cmp( char, char ) ;
static	int	Cmpa( char, char ) ;
static	int	Cmpm( char, char ) ;
static	int	Eor( char, char ) ;

/*
 　機能：Ｂライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	lineb( char *pc_ptr )
{
	char	code1, code2 ;

	code1 = *(pc_ptr++) ;
	code2 = *pc_ptr ;
	pc += 2 ;

	if ( (code1 & 0x01) == 0x00 ) {
		if ( (code2 & 0xC0) == 0xC0 )
			return( Cmpa( code1, code2 ) ) ;
		return( Cmp( code1, code2 ) ) ;
	}

	if ( (code2 & 0xC0) == 0xC0 )
		return( Cmpa( code1, code2 ) ) ;

	if ( (code2 & 0x38) == 0x08 )
		return( Cmpm( code1, code2 ) ) ;

	return( Eor( code1, code2 ) ) ;
}

/*
 　機能：cmpi命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Cmp( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	src_data ;
	long	save_pc ;
	short	save_x ;
	long	dest_data;
	long	result;

#ifdef TEST_CCR
	short	before;
#endif

	save_pc = pc ;
	size = ((code2 >> 6) & 0x03) ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (mode == EA_AD && size == S_BYTE) {
		err68a( "不正な命令: cmp.b An, Dn を実行しようとしました。", __FILE__, __LINE__ ) ;
		return(TRUE);
	} else if (get_data_at_ea(EA_All, mode, src_reg, size, &src_data)) {
		return(TRUE);
	}

	/* ディスティネーションのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_All, EA_DD, dst_reg, size, &dest_data)) {
		return(TRUE);
	}

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif

	/* サイズに応じてCCRをセットする */
	save_x = CCR_X_REF() ;
//	result = sub_rd( dst_reg, src_data, size ) ;
	result = sub_long(src_data, dest_data, size);
//	if ( save_x == 0 )
//		CCR_X_OFF() ;
//	else
//		CCR_X_ON() ;

	/* 先のフラグ変化を無視する */
	/* フラグの変化 */
	cmp_conditions(src_data, dest_data, result, size);

#ifdef TEST_CCR
	check("cmp", src_data, dest_data, result, size, before);
#endif

#ifdef	TRACE
	switch( size ) {
		case S_BYTE:
			rd [ 8 ] = ( rd [ dst_reg ] & 0xFF ) ;
			break ;
		case S_WORD:
			rd [ 8 ] = ( rd [ dst_reg ] & 0xFFFF) ;
			break ;
		default:	/* S_LONG */
			rd [ 8 ] = rd [ dst_reg ] ;
			break ;
	}
	printf( "trace: cmp.%c    src=%d dst=%d PC=%06lX\n",
		size_char [ size ], src_data, rd [ 8 ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：cmpa命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Cmpa( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	src_data ;
	long	save_pc ;
	long	old ;
	long	ans ;
	long	dest_data;

#ifdef TEST_CCR
	short	before;
#endif

	save_pc = pc ;
	if ( (code1 & 0x01) == 0 )
		size = S_WORD ;
	else
		size = S_LONG ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (size == S_BYTE) {
		err68a( "不正な命令: cmp.b <ea>, An を実行しようとしました。", __FILE__, __LINE__ ) ;
		return(TRUE);
	} else if (get_data_at_ea(EA_All, mode, src_reg, size, &src_data)) {
		return(TRUE);
	}

	/* ディスティネーションのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_All, EA_AD, dst_reg, size, &dest_data)) {
		return(TRUE);
	}

	if ( size == S_WORD ) {
		if ( (src_data & 0x8000) != 0 )
			src_data |= 0xFFFF0000 ;
	}

#ifdef	TRACE
	printf( "trace: cmpa.%c   src=%d PC=%06lX\n",
		size_char [ size ], src_data, save_pc ) ;
#endif

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif
	old = ra [ dst_reg ] ;
	ans = old - src_data ;

#if 0
	carry = ((old >> 1) & 0x7FFFFFFF) - ((src_data >> 1) & 0x7FFFFFFF) ;
	if ( (old & 0x1) == 0 && (src_data & 0x1) > 0 )
		carry -- ;
	if (carry < 0) {
		CCR_C_ON() ;
		CCR_V_OFF() ;
	} else {
		CCR_C_OFF() ;
		if ( (old & 0x80000000) == 0 && (ans & 0x80000000) != 0 )
			CCR_V_ON() ;
		else
			CCR_V_OFF() ;
	}
	if ( ans < 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( ans == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	/* 先のフラグ変化を無視する */
#endif

	/* フラグの変化 */
	cmp_conditions(src_data, old, ans, size);

#ifdef TEST_CCR
	check("cmpa", src_data, dest_data, ans, size, before);
#endif

	return( FALSE ) ;
}

/*
 　機能：cmpm命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Cmpm( char code1, char code2 )
{
	char	size ;
	char	src_reg ;
	char	dst_reg ;
	long	src_data ;
	long	dest_data;
	long	result;

	size = ((code2 >> 6) & 0x03) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_All, EA_AIPI, src_reg, size, &src_data)) {
		return(TRUE);
	}

	/* ディスティネーションのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_All, EA_AIPI, dst_reg, size, &dest_data)) {
		return(TRUE);
	}

	rd [ 8 ] = dest_data;

	/* サイズに応じてCCRをセットする */
//	save_x = CCR_X_REF() ;
	// result = sub_rd( 8, src_data, size ) ;
	result = sub_long(src_data, dest_data, size) ;
//	if ( save_x == 0 )
//		CCR_X_OFF() ;
//	else
//		CCR_X_ON() ;

	/* 先のフラグ変化を無視する */
	/* フラグの変化 */
	cmp_conditions(src_data, dest_data, result, size);


#ifdef	TRACE
	printf( "trace: cmpm.%c   src=%d dst=%d PC=%06lX\n",
		size_char [ size ], src_data, rd [ 8 ], pc ) ;
#endif


	return( FALSE ) ;
}

/*
 　機能：eor命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Eor( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	data ;
	long	save_pc ;
	long	src_data;
	int	work_mode;

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

	/* EOR演算 */
	data ^= src_data;

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
	printf( "trace: eor.%c    src=%d PC=%06lX\n",
		size_char [ size ], rd [ src_reg ], save_pc ) ;
#endif

	return( FALSE ) ;
}

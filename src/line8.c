/* $Id: line8.c,v 1.1.1.1 2001-05-23 11:22:07 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.6  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.5  1999/12/07  12:45:00  yfujii
 * *** empty log message ***
 *
 * Revision 1.5  1999/11/22  03:57:08  yfujii
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

static	int	Divu( char, char ) ;
static	int	Divs( char, char ) ;
static	int	Or1( char, char ) ;
static	int	Or2( char, char ) ;

/*
 　機能：８ライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	line8( char *pc_ptr )
{
	char	code1, code2 ;

	code1 = *(pc_ptr++) ;
	code2 = *pc_ptr ;
	pc += 2 ;

	if ( (code2 & 0xC0) == 0xC0 ) {
		if ( (code1 & 0x01) == 0 )
			return( Divu( code1, code2 ) ) ;
		else
			return( Divs( code1, code2 ) ) ;
	}
	if ( (code1 & 0x01) == 0x01 && (code2 & 0xF0) == 0 ) {
		/* sbcd */
		err68a( "未定義命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

	if ( (code1 & 0x01) == 0x01 )
		return ( Or1( code1, code2 ) ) ;
	else
		return ( Or2( code1, code2 ) ) ;
}

/*
 　機能：divu命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Divu( char code1, char code2 )
{
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	UShort	waru ;
	ULong	data ;
	ULong	ans ;
	UShort	mod ;
	long	save_pc ;
	long	waru_l;

	save_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;
	data = rd [ dst_reg ] ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_Data, mode, src_reg, S_WORD, &waru_l)) {
		return(TRUE);
	}
	waru = (UShort)waru_l;

	if ( waru == 0 ) {
		err68a( "０で除算しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

	CCR_C_OFF() ;
	ans = data / waru ;
	mod = (unsigned char)(data % waru) ;
	if ( ans > 0xFFFF ) {
		CCR_V_ON() ;
		return( FALSE ) ;
	}
	rd [ dst_reg ] = ((mod << 16) | ans) ;

	CCR_V_OFF() ;
	if ( ans >= 0x8000 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( ans == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}
	return( FALSE ) ;
}

/*
 　機能：divs命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Divs( char code1, char code2 )
{
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	short	waru ;
	long	data ;
	long	ans ;
	short	mod ;
	long	save_pc ;
	long	waru_l;

	save_pc = pc ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;
	data = rd [ dst_reg ] ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (get_data_at_ea(EA_Data, mode, src_reg, S_WORD, &waru_l)) {
		return(TRUE);
	}

	waru = (UShort)waru_l;

	if ( waru == 0 ) {
		err68a( "０で除算しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

	CCR_C_OFF() ;
	ans = data / waru ;
	mod = data % waru ;
	if ( ans > 32767 || ans < -32768 ) {
		CCR_V_ON() ;
		return( FALSE ) ;
	}
	rd [ dst_reg ] = ((mod << 16) | (ans & 0xFFFF)) ;

	CCR_V_OFF() ;
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
	return( FALSE ) ;
}

/*
 　機能：or Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Or1( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	data ;
	long	save_pc ;
	long	src_data ;
	long	work_mode ;

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

	/* OR演算 */
	data |= src_data;

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

	return( FALSE ) ;
}

/*
 　機能：or <ea>,Dn命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Or2( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	src_data ;
	long	save_pc ;
	long	data;

	save_pc = pc ;
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

	data |= src_data;

	/* デスティネーションのアドレッシングモードに応じた処理 */
	if (set_data_at_ea(EA_All, EA_DD, dst_reg, size, data)) {
		return(TRUE);
	}

	/* フラグの変化 */
	general_conditions(data, size);

	return( FALSE ) ;
}

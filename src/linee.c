/* $Id: linee.c,v 1.1.1.1 2001-05-23 11:22:08 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.3  1999/12/07  12:46:06  yfujii
 * *** empty log message ***
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

static	int	Asl( char, char ) ;
static	int	Asl2( char ) ;
static	int	Asr( char, char ) ;
static	int	Asr2( char ) ;
static	int	Lsl( char, char ) ;
static	int	Lsl2( char ) ;
static	int	Lsr( char, char ) ;
static	int	Lsr2( char ) ;
static	int	Rol( char, char ) ;
static	int	Rol2( char ) ;
static	int	Roxl( char, char ) ;
static	int	Roxl2( char ) ;
static	int	Ror( char, char ) ;
static	int	Ror2( char ) ;
static	int	Roxr( char, char ) ;
static	int	Roxr2( char ) ;

/*
 　機能：Ｅライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	linee( char *pc_ptr )
{
	char	code1, code2 ;

	code1 = *(pc_ptr++) ;
	code2 = *pc_ptr ;
	pc += 2 ;

	if ( (code1 & 0x01) != 0 ) {
		/* 左 */
		if ((code2 & 0xC0) == 0xC0) {
			/* xxl{.w} <ea> 形式 */
			switch(code1 & 0x0e) {
				case 0x00:
					return( Asl2( code2 ) ) ;	/* asl{.w} <ea> */
				case 0x02:
					return( Lsl2( code2 ) ) ;	/* lsl{.w} <ea> */
				case 0x04:
					return( Roxl2( code2 ) ) ;	/* roxl{.w} <ea> */
				case 0x06:
					return( Rol2( code2 ) ) ;	/* rol{.w} <ea> */
				default:
					err68a( "おかしな命令を実行しました", __FILE__, __LINE__ ) ;
					return( TRUE ) ;
			}
		} else {
			switch(code2 & 0x18) {
				case 0x00:
					return( Asl( code1, code2 ) ) ;
				case 0x08:
					return( Lsl( code1, code2 ) ) ;
				case 0x10:
					return( Roxl( code1, code2 ) ) ;
				case 0x18:
					return( Rol( code1, code2 ) ) ;
				default:
					err68a( "おかしな命令を実行しました", __FILE__, __LINE__ ) ;
					return( TRUE ) ;
			}
		}
	} else {
		/* 右 */
		if ((code2 & 0xC0) == 0xC0) {
			/* xxr{.w} <ea> 形式 */
			switch(code1 & 0x0e) {
				case 0x00:
					return( Asr2( code2 ) ) ;	/* asr{.w} <ea> */
				case 0x02:
					return( Lsr2( code2 ) ) ;	/* lsr{.w} <ea> */
				case 0x04:
					return( Roxr2( code2 ) ) ;	/* roxr{.w} <ea> */
				case 0x06:
					return( Ror2( code2 ) ) ;	/* ror{.w} <ea> */
				default:
					err68a( "おかしな命令を実行しました", __FILE__, __LINE__ ) ;
					return( TRUE ) ;
			}
		} else {
			switch(code2 & 0x18) {
				case 0x00:
					return( Asr( code1, code2 ) ) ;
				case 0x08:
					return( Lsr( code1, code2 ) ) ;
				case 0x10:
					return( Roxr( code1, code2 ) ) ;
				case 0x18:
					return( Ror( code1, code2 ) ) ;
				default:
					err68a( "おかしな命令を実行しました", __FILE__, __LINE__ ) ;
					return( TRUE ) ;
			}
		}
	}

	err68a( "未定義命令を実行しました", __FILE__, __LINE__ ) ;
	return( TRUE ) ;
}

/*
 　機能：asl命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Asl( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	ULong	top ;
	ULong	mask ;
	ULong	src ;
	ULong	flag ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;
	top = (src & mask) ;
	flag = top ;

#ifdef	TRACE
	printf( "trace: asl.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	CCR_C_OFF() ;
	CCR_V_OFF() ;
	for( ; cnt > 0 ; cnt -- ) {
		src <<= 1 ;
		if ( top != 0 ) {
			CCR_X_ON() ;
			CCR_C_ON() ;
		} else {
			CCR_X_OFF() ;
			CCR_C_OFF() ;
		}
		top = (src & mask) ;
		if ( top != flag )
			CCR_V_ON() ;
	}

	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：asl.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Asl2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	ULong	msb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsl.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最上位ビットを保存する */
	msb = src & 0x8000 ;

	src <<= 1 ;
	src &= 0xFFFF ;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* C および X フラグはシフト前の最上位ビット値を取る */
	if ( msb ) {
		CCR_X_ON() ;
		CCR_C_ON() ;
	} else {
		CCR_X_OFF() ;
		CCR_C_OFF() ;
	}

	/* V フラグはシフト前の最上位ビットと */
	/* 現在の最上位ビットが異なるとき 1 を立てる */
	if ( msb != (src & 0x8000) ) {
		CCR_V_ON() ;
	} else {
		CCR_V_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：asr命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Asr( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	char	btm ;
	ULong	mask ;
	ULong	src ;
	ULong	flag ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;
	flag = (src & mask) ;
	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			break ;
	}

#ifdef	TRACE
	printf( "trace: asr.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	CCR_C_OFF() ;
	for( ; cnt > 0 ; cnt -- ) {
		btm = (char)(src & 0x01) ;
		src >>= 1 ;
		if ( btm != 0 ) {
			CCR_X_ON() ;
			CCR_C_ON() ;
		} else {
			CCR_X_OFF() ;
			CCR_C_OFF() ;
		}
		src |= flag ;
	}

	switch( size ) {
		case S_BYTE:
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	CCR_V_OFF() ;
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：asr.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Asr2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	int		msb ;
	int		lsb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsl.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最上位ビットを保存する */
	msb = src & 0x8000 ;

	/* シフト前の最下位ビットを保存する */
	lsb = src & 0x1 ;

	src >>= 1 ;
	src |= msb ;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* C および X フラグはシフト前の最下位ビット値を取る */
	if ( lsb ) {
		CCR_X_ON() ;
		CCR_C_ON() ;
	} else {
		CCR_X_OFF() ;
		CCR_C_OFF() ;
	}

	/* V フラグは常に0 */
	CCR_V_OFF() ;

	return( FALSE ) ;
}

/*
 　機能：lsl命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Lsl( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	ULong	mask ;
	ULong	src ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;

#ifdef	TRACE
	printf( "trace: lsl.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	CCR_C_OFF() ;
	for( ; cnt > 0 ; cnt -- ) {
		if ( (src & mask) != 0 ) {
			CCR_X_ON() ;
			CCR_C_ON() ;
		} else {
			CCR_X_OFF() ;
			CCR_C_OFF() ;
		}
		src <<= 1 ;
	}

	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	CCR_V_OFF() ;
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：lsl.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Lsl2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	int		msb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsl.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最上位ビットを保存する */
	msb = src & 0x8000 ;

	src <<= 1 ;
	src &= 0xFFFF ;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* C および X フラグはシフト前の最上位ビット値を取る */
	if ( msb ) {
		CCR_X_ON() ;
		CCR_C_ON() ;
	} else {
		CCR_X_OFF() ;
		CCR_C_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：lsr命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Lsr( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	ULong	mask ;
	ULong	src ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;
	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			break ;
	}

#ifdef	TRACE
	printf( "trace: lsr.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	CCR_C_OFF() ;
	for( ; cnt > 0 ; cnt -- ) {
		if ( (src & 0x01) != 0 ) {
			CCR_X_ON() ;
			CCR_C_ON() ;
		} else {
			CCR_X_OFF() ;
			CCR_C_OFF() ;
		}
		src >>= 1 ;
	}

	switch( size ) {
		case S_BYTE:
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	CCR_V_OFF() ;
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：lsr.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Lsr2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	int		lsb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsr.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最下位ビットを保存する */
	lsb = src & 0x1 ;

	src >>= 1 ;
	src &= 0xFFFF ;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* C および X フラグはシフト前の最下位ビット値を取る */
	if ( lsb ) {
		CCR_X_ON() ;
		CCR_C_ON() ;
	} else {
		CCR_X_OFF() ;
		CCR_C_OFF() ;
	}

	return( FALSE ) ;
}


/*
 　機能：rol命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Rol( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	ULong	top ;
	ULong	mask ;
	ULong	src ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;

#ifdef	TRACE
	printf( "trace: rol.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	CCR_C_OFF() ;
	for( ; cnt > 0 ; cnt -- ) {
		top = (src & mask) ;
		src <<= 1 ;
		if ( top != 0 ) {
			CCR_C_ON() ;
			src |= 0x01 ;
		} else {
			CCR_C_OFF() ;
		}
	}

	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	CCR_V_OFF() ;
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：rol.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Rol2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	int		msb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsl.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最上位ビットを保存する */
	msb = src & 0x8000 ;

	src <<= 1 ;
	src &= 0xFFFE ;
    if (msb) {
		src |= 1;
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* X フラグは変化せず */
	/* C および X フラグはシフト前の最上位ビット値を取る */
	if ( msb ) {
		CCR_C_ON() ;
	} else {
		CCR_C_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：roxl命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Roxl( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	ULong	top ;
	ULong	mask ;
	ULong	src ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;

#ifdef	TRACE
	printf( "trace: roxl.%c   src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	if ( CCR_X_REF() != 0 )
		CCR_C_ON() ;
	else
		CCR_C_OFF() ;
	for( ; cnt > 0 ; cnt -- ) {
		top = (src & mask) ;
		src <<= 1 ;
		if ( CCR_X_REF() != 0 )
			src |= 0x01 ;
		if ( top != 0 ) {
			CCR_C_ON() ;
			CCR_X_ON() ;
		} else {
			CCR_C_OFF() ;
			CCR_X_OFF() ;
		}
	}

	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	CCR_V_OFF() ;
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：roxl.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Roxl2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	int		msb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsl.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最上位ビットを保存する */
	msb = src & 0x8000 ;

	src <<= 1 ;
	src &= 0xFFFE ;
	if (CCR_X_REF()) {
		src |= 1;
	}

	/* X フラグは押し出されたビット */
	if (msb) {
		CCR_X_ON();
	} else {
		CCR_X_OFF();
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* C および X フラグはシフト前の最上位ビット値を取る */
	if ( msb ) {
		CCR_C_ON() ;
	} else {
		CCR_C_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：ror命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Ror( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	char	btm ;
	ULong	mask ;
	ULong	src ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;
	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			break ;
	}

#ifdef	TRACE
	printf( "trace: ror.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	CCR_C_OFF() ;
	for( ; cnt > 0 ; cnt -- ) {
		btm = (char)(src & 0x01) ;
		src >>= 1 ;
		if ( btm != 0 ) {
			CCR_C_ON() ;
			src |= mask ;
		} else {
			CCR_C_OFF() ;
		}
	}

	switch( size ) {
		case S_BYTE:
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	CCR_V_OFF() ;
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：ror.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Ror2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	int		lsb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsr.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最下位ビットを保存する */
	lsb = src & 0x1 ;

	src >>= 1 ;
	src &= 0x7FFF ;
	if (lsb) {
		src |= 0x8000 ;
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* X フラグは変化せず */
	/* C はシフト前の最下位ビット値を取る */
	if ( lsb ) {
		CCR_C_ON() ;
	} else {
		CCR_C_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：roxr命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Roxr( char code1, char code2 )
{
	char	cnt ;
	char	size ;
	char	reg ;
	char	btm ;
	ULong	mask ;
	ULong	src ;
	int	i ;

	cnt = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;
	switch( size ) {
		case S_BYTE:
			mask = 0x80 ;
			break ;
		case S_WORD:
			mask = 0x8000 ;
			break ;
		default:	/* S_LONG */
			mask = 0x80000000 ;
			break ;
	}
	reg = (code2 & 0x07) ;
	if ((code2 & 0x20) != 0) {
		cnt = (char)((ULong)(rd [ cnt ]) % 64) ;
	} else {
		if ( cnt == 0 )
			cnt = 8 ;
	}
	src = rd [ reg ] ;
	switch( size ) {
		case S_BYTE:
			src &= 0xFF ;
			break ;
		case S_WORD:
			src &= 0xFFFF ;
			break ;
	}

	if ( CCR_X_REF() != 0 )
		CCR_C_ON() ;
	else
		CCR_C_OFF() ;
	for( i = 0 ; i < cnt ; i++ ) {
		btm = (char)(src & 0x01) ;
		src >>= 1 ;
		if ( CCR_X_REF() != 0 )
			src |= mask ;
		if ( btm != 0 ) {
			CCR_C_ON() ;
			CCR_X_ON() ;
		} else {
			CCR_C_OFF() ;
			CCR_X_OFF() ;
		}
	}

#ifdef	TRACE
	printf( "trace: roxr.%c   src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	switch( size ) {
		case S_BYTE:
			rd [ reg ] = ((rd [ reg ] & 0xFFFFFF00) | src) ;
			break ;
		case S_WORD:
			rd [ reg ] = ((rd [ reg ] & 0xFFFF0000) | src) ;
			break ;
		default:	/* S_LONG */
			rd [ reg ] = src ;
			break ;
	}

	/* CCRセット */
	CCR_V_OFF() ;
	if ( (src & mask) != 0 ) {
		CCR_N_ON() ;
		CCR_Z_OFF() ;
	} else {
		CCR_N_OFF() ;
		if ( src == 0 )
			CCR_Z_ON() ;
		else
			CCR_Z_OFF() ;
	}

	return( FALSE ) ;
}

/*
 　機能：roxr.w <ea> 命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Roxr2( char code2 )
{
	char	reg ;
	ULong	src ;
	int		mode ;
	int		work_mode ;
	int		lsb ;

	reg = ( code2 & 0x07 ) ;
	mode = ( ( code2 >> 3 ) & 0x07 ) ;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (get_data_at_ea_noinc(EA_VariableMemory, work_mode, reg, S_WORD, &src)) {
		return(TRUE);
	}

#ifdef	TRACE
	printf( "trace: lsr.%c    src=%d PC=%06lX\n", size_char [ size ], cnt, pc ) ;
#endif

	/* シフト前の最下位ビットを保存する */
	lsb = src & 0x1 ;

	src >>= 1 ;
	src &= 0x7FFF ;
	if (CCR_X_REF()) {
		src |= 0x8000 ;
	}

	/* X フラグは押し出されたビット */
	if (lsb) {
		CCR_X_ON();
	} else {
		CCR_X_OFF();
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}
	if (set_data_at_ea(EA_VariableMemory, work_mode, reg, S_WORD, src)) {
		return(TRUE);
	}

	/* フラグの設定 */
	general_conditions(src, S_WORD);

	/* C はシフト前の最下位ビット値を取る */
	if ( lsb ) {
		CCR_C_ON() ;
	} else {
		CCR_C_OFF() ;
	}

	return( FALSE ) ;
}

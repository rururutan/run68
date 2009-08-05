/* $Id: line9.c,v 1.2 2009-08-05 14:44:33 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.6  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.5  1999/12/07  12:45:13  yfujii
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

static	int	Suba( char, char ) ;
static	int	Subx( char, char ) ;
static	int	Sub1( char, char ) ;
static	int	Sub2( char, char ) ;

/*
 　機能：９ライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	line9( char *pc_ptr )
{
	char	code1, code2 ;

	code1 = *(pc_ptr++) ;
	code2 = *pc_ptr ;
	pc += 2 ;

	if ( (code2 & 0xC0) == 0xC0 ) {
		return( Suba( code1, code2 ) ) ;
	} else {
		if ( (code1 & 0x01) == 1 ) {
			if ( (code2 & 0x30) == 0x00 )
				return( Subx( code1, code2 ) ) ;
			else
				return( Sub1( code1, code2 ) ) ;
		} else {
			return( Sub2( code1, code2 ) ) ;
		}
	}
}

static	int	Suba( char code1, char code2 )
{
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	char	size ;
	long	src_data ;
	long	save_pc ;
	long	dest_data;

	save_pc = pc ;
	dst_reg = ((code1 & 0x0E) >> 1) ;
	if ( (code1 & 0x01) == 0x01 )
		size = S_LONG ;
	else
		size = S_WORD ;
	mode = ((code2 & 0x38) >> 3) ;
	src_reg = (code2 & 0x07) ;

	/* ソースのアドレッシングモードに応じた処理 */
	if (size == S_BYTE) {
		err68a( "不正な命令: suba.b <ea>, An を実行しようとしました。", __FILE__, __LINE__ ) ;
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

	dest_data = ra[dst_reg];

	// sub演算
	ra[dst_reg] = sub_long(src_data, dest_data, S_LONG);
	// ra [ dst_reg ] -= src_data ;

#ifdef	TRACE
	printf( "trace: suba.%c   src=%d PC=%06lX\n",
		size_char [ size ], src_data, save_pc ) ;
#endif

	/* フラグの変化はなし */
	// sub_conditions(src_data, dest_data, ra[dst_reg], size, 1);

	return( FALSE ) ;
}

/*
 　機能：subx命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Subx( char code1, char code2 )
{
	char	size ;
	char	src_reg ;
	char	dst_reg ;
	short	save_z ;
	short	save_x;
	long	dest_data;

#ifdef TEST_CCR
	short	before;
#endif

	src_reg = (code2 & 0x07) ;
	dst_reg = ((code1 & 0x0E) >> 1) ;
	size = ((code2 >> 6) & 0x03) ;

	if ( (code2 & 0x08) != 0 ) {
		/* -(An), -(An) */
		err68a( "未定義命令を実行しました", __FILE__, __LINE__ ) ;
		return( TRUE ) ;
	}

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif
	dest_data = rd [ dst_reg ];

	save_z = CCR_Z_REF() != 0 ? 1 : 0;
	save_x = CCR_X_REF() != 0 ? 1 : 0;
//	if ( CCR_X_REF() == 0 ) {
//		//rd [ dst_reg ] = sub_rd( dst_reg, rd [ src_reg ], size ) ;
//		rd [ dst_reg ] = sub_long(rd [ src_reg ], dest_data, size ) ;
//	} else {
		//rd [ dst_reg ] = sub_rd( dst_reg, rd [ src_reg ] + 1, size ) ;
		rd [ dst_reg ] = sub_long(rd [ src_reg ] + save_x, dest_data , size ) ;
//	}

//	if ( rd [ dst_reg ] == 0 ) {
//		if ( save_z == 0 )
//			CCR_Z_OFF() ;
//	}

	/* フラグの変化 */
	sub_conditions(rd[src_reg], dest_data, rd[dst_reg], size, save_z);

#ifdef TEST_CCR
	check("subx", rd[src_reg], dest_data, rd[dst_reg], size, before);
#endif

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
	printf( "trace: subx.%c   src=%d PC=%06lX\n",
		size_char [ size ], rd [ 8 ], pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：sub Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Sub1( char code1, char code2 )
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

#ifdef TEST_CCR
	short	before;
#endif

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

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif

	/* ワークレジスタへコピー */
	rd [ 8 ] = dest_data;

	/* Sub演算 */
	// rd [ 8 ] = sub_rd( 8, src_data, size ) ;
	rd [ 8 ] = sub_long(src_data, dest_data, size ) ;

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

#ifdef TEST_CCR
	check("sub", src_data, dest_data, rd[8], size, before);
#endif

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
	printf( "trace: sub.%c    src=%d PC=%06lX\n",
		size_char [ size ], rd [ 8 ], save_pc ) ;
#endif

	return( FALSE ) ;
}

/*
 　機能：sub <ea>,Dn命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Sub2( char code1, char code2 )
{
	char	size ;
	char	mode ;
	char	src_reg ;
	char	dst_reg ;
	long	src_data ;
	long	save_pc ;
	long	dest_data;

#ifdef TEST_CCR
	short	before;
#endif

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

	/* レジスタへの格納である為、long で値を得ておかないと、格納時に上位ワードを破壊してしまう */
	if (get_data_at_ea(EA_All, EA_DD, dst_reg, S_LONG /*size*/, &dest_data)) {
		return(TRUE);
	}

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif

	//rd [ dst_reg ] = sub_rd( dst_reg, src_data, size ) ;
	rd [ dst_reg ] = sub_long(src_data, dest_data, size ) ;

	/* フラグの変化 */
	sub_conditions(src_data, dest_data, rd[ dst_reg ], size, 1);

#ifdef TEST_CCR
	check("sub2", src_data, dest_data, rd[dst_reg], size, before);
#endif

#ifdef	TRACE
	switch( size ) {
		case S_BYTE:
			rd [ 8 ] = (rd [ dst_reg ] & 0xFF) ;
			break ;
		case S_WORD:
			rd [ 8 ] = (rd [ dst_reg ] & 0xFFFF) ;
			break ;
		default:	/* S_LONG */
			rd [ 8 ] = rd [ dst_reg ] ;
			break ;
	}
	printf( "trace: sub.%c    src=%d dst=%d PC=%06lX\n",
		size_char [ size ], src_data, rd [ 8 ], save_pc ) ;
#endif

	return( FALSE ) ;
}

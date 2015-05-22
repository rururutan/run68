/* $Id: line0.c,v 1.2 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.6  1999/12/21  10:08:59  yfujii
 * Uptodate source code from Beppu.
 *
 * Revision 1.5  1999/12/07  12:43:24  yfujii
 * *** empty log message ***
 *
 * Revision 1.5  1999/11/22  03:57:08  yfujii
 * Condition code calculations are rewriten.
 *
 * Revision 1.3  1999/10/20  02:39:39  masamichi
 * Add showing more information about errors.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include "run68.h"

static	int	Ori( char );
static	int	Ori_t_ccr( void );
static	int	Ori_t_sr( void );
static	int	Andi( char );
static	int	Andi_t_ccr( void );
static	int	Andi_t_sr( void );
static	int	Addi( char );
static	int	Subi( char );
static	int	Eori( char );
static	int	Eori_t_ccr( void );
static	int	Cmpi( char );
static	int	Btsti( char );
static	int	Btst( char, char );
static	int	Bchgi( char );
static	int	Bchg( char, char );
static	int	Bclri( char );
static	int	Bclr( char, char );
static	int	Bseti( char );
static	int	Bset( char, char );
static	int	Movep_f( char, char );
static	int	Movep_t( char, char );

/*
 　機能：0ライン命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
int	line0( char *pc_ptr )
{
	char	code1, code2;

	code1 = *(pc_ptr++);
	code2 = *pc_ptr;
	pc += 2;

	switch( code1 ) {
		case 0x00:
			if ( code2 == 0x3C )
				return( Ori_t_ccr() );
			else if ( code2 == 0x7C )
				return( Ori_t_sr() );
			else
				return( Ori( code2 ) );
		case 0x02:
			if ( code2 == 0x3C )
				return( Andi_t_ccr() );
			else if ( code2 == 0x7C )
				return( Andi_t_sr() );
			else
				return( Andi( code2 ) );
		case 0x04:
			return( Subi( code2 ) );
		case 0x06:
			return( Addi( code2 ) );
		case 0x08:
			switch(code2 & 0xC0) {
				case 0x00:
					return( Btsti( code2 ) );
				case 0x40:
					return( Bchgi( code2 ) );
				case 0x80:
					return( Bclri( code2 ) );
				default:	/* 0xC0 */
					return( Bseti( code2 ) );
			}
		case 0x0A:
			if ( code2 == 0x3C )
				return( Eori_t_ccr() );
			if ( code2 == 0x7C ) {	/* eori to SR */
				err68a( "未定義命令を実行しました", __FILE__, __LINE__ );
				return( TRUE );
			}
			return( Eori( code2 ) );
		case 0x0C:
			return( Cmpi( code2 ) );
		default:
			if ((code2 & 0x38) == 0x08) {
				if ( (code2 & 0x80) != 0 )
					return( Movep_f( code1, code2 ) );
				else
					return( Movep_t( code1, code2 ) );
			}
			switch(code2 & 0xC0) {
				case 0x00:
					return( Btst( code1, code2 ) );
				case 0x40:
					return( Bchg( code1, code2 ) );
				case 0x80:
					return( Bclr( code1, code2 ) );
				default:	/* 0xC0 */
					return( Bset( code1, code2 ) );
			}
	}
}

/*
 　機能：ori命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Ori( char code )
{
	long	src_data;
	char	mode;
	char	reg;
	char	size;
	long	save_pc;
	int	work_mode;
	long	data;

	save_pc = pc;
	size = ((code >> 6) & 0x03);
	if ( size == 3 ) {
		err68a( "不正なアクセスサイズです", __FILE__, __LINE__ );
		return( TRUE );
	}
	mode = (code & 0x38) >> 3;
	reg  = (code & 0x07);
	src_data = imi_get( size );

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
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

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	/* フラグのセット */
	general_conditions(data, size);

	return( FALSE );
}

/*
 　機能：ori to CCR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Ori_t_ccr()
{
	char	data;

	data = (char)imi_get( S_BYTE );

#ifdef	TRACE
	printf( "trace: ori_t_ccr src=0x%02X PC=%06lX\n", data, pc - 2 );
#endif

	/* CCRをセット */
	if ( (data & 0x10) != 0 )
		CCR_X_ON();
	if ( (data & 0x08) != 0 )
		CCR_N_ON();
	if ( (data & 0x04) != 0 )
		CCR_Z_ON();
	if ( (data & 0x02) != 0 )
		CCR_V_ON();
	if ( (data & 0x01) != 0 )
		CCR_C_ON();

	return( FALSE );
}

/*
 　機能：ori to SR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Ori_t_sr()
{
	short	data;

	if ( SR_S_REF() == 0 ) {
		err68a( "特権命令を実行しました", __FILE__, __LINE__ );
		return( TRUE );
	}

	data = (short)imi_get( S_WORD );

#ifdef	TRACE
	printf( "trace: ori_t_sr src=0x%02X PC=%06lX\n", data, pc - 2 );
#endif

	/* SRをセット */
	sr |= data;

	return( FALSE );
}

/*
 　機能：andi命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Andi( char code )
{
	long	src_data;
	char	mode;
	char	reg;
	char	size;
	long	save_pc;
	long	work_mode;
	long	data;

	save_pc = pc;
	size = ((code >> 6) & 0x03);
	if ( size == 3 ) {
		err68a( "不正なアクセスサイズです。", __FILE__, __LINE__ );
		return( TRUE );
	}
	mode = (code & 0x38) >> 3;
	reg  = (code & 0x07);

	src_data = imi_get( size );

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
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

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	/* フラグのセット */
	general_conditions(data, size);

	return( FALSE );
}

/*
 　機能：andi to CCR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Andi_t_ccr()
{
	char	data;

	data = (char)imi_get( S_BYTE );

#ifdef	TRACE
	printf( "trace: andi_t_ccr src=0x%02X PC=%06lX\n", data, pc - 2 );
#endif

	/* CCRをセット */
	if ( (data & 0x10) == 0 )
		CCR_X_OFF();
	if ( (data & 0x08) == 0 )
		CCR_N_OFF();
	if ( (data & 0x04) == 0 )
		CCR_Z_OFF();
	if ( (data & 0x02) == 0 )
		CCR_V_OFF();
	if ( (data & 0x01) == 0 )
		CCR_C_OFF();

	return( FALSE );
}

/*
 　機能：andi to SR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Andi_t_sr()
{
	short	data;

	if ( SR_S_REF() == 0 ) {
		err68a( "特権命令を実行しました", __FILE__, __LINE__ );
		return( TRUE );
	}

	data = (short)imi_get( S_WORD );

#ifdef	TRACE
	printf( "trace: andi_t_sr src=0x%02X PC=%06lX\n", data, pc - 2 );
#endif

	/* SRをセット */
	sr &= data;

	return( FALSE );
}

/*
 　機能：addi命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Addi( char code )
{
	long	src_data;
	char	mode;
	char	reg;
	char	size;
	long	save_pc;
	int	work_mode;
	long	dest_data;

#ifdef TEST_CCR
	short before;
#endif

	save_pc = pc;
	size = ((code >> 6) & 0x03);
	if ( size == 3 ) {
		err68a( "不正なアクセスサイズです。", __FILE__, __LINE__ );
		return( TRUE );
	}
	mode = (code & 0x38) >> 3;
	reg  = (code & 0x07);

	src_data = imi_get( size );

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &dest_data)) {
		return(TRUE);
	}

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif

	/* ワークレジスタへコピー */
	rd[8] = dest_data;

	/* Add演算 */
	// rd [ 8 ] = add_rd( 8, src_data, size );
	rd [ 8 ] = add_long(src_data, dest_data, size );

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, rd[8])) {
		return(TRUE);
	}

	/* フラグの変化 */
	add_conditions(src_data, dest_data, rd[8], size, 1);

#ifdef TEST_CCR
	check("addi", src_data, dest_data, rd[8], size, before);
#endif

	return( FALSE );
}

/*
 　機能：subi命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Subi( char code )
{
	long	src_data;
	char	mode;
	char	reg;
	char	size;
	long	save_pc;
	int	work_mode;
	long	dest_data;

#ifdef TEST_CCR
	short before;
#endif

	save_pc = pc;
	size = ((code >> 6) & 0x03);
	if ( size == 3 ) {
		err68a( "不正なアクセスサイズです。", __FILE__, __LINE__ );
		return( TRUE );
	}
	mode = (code & 0x38) >> 3;
	reg  = (code & 0x07);

	src_data = imi_get( size );

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &dest_data)) {
		return(TRUE);
	}

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif

	/* ワークレジスタへコピー */
	rd[8] = dest_data;

	/* Sub演算 */
	//rd [ 8 ] = sub_rd( 8, src_data, size );
	rd [ 8 ] = sub_long(src_data, dest_data, size );

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, rd[8])) {
		return(TRUE);
	}

	/* フラグの変化 */
	sub_conditions(src_data, dest_data, rd[8], size, 1);

#ifdef TEST_CCR
	check("subi", src_data, dest_data, rd[8], size, before);
#endif

	return( FALSE );
}

/*
 　機能：eori命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Eori( char code )
{
	char	size;
	char	mode;
	char	reg;
	long	data;
	long	src_data;
	long	save_pc;
	long	work_mode;

	save_pc = pc;
	size = ((code >> 6) & 0x03);
	mode = ((code & 0x38) >> 3);
	reg  = (code & 0x07);

	src_data = imi_get( size );

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* Eor演算 */
	data ^= src_data;

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	/* フラグのセット */
	general_conditions(data, size);

	return( FALSE );
}

/*
 　機能：eori to CCR命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Eori_t_ccr()
{
	char	data;

	data = (char)imi_get( S_BYTE );

#ifdef	TRACE
	printf( "trace: eori_t_ccr src=0x%02X PC=%06lX\n", data, pc - 2 );
#endif

	/* CCRをセット */
	if ( (data & 0x10) != 0 ) {
		if ( CCR_X_REF() == 0 )
			CCR_X_ON();
		else
			CCR_X_OFF();
	}
	if ( (data & 0x08) != 0 ) {
		if ( CCR_N_REF() == 0 )
			CCR_N_ON();
		else
			CCR_N_OFF();
	}
	if ( (data & 0x04) != 0 ) {
		if ( CCR_Z_REF() == 0 )
			CCR_Z_ON();
		else
			CCR_Z_OFF();
	}
	if ( (data & 0x02) != 0 ) {
		if ( CCR_V_REF() == 0 )
			CCR_V_ON();
		else
			CCR_V_OFF();
	}
	if ( (data & 0x01) != 0 ) {
		if ( CCR_C_REF() == 0 )
			CCR_C_ON();
		else
			CCR_C_OFF();
	}

	return( FALSE );
}

/*
 　機能：cmpi命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Cmpi( char code )
{
	char	mode;
	char	reg;
	char	size;
	long	src_data;
	long	save_pc;
	short	save_x;
	long	dest_data;

#ifdef TEST_CCR
	short	before;
	long	result;
#endif

	save_pc = pc;
	size = ((code >> 6) & 0x03);
	if ( size == 3 ) {
		err68a( "不正なアクセスサイズです。", __FILE__, __LINE__ );
		return( TRUE );
	}
	mode = (code & 0x38) >> 3;
	reg  = (code & 0x07);
	save_x = CCR_X_REF();

	src_data = imi_get( size );

	if (get_data_at_ea(EA_VariableData, mode, reg, size, &dest_data)) {
		return(TRUE);
	}

	/* ワークレジスタへコピー */
	rd[8] = dest_data;

#ifdef TEST_CCR
	before = sr & 0x1f;
#endif

	/* Sub演算 */
	// rd[8] = sub_rd( 8, src_data, size );
	rd[8] = sub_long(src_data, dest_data, size );

	if ( save_x == 0 )
		CCR_X_OFF();
	else
		CCR_X_ON();

	/* フラグの変化 */
	cmp_conditions(src_data, dest_data, rd[8], size);

#ifdef TEST_CCR
	check("cmpi", src_data, dest_data, rd[8], size, before);
#endif

	return( FALSE );
}

/*
 　機能：btst #data,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Btsti( char code )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	long	data;
	long	mask = 1;
	int	size;

	save_pc = pc;
	mode = (code & 0x38) >> 3;
	reg = (code & 0x07);
	bitno = (UChar)imi_get( S_BYTE );
	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* 実効アドレスで示されたデータを取得 */
	if (get_data_at_ea(EA_Data, mode, reg, size, &data)) {
		return(TRUE);
	}

	/* Zフラグに反映 */
	if ( (data & mask) == 0 )
		CCR_Z_ON();
	else
		CCR_Z_OFF();

#ifdef	TRACE
	printf( "trace: btst     src=%d PC=%06lX\n", bitno, save_pc );
#endif

	return( FALSE );
}

/*
 　機能：btst Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Btst( char code1, char code2 )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	long	data;
	long	mask = 1;
	int	size;

	save_pc = pc;
	mode = (code2 & 0x38) >> 3;
	reg = (code2 & 0x07);
	bitno = ((code1 >> 1) & 0x07);
	bitno = (UChar)(rd [ bitno ]);

	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* 実効アドレスで示されたデータを取得 */
	if (get_data_at_ea(EA_Data, mode, reg, size, &data)) {
		return(TRUE);
	}

	/* Zフラグに反映 */
	if ( (data & mask) == 0 )
		CCR_Z_ON();
	else
		CCR_Z_OFF();

#ifdef	TRACE
	printf( "trace: btst     src=%d PC=%06lX\n", bitno, save_pc );
#endif

	return( FALSE );
}

/*
 　機能：bchg #data,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Bchgi( char code )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	long	mask = 1;
	int	size;
	int	work_mode;
	long	data;

	save_pc = pc;
	mode = (code & 0x38) >> 3;
	reg = (code & 0x07);
	bitno = (UChar)imi_get( S_BYTE );

	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* bchg演算 */
	if ( (data & mask) == 0 ) {
		CCR_Z_ON();
		data |= mask;
	} else {
		CCR_Z_OFF();
		data &= ~mask;
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	return( FALSE );
}

/*
 　機能：bchg Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Bchg( char code1, char code2 )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	long	data;
	long	mask = 1;
	int	size;
	int	work_mode;

	save_pc = pc;
	mode = (code2 & 0x38) >> 3;
	reg = (code2 & 0x07);
	bitno = ((code1 >> 1) & 0x07);
	bitno = (UChar)(rd [ bitno ]);

	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* bchg演算 */
	if ( (data & mask) == 0 ) {
		CCR_Z_ON();
		data |= mask;
	} else {
		CCR_Z_OFF();
		data &= ~mask;
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	return( FALSE );
}

/*
 　機能：bclr #data,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Bclri( char code )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	short	disp = 0;
	long	data;
	long	mask = 1;
	int	size;
	int	work_mode;

	save_pc = pc;
	mode = (code & 0x38) >> 3;
	reg = (code & 0x07);
	bitno = (UChar)imi_get( S_BYTE );

	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* bclr演算 */
	if ( (data & mask) == 0 ) {
		CCR_Z_ON();
	} else {
		CCR_Z_OFF();
		data &= ~mask;
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	return( FALSE );
}

/*
 　機能：bclr Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Bclr( char code1, char code2 )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	short	disp = 0;
	long	data;
	long	mask = 1;
	int	size;
	int	work_mode;

	save_pc = pc;
	mode = (code2 & 0x38) >> 3;
	reg = (code2 & 0x07);
	bitno = ((code1 >> 1) & 0x07);
	bitno = (UChar)(rd [ bitno ]);

	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* bclr演算 */
	if ( (data & mask) == 0 ) {
		CCR_Z_ON();
	} else {
		CCR_Z_OFF();
		data &= ~mask;
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	return( FALSE );
}

/*
 　機能：bset #data,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Bseti( char code )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	long	data;
	short	disp = 0;
	ULong	mask = 1;
	int	size;
	int	work_mode;

	save_pc = pc;
	mode = (code & 0x38) >> 3;
	reg = (code & 0x07);
	bitno = (UChar)imi_get( S_BYTE );

	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* bset演算 */
	if ( (data & mask) == 0 ) {
		CCR_Z_ON();
		data |= mask;
	} else {
		CCR_Z_OFF();
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	return( FALSE );
}

/*
 　機能：bset Dn,<ea>命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Bset( char code1, char code2 )
{
	long	save_pc;
	char	mode;
	char	reg;
	UChar	bitno;
	long	data;
	short	disp = 0;
	ULong	mask = 1;
	int	size;
	int	work_mode;

	save_pc = pc;
	mode = (code2 & 0x38) >> 3;
	reg = (code2 & 0x07);
	bitno = ((code1 >> 1) & 0x07);
	bitno = (UChar)(rd [ bitno ]);

	if ( mode == MD_DD ) {
		bitno = (bitno % 32);
		size  = S_LONG;
	} else {
		bitno = (bitno % 8);
		size  = S_BYTE;
	}

	mask <<= bitno;

	/* アドレッシングモードがポストインクリメント間接の場合は間接でデータの取得 */
	if (mode == EA_AIPI) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (get_data_at_ea_noinc(EA_VariableData, work_mode, reg, size, &data)) {
		return(TRUE);
	}

	/* bset演算 */
	if ( (data & mask) == 0 ) {
		CCR_Z_ON();
		data |= mask;
	} else {
		CCR_Z_OFF();
	}

	/* アドレッシングモードがプレデクリメント間接の場合は間接でデータの設定 */
	if (mode == EA_AIPD) {
		work_mode = EA_AI;
	} else {
		work_mode = mode;
	}

	if (set_data_at_ea(EA_VariableData, work_mode, reg, size, data)) {
		return(TRUE);
	}

	return( FALSE );
}

/*
 　機能：movep from Dn命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Movep_f( char code1, char code2 )
{
	char	d_reg;
	char	a_reg;
	short	disp;
	long	adr;

	d_reg = ((code1 >> 1) & 0x07);
	a_reg = (code2 & 0x07);
	disp = (UChar)imi_get( S_WORD );
	adr = ra [ a_reg ] + disp;

	if ( (code2 & 0x40) != 0 ) {
		/* LONG */
		mem_set( adr, ((rd [ d_reg ] >> 24) & 0xFF), S_BYTE );
		mem_set( adr + 2, ((rd [ d_reg ] >> 16) & 0xFF), S_BYTE );
		mem_set( adr + 4, ((rd [ d_reg ] >> 8) & 0xFF), S_BYTE );
		mem_set( adr + 6, rd [ d_reg ] & 0xFF, S_BYTE );
	} else {
		/* WORD */
		mem_set( adr, ((rd [ d_reg ] >> 8) & 0xFF), S_BYTE );
		mem_set( adr + 2, rd [ d_reg ] & 0xFF, S_BYTE );
	}

#ifdef	TRACE
	printf( "trace: movep_f  src=%d PC=%06lX\n", rd [ d_reg ], pc - 2 );
#endif

	return( FALSE );
}

/*
 　機能：movep to Dn命令を実行する
 戻り値： TRUE = 実行終了
         FALSE = 実行継続
*/
static	int	Movep_t( char code1, char code2 )
{
	char	d_reg;
	char	a_reg;
	short	disp;
	ULong	data;
	long	adr;

	d_reg = ((code1 >> 1) & 0x07);
	a_reg = (code2 & 0x07);
	disp = (UChar)imi_get( S_WORD );
	adr = ra [ a_reg ] + disp;

	data = mem_get( adr, S_BYTE );
	data = ((data << 8) | (mem_get( adr + 2, S_BYTE ) & 0xFF));
	if ( (code2 & 0x40) != 0 ) {	/* LONG */
		data = ((data << 8) | (mem_get( adr + 4, S_BYTE ) & 0xFF));
		data = ((data << 8) | (mem_get( adr + 6, S_BYTE ) & 0xFF));
		rd [ d_reg ] = data;
	} else {
		rd [ d_reg ] = ((rd [ d_reg ] & 0xFFFF0000) | (data & 0xFFFF));
	}

#ifdef	TRACE
	printf( "trace: movep_t  PC=%06lX\n", pc - 2 );
#endif

	return( FALSE );
}

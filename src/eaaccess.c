/* $Id: eaaccess.c,v 1.3 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2009/08/05 14:44:33  masamic
 * Some Bug fix, and implemented some instruction
 * Following Modification contributed by TRAP.
 *
 * Fixed Bug: In disassemble.c, shift/rotate as{lr},ls{lr},ro{lr} alway show word size.
 * Modify: enable KEYSNS, register behaiviour of sub ea, Dn.
 * Add: Nbcd, Sbcd.
 *
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.6  2000/01/09  06:49:20  yfujii
 * Push/Pop instruction's word alignment is adjusted.
 *
 * Revision 1.3  1999/12/07  12:42:08  yfujii
 * *** empty log message ***
 *
 * Revision 1.3  1999/11/04  09:05:57  yfujii
 * Wrong addressing mode selection problem is fixed.
 *
 * Revision 1.1  1999/11/01  10:36:33  masamichi
 * Initial revision
 *
 *
 */

/* Get from / Set to Effective Address */

#include "run68.h"

/*
 * 【説明】
 *   実効アドレスを取得する。
 *
 * 【関数書式】
 *   retcode = get_ea(save_pc, AceptAdrMode, mode, reg, &data);
 *
 * 【引数】
 *   long save_pc;      <in>  PC相対時の基準となるPC値
 *   int  AceptAdrMode; <in>  アドレッシングモード MD_??
 *   int  mode;         <in>  アドレッシングモード MD_??
 *   int  reg;          <in>  レジスタ番号またはアドレッシングモード　MR_??
 *   long *data;        <out> 取得するデータを格納する場所へのポインタ
 *
 * 【返値】
 *   TURE:  エラー
 *   FALSE: 正常
 *
 */

BOOL get_ea(long save_pc, int AceptAdrMode, int mode, int reg, long *data)
{
	short	disp;
	long	idx;
	BOOL	retcode = FALSE;

	/* 操作しやすいようにモードを統合 */
	int gmode = (mode < 7) ? mode : (7 + reg);	/* gmode = 0-11 */

	/* AceptAdrMode で許されたアドレッシングモードでなければエラー */

	if ((AceptAdrMode & (1 << gmode)) == 0) {

		err68a( "アドレッシングモードが異常です。", __FILE__, __LINE__ );
		return TRUE;

	}

	/* アドレッシングモードに応じた処理 */
	switch (gmode) {
		case EA_AI:
			*data = ra [ reg ];
			break;
		case EA_AID:
			disp = (short)imi_get( S_WORD );
			*data = ra [ reg ] + (int)disp;
			break;
		case EA_AIX:
			idx = idx_get();
			*data = ra [ reg ] + idx;
			break;
		case EA_SRT:
			idx = imi_get( S_WORD );
			if ( (idx & 0x8000) != 0 )
				idx |= 0xFFFF0000;
			*data = idx;
			break;
		case EA_LNG:
			*data = imi_get( S_LONG );
			break;
		case EA_PC:
			disp = (short)imi_get( S_WORD );
			*data = save_pc + (int)disp;
			break;
		case EA_PCX:
			idx = idx_get();
			*data = save_pc + idx;
			break;
		default:
			err68a( "アドレッシングモードが異常です。", __FILE__, __LINE__ );
			retcode = TRUE;
	}
	return( retcode );
}

/* Get Data at Effective Address */

/*
 * 【説明】
 *   実効アドレスで示された値を取得する。
 *
 * 【関数書式】
 *   retcode = get_data_at_ea(AceptAdrMode, mode, reg, &data);
 *
 * 【引数】
 *   int AceptAdrMode; <in>  処理可能なアドレッシングモード群 EA_????*
 *   int mode;         <in>  アドレッシングモード MD_??
 *   int reg;          <in>  レジスタ番号またはアドレッシングモード　MR_??
 *   long *data;       <out> 取得するデータを格納する場所へのポインタ
 *
 * 【返値】
 *   TURE:  エラー
 *   FALSE: 正常
 *
 */

BOOL get_data_at_ea(int AceptAdrMode, int mode, int reg, int size, long *data)
{
	short	disp;
	long	idx;
	BOOL	retcode;
	int	gmode;
	long	save_pc;

	save_pc = pc;
	retcode = FALSE;

	/* 操作しやすいようにモードを統合 */
	gmode = mode < 7 ? mode : 7 + reg;	/* gmode = 0-11 */

	/* AceptAdrMode で許されたアドレッシングモードでなければエラー */

	if ((AceptAdrMode & (1 << gmode)) == 0) {

		err68a( "アドレッシングモードが異常です。", __FILE__, __LINE__ );
		retcode = TRUE;

	} else {

		/* アドレッシングモードに応じた処理 */
		switch (gmode) {
			case EA_DD:
				switch( size ) {
					case S_BYTE:
						*data = (rd [ reg ] & 0xFF);
						break;
					case S_WORD:
						*data = (rd [ reg ] & 0xFFFF);
						break;
					case S_LONG:
						*data = rd [ reg ];
						break;
				}
				break;
			case EA_AD:
				switch( size ) {
					case S_BYTE:
						*data = (ra [ reg ] & 0xFF);
						break;
					case S_WORD:
						*data = (ra [ reg ] & 0xFFFF);
						break;
					case S_LONG:
						*data = ra [ reg ];
						break;
				}
				break;
			case EA_AI:
				*data = mem_get( ra [ reg ], (char)size );
				break;
			case EA_AIPI:
				*data = mem_get( ra [ reg ], (char)size );
				if ( reg == 7 && size == S_BYTE ) {
					/* システムスタックのポインタは常に偶数 */
					inc_ra( (char)reg, (char)S_WORD );
				} else {
					inc_ra( (char)reg, (char)size );
				}
				break;
			case EA_AIPD:
				if ( reg == 7 && size == S_BYTE ) {
					/* システムスタックのポインタは常に偶数 */
					dec_ra( (char)reg, (char)S_WORD );
				} else {
					dec_ra( (char)reg, (char)size );
				}
				*data = mem_get( ra [ reg ], (char)size );
				break;
			case EA_AID:
				disp = (short)imi_get( S_WORD );
				*data = mem_get( ra [ reg ] + disp, (char)size );
				break;
			case EA_AIX:
				idx = idx_get();
				*data = mem_get( ra [ reg ] + (int)idx, (char)size );
				break;
			case EA_SRT:
				idx = imi_get( S_WORD );
				if ( (idx & 0x8000) != 0 )
					idx |= 0xFFFF0000;
				*data = mem_get( idx, (char)size );
				break;
			case EA_LNG:
				idx = imi_get( S_LONG );
				*data = mem_get( idx, (char)size );
				break;
			case EA_PC:
				disp = (short)imi_get( S_WORD );
				*data = mem_get( save_pc + disp, (char)size );
				break;
			case EA_PCX:
				idx = idx_get();
				*data = mem_get( save_pc + idx, (char)size );
				break;
			case EA_IM:
				*data = imi_get( (char)size );
				break;
			default:
				err68a( "アドレッシングモードが異常です。", __FILE__, __LINE__ );
				retcode = TRUE;
		}
	}
	return( retcode );
}

/*
 * 【説明】
 *   与えられたデータを実効アドレスで示された場所に設定する。
 *
 * 【関数書式】
 *   retcode = set_data_at_ea(AceptAdrMode, mode, reg, data);
 *
 * 【引数】
 *   int AceptAdrMode; <in>  処理可能なアドレッシングモード群 EA_????*
 *   int mode;         <in>  アドレッシングモード MD_??
 *   int reg;          <in>  レジスタ番号またはアドレッシングモード　MR_??
 *   long data;        <in>  設定するデータ
 *
 * 【返値】
 *   TURE:  エラー
 *   FALSE: 正常
 *
 */

BOOL set_data_at_ea(int AceptAdrMode, int mode, int reg, int size, long data)
{
	short	disp;
	long	idx;
	BOOL	retcode;
	int	gmode;
	long	save_pc;

	save_pc = pc;
	retcode = FALSE;

	/* 操作しやすいようにモードを統合 */
	gmode = mode < 7 ? mode : 7 + reg;	/* gmode = 0-11 */

	/* AceptAdrMode で許されたアドレッシングモードでなければエラー */

	if ((AceptAdrMode & (1 << gmode)) == 0) {

		err68a( "アドレッシングモードが異常です。", __FILE__, __LINE__ );
		retcode = TRUE;

	} else {

		/* ディスティネーションのアドレッシングモードに応じた処理 */
		switch( gmode ) {
			case EA_DD:
				switch( size ) {
					case S_BYTE:
						rd [ reg ] = ( rd [ reg ] & 0xFFFFFF00 ) |
								( data & 0xFF);
						break;
					case S_WORD:
						rd [ reg ] = ( rd [ reg ] & 0xFFFF0000 ) |
								( data & 0xFFFF);
						break;
					case S_LONG:
						rd [ reg ] = data;
						break;
				}
				break;
			case EA_AD:
				switch( size ) {
					case S_BYTE:
						ra [ reg ] = ( ra [ reg ] & 0xFFFFFF00 ) |
								( data & 0xFF );
						break;
					case S_WORD:
						ra [ reg ] = ( ra [ reg ] & 0xFFFF0000 ) |
								( data & 0xFFFF );
						break;
					case S_LONG:
						ra [ reg ] = data;
						break;
				}
				break;
			case EA_AI:
				mem_set( ra [ reg ], data, (char)size );
				break;
			case EA_AIPI:
				mem_set( ra [ reg ], data, (char)size );
				if ( reg == 7 && size == S_BYTE ) {
					/* システムスタックのポインタは常に偶数 */
					inc_ra( (char)reg, (char)S_WORD );
				} else {
					inc_ra ( (char)reg , (char)size );
				}
				break;
			case EA_AIPD:
				if ( reg == 7 && size == S_BYTE ) {
					/* システムスタックのポインタは常に偶数 */
					dec_ra( (char)reg, (char)S_WORD );
				} else {
					dec_ra ( (char)reg , (char)size );
				}

				mem_set( ra [ reg ], data, (char)size );
				break;
			case EA_AID:
				disp = (short)imi_get( S_WORD );
				mem_set( ra [ reg ] + (int)disp, data, (char)size );
				break;
			case EA_AIX:
				idx = idx_get();
				mem_set( ra [ reg ] + idx, data, (char)size );
				break;
			case EA_SRT:
				idx = imi_get( S_WORD );
				if ( (idx & 0x8000) != 0 )
					idx |= 0xFFFF0000;
				mem_set( idx, data, (char)size );
				break;
			case EA_LNG:
				idx = imi_get( S_LONG );
				mem_set( idx, data, (char)size );
				break;
			case EA_PC:
				disp = (short)imi_get( S_WORD );
				mem_set( save_pc + (int)disp, data, (char)size );
				break;
			case EA_PCX:
				idx = idx_get();
				mem_set( save_pc + idx, data, (char)size );
				break;
			default:
				err68a( "アドレッシングモードが異常です。", __FILE__, __LINE__ );
				retcode = TRUE;
		}
	}

	return( retcode );
}

/*
 * 【説明】
 *   実効アドレスで示された値を取得する。
 *   この時、PCを移動させない。
 *
 * 【関数書式】
 *   retcode = get_data_at_ea_noinc(AceptAdrMode, mode, reg, &data);
 *
 * 【引数】
 *   int AceptAdrMode; <in>  処理可能なアドレッシングモード群 EA_????*
 *   int mode;         <in>  アドレッシングモード MD_??
 *   int reg;          <in>  レジスタ番号またはアドレッシングモード　MR_??
 *   long *data;       <out> 取得するデータを格納する場所へのポインタ
 *
 * 【返値】
 *   TURE:  エラー
 *   FALSE: 正常
 *
 */

BOOL get_data_at_ea_noinc(int AceptAdrMode, int mode, int reg, int size, long *data)
{
	long save_pc;
	BOOL retcode;

	save_pc = pc;
	retcode = get_data_at_ea(AceptAdrMode, mode, reg, size, data);
	pc = save_pc;

	return(retcode);
}

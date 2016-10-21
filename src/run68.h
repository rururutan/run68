/* $Id: run68.h,v 1.5 2009/08/08 06:49:44 masamic Exp $ */

/*
 * $Log: run68.h,v $
 * Revision 1.5  2009/08/08 06:49:44  masamic
 * Convert Character Encoding Shifted-JIS to UTF-8.
 *
 * Revision 1.4  2009/08/05 14:44:33  masamic
 * Some Bug fix, and implemented some instruction
 * Following Modification contributed by TRAP.
 *
 * Fixed Bug: In disassemble.c, shift/rotate as{lr},ls{lr},ro{lr} alway show word size.
 * Modify: enable KEYSNS, register behaiviour of sub ea, Dn.
 * Add: Nbcd, Sbcd.
 *
 * Revision 1.3  2004/12/17 07:51:06  masamic
 * Support TRAP instraction widely. (but not be tested)
 *
 * Revision 1.2  2004/12/16 12:25:12  masamic
 * It has become under GPL.
 * Maintenor name has changed.
 * Modify codes for aboves.
 *
 * Revision 1.1.1.1  2001/05/23 11:22:08  masamic
 * First imported source code and docs
 *
 * Revision 1.14  1999/12/07  12:47:54  yfujii
 * *** empty log message ***
 *
 * Revision 1.14  1999/11/29  06:24:55  yfujii
 * Some functions' prototypes are added.
 *
 * Revision 1.13  1999/11/08  10:29:30  yfujii
 * Calling convention to eaaccess.c is changed.
 *
 * Revision 1.12  1999/11/08  03:09:41  yfujii
 * Debugger command "wathchc" is added.
 *
 * Revision 1.11  1999/11/01  10:36:33  masamichi
 * Reduced move[a].l routine. and Create functions about accessing effective address.
 *
 * Revision 1.10  1999/11/01  06:23:33  yfujii
 * Some debugging functions are introduced.
 *
 * Revision 1.9  1999/10/29  13:44:04  yfujii
 * Debugging facilities are introduced.
 *
 * Revision 1.8  1999/10/27  03:44:01  yfujii
 * Macro RUN68VERSION is defined.
 *
 * Revision 1.7  1999/10/26  12:26:08  yfujii
 * Environment variable function is drasticaly modified.
 *
 * Revision 1.6  1999/10/26  01:31:54  yfujii
 * Execution history and address trap is added.
 *
 * Revision 1.5  1999/10/25  03:26:27  yfujii
 * Declarations for some flags are added.
 *
 * Revision 1.4  1999/10/20  12:52:10  yfujii
 * Add an #if directive.
 *
 * Revision 1.3  1999/10/20  06:31:09  yfujii
 * Made a little modification for Cygnus GCC.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#define RUN68VERSION "0.09a"
#if !defined(_RUN68_H_)
#define _RUN68_H_

#if defined(__GNUC__)
#define __int64 long long
#endif

#if defined(_WIN32) /* for Cygnus GCC */
#if !defined(WIN32)
#define WIN32
#endif
#endif

/*
#undef	TRACE
#undef	FNC_TRACE
*/

#if defined(WIN32)              /* Win32 APIでDOSコールをエミュレートする。*/
  #undef DOSX
#endif

#if defined(WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <setjmp.h>
#if !defined(WIN32)              /* Win32 APIでDOSコールをエミュレートする。*/
#if !defined(DOSX)
#include <limits.h>
#define MAX_PATH	PATH_MAX
#define _fcvt		fcvt
#define _gcvt		gcvt
#define _stricmp	strcasecmp
#define _strlwr(p)	{ char *s; for (s = p; *s; s++) *s = tolower(*s); }
#define _ltoa(v, p, n)	snprintf(p, n, "%l", v)
#define BOOL    	int
#endif
#define	TRUE		-1
#define	FALSE		0
#endif
#define	XHEAD_SIZE	0x40		/* Xファイルのヘッダサイズ */
#define	HUMAN_HEAD	0x6800		/* Humanのメモリ管理ブロック位置 */
#define	FCB_WORK	0x20F00		/* DOSCALL GETFCB用ワーク領域 */
#define	HUMAN_WORK	0x21000		/* 割り込み処理先等のワーク領域 */
#define	TRAP0_WORK	0x20FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP1_WORK	0x21FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP2_WORK	0x22FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP3_WORK	0x23FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP4_WORK	0x24FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP5_WORK	0x25FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP6_WORK	0x26FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP7_WORK	0x27FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	TRAP8_WORK	0x28FF0000	/* TRAP割り込み処理先等のワーク領域 */
#define	ENV_TOP		0x21C00
#define	ENV_SIZE	0x2000
#define	STACK_TOP	ENV_TOP + ENV_SIZE
#define	STACK_SIZE	0x10000		/* 64KB */
#define	MB_SIZE		16
#define	PSP_SIZE	MB_SIZE + 240
#define	PROG_TOP	(STACK_TOP + STACK_SIZE + PSP_SIZE)
#define	NEST_MAX	20
#define	FILE_MAX	20

#define	RAS_INTERVAL	10000	/* ラスタ割り込みの間隔 */

#define	S_BYTE	0	/* BYTEサイズ */
#define	S_WORD	1	/* WORDサイズ */
#define	S_LONG	2	/* LONGサイズ */

#define	MD_DD	0	/* データレジスタ直接 */
#define	MD_AD	1	/* アドレスレジスタ直接 */
#define	MD_AI	2	/* アドレスレジスタ間接 */
#define	MD_AIPI	3	/* ポストインクリメント・アドレスレジスタ間接 */
#define	MD_AIPD	4	/* プリデクリメント・アドレスレジスタ間接 */
#define	MD_AID	5	/* ディスプレースメント付きアドレスレジスタ間接 */
#define	MD_AIX	6	/* インデックス付きアドレスレジスタ間接 */
#define	MD_OTH	7	/* その他 */

#define	MR_SRT	0	/* 絶対ショート */
#define	MR_LNG	1	/* 絶対ロング */
#define	MR_PC	2	/* プログラムカウンタ相対 */
#define	MR_PCX	3	/* インデックス付きプログラムカウンタ相対 */
#define	MR_IM	4	/* イミディエイトデータ */

/* Replace from MD_xx, MR_xx */
#define	EA_DD	0	/* データレジスタ直接 */
#define	EA_AD	1	/* アドレスレジスタ直接 */
#define	EA_AI	2	/* アドレスレジスタ間接 */
#define	EA_AIPI	3	/* ポストインクリメント・アドレスレジスタ間接 */
#define	EA_AIPD	4	/* プリデクリメント・アドレスレジスタ間接 */
#define	EA_AID	5	/* ディスプレースメント付きアドレスレジスタ間接 */
#define	EA_AIX	6	/* インデックス付きアドレスレジスタ間接 */
#define	EA_SRT	7	/* 絶対ショート */
#define	EA_LNG	8	/* 絶対ロング */
#define	EA_PC	9	/* プログラムカウンタ相対 */
#define	EA_PCX	10	/* インデックス付きプログラムカウンタ相対 */
#define	EA_IM	11	/* イミディエイトデータ */

/* 選択可能実効アドレス組み合わせ          fedc ba98 7654 3210 */
#define EA_All			0x0fff	/* 0000 1111 1111 1111 */
#define EA_Control		0x07e4	/* 0000 0111 1110 0100 */
#define EA_Data			0x0ffd	/* 0000 1111 1111 1101 */
#define EA_PreDecriment		0x01f4	/* 0000 0001 1111 0100 */
#define EA_PostIncrement	0x07ec	/* 0000 0111 1110 1100 */
#define EA_VariableData		0x01fd	/* 0000 0001 1111 1101 */
#define EA_Variable		0x01ff	/* 0000 0001 1111 1111 */
#define EA_VariableMemory	0x01fc	/* 0000 0001 1111 1100 */

/* EaAccess.c */
BOOL get_data_at_ea(int AceptAdrMode, int mode, int reg, int size, long *data) ;
BOOL set_data_at_ea(int AceptAdrMode, int mode, int reg, int size, long data) ;
BOOL get_ea(long save_pc, int AceptAdrMode, int mode, int reg, long *data) ;

#define	CCR_X_ON()	sr |= 0x0010
#define	CCR_X_OFF()	sr &= 0xFFEF
#define	CCR_X_REF()	(sr & 0x0010)
#define	CCR_N_ON()	sr |= 0x0008
#define	CCR_N_OFF()	sr &= 0xFFF7
#define	CCR_N_REF()	(sr & 0x0008)
#define	CCR_Z_ON()	sr |= 0x0004
#define	CCR_Z_OFF()	sr &= 0xFFFB
#define	CCR_Z_REF()	(sr & 0x0004)
#define	CCR_V_ON()	sr |= 0x0002
#define	CCR_V_OFF()	sr &= 0xFFFD
#define	CCR_V_REF()	(sr & 0x0002)
#define	CCR_C_ON()	sr |= 0x0001
#define	CCR_C_OFF()	sr &= 0xFFFE
#define	CCR_C_REF()	(sr & 0x0001)
#define	SR_S_ON()	sr |= 0x2000
#define	SR_S_OFF()	sr &= 0xDFFF
#define	SR_S_REF()	(sr & 0x2000)
#define	SR_T_REF()	(sr & 0x8000)

typedef	unsigned char	UChar ;
typedef	unsigned short	UShort ;
typedef	unsigned long	ULong ;


typedef struct	{
#if defined(WIN32)
    HANDLE   fh ;
#else
	FILE     *fh ;
#endif
	unsigned date ;
	unsigned time ;
	short    mode ;
	char     nest ;
	char     name [ 89 ] ;
} FILEINFO ;

typedef struct	{
	char	env_lower ;
	char	trap_emulate ;
	char	pc98_key ;
	char	io_through ;
} INI_INFO ;

/* デバッグ用に実行した命令の情報を保存しておく構造体 */
typedef struct {
    long    pc;
    /* 本当は全レジスタを保存しておきたい。*/
    unsigned short code; /* OPコード */
    long    rmem;        /* READしたメモリ */
    char    rsize;       /* B/W/L or N(READなし) movemの場合は最後の一つ */
    long    wmem;        /* WRITEしたメモリ */
    char    wsize;       /* B/W/L or N(WRITEなし) movemの場合は最後の一つ */
    char    mnemonic[64]; /* ニーモニック(できれば) */
} EXEC_INSTRUCTION_INFO;

/* run68.c */
 /* フラグ */
extern BOOL func_trace_f;
extern BOOL trace_f;
extern long trap_pc;
extern jmp_buf jmp_when_abort;
extern unsigned short cwatchpoint;
/* 標準入力のハンドル */
#if defined(WIN32)
extern HANDLE stdin_handle;
#endif

/* 命令実行情報 */
extern EXEC_INSTRUCTION_INFO OP_info;
void	term( int ) ;

/* getini.c */
void	read_ini( char *path, char *prog ) ;
void	readenv_from_ini(char *path);

/* load.c */
FILE	*prog_open(char *, int ) ;
long	prog_read( FILE *, char *, long, long *, long *, int ) ;
int	make_psp( char *, long, long, long, long ) ;

/* exec.c */
int	prog_exec( void ) ;
int	get_cond( char ) ;
void	err68( char * ) ;
void    err68a(char *mes, char *file, int line);
void    err68b(char *mes, long pc, long ppc);
void	inc_ra( char, char ) ;
void	dec_ra( char, char ) ;
void	text_color( short ) ;
long	get_locate( void ) ;
void    OPBuf_insert(const EXEC_INSTRUCTION_INFO *op);
void    OPBuf_clear();
int     OPBuf_numentries();
const   EXEC_INSTRUCTION_INFO *OPBuf_getentry(int no);
void    OPBuf_display(int n);

/* calc.c */
long add_long(long src, long dest, int size);
long sub_long(long src, long dest, int size);

/* mem.c */
long	idx_get( void ) ;
long	imi_get ( char ) ;
long	mem_get ( long, char ) ;
void	mem_set ( long, long, char ) ;

/* doscall.c */
int	dos_call( UChar ) ;

/* iocscall.c */
int	iocs_call( void ) ;

/* key.c */
void	get_fnckey( int, char * ) ;
void	put_fnckey( int, char * ) ;
UChar	cnv_key98( UChar ) ;

/* line?.c */
int	line0( char * ) ;
int	line2( char * ) ;
int	line4( char * ) ;
int	line5( char * ) ;
int	line6( char * ) ;
int	line7( char * ) ;
int	line8( char * ) ;
int	line9( char * ) ;
int	lineb( char * ) ;
int	linec( char * ) ;
int	lined( char * ) ;
int	linee( char * ) ;
int	linef( char * ) ;

/* eaaccess.c */
BOOL get_data_at_ea_noinc(int AceptAdrMode, int mode, int reg, int size, long *data) ;

/* debugger.c */
typedef enum {
    RUN68_COMMAND_BREAK,  /* ブレークポイントの設定 */
    RUN68_COMMAND_CLEAR,  /* ブレークポイントのクリア */
    RUN68_COMMAND_CONT,   /* 実行の継続 */
    RUN68_COMMAND_DUMP,   /* メモリをダンプする */
    RUN68_COMMAND_HELP,   /* デバッガのヘルプ */
    RUN68_COMMAND_HISTORY, /* 命令の実行履歴 */
    RUN68_COMMAND_LIST,   /* ディスアセンブル */
    RUN68_COMMAND_NEXT,   /* STEPと同じ。ただし、サブルーチン呼出しはスキップ */
    RUN68_COMMAND_QUIT,   /* run68を終了する */
    RUN68_COMMAND_REG,    /* レジスタの内容を表示する */
    RUN68_COMMAND_RUN,    /* 環境を初期化してプログラム実行 */
    RUN68_COMMAND_SET,    /* メモリに値をセットする */
    RUN68_COMMAND_STEP,   /* 一命令分ステップ実行 */
    RUN68_COMMAND_WATCHC, /* 命令ウォッチ */
    RUN68_COMMAND_NULL,   /* コマンドではない(移動禁止) */
    RUN68_COMMAND_ERROR   /* コマンドエラー(移動禁止) */
} RUN68_COMMAND;

RUN68_COMMAND debugger(BOOL running);

/* conditions.c */
void general_conditions(long dest, int size);
void add_conditions(long src , long dest, long result, int size, BOOL zero_flag);
void cmp_conditions(long src , long dest, long result, int size);
void sub_conditions(long src , long dest, long result, int size, BOOL zero_flag);
void neg_conditions(long dest, long result, int size, BOOL zero_flag);
void check(char *mode, long src, long dest, long result, int size, short before);

#ifdef	MAIN
	FILEINFO finfo [ FILE_MAX ] ;	/* ファイル管理テーブル */
	INI_INFO ini_info ;		/* iniファイルの内容 */
	char	size_char [ 3 ] = { 'b', 'w', 'l' } ;
	long	ra [ 8 ] ;	/* アドレスレジスタ */
	long	rd [ 8 + 1 ] ;	/* データレジスタ */
	long	usp ;		/* USP */
	long	pc ;		/* プログラムカウンタ */
	short	sr ;		/* ステータスレジスタ */
	char	*prog_ptr ;	/* プログラムをロードしたメモリへのポインタ */
	int	trap_count ;	/* 割り込み処理中なら０ */
	long	superjsr_ret ;	/* DOSCALL SUPER_JSRの戻りアドレス */
	long	psp [ NEST_MAX ] ;	/* PSP */
	long	nest_pc [ NEST_MAX ] ;	/* 親プロセスへの戻りアドレスを保存 */
	long	nest_sp [ NEST_MAX ] ;	/* 親プロセスのスタックポインタを保存 */
	char	nest_cnt ;	/* 子プロセスを起動するたびに＋１ */
	long	mem_aloc ;	/* メインメモリの大きさ */
#else
	extern	FILEINFO finfo [ FILE_MAX ] ;
	extern	INI_INFO ini_info ;
	extern	char	size_char [ 3 ] ;
	extern	long	ra [ 8 ] ;
	extern	long	rd [ 8 + 1 ] ;
	extern	long	usp ;
	extern	long	pc ;
	extern	short	sr ;
	extern	char	*prog_ptr ;
	extern	int	trap_count ;
	extern	long	superjsr_ret ;
	extern	long	psp [ NEST_MAX ] ;
	extern	long	nest_pc [ NEST_MAX ] ;
	extern	long	nest_sp [ NEST_MAX ] ;
	extern	char	nest_cnt ;
	extern	long	mem_aloc ;
#endif

/*
０ライン命令：movep, addi, subi, cmpi, andi, eori, ori, btst, bset, bclr, bchg
１ライン命令：move.b
２ライン命令：move.l, movea.l
３ライン命令：move.w, movea.w
４ライン命令：moveccr, movesr, moveusp, movem, swap, lea, pea, link, unlk,
　　　　　　　clr, ext, neg, negx, tst, tas, not, nbcd, jmp, jsr, rtr, rts,
　　　　　　　trap, trapv, chk, rte, reset, stop, nop
５ライン命令：addq, subq, dbcc, scc
６ライン命令：bcc, bra, bsr
７ライン命令：moveq
８ライン命令：divs, divu, or, sbcd
９ライン命令：sub, suba, subx
Ｂライン命令：cmp, cmpa, cmpm, eor
Ｃライン命令：exg, muls, mulu, and, abcd
Ｄライン命令：add, adda, addx
Ｅライン命令：asl, asr, lsl, lsr, rol, ror, roxl, roxr
*/

#endif /* !defined(_RUN68_H_) */

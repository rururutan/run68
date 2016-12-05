/* $Id: load.c,v 1.2 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:08  masamic
 * First imported source code and docs
 *
 * Revision 1.5  1999/12/24  04:04:37  yfujii
 * BUGFIX:When .x or .r is ommited and specified drive or path,
 * run68 couldn't find the executable file.
 *
 * Revision 1.4  1999/12/07  12:47:10  yfujii
 * *** empty log message ***
 *
 * Revision 1.4  1999/11/29  06:11:28  yfujii
 * *** empty log message ***
 *
 * Revision 1.3  1999/10/21  13:32:01  yfujii
 * DOS calls are replaced by win32 functions.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include <string.h>
#if defined(WIN32)
#include <direct.h>
#endif
#include "run68.h"

static	UChar	xhead [ XHEAD_SIZE ];

static	long	xfile_cnv( long *, long, int );
static	int	xrelocate( long, long, long );
static	long	xhead_getl( int );
static	int	set_fname( char *, long );

/* doscall.c */
long Getenv_common(const char *name_p, char *buf_p);

static char *GetAPath(char **path_p, char *buf);

/*
  機能：
    実行ファイルをオープンする。環境変数のPATHから取得したパスを
    順番に探索して最初に見付かったファイルをオープンする。
    最初にカレントディレクトリを検索する。
  引数：
    char *fname     -- ファイル名文字列
    int  msg_flag   -- 0でない時メッセージを標準エラー出力に出力
  戻り値：
    NULL = オープンできない
    !NULL = 実行ファイルのファイルポインタ
*/
FILE    *prog_open(char *fname, int mes_flag)
{
    char    dir[MAX_PATH], fullname[MAX_PATH], cwd[MAX_PATH];
    FILE    *fp = 0;
    char    *exp = strrchr(fname, '.');
    char    env_p[4096], *p;
#if defined(WIN32) || defined(DOSX)
    char    sep_chr = '\\';
    char    sep_str[] = "\\";
#else
    char    sep_chr = '/';
    char    sep_str[] = "/";
#endif

    if (strchr(fname, sep_chr) != NULL || strchr(fname, ':') != NULL)
    {
        strcpy(fullname, fname);
        if ((fp=fopen(fullname, "rb")) != NULL)
            goto EndOfFunc;
        // ここから追加(by Yokko氏)
        strcat(fullname, ".r");
        if ((fp=fopen(fullname, "rb")) != NULL)
            goto EndOfFunc;
        strcpy(fullname, fname);
        strcat(fullname, ".x");
        if ((fp=fopen(fullname, "rb")) != NULL)
            goto EndOfFunc;
        // ここまで追加(by Yokko氏)
        goto ErrorRet;
    }
    if (exp != NULL && !_stricmp(exp, ".x") && !_stricmp(exp, ".r"))
        goto ErrorRet; /* 拡張子が違う */
#if defined(WIN32)
    GetCurrentDirectory(sizeof(cwd), cwd);
#else
    getcwd(cwd, sizeof(cwd));
#endif
    /* PATH環境変数を取得する */
#if defined(WIN32)
    Getenv_common("PATH", env_p);
    p = env_p;
#else
    p = getenv("PATH");
#endif
    for (strcpy(dir, cwd); strlen(dir) != 0; GetAPath(&p, dir))
    {
        if (exp != NULL)
        {
            strcpy(fullname, dir);
            if (dir[strlen(dir)-1] != sep_chr)
                strcat(fullname, sep_str);
            strcat(fullname, fname);
            if ((fp = fopen(fullname, "rb")) != NULL)
	        	goto EndOfFunc;
        } else
        {
            strcpy(fullname, dir);
            if (fullname[strlen(fullname)-1] != sep_chr)
                strcat(fullname, sep_str);
	        strcat(fullname, fname);
	        strcat(fullname, ".r");
    	    if ((fp=fopen(fullname, "rb")) != NULL)
	        	goto EndOfFunc;
            strcpy(fullname, dir);
            if (fullname[strlen(fullname)-1] != sep_chr)
                strcat(fullname, sep_str);
	        strcat(fullname, fname);
            strcat(fullname, ".x");
            if ((fp=fopen(fullname, "rb")) != NULL)
	        	goto EndOfFunc;
        }
    }
EndOfFunc:
    strcpy(fname, fullname);
    return fp;
ErrorRet:
    if (mes_flag == TRUE)
        fprintf(stderr, "ファイルがオープンできません\n");
    return NULL;
}

static char *GetAPath(char **path_p, char *buf)
{
    unsigned int i;

    if (path_p == NULL || *path_p == NULL || strlen(*path_p) == 0)
    {
        *buf = '\0';
        goto ErrorReturn;
    }
    for (i = 0; i < strlen(*path_p) && (*path_p)[i] != ';'; i ++)
    {
        /* 2バイトコードのスキップ */
       ;
    }
    strncpy(buf, *path_p, i);
    buf[i] = '\0';
    if ((*path_p)[i] == '\0')
    {
        *path_p = &((*path_p)[i]);
    } else
    {
        *path_p += i + 1;
    }
    return buf;
ErrorReturn:
    return NULL;
}

/*
 　機能：プログラムをメモリに読み込む(fpはクローズされる)
 戻り値：正 = 実行開始アドレス
 　　　　負 = エラーコード
*/
long	prog_read( FILE *fp, char *fname, long read_top,
		   long *prog_sz, long *prog_sz2, int mes_flag )
		/* prog_sz2はロードモード＋リミットアドレスの役割も果たす */
{
	char	*read_ptr;
	long	read_sz;
	long	pc_begin;
	int	x_flag = FALSE;
	int	loadmode;
	int	i;

	loadmode = ((*prog_sz2 >> 24) & 0x03);
	*prog_sz2 &= 0xFFFFFF;

	if ( fseek( fp, 0, SEEK_END ) != 0 ) {
		fclose( fp );
		if ( mes_flag == TRUE )
			fprintf(stderr, "ファイルのシークに失敗しました\n");
		return( -11 );
	}
	if ( (*prog_sz=ftell( fp )) <= 0 ) {
		fclose( fp );
		if ( mes_flag == TRUE )
			fprintf(stderr, "ファイルサイズが０です\n");
		return( -11 );
	}
	if ( fseek( fp, 0, SEEK_SET ) != 0 ) {
		fclose( fp );
		if ( mes_flag == TRUE )
			fprintf(stderr, "ファイルのシークに失敗しました\n");
		return( -11 );
	}
	if ( read_top + *prog_sz > *prog_sz2 ) {
		fclose( fp );
		if ( mes_flag == TRUE )
			fprintf(stderr, "ファイルサイズが大きすぎます\n");
		return( -8 );
	}

	read_sz  = *prog_sz;
	read_ptr = prog_ptr + read_top;
	pc_begin = read_top;

	/* XHEAD_SIZEバイト読み込む */
	if ( *prog_sz >= XHEAD_SIZE ) {
		if ( fread( read_ptr, 1, XHEAD_SIZE, fp ) != XHEAD_SIZE ) {
			fclose( fp );
			if ( mes_flag == TRUE )
				fprintf(stderr, "ファイルの読み込みに失敗しました\n");
			return( -11 );
		}
		read_sz -= XHEAD_SIZE;
		if ( loadmode == 1 )
			i = 0;		/* Rファイル */
		else if ( loadmode == 3 )
			i = 1;		/* Xファイル */
		else
			i = strlen( fname ) - 2;
		if ( mem_get( read_top, S_WORD ) == 0x4855 && i > 0 )
		{
			if ( loadmode == 3 ||
			     strcmp( &(fname [ i ]), ".x" ) == 0 ||
			     strcmp( &(fname [ i ]), ".X" ) == 0 ) {
				x_flag = TRUE;
				memcpy( xhead, read_ptr, XHEAD_SIZE );
				*prog_sz = read_sz;
			}
		}
		if ( x_flag == FALSE )
			read_ptr += XHEAD_SIZE;
	}

	if ( fread( read_ptr, 1, read_sz, fp ) != (size_t)read_sz ) {
		fclose( fp );
		if ( mes_flag == TRUE )
			fprintf(stderr, "ファイルの読み込みに失敗しました\n");
		return( -11 );
	}

	/* 実行ファイルのクローズ */
	fclose( fp );

	/* Xファイルの処理 */
	*prog_sz2 = *prog_sz;
	if ( x_flag == TRUE ) {
		if ( (pc_begin=xfile_cnv( prog_sz, read_top, mes_flag )) == 0 )
			return( -11 );
	}

	return( pc_begin );
}

/*
 　機能：Xファイルをコンバートする
 戻り値： 0 = エラー
 　　　　!0 = プログラム開始アドレス
*/
static	long	xfile_cnv( long *prog_size, long read_top, int mes_flag )
{
	long	pc_begin;
	long	code_size;
	long	data_size;
	long	bss_size;
	long	reloc_size;

	if ( xhead_getl( 0x3C ) != 0 ) {
		if ( mes_flag == TRUE )
			fprintf(stderr, "BINDされているファイルです\n");
		return( 0 );
	}
	pc_begin   = xhead_getl( 0x08 );
	code_size  = xhead_getl( 0x0C );
	data_size  = xhead_getl( 0x10 );
	bss_size   = xhead_getl( 0x14 );
	reloc_size = xhead_getl( 0x18 );

	if ( reloc_size != 0 ) {
		if ( xrelocate( code_size + data_size, reloc_size, read_top )
		     == FALSE ) {
			if ( mes_flag == TRUE )
				fprintf(stderr, "未対応のリロケート情報があります\n");
			return( 0 );
		}
	}

	memset( prog_ptr + read_top + code_size + data_size, 0, bss_size );
	*prog_size += bss_size;

	return( read_top + pc_begin );
}

/*
 　機能：Xファイルをリロケートする
 戻り値： TRUE = 正常終了
 　　　　FALSE = 異常終了
*/
static	int	xrelocate( long reloc_adr, long reloc_size, long read_top )
{
	long	prog_adr;
	long	data;
	UShort	disp;

	prog_adr = read_top;
	for(; reloc_size > 0; reloc_size -= 2, reloc_adr += 2 ) {
		disp = (UShort)mem_get( read_top + reloc_adr, S_WORD );
		if ( disp == 1 )
			return ( FALSE );
		prog_adr += disp;
		data = mem_get( prog_adr, S_LONG ) + read_top;
		mem_set( prog_adr, data, S_LONG );
	}

	return( TRUE );
}

/*
 　機能：xheadからロングデータをゲットする
 戻り値：データの値
*/
static	long	xhead_getl( int adr )
{
	UChar	*p;
	long	d;

	p = &( xhead [ adr ] );

	d = *(p++);
	d = ((d << 8) | *(p++));
	d = ((d << 8) | *(p++));
	d = ((d << 8) | *p);
	return( d );
}

/*
 　機能：プロセス管理テーブルを作成する
 戻り値： TRUE = 正常終了
 　　　　FALSE = 異常終了
*/
int	make_psp( char *fname, long prev_adr, long end_adr, long process_id,
		  long prog_size2 )
{
	char	*mem_ptr;

	mem_ptr = prog_ptr + ra [ 0 ];
	memset( mem_ptr, 0, PSP_SIZE );
	mem_set( ra [ 0 ],        prev_adr,   S_LONG );		/* 前 */
	mem_set( ra [ 0 ] + 0x04, process_id, S_LONG );		/* 確保プロセス */
	mem_set( ra [ 0 ] + 0x08, end_adr,    S_LONG );		/* 終わり+1 */
	mem_set( ra [ 0 ] + 0x0c, 0,          S_LONG );		/* 次 */

	mem_set( ra [ 0 ] + 0x10, ra [ 3 ], S_LONG );
	mem_set( ra [ 0 ] + 0x20, ra [ 2 ], S_LONG );
	mem_set( ra [ 0 ] + 0x30, ra [ 0 ] + PSP_SIZE + prog_size2, S_LONG );
	mem_set( ra [ 0 ] + 0x34, ra [ 0 ] + PSP_SIZE + prog_size2, S_LONG );
	mem_set( ra [ 0 ] + 0x38, ra [ 1 ], S_LONG );
	mem_set( ra [ 0 ] + 0x44, sr, S_WORD );	/* 親のSRの値 */
	mem_set( ra [ 0 ] + 0x60, 0, S_LONG );		/* 親あり */
	if ( set_fname( fname, ra [ 0 ] ) == FALSE )
		return( FALSE );

	psp [ nest_cnt ] = ra [ 0 ];
	return( TRUE );
}

/*
 　機能：プロセス管理テーブルにファイル名をセットする
 戻り値： TRUE = 正常終了
 　　　　FALSE = 異常終了
*/
static	int	set_fname( char *p, long psp_adr )
{
	char	 cud [ 67 ];
	char	 *mem_ptr;
	int	 i;

	for( i = strlen( p ) - 1; i >= 0; i-- ) {
		if ( p [ i ] == '\\' || p [ i ] == '/' || p [ i ] == ':' )
			break;
	}
	i ++;
	if ( strlen( &(p [ i ]) ) > 22 )
		return( FALSE );
	mem_ptr = prog_ptr + psp_adr + 0xC4;
	strcpy( mem_ptr, &(p [ i ]) );

	mem_ptr = prog_ptr + psp_adr + 0x82;
	if ( i == 0 ) {
		/* カレントディレクトリをセット */
#if defined(WIN32)
        {
        BOOL b;
        b = GetCurrentDirectoryA(sizeof(cud), cud);
        cud[sizeof(cud)-1] = '\0';
        }
        if (FALSE) {
#else
		if ( getcwd( cud, 66 ) == NULL ) {
#endif
            strcpy( mem_ptr, ".\\" );
		} else {
			mem_ptr -= 2;
			strcpy( mem_ptr, cud );
			if ( cud [ strlen( cud ) - 1 ] != '\\' )
				strcat( mem_ptr, "\\" );
			return( TRUE );
		}
	} else {
		p [ i ] = '\0';
		for( i--; i >= 0; i-- ) {
			if ( p [ i ] == ':' )
				break;
		}
		i ++;
		if ( strlen( &(p [ i ]) ) > 64 )
			return( FALSE );
		strcpy( mem_ptr, &(p [ i ]) );
	}

	mem_ptr = prog_ptr + psp_adr + 0x80;
	if ( i == 0 ) {
		/* カレントドライブをセット */
#if defined(WIN32)
        {
        char cpath[MAX_PATH];
        BOOL b;
        b = GetCurrentDirectoryA(sizeof(cpath), cpath);
        mem_ptr[0] = cpath[0];
        }
#elif defined(DOSX)
        dos_getdrive( &drv );
		mem_ptr [ 0 ] = drv - 1 + 'A';
#else
	mem_ptr [ 0 ] = 'A';
#endif
        mem_ptr [ 1 ] = ':';
	} else {
		memcpy( mem_ptr, p, 2 );
	}

	return( TRUE );
}

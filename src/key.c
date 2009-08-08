/* $Id: key.c,v 1.2 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.2  1999/12/07  12:43:13  yfujii
 * *** empty log message ***
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include <string.h>
#include "run68.h"

static	char	fnc_key1 [ 20 ] [ 32 ] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", ""
} ;
static	char	fnc_key2 [ 12 ] [ 6 ] = {
  "", "", "", "", "", "",
  "", "", "", "", "", ""
} ;

static	void	put_fnckey1( int, char * ) ;
static	void	put_fnckey2( int, char * ) ;

/*
 　機能：ファンクションキーに割り当てた文字列を得る
 戻り値：なし
*/
void	get_fnckey( int no, char *p )
{
	if ( no == 0 ) {
		memcpy( p, fnc_key1, 20 * 32 ) ;
		memcpy( p + 20 * 32, fnc_key2, 12 * 6 ) ;
	}
	if ( no >= 1 && no <= 20 )
		memcpy( p, fnc_key1 [ no - 1 ], 32 ) ;
	if ( no >= 21 && no <= 32 )
		memcpy( p, fnc_key2 [ no - 21 ], 6 ) ;
}

/*
 　機能：ファンクションキーに文字列を割り当てる
 戻り値：なし
*/
void	put_fnckey( int no, char *p )
{
	int	i ;

	if ( no == 0 ) {
		for( i = 0 ; i < 20 ; i++, p += 32 )
			put_fnckey1( i, p ) ;
		for( i = 0 ; i < 12 ; i++, p += 6 )
			put_fnckey2( i, p ) ;
	}
	if ( no >= 1 && no <= 20 )
		put_fnckey1( no - 1, p ) ;
	if ( no >= 21 && no <= 32 )
		put_fnckey2( no - 21, p ) ;
}

/*
 　機能：個々のファンクションキーに文字列を割り当てる（その１）
 戻り値：なし
*/
void	put_fnckey1( int no, char *p )
{
	int	kno ;
	int	i ;

	if ( *p == (char)0xFE ) {
		memcpy( fnc_key1 [ no ], p, 8 ) ;
		p += 8 ;
		strcpy( &(fnc_key1 [ no ] [ 8 ]), p ) ;
	} else {
		strcpy( fnc_key1 [ no ], p ) ;
	}
	if ( no < 10 )
		kno = 0x3B + no ;
	else
		kno = 0x54 + no - 10 ;
	if ( *p == '\0' ) {
		printf("%c[0;%d;\"%c%c\"p", 0x1B, kno, 0, kno) ;
	} else {
		for( i = 0 ; p [ i ] != '\0' ; i++ ) {
			if ( p [ i ] == 0x1A )
				return ;
		}
		printf("%c[0;%d;\"%s\"p", 0x1B, kno, p) ;
	}
}

/*
 　機能：個々のファンクションキーに文字列を割り当てる（その２）
 戻り値：なし
*/
void	put_fnckey2( int no, char *p )
{
	int	kno ;
	int	i ;

	strcpy( fnc_key2 [ no ], p ) ;
	switch( no ) {
		case 0:	/* ROLL UP */
			kno = 0x51 ;	/* PAGE DOWN */
			break ;
		case 1:	/* ROLL DOWN */
			kno = 0x49 ;	/* PAGE UP */
			break ;
		case 2:	/* INS */
			kno = 0x52 ;
			break ;
		case 3:	/* DEL */
			kno = 0x53 ;
			break ;
		case 4:	/* ↑ */
			kno = 0x48 ;
			break ;
		case 5:	/* ← */
			kno = 0x4B ;
			break ;
		case 6:	/* → */
			kno = 0x4D ;
			break ;
		case 7:	/* ↓ */
			kno = 0x50 ;
			break ;
		case 8:	/* CLR */
			kno = 0x97 ;	/* ALT+HOME */
			break ;
		case 9:	/* HELP */
			kno = 0x86 ;	/* F12 */
			break ;
		case 10:/* HOME */
			kno = 0x47 ;
			break ;
		default:/* UNDO */
			kno = 0x4F ;	/* END */
			break ;
	}
	if ( *p == '\0' ) {
		printf("%c[0;%d;\"%c%c\"p", 0x1B, kno, 0, kno) ;
	} else {
		for( i = 0 ; p [ i ] != '\0' ; i++ ) {
			if ( p [ i ] == 0x1A )
				return ;
		}
		printf("%c[0;%d;\"%s\"p", 0x1B, kno, p) ;
	}
}

/*
 　機能：キーコードを変換する
 戻り値：変換後のキーコード
*/
UChar	cnv_key98( UChar c )
{
	switch( c ) {
		case 0x0A:	/* ↓ */
			c = 0x1F;
			break ;
		case 0x0B:	/* ↑ */
			c = 0x1E;
			break ;
		case 0x0C:	/* → */
			c = 0x1C;
			break ;
		case 0x1A:	/* CLR */
			c = 0x0C;
			break ;
		case 0x1E:	/* HOME */
			c = 0x0B;
			break ;
	}
	return( c ) ;
}

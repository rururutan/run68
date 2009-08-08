/* $Id: disassemble.c,v 1.3 2009-08-08 06:49:44 masamic Exp $ */

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
 * Revision 1.1.1.1  2001/05/23 11:22:06  masamic
 * First imported source code and docs
 *
 * Revision 1.5  2000/01/09  04:22:42  yfujii
 * Automaton for making register list is fixed for buginfo0002.
 *
 * Revision 1.4  1999/12/23  08:08:16  yfujii
 * Wrong instruction strings generated for some instructions, are fixed.
 *
 * Revision 1.3  1999/12/07  12:41:31  yfujii
 * *** empty log message ***
 *
 * Revision 1.3  1999/11/30  13:27:38  yfujii
 * Wrong interpretation of 'DBcc' instruction is fixed.
 * Wrong interpretation of 'MOVE #$xxxx,$yyyyyy' is fixed.
 *
 * Revision 1.2  1999/11/22  03:58:02  yfujii
 * Wrong treatment of 'movem' instruction is fixed.
 *
 * Revision 1.1  1999/11/01  06:22:26  yfujii
 * Initial revision
 *
 */

#include <assert.h>
#include "run68.h"

/* prog_ptr_uは符号付きcharで不便なので、符号なしcharに変換しておく。*/
#define prog_ptr_u ((unsigned char *)prog_ptr)

static char *disa0(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disa1_2_3(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disa4(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disa5(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disa6(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disa7(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disa8(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disa9_d(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disab(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disac(long addr, unsigned short code, long *next_addr, char *mnemonic);
static char *disae(long addr, unsigned short code, long *next_addr, char *mnemonic);

/*
   機能：
     指定したアドレスから始まるMPU命令を文字列に変換する。
   パラメータ：
     long  addr      <in>  命令のアドレス
     long *next_addr <out> 次の命令のアドレス
   戻り値：
*/

char *disassemble(long addr, long* next_addr)
{
    static char mnemonic[64], *ptr;
    unsigned short code;

    ptr = NULL;
    *next_addr = addr;
    mnemonic[0] = '\0';
    code = (((unsigned short)prog_ptr_u[addr]) << 8) + (unsigned short)prog_ptr_u[addr+1];
    switch((code & 0xf000) >> 12)
    {
    case 0x0:
        ptr = disa0(addr, code, next_addr, mnemonic);
        break;
    case 0x1:
    case 0x2:
    case 0x3:
        ptr = disa1_2_3(addr, code, next_addr, mnemonic);
        break;
    case 0x4:
        ptr = disa4(addr, code, next_addr, mnemonic);
        break;
    case 0x5:
        ptr = disa5(addr, code, next_addr, mnemonic);
        break;
    case 0x6:
        ptr = disa6(addr, code, next_addr, mnemonic);
        break;
    case 0x7:
        ptr = disa7(addr, code, next_addr, mnemonic);
        break;
    case 0x8:
        ptr = disa8(addr, code, next_addr, mnemonic);
        break;
    case 0x9:
    case 0xd:
        ptr = disa9_d(addr, code, next_addr, mnemonic);
        break;
    case 0xb:
        ptr = disab(addr, code, next_addr, mnemonic);
        break;
    case 0xc:
        ptr = disac(addr, code, next_addr, mnemonic);
        break;
    case 0xe:
        ptr = disae(addr, code, next_addr, mnemonic);
        break;
    case 0xf:
        switch(code & 0x0f00)
        {
        case 0x0f00:
            sprintf(mnemonic, "FCALL $%02X", code & 0xff);
            ptr = mnemonic;
            break;
        case 0x0e00:
            sprintf(mnemonic, "FLOAT $%02X", code & 0xff);
            ptr = mnemonic;
            break;
        }
        *next_addr = addr + 2;
    }
    if (ptr == NULL)
    {
        return NULL;
    }
    return ptr;
}

static BOOL effective_address(long addr, short mode, short reg, char size,
                              unsigned short mask, char *str, long *next_addr);

static void fill_space(char *str, unsigned int n)
{
    unsigned int i;
    if (strlen(str) >= n)
        return;
    for (i = strlen(str); i < n; i ++)
    {
        str[i] = ' ';
    }
    str[n] = '\0';
}

static char *disa0(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    char *p;
    unsigned long d1;
    char size = '?';

    /* まずは全ビット固定の命令を調べる */
    switch(code)
    {
    case 0x003c:  /* OR Immediate to CCR */
        strcpy(mnemonic, "or");
        goto L0;
    case 0x023c:  /* AND Immediate to CCR */
        strcpy(mnemonic, "and");
        goto L0;
    case 0x0a3c:  /* XOR Immediate to CCR */
        strcpy(mnemonic, "xor");
L0:
        d1 = (unsigned short)prog_ptr_u[addr + 3];
        fill_space(mnemonic, 8);
        p = mnemonic + strlen(mnemonic);
        sprintf(p, "#$%02x,ccr", d1);
        *next_addr = addr + 4;
        return mnemonic;
    case 0x007c:  /* OR Immediate to SR */
        strcpy(mnemonic, "or");
        goto L1;
    case 0x027c:  /* AND Immediate to SR */
        strcpy(mnemonic, "and");
        goto L1;
    case 0x0a7c:  /* EOR Immediate to SR */
        strcpy(mnemonic, "eor");
L1:
        d1 = ((unsigned short)prog_ptr_u[addr + 2] << 8) + (unsigned short)prog_ptr_u[addr + 3];
        fill_space(mnemonic, 8);
        p = mnemonic + strlen(mnemonic);
        sprintf(p, "#$%04x,sr", d1);
        *next_addr = addr + 4;
        goto EndOfFunc;
    }
    /* 次に、上位8ビットのみ固定の命令を調べる */
    switch(code & 0xff00)
    {
    case 0x0000:  /* OR Immediate */
        strcpy(mnemonic, "or");
        goto L2;
    case 0x0200:  /* AND Immediate */
        strcpy(mnemonic, "and");
        goto L2;
    case 0x0400:  /* SUB Immediate */
        strcpy(mnemonic, "sub");
        goto L2;
    case 0x0600:  /* ADD Immediate */
        strcpy(mnemonic, "add");
        goto L2;
    case 0x0800:  /* Static Bit Operations */
        switch (code & 0x00c0)
        {
        case 0x0000:
            strcpy(mnemonic, "btst");
            break;
        case 0x0040:
            strcpy(mnemonic, "bchg");
            break;
        case 0x0080:
            strcpy(mnemonic, "bclr");
            break;
        case 0x00c0:
            strcpy(mnemonic, "bset");
        }
        *next_addr = addr + 4;
        d1 = ((unsigned short)prog_ptr_u[addr + 2] << 8) + (unsigned short)prog_ptr_u[addr + 3];
        size = ' ';  /* Data registers are long only. Others are byte only. */
        fill_space(mnemonic, 8);
        p = mnemonic + strlen(mnemonic);
        sprintf(p, "#%d,", d1);
        p = mnemonic + strlen(mnemonic);
        goto AddEA;
    case 0x0a00:  /* EOR Immediate */
        strcpy(mnemonic, "eor");
        goto L2;
    case 0x0c00:  /* CMP Immediate */
        strcpy(mnemonic, "cmp");
L2:
        switch (code & 0x00c0)
        {
        case 0x0000:
            strcat(mnemonic, ".b");
            *next_addr = addr + 4;
            d1 = (unsigned short)prog_ptr_u[addr + 3];
            size = 'b';
            break;
        case 0x0040:
            strcat(mnemonic, ".w");
            *next_addr = addr + 4;
            d1 = ((unsigned short)prog_ptr_u[addr + 2] << 8) + (unsigned short)prog_ptr_u[addr + 3];
            size = 'w';
            break;
        case 0x0080:
            strcat(mnemonic, ".l");
            *next_addr = addr + 6;
            d1 = ((unsigned long)prog_ptr_u[addr + 2] << 24) + ((unsigned long)prog_ptr_u[addr + 3] << 16)
               + ((unsigned long)prog_ptr_u[addr + 4] << 8) + (unsigned long)prog_ptr_u[addr + 5];
            size = 'l';
            break;
        case 0x00c0:
            /* サイズ不明 */
            goto ErrorReturn;
        }
        fill_space(mnemonic, 8);
        p = mnemonic + strlen(mnemonic);
        switch(size)
        {
        case 'b':
            sprintf(p, "#$%02x,", d1);
            break;
        case 'w':
            sprintf(p, "#$%04x,", d1);
            break;
        case 'l':
            sprintf(p, "#$%08x,", d1);
            break;
        default:
            /* 命令デコードエラー */
            goto ErrorReturn;
        }
        goto AddEA;
    }
    /* 残った命令を拾う(二つある) */
    if (code & 0x0100)
    {
        /* Dynamic Bit Operation */
        switch (code & 0x00c0)
        {
        case 0x0000:
            strcat(mnemonic, "btst");
            break;
        case 0x0040:
            strcat(mnemonic, "bchg");
            break;
        case 0x0080:
            strcpy(mnemonic, "bclr");
            break;
        case 0x00c0:
            strcat(mnemonic, "bset");
        }
        fill_space(mnemonic, 8);
        p = mnemonic + strlen(mnemonic);
        sprintf(p, "d%01d,", (code & 0x0e00) >> 9);
        *next_addr = addr + 2;
        size = ' ';  /* Data registers are long only. Others are byte only. */
        goto AddEA;
    } else if (code & 0x0038 == 0x0008)
    {
        /* MOVEP命令 */
        strcat(mnemonic, "movep");
        switch((code & 0x1c0) >> 6)
        {
        case 0x04:
            strcat(mnemonic, ".w");
            goto L4;
        case 0x05:
            strcat(mnemonic, ".l");
L4:
            fill_space(mnemonic, 8);
            p = mnemonic + strlen(mnemonic);
            d1 = ((unsigned short)prog_ptr_u[addr + 2] << 8) + (unsigned short)prog_ptr_u[addr + 3];
            sprintf(p, "%d(a%1d),d%1d", d1, code & 0x07, (code & 0x0e00) >> 9);
            *next_addr = addr + 4;
            goto EndOfFunc;
        case 0x06:
            strcat(mnemonic, ".w");
            goto L5;
        case 0x07:
            strcat(mnemonic, ".l");
L5:
            fill_space(mnemonic, 8);
            p = mnemonic + strlen(mnemonic);
            d1 = ((unsigned short)prog_ptr_u[addr + 2] << 8) + (unsigned short)prog_ptr_u[addr + 3];
            sprintf(p, "d%1d,%d(a%1d)", (code & 0x0e00) >> 9, d1, code & 0x07);
            *next_addr = addr + 4;
            goto EndOfFunc;
        default:
            goto ErrorReturn;
        }
        goto EndOfFunc;
    } else
    {
        goto ErrorReturn;
    }
AddEA:
    p = &mnemonic[strlen(mnemonic)];
    /* 即値は有り得ないのでデータサイズには' 'を与える。*/
    effective_address(*next_addr, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
EndOfFunc:
    return mnemonic;
ErrorReturn:
    return NULL;
}

/*
   機能：
   パラメータ：
     long   addr       <in>  オペランドのアドレス(使うとは限らない)
     ushort  mode       <in>  実効アドレスのモードフィールド(0-7)
     ushort  reg        <in>  実効アドレスのレジスタフィールド(0-7)
     char   size       <in>  即値の場合のデータサイズ('b'/'w'/'l')
     ushort mask       <in>  無効なモードをビット位置の1により指定
     char   *str       <out> 実効アドレスを文字列にして書き込む
     long   *next_addr <out> 次のオペランドまたは命令のアドレス
   戻り値：
     BOOL   FALSEならエラー。エラー時もnext_addrは有効。
*/
static BOOL effective_address(long addr, short mode, short reg, char size,
                              unsigned short mask, char *str, long *next_addr)
{
    short disp, ext;
    unsigned short absw;
    unsigned long  absl;
    unsigned long  imm;

    switch(mode)
    {
    case 0:  /* パターン0:データレジスタ直接 */
        sprintf(str, "d%1d", reg);
        *next_addr = addr;
        break;
    case 1:  /* パターン1:アドレスレジスタ直接 */
        sprintf(str, "a%1d", reg);
        *next_addr = addr;
        break;
    case 2:  /* パターン2:アドレスレジスタ間接 */
        sprintf(str, "(a%1d)", reg);
        *next_addr = addr;
        break;
    case 3:  /* パターン3:ポストインクリメント付きアドレスレジスタ間接 */
        sprintf(str, "(a%1d)+", reg);
        *next_addr = addr;
        break;
    case 4:  /* パターン4:プリデクリメント付きアドレスレジスタ間接 */
        sprintf(str, "-(a%1d)", reg);
        *next_addr = addr;
        break;
    case 5:  /* パターン5:ディスプレースメント付きアドレスレジスタ間接 */
        /* ディスプレースメントは符号付きのワード値である */
        disp = (short)((unsigned short)prog_ptr_u[addr] << 8) + (unsigned short)prog_ptr_u[addr + 1];
        sprintf(str, "%d(a%1d)", disp, reg);
        *next_addr = addr + 2;
        break;
    case 6:  /* パターン6:インデックス付きアドレスレジスタ間接 */
        ext = (short)((unsigned short)prog_ptr_u[addr] << 8) + (unsigned short)prog_ptr_u[addr + 1];
        sprintf(str, "%d(a%1d,%c%1d.%c)", (signed char)(ext & 0xff),
                reg, ext & 0x8000?'a':'d',
                (ext & 0x7000) >> 12, ext & 0x0800?'l':'w');
        *next_addr = addr + 2;
        break;
    case 7:  /* regフィールドで更に場合分け */
        switch(reg)
        {
        case 0x0: /* パターン7:絶対ショートアドレス */
            absw = ((unsigned short)prog_ptr_u[addr] << 8)
                 + (unsigned short)prog_ptr_u[addr + 1];
            sprintf(str, "$%06x", absw);
            *next_addr = addr + 2;
            break;
        case 0x1: /* パターン8:絶対ロングアドレス */
            absl = ((unsigned long)prog_ptr_u[addr] << 24)
                 + ((unsigned long)prog_ptr_u[addr + 1] << 16)
                 + ((unsigned long)prog_ptr_u[addr + 2] << 8)
                 + (unsigned long)prog_ptr_u[addr + 3];
            sprintf(str, "$%06x", absl);
            *next_addr = addr + 4;
            break;
        case 0x2: /* パターン9:ディスプレースメント付きPC相対 */
            /* ディスプレースメントは符号付きのワード値である */
            disp = (short)((unsigned short)prog_ptr_u[addr] << 8) + (unsigned short)prog_ptr_u[addr + 1];
            sprintf(str, "%d(pc)", disp);
            *next_addr = addr + 2;
            break;
        case 0x3: /* パターン10:インデックス付きPC相対 */
            ext = (short)((unsigned short)prog_ptr_u[addr] << 8) + (unsigned short)prog_ptr_u[addr + 1];
            sprintf(str, "%d(pc,%c%1d.%c)", (signed char)(ext & 0xff),
                    ext & 0x8000?'a':'d',
                    (ext & 0x7000) >> 12, ext & 0x0800?'l':'w');
            *next_addr = addr + 2;
            break;
        case 0x4: /* パターン11:即値(またはステータスレジスタ) */
            /* ステータスレジスタの場合はここには現れない */
            switch(size)
            {
            case 'b':
                imm = (unsigned long)prog_ptr_u[addr + 1];
                *next_addr = addr + 2;
                break;
            case 'w':
                imm = ((unsigned long)prog_ptr_u[addr] << 8)
                    + (unsigned long)prog_ptr_u[addr + 1];
                *next_addr = addr + 2;
                break;
            case 'l':
                imm = ((unsigned long)prog_ptr_u[addr] << 24)
                    + ((unsigned long)prog_ptr_u[addr + 1] << 16)
                    + ((unsigned long)prog_ptr_u[addr + 2] << 8)
                    + (unsigned long)prog_ptr_u[addr + 3];
                *next_addr = addr + 4;
                break;
            default:
                /* ここには来ないはず。*/
                goto ErrorReturn;
            }
            sprintf(str, "#$%x", imm);
            break;
        default: /* 存在しないアドレッシングモード */
            *next_addr = addr;
            goto ErrorReturn;
        }
    }
    return TRUE;
ErrorReturn:
    return FALSE;
}

static char *disa1_2_3(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    char dstr[64], sstr[64];
    BOOL b;
    char size = ' ';

    if ((code & 0x1c0) == 0x40)
    {
        /* アドレスレジスタがデスティネーションの場合は"movea"とする。*/
        switch(code & 0xf000)
        {
        case 0x2000:
            strcat(mnemonic, "movea.l");
            size = 'l';
            break;
        case 0x3000:
            strcat(mnemonic, "movea.w");
            size = 'w';
            break;
        }
    } else
    {
        switch(code & 0xf000)
        {
        case 0x1000:
            strcat(mnemonic, "move.b");
            size = 'b';
            break;
        case 0x2000:
            strcat(mnemonic, "move.l");
            size = 'l';
            break;
        case 0x3000:
            strcat(mnemonic, "move.w");
            size = 'w';
            break;
        }
    }
    fill_space(mnemonic, 8);
    b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, sstr, &addr);
    if (b == FALSE)
        goto ErrorReturn;
    b = effective_address(addr, (short)((code & 0x1c0) >> 6), (short)((code & 0xe00) >> 9), size, 0xfff, dstr, next_addr);
    if (b == FALSE)
        goto ErrorReturn;
    strcat(mnemonic, sstr);
    strcat(mnemonic, ",");
    strcat(mnemonic, dstr);
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disa4(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    signed short disp;
    char reg[10], *p, size;
    BOOL b;
    short stat, dstat;
    unsigned short regmask;
    int i;

    *next_addr = addr + 2;
    /* まずは全ビット固定の命令を処理する */
    switch(code)
    {
    case 0x4afc:
        strcat(mnemonic,"illegal");
        goto EndOfFunc;
    case 0x4e70:
        strcat(mnemonic,"reset");
        goto EndOfFunc;
    case 0x4e71:
        strcat(mnemonic,"nop");
        goto EndOfFunc;
    case 0x4e72:
        strcat(mnemonic,"stop");
        goto EndOfFunc;
    case 0x4e73:
        strcat(mnemonic,"rte");
        goto EndOfFunc;
    case 0x4e75:
        strcat(mnemonic,"rts");
        goto EndOfFunc;
    case 0x4e76:
        strcat(mnemonic,"trapv");
        goto EndOfFunc;
    case 0x4e77:
        strcat(mnemonic,"rtr");
        goto EndOfFunc;
    }
    /* 次に、13ビット固定の命令を処理する */
    switch(code & 0xfff8)
    {
    case 0x4840:
        sprintf(mnemonic, "swap    d%1d", code & 0x7);
        goto EndOfFunc;
    case 0x4880:
        sprintf(mnemonic, "ext.w   d%1d", code & 0x7);
        goto EndOfFunc;
    case 0x48c0:
        sprintf(mnemonic, "ext.l   d%1d", code & 0x7);
        goto EndOfFunc;
    case 0x4e50:
        disp = (signed short)(((unsigned short)prog_ptr_u[addr + 2] << 8)
                    + (unsigned short)prog_ptr_u[addr + 3]);
        sprintf(mnemonic, "link    a%1d,#%d", code & 0x7, disp);
        *next_addr += 2;
        goto EndOfFunc;
    case 0x4e58:
        sprintf(mnemonic, "unlk    a%1d", code & 0x7);
        goto EndOfFunc;
    case 0x4e60:
        sprintf(mnemonic, "move    a%1d,usp", code & 0x7);
        goto EndOfFunc;
    case 0x4e68:
        sprintf(mnemonic, "move    usp,a%1d", code & 0x7);
        goto EndOfFunc;
    }
    /* 次に、12ビット固定の命令を処理する */
    switch(code & 0xfff0)
    {
    case 0x4e40:
        sprintf(mnemonic, "trap    #%d", code & 0xf);
        goto EndOfFunc;
    }
    /* 次に、10ビット固定の命令を処理する */
    switch(code & 0xffc0)
    {
    case 0x40c0:
        strcat(mnemonic, "move.w  sr,");
        size = 'w';
        goto AddEA;
    case 0x44c0:
        strcat(mnemonic, "move.w  ");
        size = 'w';
        p = mnemonic + strlen(mnemonic);
        b = effective_address(addr+2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
        strcat(mnemonic, ",ccr");
        if (b == FALSE)
            goto ErrorReturn;
        goto EndOfFunc;
    case 0x46c0:
        strcat(mnemonic, "move.w  ");
        size = 'w';
        p = mnemonic + strlen(mnemonic);
        b = effective_address(addr+2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
        strcat(mnemonic, ",sr");
        if (b == FALSE)
            goto ErrorReturn;
        goto EndOfFunc;
    case 0x4800:
        strcat(mnemonic, "nbcd    ");
        size = 'l';
        goto AddEA;
    case 0x4840:
        strcat(mnemonic, "pea.l   ");
        size = 'l';
        goto AddEA;
    case 0x4ac0:
        strcat(mnemonic, "tas.b   ");
        size = 'b';
        goto AddEA;
    case 0x4e80:
        strcat(mnemonic, "jsr     ");
        size = 'l';
        goto AddEA;
    case 0x4ec0:
        strcat(mnemonic, "jmp     ");
        size = 'l';
        goto AddEA;
    case 0x4000:
        strcat(mnemonic, "negx.b  ");
        size = 'b';
        goto AddEA;
    case 0x4040:
        strcat(mnemonic, "negx.w  ");
        size = 'w';
        goto AddEA;
    case 0x4080:
        strcat(mnemonic, "negx.l  ");
        size = 'l';
        goto AddEA;
    case 0x4200:
        strcat(mnemonic, "clr.b   ");
        size = 'b';
        goto AddEA;
    case 0x4240:
        strcat(mnemonic, "clr.w   ");
        size = 'w';
        goto AddEA;
    case 0x4280:
        strcat(mnemonic, "clr.l   ");
        size = 'l';
        goto AddEA;
    case 0x4400:
        strcat(mnemonic, "neg.b   ");
        size = 'b';
        goto AddEA;
    case 0x4440:
        strcat(mnemonic, "neg.w   ");
        size = 'w';
        goto AddEA;
    case 0x4480:
        strcat(mnemonic, "neg.l   ");
        size = 'l';
        goto AddEA;
    case 0x4600:
        strcat(mnemonic, "not.b   ");
        size = 'b';
        goto AddEA;
    case 0x4640:
        strcat(mnemonic, "not.w   ");
        size = 'w';
        goto AddEA;
    case 0x4680:
        strcat(mnemonic, "not.l   ");
        size = 'l';
        goto AddEA;
    case 0x4a00:
        strcat(mnemonic, "tst.b   ");
        size = 'b';
        goto AddEA;
    case 0x4a40:
        strcat(mnemonic, "tst.w   ");
        size = 'w';
        goto AddEA;
    case 0x4a80:
        strcat(mnemonic, "tst.l   ");
        size = 'l';
        goto AddEA;
    case 0x4c80: /* MOVEM */
        strcat(mnemonic, "movem.w ");
        size = 'w';
        goto L0;
    case 0x4cc0:
        strcat(mnemonic, "movem.l ");
        size = 'l';
L0:
        p = mnemonic + strlen(mnemonic);
        b = effective_address(addr+4, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
        strcat(mnemonic, ",");
        if (b == FALSE)
            goto ErrorReturn;
        goto L1;
    case 0x4880:
        strcat(mnemonic, "movem.w ");
        size = 'w';
        goto L1;
    case 0x48c0:
        strcat(mnemonic, "movem.l ");
        size = 'l';
L1:
        /* MOVEM命令のレジスタリストをAutomatonで文字列に変換する */
        regmask = ((unsigned short)prog_ptr_u[addr + 2] << 8)
              + (unsigned short)prog_ptr_u[addr + 3];
        /* データレジスタ */
        stat = 0;
        for (i = 0; i < 8; i ++)
        {
            unsigned short e;
            if ((code & 0x38) == 0x20)
            {
                /* プレデクリメントモードの時はレジスタの順序が逆 */
                e = regmask & (0x8000 >> i);
            } else
            {
                e = regmask & (1 << i);
            }
            switch(stat)
            {
            case 0:
                if (e == 0)
                {
                    /* nothing */
                } else
                {
                    stat = 1;
                    sprintf(reg, "d%1d", i);
                    strcat(mnemonic, reg);
                }
                break;
            case 1:
                if (e == 0)
                {
                    stat = 3;
                } else
                {
                    stat = 2;
                }
                break;
            case 2:
                if (e == 0)
                {
                    stat = 3;
                    sprintf(reg, "-d%1d", i - 1);
                    strcat(mnemonic, reg);
                } else
                {
                    /* nothing */
                }
                break;
            case 3:
                if (e == 0)
                {
                    /* nothing */
                } else
                {
                    stat = 1;
                    sprintf(reg, "/d%1d", i);
                    strcat(mnemonic, reg);
                }
                break;
            }
        }
        if (stat == 2)
        {
            sprintf(reg, "-d%1d", i - 1);
            strcat(mnemonic, reg);
        }
        dstat = stat;
        stat = 0;
        /* アドレスレジスタ */
        for (i = 8; i < 16; i ++)
        {
            unsigned short e;
            if ((code & 0x38) == 0x20)
            {
                /* プレデクリメントモードの時はレジスタの順序が逆 */
                e = regmask & (0x8000 >> i);
            } else
            {
                e = regmask & (1 << i);
            }
            switch(stat)
            {
            case 0:
                if (e == 0)
                {
                    /* nothing */
                } else
                {
                    stat = 1;
                    sprintf(reg, "a%1d", i - 8);
                    if (dstat != 0)
                        strcat(mnemonic, "/");
                    strcat(mnemonic, reg);
                }
                break;
            case 1:
                if (e == 0)
                {
                    stat = 3;
                } else
                {
                    stat = 2;
                }
                break;
            case 2:
                if (e == 0)
                {
                    stat = 3;
                    sprintf(reg, "-a%1d", i - 9);
                    strcat(mnemonic, reg);
                } else
                {
                    /* nothing */
                }
                break;
            case 3:
                if (e == 0)
                {
                    /* nothing */
                } else
                {
                    stat = 1;
                    sprintf(reg, "/a%1d", i - 8);
                    strcat(mnemonic, reg);
                }
                break;
            }
        }
        if (stat == 2)
        {
            sprintf(reg, "-a%1d", i - 9);
            strcat(mnemonic, reg);
        }
        if ((code & 0x0400) == 0)
        {
            strcat(mnemonic, ",");
            p = mnemonic + strlen(mnemonic);
            b = effective_address(addr + 4, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
            if (b == FALSE)
                goto ErrorReturn;
        }
        goto EndOfFunc;
    }
    /* 最後に、CHKとLEA命令を処理する */
    switch(code & 0xf1c0)
    {
    case 0x4180:
        strcat(mnemonic, "chk.w   ");
        size = 'w';
        p = mnemonic + strlen(mnemonic);
        b = effective_address(addr+2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
        p = mnemonic + strlen(mnemonic);
        if (b == FALSE)
            goto ErrorReturn;
        sprintf(p, ",d%1d", (code & 0xe00) >> 9);
        goto EndOfFunc;
    case 0x41c0:
        strcat(mnemonic, "lea.l   ");
        size = 'l';
        p = mnemonic + strlen(mnemonic);
        b = effective_address(addr+2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
        p = mnemonic + strlen(mnemonic);
        if (b == FALSE)
            goto ErrorReturn;
        sprintf(p, ",a%1d", (code & 0xe00) >> 9);
        goto EndOfFunc;
    default:
        goto ErrorReturn;
    }
    strcat(mnemonic, "No instruction found.");
    goto ErrorReturn;
AddEA:
    p = mnemonic + strlen(mnemonic);
    b = effective_address(addr+2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
    if (b == FALSE)
        goto ErrorReturn;
EndOfFunc:
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disa5(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    char size = ' ';
    char *p;
    BOOL b;
    signed short offset;

    if ((code & 0xf8) == 0xc8)
    {
        /* DBcc */
        strcat(mnemonic, "db");
        goto L0;
    } else if ((code & 0xc0) == 0xc0)
    {
        /* Scc */
        strcat(mnemonic, "s");
L0:
        switch(code & 0xf00)
        {
        case 0x000:
            strcat(mnemonic, "t");
            break;
        case 0x100:
            strcat(mnemonic, "f");
            break;
        case 0x200:
            strcat(mnemonic, "hi");
            break;
        case 0x300:
            strcat(mnemonic, "ls");
            break;
        case 0x400:
            strcat(mnemonic, "cc");
            break;
        case 0x500:
            strcat(mnemonic, "cl");
            break;
        case 0x600:
            strcat(mnemonic, "ne");
            break;
        case 0x700:
            strcat(mnemonic, "eq");
            break;
        case 0x800:
            strcat(mnemonic, "vc");
            break;
        case 0x900:
            strcat(mnemonic, "vs");
            break;
        case 0xa00:
            strcat(mnemonic, "pl");
            break;
        case 0xb00:
            strcat(mnemonic, "mi");
            break;
        case 0xc00:
            strcat(mnemonic, "ge");
            break;
        case 0xd00:
            strcat(mnemonic, "lt");
            break;
        case 0xe00:
            strcat(mnemonic, "gt");
            break;
        case 0xf00:
            strcat(mnemonic, "le");
            break;
        }
        fill_space(mnemonic, 8);
        if ((code & 0xf8) != 0xc8)
            goto AddEA;  /* It must be Scc. */
        /* DBcc */
        offset = (signed short)((prog_ptr_u[addr + 2] << 8) + prog_ptr_u[addr + 3]);
        p = mnemonic + strlen(mnemonic);
        sprintf(p, "d%1d,$%06x", code & 7, addr + 2 + offset);
        *next_addr = addr + 4;
        goto EndOfFunc;
    } else if (code & 0x100)
    {
        /* SUBQ */
        strcat(mnemonic, "subq.");
        goto L1;
    } else
    {
        int v;
        /* ADDQ */
        strcat(mnemonic, "addq.");
L1:
        p = mnemonic + strlen(mnemonic);
        switch(code & 0xc0)
        {
        case 0x00:
            size = 'b';
            break;
        case 0x40:
            size = 'w';
            break;
        case 0x80:
            size = 'l';
            break;
        }
        *(p++) = size; *(p++) = '\0';
        fill_space(mnemonic, 8);
        p = mnemonic + strlen(mnemonic);
        v = (code & 0xe00) >> 9;
        sprintf(p, "#%d,", v==0 ? 8:v);
    }
AddEA:
    p = mnemonic + strlen(mnemonic);
    b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
    if (b == FALSE)
        goto ErrorReturn;
EndOfFunc:
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disa6(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    long jaddr;
    char *p;

    switch(code & 0xf00)
    {
    case 0x000:
        strcat(mnemonic, "bra");
        break;
    case 0x100:
        strcat(mnemonic, "bsr");
        break;
    case 0x200:
        strcat(mnemonic, "bhi");
        break;
    case 0x300:
        strcat(mnemonic, "bls");
        break;
    case 0x400:
        strcat(mnemonic, "bcc");
        break;
    case 0x500:
        strcat(mnemonic, "bcs");
        break;
    case 0x600:
        strcat(mnemonic, "bne");
        break;
    case 0x700:
        strcat(mnemonic, "beq");
        break;
    case 0x800:
        strcat(mnemonic, "bvc");
        break;
    case 0x900:
        strcat(mnemonic, "bvs");
        break;
    case 0xa00:
        strcat(mnemonic, "bpl");
        break;
    case 0xb00:
        strcat(mnemonic, "bmi");
        break;
    case 0xc00:
        strcat(mnemonic, "bge");
        break;
    case 0xd00:
        strcat(mnemonic, "blt");
        break;
    case 0xe00:
        strcat(mnemonic, "bgt");
        break;
    case 0xf00:
        strcat(mnemonic, "ble");
        break;
    default:
        goto ErrorReturn;
    }
    if (prog_ptr[addr + 1] != 0)
    {
        jaddr = addr + 2 + prog_ptr[addr + 1];
        (*next_addr) = addr + 2;
        strcat(mnemonic, ".b");
    } else
    {
        jaddr = addr + 2 +
            (short)(((unsigned short)prog_ptr[addr + 2] << 8) +
                     (unsigned short)prog_ptr[addr + 3]);
        (*next_addr) = addr + 4;
        strcat(mnemonic, ".w");
    }
    fill_space(mnemonic, 8);
    p = mnemonic + strlen(mnemonic);
    sprintf(p, "$%06X", jaddr);
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disa7(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    if (code & 0x0100)
    {
        *next_addr = addr;
        return NULL;
    } else
    {
        sprintf(mnemonic, "moveq.l #%d,d%1d", (long)((signed char)(code & 0xff)),
                (code & 0x0e00) >> 9);
    }
    *next_addr = addr + 2;
    return mnemonic;
}

static char *disa8(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    BOOL b;
    char *p, size = ' ';

    if ((code & 0x1f0) == 0x100)
    {
        /* SBCD */
        if (code & 0x8)
        {
            sprintf(mnemonic, "sbcd    (a%1d)+,(a%1d)+", (code & 0x7), (code & 0x0e00) >> 9);
        } else
        {
            sprintf(mnemonic, "sbcd    d%1d,d%1d", (code & 0x7), (code & 0x0e00) >> 9);
        }
        addr += 2;
        goto EndOfFunc;
    } else if ((code & 0x1c0) == 0x1c0)
    {
        /* DIVS */
        sprintf(mnemonic, "divs    d%1d,", (code & 0x0e00) >> 9);
        size = 'w';
        goto L0;
    } else if ((code & 0x1c0) == 0x0c0)
    {
        /* DIVU */
        sprintf(mnemonic, "divu    d%1d,", (code & 0x0e00) >> 9);
        size = 'w';
L0:
        fill_space(mnemonic, 8);
        p = mnemonic + strlen(mnemonic);
        b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, &addr);
        if (b == FALSE)
            goto ErrorReturn;
        goto EndOfFunc;
    } else
    {
        char size;
        char ea[64];
        /* OR */
        switch((code & 0x1c0) >> 6)
        {
        case 0:
            size = 'b';
            strcat(mnemonic, "or.b");
            goto L1;
        case 1:
            size = 'w';
            strcat(mnemonic, "or.w");
            goto L1;
        case 2:
            size = 'l';
            strcat(mnemonic, "or.l");
L1:
            b = effective_address(addr, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, ea, &addr);
            if (b == FALSE)
                goto ErrorReturn;
            fill_space(mnemonic, 8);
            p = mnemonic + strlen(mnemonic);
            sprintf(p, "%s,d%1d", ea, (code & 0xe00) >> 9);
            break;
        case 4:
            size = 'b';
            strcat(mnemonic, "or.b");
            goto L2;
        case 5:
            size = 'w';
            strcat(mnemonic, "or.w");
            goto L2;
        case 6:
            size = 'l';
            strcat(mnemonic, "or.l");
L2:
            b = effective_address(addr, (short)((code & 0x38) >> 3), (short)(code & 0x7), ' ', 0xfff, ea, &addr);
            if (b == FALSE)
                goto ErrorReturn;
            fill_space(mnemonic, 8);
            p = mnemonic + strlen(mnemonic);
            sprintf(p, "d%1d,%s", (code & 0xe00) >> 9, ea);
            break;
        default:
            goto ErrorReturn;
        }
    }
EndOfFunc:
    *next_addr = addr;
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disa9_d(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    BOOL b;
    char *p, size, reg = 'd';

    if ((code & 0xf130) == 0x9100 && ((code & 0xc0) >> 6) <= 2)
    {
        /* SUBX */
        strcat(mnemonic, "subx");
        goto L0;
    } else if ((code & 0xf130) == 0xd100 && ((code & 0xc0) >> 6) <= 2)
    {
        /* ADDX */
        strcat(mnemonic, "addx");
L0:
        switch((code & 0xc0) >> 6)
        {
        case 0:
            size = 'b';
            break;
        case 1:
            size = 'w';
            break;
        case 2:
            size = 'l';
            break;
        default:
            goto ErrorReturn;
        }
        p = mnemonic + strlen(mnemonic);
        if (code & 0x8)
        {
            sprintf(p, ".%c  -(a%1d),-(a%1d)", size, (code & 0x7), (code & 0xe00) >> 9);
        } else
        {
            sprintf(p, ".%c  d%1d,d%1d", size, (code & 0x7), (code & 0xe00) >> 9);
        }
        goto EndOfFunc;
    } else if ((code & 0xf000) == 0x9000)
    {
        /* SUB or SUBA*/
        strcat(mnemonic, "sub");
        goto L1;
    } else
    {
        /* ADD or ADDA */
        strcat(mnemonic, "add");
L1:
        /* ADD & SUB共通処理 */
        switch((code & 0x1c0) >> 6)
        {
        case 0:
            strcat(mnemonic, ".b");
            size = 'b';
            goto L2;
        case 3:
            strcat(mnemonic, "a");
            reg = 'a';
        case 1:
            strcat(mnemonic, ".w");
            size = 'w';
            goto L2;
        case 7:
            strcat(mnemonic, "a");
            reg = 'a';
        case 2:
            strcat(mnemonic, ".l");
            size = 'l';
L2:
            fill_space(mnemonic, 8);
            p = mnemonic + strlen(mnemonic);
            b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
            if (b == FALSE)
                goto ErrorReturn;
            p = mnemonic + strlen(mnemonic);
            sprintf(p, ",%c%1d", reg, (code & 0xe00) >> 9);
            break;
        case 4:
            strcat(mnemonic, ".b");
            goto L3;
        case 5:
            strcat(mnemonic, ".w");
            goto L3;
        case 6:
            strcat(mnemonic, ".l");
L3:
            fill_space(mnemonic, 8);
            p = mnemonic + strlen(mnemonic);
            sprintf(p, "d%1d,", (code & 0xe00) >> 9);
            p = mnemonic + strlen(mnemonic);
            b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), ' ', 0xfff, p, next_addr);
            if (b == FALSE)
                goto ErrorReturn;
            break;
        default:
            goto ErrorReturn;
        }
    }
EndOfFunc:
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disab(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    char size, reg = 'd';
    char *p;
    BOOL b;

    if ((code & 0xf138) == 0xb108 && ((code & 0xc0) >> 6) <= 2)
    {
        /* CMPM */
        strcat(mnemonic, "cmpm");
        switch((code & 0xc0) >> 6)
        {
        case 0:
            size = 'b';
            break;
        case 1:
            size = 'w';
            break;
        case 2:
            size = 'l';
            break;
        default:
            goto ErrorReturn;
        }
        sprintf(mnemonic, "cmpm.%c  (a%1d)+,(a%1d)+", size, (code & 0x3), (code & 0xe00) >> 9);
        *next_addr = addr + 2;
        goto EndOfFunc;
    }
    /* CMP or EOR*/
    switch((code & 0x1c0) >> 6)
    {
    case 0:
    case 4:
        size = 'b';
        break;
    case 3:
        reg = 'a';
    case 1:
    case 5:
        size = 'w';
        break;
    case 7:
        reg = 'a';
    case 2:
    case 6:
        size = 'l';
        break;
    default:
        goto ErrorReturn;
    }
    if ((code & 0xf100) == 0xb100 && reg != 'a')
    {
        /* EOR */
        sprintf(mnemonic, "eor.%c   d%1d,", size, (code & 0xe00) >> 9);
    } else
    {
        /* CMP */
        if (reg == 'd')
        {
            sprintf(mnemonic, "cmp.%c   ", size);
        } else
        {
            sprintf(mnemonic, "cmpa.%c  ", size);
        }
    }
    p = mnemonic + strlen(mnemonic);
    b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
    if (b == FALSE)
        goto ErrorReturn;
    if ((code & 0xf100) == 0xb100 && reg != 'a')
    {
        /* EOR */
        goto EndOfFunc;
    }
    p = mnemonic + strlen(mnemonic);
    sprintf(p, ",%c%1d", reg, (code & 0xe00) >> 9);
EndOfFunc:
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disac(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    char size;
    char *p;
    BOOL b;

    /* まず、10ビット固定の命令を処理する */
    switch(code & 0xf1f8)
    {
    case 0xc100:
        sprintf(mnemonic, "abcd.b  d%1d,d%1d", (code & 0x7), (code & 0x38) >> 9);
        *next_addr = addr + 2;
        goto EndOfFunc;
    case 0xc108:
        sprintf(mnemonic, "abcd.b  -(a%1d),-(a%1d)", (code & 0x7), (code & 0x38) >> 9);
        *next_addr = addr + 2;
        goto EndOfFunc;
    case 0xc140:
        sprintf(mnemonic, "exg.l  d%1d,d%1d", (code & 0x38) >> 9, (code & 0x7));
        *next_addr = addr + 2;
        goto EndOfFunc;
    case 0xc141:
        sprintf(mnemonic, "exg.l  a%1d,a%1d", (code & 0x38) >> 9, (code & 0x7));
        *next_addr = addr + 2;
        goto EndOfFunc;
    case 0xc181:
        sprintf(mnemonic, "exg.l  d%1d,a%1d", (code & 0x38) >> 9, (code & 0x7));
        *next_addr = addr + 2;
        goto EndOfFunc;
    }
    /* 残りの3命令を処理する */
    switch(code & 0xf1c0)
    {
    case 0xc0c0:
        /* MULU */
        strcat(mnemonic, "mulu    ");
        size = 'w';
        goto L2;
    case 0xc1c0:
        /* MULS */
        strcat(mnemonic, "muls    ");
        size = 'w';
        goto L2;
    default:
        /* AND */
        switch((code & 0x1c0) >> 6)
        {
        case 0:
            size = 'b';
            goto L0;
        case 1:
            size = 'w';
            goto L0;
        case 2:
            size = 'l';
L0:
            sprintf(mnemonic, "and.%c   ", size);
L2:
            p = mnemonic + strlen(mnemonic);
            b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
            if (b == FALSE)
                goto ErrorReturn;
            p = mnemonic + strlen(mnemonic);
            sprintf(p, ",d%1d", (code & 0xe00) >> 9);
            goto EndOfFunc;
        case 4:
            size = 'b';
            goto L1;
        case 5:
            size = 'w';
            goto L1;
        case 6:
            size = 'l';
L1:
            sprintf(mnemonic, "and.%c   d%1d,", size, (code & 0xe00) >> 9);
            p = mnemonic + strlen(mnemonic);
            b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), size, 0xfff, p, next_addr);
            if (b == FALSE)
                goto ErrorReturn;
            goto EndOfFunc;
        }
    }
EndOfFunc:
    return mnemonic;
ErrorReturn:
    return NULL;
}

static char *disae(long addr, unsigned short code, long *next_addr, char *mnemonic)
{
    char size, dir, count[10];
    char *p;
    BOOL b;

    if (code & 0x0100)
    {
        dir = 'l';
    } else
    {
        dir = 'r';
    }
    if ((code & 0xf0c0) == 0xe0c0)
    {
        switch((code & 0xc0) >> 6)
        {
        case 0:
            size = 'b';
            break;
        case 1:
            size = 'w';
            break;
        case 2:
            size = 'l';
            break;
        default:
            goto ErrorReturn;
        }

        switch((code & 0x0600) >> 9)
        {
        case 0:
            sprintf(mnemonic, "as%c.%c   ", dir, size);
            break;
        case 1:
            sprintf(mnemonic, "ls%c.%c   ", dir, size);
            break;
        case 2:
            sprintf(mnemonic, "ro%cx.%c  ", dir, size);
            break;
        case 3:
            sprintf(mnemonic, "ro%c.%c   ", dir, size);
            break;
        }
        p = mnemonic + strlen(mnemonic);
        b = effective_address(addr + 2, (short)((code & 0x38) >> 3), (short)(code & 0x7), ' ', 0xfff, p, next_addr);
        if (b == FALSE)
            goto ErrorReturn;
    } else
    {
        switch((code & 0xc0) >> 6)
        {
        case 0:
            size = 'b';
            break;
        case 1:
            size = 'w';
            break;
        case 2:
            size = 'l';
            break;
        default:
            goto ErrorReturn;
        }

        if (code & 0x20)
        {
            int iw = (code & 0x0e00) >> 9;

            sprintf(count, "d%1d", iw);
        } else
        {
            int iw = (code & 0x0e00) >> 9;

            iw = (iw == 0) ? 8 : iw;
            sprintf(count, "#%1d", iw);
        }
        switch((code & 0x0018) >> 3)
        {
        case 0:
            sprintf(mnemonic, "as%c.%c   %s,d%1d", dir, size, count, code & 0x7);
            break;
        case 1:
            sprintf(mnemonic, "ls%c.%c   %s,d%1d", dir, size, count, code & 0x7);
            break;
        case 2:
            sprintf(mnemonic, "ro%cx.%c  %s,d%1d", dir, size, count, code & 0x7);
            break;
        case 3:
            sprintf(mnemonic, "ro%c.%c   %s,d%1d", dir, size, count, code & 0x7);
            break;
        }
        *next_addr = addr + 2;
    }
    return mnemonic;
ErrorReturn:
    return NULL;
}

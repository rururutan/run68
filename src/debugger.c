/* $Id: debugger.c,v 1.2 2009-08-08 06:49:44 masamic Exp $*/

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:06  masamic
 * First imported source code and docs
 *
 * Revision 1.5  1999/12/23  08:07:58  yfujii
 * Help messages are changed.
 *
 * Revision 1.4  1999/12/07  12:40:12  yfujii
 * *** empty log message ***
 *
 * Revision 1.4  1999/12/01  13:53:48  yfujii
 * Help messages are modified.
 *
 * Revision 1.3  1999/11/29  06:16:47  yfujii
 * Disassemble and step command are implemented.
 *
 * Revision 1.2  1999/11/01  06:23:33  yfujii
 * Some debugging functions are introduced.
 *
 * Revision 1.1  1999/10/29  13:41:07  yfujii
 * Initial revision
 *
 */

#include "run68.h"

/* デバッグモードのプロンプト */
#define PROMPT "(run68)"
/* コマンドラインの最大文字列長 */
#define MAX_LINE 256

static char *command_name[] = {
    "BREAK",  /* ブレークポイントの設定 */
    "CLEAR",  /* ブレークポイントの解除 */
    "CONT",   /* 実行の継続 */
    "DUMP",   /* メモリをダンプする */
    "HELP",   /* 命令の実行履歴 */
    "HISTORY", /* 命令の実行履歴 */
    "LIST",   /* ディスアセンブル */
    "NEXT",   /* STEPと同じ。ただし、サブルーチン呼出しはスキップ */
    "QUIT",   /* run68を終了する */
    "REG",    /* レジスタの内容を表示する */
    "RUN",    /* 環境を初期化してプログラム実行 */
    "SET",    /* メモリに値をセットする */
    "STEP",   /* 一命令分ステップ実行 */
    "WATCHC"  /* 命令ウォッチ */
};

/* prog_ptr_uは符号付きcharで不便なので、符号なしcharに変換しておく。*/
#define prog_ptr_u ((unsigned char *)prog_ptr)
unsigned long stepcount;

static RUN68_COMMAND analyze(const char *line, int *argc, char** argv);
static short determine_string(const char *str);
static void display_help();
static void display_history(int argc, char **argv);
static void display_list(int argc, char **argv);
static void run68_dump(int argc, char **argv);
static void display_registers();
static void set_breakpoint(int argc, char **argv);
static void clear_breakpoint();
static unsigned long get_stepcount(int argc, char **argv);
extern char *disassemble(long addr, long* next_addr);
static unsigned short watchcode(int argc, char **argv);


/*
   機能：
     run68をデバッグモードで起動すると、この関数が呼出される。
   パラメータ：
     BOOL running  - アプリケーションプログラムの実行中はTRUEで
                     呼出される。
   戻り値：
     COMMAND - 呼び側のコードで実行すべきコマンドを表している。
*/
RUN68_COMMAND debugger(BOOL running)
{
    RUN68_COMMAND cmd;

    if (running)
    {
        long naddr, addr = pc;
        char hex[64];
        char *s = disassemble(addr, &naddr);
        unsigned short code;
        int j;

        /* まず全レジスタを表示し、*/
        display_registers();
        /* 1命令分、逆アセンブルして表示する。*/
        sprintf(hex, "$%06X ", addr);
        if (addr == naddr)
        {
            /* ディスアセンブルできなかった */
            naddr += 2;
        }
        while (addr < naddr)
        {
            char *p = hex + strlen(hex);
            code = (((unsigned short)prog_ptr_u[addr]) << 8) + (unsigned short)prog_ptr_u[addr + 1];
            sprintf(p, "%04X ", code);
            addr += 2;
        }
        for (j = strlen(hex); j < 34; j ++)
        {
            hex[j] = ' ';
        }
        hex[j] = '\0';
        if (s == NULL)
        {
            fprintf(stderr, "%s%s\n", hex, "????");
        } else
        {
            fprintf(stderr, "%s%s\n", hex, s);
        }
    } else
    {
        stepcount = 0;
    }
    if (stepcount != 0)
    {
        stepcount --;
        cmd = RUN68_COMMAND_STEP;
        goto EndOfLoop;
    }
    /* コマンドループ */
    while(TRUE)
    {
        char line[MAX_LINE];
        char *argv[MAX_LINE];
        int argc;
        fprintf(stderr, "%s", PROMPT);
        fgets(line, MAX_LINE, stdin);
        cmd = analyze(line, &argc, argv);
        if (argc == 0)
        {
            continue;
        }
        switch(cmd) {
        case RUN68_COMMAND_BREAK:  /* ブレークポイントの設定 */
            set_breakpoint(argc, argv);
            break;
        case RUN68_COMMAND_CLEAR:  /* ブレークポイントの解除 */
            clear_breakpoint();
            break;
        case RUN68_COMMAND_CONT:   /* 実行の継続 */
            if (!running)
            {
                fprintf(stderr, "Program is not running!\n");
                break;
            }
            stepcount = get_stepcount(argc, argv);
            goto EndOfLoop;
        case RUN68_COMMAND_DUMP:   /* メモリをダンプする */
            run68_dump(argc, argv);
            break;
        case RUN68_COMMAND_HELP:   /* デバッガのヘルプ */
            display_help();
            break;
        case RUN68_COMMAND_HISTORY: /* 命令の実行履歴 */
            display_history(argc, argv);
            break;
        case RUN68_COMMAND_LIST:   /* ディスアセンブル */
            display_list(argc, argv);
            break;
        case RUN68_COMMAND_NEXT:   /* STEPと同じ。ただし、サブルーチン呼出しはスキップ */
            if (!running)
            {
                fprintf(stderr, "Program is not running!\n");
                break;
            }
            goto EndOfLoop;
        case RUN68_COMMAND_QUIT:   /* run68を終了する */
            goto EndOfLoop;
        case RUN68_COMMAND_REG:    /* レジスタの値を表示する */
            display_registers();
            break;
        case RUN68_COMMAND_RUN:    /* 環境を初期化してプログラム実行 */
            goto EndOfLoop;
        case RUN68_COMMAND_SET:    /* メモリに値をセットする */
            fprintf(stderr, "cmd:%s is not implemented yet.\n", argv[0]);
            break;
        case RUN68_COMMAND_STEP:   /* 一命令分ステップ実行 */
            if (!running)
            {
                fprintf(stderr, "Program is not running!\n");
                break;
            }
            stepcount = get_stepcount(argc, argv);
            goto EndOfLoop;
        case RUN68_COMMAND_WATCHC: /* 命令ウォッチ */
            cwatchpoint = watchcode(argc, argv);
            break;
        case RUN68_COMMAND_NULL:   /* コマンドではない(移動禁止) */
            fprintf(stderr, "cmd:%s is not a command.\n", argv[0]);
            break;
        case RUN68_COMMAND_ERROR:  /* コマンドエラー(移動禁止) */
            fprintf(stderr, "Command line error:\"%s\"\n", argv[0]);
            break;
        }
    }
EndOfLoop:
    return cmd;
}

/*
   機能：
     コマンドライン文字列を解析し、コマンドとその引き数を取り出す。
   パラメータ：
     const char* line  <in>  コマンドライン文字列
     int*        argc  <out> コマンドラインに含まれるトークン数
     char**      argv  <out> トークンに分解された文字列の配列
   戻り値：
     COMMAND コマンドの列挙値
 */
static RUN68_COMMAND analyze(const char *line, int *argc, char** argv)
{
    static char cline[MAX_LINE*2];
    unsigned int ac = 0, i;
    char *q = cline;

    *argc = 0;
    for (i = 0; i < strlen(line); i ++)
    {
        /* 空白文字を読み飛ばす。*/
        const char *p = &line[i];
        char c = toupper(*p++);
        if (c == ' ' || c == '\t')
        {
            continue;
        } else if ('A' <= c && c <= 'Z')
        {
            /* コマンド等の名前 */
            argv[(*argc)++] = q;
            do {
                *q++ = c;
                c = toupper(*p++);
            } while('A' <= c && c <= 'Z' || '0' <= c && c <= '9' || c == '_');
            *q++ = '\0';
            i += strlen(argv[*argc - 1]);
        } else if ('0' <= c && c <= '9')
        {
            /* 10進数 */
            argv[(*argc)++] = q;
            do {
                *(q++) = c;
                c = toupper(*p++);
            } while('0' <= c && c <= '9');
            *q++ = '\0';
            i += strlen(argv[*argc - 1]);
        } else if (c == '$' && 'A' <= toupper(*p) && toupper(*p) <= 'F' || '0' <= *p && *p <= '9')
        {
            /* 16進数は$記号を付ける。*/
            argv[(*argc)++] = q;
            *q++ = c;
            c = toupper(*p++);
            do {
                *q++ = c;
                c = toupper(*p++);
            } while('A' <= c && c <= 'F' || '0' <= c && c <= '9');
            *q++ = '\0';
            i += strlen(argv[*argc - 1]);
        }
    }
    if (*argc == 0)
    {
        return RUN68_COMMAND_NULL;
    } else if ('A' <= argv[0][0] && argv[0][0] <= 'Z')
    {
        RUN68_COMMAND cmd;
        for (cmd = (RUN68_COMMAND)0; cmd < RUN68_COMMAND_NULL; cmd ++)
        {
            if (strcmp(argv[0], command_name[cmd]) == 0)
            {
                return cmd;
            }
        }
        return RUN68_COMMAND_ERROR;
    } else
    {
        return RUN68_COMMAND_ERROR;
    }
}

/* 文字列が名前か、10進数値か、16進数か、あるいは記号かを判定する。*/
static short determine_string(const char *str)
{
    /* とりあえずいい加減な実装をする。*/
    if ('A' <= str[0] && str[0] <= 'Z')
    {
        return 0; /* 名前 */
    } else if ('0' <= str[0] && str[0] <= '9')
    {
        return 1; /* 10進数 */
    } else if (str[0] == '$')
    {
        return 2; /* 16進数 */
    }
    return 3; /* 記号 */
}

static void display_help()
{
    fprintf(stderr, "       ============= run68 debugger commands =============\n");
    fprintf(stderr, "break $adr      - Set a breakpoint.\n");
    fprintf(stderr, "clear           - Clear the breakpoint.\n");
    fprintf(stderr, "cont            - Continue running.\n");
    fprintf(stderr, "cont n          - Continue running and stops after executing n instructions.\n");
    fprintf(stderr, "dump $adr [n]   - Dump memory (n bytes) from $adr.\n");
    fprintf(stderr, "dump [n]        - Dump memory (n bytes) continuously.\n");
    fprintf(stderr, "help            - Show this menu.\n");
    fprintf(stderr, "history [n]     - Show last n instructions executed.\n");
    fprintf(stderr, "list $adr [n]   - Disassemble from $adr.\n");
    fprintf(stderr, "list [n]        - Disassemble n instructions from current PC.\n");
    fprintf(stderr, "quit            - Quit from run68.\n");
    fprintf(stderr, "reg             - Display registers.\n");
    fprintf(stderr, "run             - Run Human68k program from the begining.\n");
    fprintf(stderr, "step            - Execute only one instruction.\n");
    fprintf(stderr, "step n          - Continue running with showing all registers\n");
    fprintf(stderr, "                  and stops after executing n instructions.\n");
}

static void run68_dump(int argc, char **argv)
{
    static long dump_addr = -1;
    static long size = 32;
    long sadr;
    int i, j;

    if (dump_addr == -1)
    {
        if (argc == 1)
        {
            fprintf(stderr, "run68-dump:You must specify $adr at least once.\n");
            return;
        }
    } else
    {
        sadr = dump_addr;
    }
    if (2 <= argc)
    {
        if (determine_string(argv[1]) == 1)
        {
            sscanf(argv[1], "%d", &size);
        } else if (argc >= 2 && determine_string(argv[1]) == 2)
        {
            sscanf(&argv[1][1], "%x", &sadr);
        } else
        {
            fprintf(stderr, "run68-dump:Argument error.\n");
            return;
        }
        if (argc == 3)
        {
            if (determine_string(argv[2]) == 1)
            {
                sscanf(argv[2], "%d", &size);
            } else
            {
                fprintf(stderr, "run68-dump:Argument error.\n");
                return;
            }
        }
    }
    for (i = 0; i < size; i ++)
    {
        unsigned long d;
        d = (unsigned char)prog_ptr_u[sadr+i];
        if (i % 16 == 0)
        {
            fprintf(stderr, "%06X:", sadr+i);
        } else if (i % 8 == 0)
        {
            fprintf(stderr, "-");
        } else
        {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "%02X", d);
        if (i % 16 == 15 || i == size - 1)
        {
            if (i % 16 != 15)
            {
                for (j = i % 16 + 1; j < 16; j ++)
                {
                    fprintf(stderr, "   ");
                }
            }
            fprintf(stderr, ":");
            for (j = i & 0xfffffff0; j <= i; j ++)
            {
                d = (unsigned char)prog_ptr_u[sadr+j];
                fprintf(stderr, "%c", (' ' <= d && d <= 0x7e) ? d : '.');
            }
            fprintf(stderr, "\n");
        }
    }
    dump_addr = sadr + size;
}

static void display_registers()
{
    int i;
	fprintf(stderr, "D0-D7=%08lX" , rd [ 0 ] );
	for ( i = 1; i < 8; i++ ) {
		fprintf(stderr, ",%08lX" , rd [ i ] );
	}
	fprintf(stderr,"\n");
	fprintf(stderr, "A0-A7=%08lX" , ra [ 0 ] );
	for ( i = 1; i < 8; i++ ) {
		fprintf(stderr, ",%08lX" , ra [ i ] );
	}
	fprintf(stderr,"\n");
	fprintf(stderr, "  PC=%08lX    SR=%04X\n" , pc, sr );
}

static void set_breakpoint(int argc, char **argv)
{
    if (argc < 2)
    {
        if (trap_pc == 0)
        {
            fprintf(stderr, "run68-break:No breakpoints set.\n");
        } else
        {
            fprintf(stderr, "run68-break:Breakpoint is set to $%06X.\n", trap_pc);
        }
        return;
    } else if (determine_string(argv[1]) != 2)
    {
        fprintf(stderr, "run68-break:Address expression error.\n");
        return;
    }
    sscanf(&argv[1][1], "%lx", &trap_pc);
}

static void clear_breakpoint()
{
    trap_pc = 0;
}

static void display_history(int argc, char **argv)
{
    int n = 0;
    if (argc == 1)
    {
        n = 10;
    }else if (determine_string(argv[1]) != 1)
    {
        fprintf(stderr, "run68-history:Argument error.\n");
    } else
    {
        sscanf(argv[1], "%d", &n);
    }
    OPBuf_display(n);
}

static void display_list(int argc, char **argv)
{
    static long list_addr = 0;
    static long old_pc = 0;
    long addr, naddr;
    int  i, j, n;

    n = 10;
    if (old_pc == 0)
    {
        old_pc = pc;
    } else if (old_pc != pc)
    {
        old_pc = pc;
        list_addr = 0;
    }
    if (list_addr == 0)
    {
        addr = pc;
    } else
    {
        addr = list_addr;
    }
    if (2 <= argc)
    {
        if (argc == 2 && determine_string(argv[1]) == 1)
        {
            sscanf(argv[1], "%d", &n);
        } else if (argc >= 2 && determine_string(argv[1]) == 2)
        {
            sscanf(&argv[1][1], "%x", &addr);
        }
        if (argc == 3 && determine_string(argv[2]) == 1)
        {
            sscanf(argv[2], "%d", &n);
        }
    }
    for (i = 0; i < n; i ++)
    {
        char *s = disassemble(addr, &naddr);
        char hex[64];
        unsigned short code;

        sprintf(hex, "$%06X ", addr);
        if (addr == naddr)
        {
            /* ディスアセンブルできなかった */
            naddr += 2;
        }
        while (addr < naddr)
        {
            char *p = hex + strlen(hex);
            code = (((unsigned short)prog_ptr_u[addr]) << 8) + (unsigned short)prog_ptr_u[addr + 1];
            sprintf(p, "%04X ", code);
            addr += 2;
        }
        for (j = strlen(hex); j < 34; j ++)
        {
            hex[j] = ' ';
        }
        hex[j] = '\0';
        if (s == NULL)
        {
            fprintf(stderr, "%s%s\n", hex, "????");
        } else
        {
            fprintf(stderr, "%s%s\n", hex, s);
        }
    }
    list_addr = naddr;
}

static unsigned long get_stepcount(int argc, char **argv)
{
    unsigned long count = 0;
    if (argc == 1)
    {
        return 0;
    } else if (determine_string(argv[1]) == 1)
    {
        sscanf(argv[1], "%lu", &count);
    }
    return count;
}

static unsigned short watchcode(int argc, char **argv)
{
    unsigned short wcode;

    if (argc < 2)
    {
        fprintf(stderr, "run68-watchcode:Too few arguments.\n");
        return 0x4afc;
    } else if (determine_string(argv[1]) != 2)
    {
        fprintf(stderr, "run68-watchcode:Instruction code expression error.\n");
        return 0x4afc;
    }
    sscanf(&argv[1][1], "%hx", &wcode);
    return wcode;
}

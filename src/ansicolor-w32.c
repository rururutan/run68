//
// Console out with human68k escape sequence suuport.
//
// original : https://github.com/mattn/ansicolor-w32.c
// auther   : mattn
// license  : MIT
// 

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <mbstring.h>
#include <io.h>

#ifndef FOREGROUND_MASK
# define FOREGROUND_MASK (FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#endif
#ifndef BACKGROUND_MASK
# define BACKGROUND_MASK (BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_INTENSITY)
#endif

int
WriteW32(short _type, HANDLE* handle, const char* _buf, size_t _len) {
  static WORD attr_olds[2] = {-1, -1}, attr_old;
  static int first = 1;
  size_t len;
  int type = 0;
  WORD attr;
  DWORD written, csize;
  CONSOLE_CURSOR_INFO cci;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  COORD coord;
  const char *ptr;
 
  if (_type == 1) {
    type = 0;
  } else if (_type == 2) {
    type = 1;
  } else {
    type = 0;
  }

  GetConsoleScreenBufferInfo(handle, &csbi);
  attr = csbi.wAttributes;

  if (attr_olds[type] == (WORD) -1) {
    attr_olds[type] = attr;
  }
  attr_old = attr;

  len = _len;
  ptr = _buf;

  while (*ptr) {
    if (*ptr == '\033') {
      unsigned char c;
      int i, n = 0, m, v[6], w, h;
      for (i = 0; i < 6; i++) v[i] = -1;
      ptr++;
retry:
      if ((c = *ptr++) == 0) break;
      if (isdigit(c)) {
        if (v[n] == -1) v[n] = c - '0';
        else v[n] = v[n] * 10 + c - '0';
        goto retry;
      }
      if (c == '[') {
        goto retry;
      }
      if (c == ',') {
        if (++n == 6) break;
        goto retry;
      }
      if (c == '>' || c == '?') {
        m = c;
        goto retry;
      }

      switch (c) {
        case 'h':
          if (m == '?') {
            for (i = 0; i <= n; i++) {
              switch (v[i]) {
                case 3:
                  GetConsoleScreenBufferInfo(handle, &csbi);
                  w = csbi.dwSize.X;
                  h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
                  csize = w * (h + 1);
                  coord.X = 0;
                  coord.Y = csbi.srWindow.Top;
                  FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
                  FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
                  SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
                  csbi.dwSize.X = 132;
                  SetConsoleScreenBufferSize(handle, csbi.dwSize);
                  csbi.srWindow.Right = csbi.srWindow.Left + 131;
                  SetConsoleWindowInfo(handle, TRUE, &csbi.srWindow);
                  break;
                case 5:
                  attr =
                    ((attr & FOREGROUND_MASK) << 4) |
                    ((attr & BACKGROUND_MASK) >> 4);
                  SetConsoleTextAttribute(handle, attr);
                  break;
                case 9:
                  break;
                case 25:
                  GetConsoleCursorInfo(handle, &cci);
                  cci.bVisible = TRUE;
                  SetConsoleCursorInfo(handle, &cci);
                  break;
                case 47:
                  coord.X = 0;
                  coord.Y = 0;
                  SetConsoleCursorPosition(handle, coord);
                  break;
                default:
                  break;
              }
            }
          } else if (m == '>' && v[0] == 5) {
            GetConsoleCursorInfo(handle, &cci);
            cci.bVisible = FALSE;
            SetConsoleCursorInfo(handle, &cci);
          }
          break;
        case 'l':
          if (m == '?') {
            for (i = 0; i <= n; i++) {
              switch (v[i]) {
                case 3:
                  GetConsoleScreenBufferInfo(handle, &csbi);
                  w = csbi.dwSize.X;
                  h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
                  csize = w * (h + 1);
                  coord.X = 0;
                  coord.Y = csbi.srWindow.Top;
                  FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
                  FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
                  SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
                  csbi.srWindow.Right = csbi.srWindow.Left + 79;
                  SetConsoleWindowInfo(handle, TRUE, &csbi.srWindow);
                  csbi.dwSize.X = 80;
                  SetConsoleScreenBufferSize(handle, csbi.dwSize);
                  break;
                case 5:
                  attr =
                    ((attr & FOREGROUND_MASK) << 4) |
                    ((attr & BACKGROUND_MASK) >> 4);
                  SetConsoleTextAttribute(handle, attr);
                  break;
                case 25:
                  GetConsoleCursorInfo(handle, &cci);
                  cci.bVisible = FALSE;
                  SetConsoleCursorInfo(handle, &cci);
                  break;
                default:
                  break;
              }
            }
          }
          else if (m == '>' && v[0] == 5) {
            GetConsoleCursorInfo(handle, &cci);
            cci.bVisible = TRUE;
            SetConsoleCursorInfo(handle, &cci);
          }
          break;
        case 'm':
          attr = attr_old;
          for (i = 0; i <= n; i++) {
            if (v[i] == -1 || v[i] == 0)
              attr = attr_olds[type];
            else if (v[i] == 1)
              attr |= FOREGROUND_INTENSITY;
            else if (v[i] == 4)
              attr |= FOREGROUND_INTENSITY;
            else if (v[i] == 5)
              attr |= FOREGROUND_INTENSITY;
            else if (v[i] == 7)
              attr =
                ((attr & FOREGROUND_MASK) << 4) |
                ((attr & BACKGROUND_MASK) >> 4);
            else if (v[i] == 10)
              ; // symbol on
            else if (v[i] == 11)
              ; // symbol off
            else if (v[i] == 22)
              attr &= ~FOREGROUND_INTENSITY;
            else if (v[i] == 24)
              attr &= ~FOREGROUND_INTENSITY;
            else if (v[i] == 25)
              attr &= ~FOREGROUND_INTENSITY;
            else if (v[i] == 27)
              attr =
                ((attr & FOREGROUND_MASK) << 4) |
                ((attr & BACKGROUND_MASK) >> 4);
            else if (v[i] >= 30 && v[i] <= 37) {
              attr = (attr & BACKGROUND_MASK);
              switch( (v[i] - 30) & 3 ) {
                case 0:
                default:
                  break;
                case 1:
                  attr |= FOREGROUND_BLUE | FOREGROUND_GREEN;
                  break;
                case 2:
                  attr |= FOREGROUND_RED | FOREGROUND_GREEN;
                  break;
                case 3:
                  attr |= FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN;
                  break;
              }
              if ((v[i] - 30) & 4)
                attr |= FOREGROUND_INTENSITY;
            }
            //else if (v[i] == 39)
            //attr = (~attr & BACKGROUND_MASK);
            else if (v[i] >= 40 && v[i] <= 47) {
              attr = (attr & FOREGROUND_MASK);
              switch( (v[i] - 40) & 3 ) {
                case 0:
                default:
                  break;
                case 1:
                  attr |= BACKGROUND_BLUE | BACKGROUND_GREEN;
                  break;
                case 2:
                  attr |= BACKGROUND_RED | BACKGROUND_GREEN;
                  break;
                case 3:
                  attr |= BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_GREEN;
                  break;
              }
              if ((v[i] - 40) & 4)
                attr |= BACKGROUND_INTENSITY;
            }
            //else if (v[i] == 49)
            //attr = (~attr & FOREGROUND_MASK);
            else if (v[i] == 100)
              attr = attr_old;
          }
          SetConsoleTextAttribute(handle, attr);
          break;
        case 'K':
          GetConsoleScreenBufferInfo(handle, &csbi);
          coord = csbi.dwCursorPosition;
          switch (v[0]) {
            default:
              case 0:
              csize = csbi.dwSize.X - coord.X;
              break;
            case 1:
              csize = coord.X;
              coord.X = 0;
              break;
            case 2:
              csize = csbi.dwSize.X;
              coord.X = 0;
              break;
          }
          FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
          FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
          SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
          break;
        case 'J':
          GetConsoleScreenBufferInfo(handle, &csbi);
          w = csbi.dwSize.X;
          h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
          coord = csbi.dwCursorPosition;
          switch (v[0]) {
            default:
            case 0:
              csize = w * (h - coord.Y) - coord.X;
              coord.X = 0;
              break;
            case 1:
              csize = w * coord.Y + coord.X;
              coord.X = 0;
              coord.Y = csbi.srWindow.Top;
              break;
            case 2:
              csize = w * (h + 1);
              coord.X = 0;
              coord.Y = csbi.srWindow.Top;
              break;
          }
          FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
          FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
          SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
          break;
        case 'H':
        case 'f':
          GetConsoleScreenBufferInfo(handle, &csbi);
          coord = csbi.dwCursorPosition;
          if (v[0] != -1) {
            if (v[1] != -1) {
              coord.Y = csbi.srWindow.Top + v[0] - 1;
              coord.X = v[1] - 1;
            } else
              coord.X = v[0] - 1;
          } else {
            coord.X = 0;
            coord.Y = csbi.srWindow.Top;
          }
          if (coord.X < csbi.srWindow.Left)
            coord.X = csbi.srWindow.Left;
          else if (coord.X > csbi.srWindow.Right)
            coord.X = csbi.srWindow.Right;
          if (coord.Y < csbi.srWindow.Top)
            coord.Y = csbi.srWindow.Top;
          else if (coord.Y > csbi.srWindow.Bottom)
            coord.Y = csbi.srWindow.Bottom;
          SetConsoleCursorPosition(handle, coord);
          break;
        default:
          break;
      }
    } else {
      const unsigned char *uptr = (const unsigned char *)ptr;
      if (_ismbslead((const unsigned char *)_buf, uptr)) {
        // X68000固有文字の変換
        if ((uptr[0] == 0x80) || // 半角
            (uptr[0] == 0xf0) || // 上付き1/4(カタカナ)
            (uptr[0] == 0xf1) || // 上付き1/4(ひらがな)
            (uptr[0] == 0xf2) || // 下付き1/4(カタカナ)
            (uptr[0] == 0xf3) )  // 下付き1/4(ひらがな)
        {
            // 前半をスキップして半角ASCII化
            ptr++;
        }
      }
      putchar(*ptr);
      ptr++;
    }
  }
  return len;
}

#endif

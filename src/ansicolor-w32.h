#pragma once

#ifdef _WIN32
int WriteW32(short _type, FILE* fp, const char* buf, size_t len);
#endif

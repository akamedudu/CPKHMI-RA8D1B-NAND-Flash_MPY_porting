#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++) {
        // 如果以后要把输出重定向到 UART，可以在这里调用 R_SCI_B_UART_Write
        // 目前只是空实现，让 printf 不报错
    }
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    errno = EINVAL;
    return -1;
}

int _close(int file)
{
    (void)file;
    return 0;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

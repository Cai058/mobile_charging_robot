#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

extern char _end;
extern char _estack;

void _exit(int status)
{
    (void)status;
    while (1) {
    }
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    int index;

    (void)file;
    for (index = 0; index < len; ++index) {
        ptr[index] = (char)fgetc(stdin);
    }
    return len;
}

int _write(int file, char *ptr, int len)
{
    int index;

    (void)file;
    for (index = 0; index < len; ++index) {
        fputc(ptr[index], stdout);
    }
    return len;
}

void *_sbrk(ptrdiff_t increment)
{
    static char *heap_end;
    char *previous_heap_end;
    intptr_t next_heap_end;
    const uintptr_t heap_start = (uintptr_t)&_end;
    const uintptr_t heap_limit = (uintptr_t)&_estack - 0x400U;

    if (heap_end == NULL) {
        heap_end = &_end;
    }

    previous_heap_end = heap_end;
    next_heap_end = (intptr_t)(uintptr_t)heap_end + increment;
    if ((uintptr_t)next_heap_end < heap_start ||
        (uintptr_t)next_heap_end > heap_limit) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end = (char *)(uintptr_t)next_heap_end;
    return previous_heap_end;
}

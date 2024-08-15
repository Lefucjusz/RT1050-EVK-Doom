/*
 * syscalls.c
 *
 *  Created on: Oct 26, 2023
 *      Author: lefucjusz
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <errno.h>
#include <fcntl.h>
#include "ff.h"
#include "fsl_debug_console.h"

/* Defines */
#define MAX_OPENED_FILES 2
#define FIRST_HANDLE 3
#define ATEXIT_MAX 32

/* Variables */
char *__env[1] = {0};
char **environ = __env;

static FIL *file_handles[MAX_OPENED_FILES];

static size_t atexit_count = 0;
static void (*atexit_funcs[ATEXIT_MAX])(void);

/* Functions */
static FIL *get_file_handle(int fd) {
	const int index = fd - FIRST_HANDLE;

	if ((index < 0) || (index >= MAX_OPENED_FILES)) {
	  errno = EBADF;
	  return NULL;
	}

	FIL *file = file_handles[index];
	if (file == NULL) {
	  errno = EBADF;
	}
	return file;
}

static int find_free_handle(void) {
	int handle = -1;

	for (size_t i = 0; i < MAX_OPENED_FILES; ++i) {
		if (file_handles[i] == NULL) {
		  handle = i;
		  break;
		}
	}
	return handle;
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit(int status)
{
	while (atexit_count-- > 0) {
		if (atexit_funcs[atexit_count] != NULL) {
			atexit_funcs[atexit_count]();
		}
	}
	_kill(status, -1);
	NVIC_SystemReset();
}

int _read(int file, char *ptr, int len)
{
	if (file == 0) {
	  errno = EINVAL;
	  return -1;
	}

	FIL *handle = get_file_handle(file);
	if (handle == NULL) {
	    return -1;
	}

	UINT bytes_read;
	const FRESULT ret = f_read(handle, ptr, len, &bytes_read);
	if (ret != FR_OK) {
	    errno = EIO;
	    return -1;
	}

	return bytes_read;
}

int _write(int file, char *ptr, int len)
{
  /* Redirect stdout and strerr to UART */
  if (file == 1 || file == 2) {
	PRINTF("%.*s", len, ptr);
	return 0;
  }

  FIL *handle = get_file_handle(file);
  if (handle == NULL) {
	  return -1;
  }

  UINT bytes_written;
  const FRESULT ret = f_write(handle, ptr, len, &bytes_written);
  if (ret != FR_OK) {
  	  errno = EIO;
  	  return -1;
  }

  return bytes_written;
}

int _close(int file)
{
	FIL *handle = get_file_handle(file);
	if (handle == NULL) {
	    return -1;
	}

	FRESULT ret = f_close(handle);
	if (ret != FR_OK) {
	    errno = EIO;
	    ret = -1;
	}

	free(handle);
	file_handles[file - FIRST_HANDLE] = NULL;

	return ret;
}

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	FRESULT ret;
	FSIZE_t pos, size;

	FIL *handle = get_file_handle(file);
	if (handle == NULL) {
		errno = EBADF;
		return -1;
	}

	switch (dir) {
		case SEEK_SET:
			ret = f_lseek(handle, ptr);
			break;
		case SEEK_CUR:
			pos = f_tell(handle);
			ret = f_lseek(handle, pos + ptr);
			break;
		case SEEK_END:
			size = f_size(handle);
			ret = f_lseek(handle, size + ptr);
			break;
		default:
			ret = FR_INVALID_PARAMETER;
			break;
	}

	if (ret != FR_OK) {
		errno = EIO;
		return -1;
	}

	return f_tell(handle);
}

int _open(char *path, int flags, int mode)
{
	const int index = find_free_handle();

	if (index < 0) {
		errno = ENFILE;
		return -1;
	}

	FIL *file = calloc(1, sizeof(FIL));
	if (file == NULL) {
		errno = ENOMEM;
		return -1;
	}

	BYTE file_mode = 0;

	/* TODO this is weird, should be analyzed and potentially fixed */
	if ((flags & 0b111) == O_RDONLY) {
		file_mode = FA_READ;
	}
	else if ((flags & 0b111) == O_WRONLY) {
		file_mode = FA_WRITE;
	}
	else if ((flags & 0b111) == O_RDWR) {
		file_mode = FA_READ | FA_WRITE;
	}

	if (flags & O_APPEND) {
		file_mode |= FA_OPEN_APPEND;
	}
	else if (flags & O_CREAT) {
		file_mode |= FA_CREATE_ALWAYS;
	}

	const FRESULT ret = f_open(file, path, file_mode);
	if (ret != FR_OK) {
		free(file);
		errno = EIO;
		return -1;
	}

	file_handles[index] = file;

	return index + FIRST_HANDLE;
}

int _wait(int *status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}

int _times(struct tms *buf)
{
	return -1;
}

int _stat(char *file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	errno = ENOMEM;
	return -1;
}

int mkdir(const char *path, mode_t mode)
{
    (void)mode;
    FRESULT ret = f_mkdir(path);
    if (ret != FR_OK) {
    	return -1;
    }
    return 0;
}

int remove(const char *path)
{
	FRESULT ret = f_unlink(path);
	if (ret != FR_OK) {
		return -1;
	}
	return 0;
}

int rename(const char *old, const char *new)
{
	FRESULT ret = f_rename(old, new);
	if (ret != FR_OK) {
		return -1;
	}
	return 0;
}

int atexit(void (*func)(void))
{
	if (atexit_count >= ATEXIT_MAX) {
		return -ENOMEM;
	}

	atexit_funcs[atexit_count] = func;
	atexit_count++;
	return 0;
}

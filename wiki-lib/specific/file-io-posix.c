#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <file-io.h>
#include <msg.h>

int wl_open(const char *filename, int flags)
{
	int f = 0;

	switch (flags) {
	case WL_O_RDONLY:
		f = O_RDONLY;
		break;
	case WL_O_WRONLY:
		f = O_WRONLY;
		break;
	case WL_O_RDWR:
		f = O_RDWR;
		break;
	}

	return open(filename, f);
}

void wl_close(int fd)
{
	close(fd);
}

int wl_read(int fd, void *buf, unsigned int count)
{
	return read(fd, buf, count);
}

int wl_write(int fd, void *buf, unsigned int count)
{
	return write(fd, buf, count);
}

int wl_seek(int fd, unsigned int pos)
{
	return lseek(fd, pos, SEEK_SET);
}

int wl_ftell(int fd)
{
	msg(MSG_ERROR, "%s() IS UNIMPLEMENTED!", __func__);
	return -1;
}

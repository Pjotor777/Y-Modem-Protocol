/* Wrapper functions for ENSC-351, Simon Fraser University, By
 *  - Craig Scratchley
 * 
 * These functions will be re-implemented later in the course.
 */

#include <unistd.h>			// for read/write/close
#include <fcntl.h>	// for open/creat
#include <stdarg.h>

int myOpen(const char *pathname, int flags, ...) //, mode_t mode)
{
    mode_t mode{0};
    // in theory we should check here whether mode is needed.
    va_list arg;
    va_start (arg, flags);
    mode = va_arg (arg, mode_t);
    va_end (arg);
    return open(pathname, flags, mode);
}

int myCreat(const char *pathname, mode_t mode)
{
	return creat(pathname, mode);
}

ssize_t myRead( int fildes, void* buf, size_t nbyte )
{
	return read(fildes, buf, nbyte );
}

ssize_t myWrite( int fildes, const void* buf, size_t nbyte )
{
	return write(fildes, buf, nbyte ); /// write from one location to another, noting the size
}

int myClose( int fd )
{
	return close(fd);
}

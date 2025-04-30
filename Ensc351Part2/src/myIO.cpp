//============================================================================
//
//% Student Name 1: student1
//% Student 1 #: 123456781
//% Student 1 userid (email): stu1 (stu1@sfu.ca)
//
//% Student Name 2: student2
//% Student 2 #: 123456782
//% Student 2 userid (email): stu2 (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P3_<userid1>_<userid2>" (eg. P3_stu1_stu2)
//% * Form groups as described at:  https://coursys.sfu.ca/docs/students
//% * Submit files to coursys.sfu.ca
//
// File Name   : myIO.cpp
// Version     : September 24, 2024
// Description : Wrapper I/O functions for ENSC-351
// Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 	// for socketpair
#include <stdarg.h>         // for va stuff
#include <termios.h>        // for tcdrain()
#include <errno.h>

#include "SocketReadcond.h"

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

int mySocketpair( int domain, int type, int protocol, int des_array[2] )
{
   return socketpair(domain, type, protocol, des_array);
}

ssize_t myRead( int des, void* buf, size_t nbyte )
{
	return read(des, buf, nbyte );
}

ssize_t myWrite( int des, const void* buf, size_t nbyte )
{
	return write(des, buf, nbyte );
}

int myClose( int des )
{
	return close(des);
}

int myTcdrain(int des)
{ //is also included for purposes of the course.
   const int retVal{tcdrain(des)};
   if (-1 == retVal && (EFAULT == errno || ENOTTY == errno)) // if des is socket then errno == 14 (EFAULT)
      return 0; // fix this for Project Part 3
   return retVal;
}

/* Arguments:
des
    The file descriptor associated with the terminal device that you want to read from.
buf
    A pointer to a buffer into which readcond() can put the data.
n
    The maximum number of bytes to read.
min, time, timeout
    When used in RAW mode, these arguments override the behavior of the MIN and TIME members of the terminal's termios structure. For more information, see...
 *
 *  https://developer.blackberry.com/native/reference/core/com.qnx.doc.neutrino.lib_ref/topic/r/readcond.html
 *
 *  */
int myReadcond(int des, void * buf, int n, int min, int time, int timeout)
{
	return wcsReadcond(des, buf, n, min, time, timeout );
}


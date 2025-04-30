/*
 * readcond.h
 * A version of readcond for non-QNX systems supporting termios.
 *
 * Arguments:
des
    The file descriptor associated with the terminal device that you want to read from.
buf
    A pointer to a buffer into which readcond() can put the data.
n
    The maximum number of bytes to read.
min, time, timeout
    When used in RAW mode, these arguments override the behavior of the MIN and TIME members of the terminal's termios structure.
 *
 * For more information, see documentation like:
 *  https://developer.blackberry.com/native/reference/core/com.qnx.doc.neutrino.lib_ref/topic/r/readcond.html
 *
 *
 *  Created on: Nov 19, 2020
 *      Author: W. Craig Scratchley
 */

#ifndef READCOND_H_
#define READCOND_H_

#ifdef __cplusplus
extern "C" {
#endif

int readcond( int fd,
              void * buf,
              int n,
              int min,
              int time,
              int timeout )
;

#ifdef __cplusplus
}
#endif

#endif /* READCOND_H_ */

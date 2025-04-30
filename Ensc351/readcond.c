/*
 * readcond.c
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

#include <termios.h>
#include <stdio.h>      // fprintf()
#include "VNPE.h"

int readcond( int fd,
              void * buf,
              int n,
              int min,
              int time,
              int timeout )
{
    int bytesRead;
    int errnoHold = errno;
            // fd not a socket, treating it as terminal devices
            struct termios termio;
            struct termios resetTermio;

            if (-1 == tcgetattr(fd, &termio))
                return -1; // perhaps the descriptor is neither for socket nor for terminal

            PE(tcgetattr(fd, &resetTermio)); // or
            // resetTermio = termio; // ?

            // Configure terminal for non canonical mode
            termio.c_lflag = ~ICANON;
            // Four distinct cases
            // http://man7.org/linux/man-pages/man3/termios.3.html

            if (timeout == 0) {
                termio.c_cc[VMIN] = min;
                termio.c_cc[VTIME] = time;
                PE(tcsetattr(fd, TCSANOW, &termio)); //Apply the new setting instantly
                //   Note  that  tcsetattr()  returns success if any of the requested changes could be successfully
                //   carried out.  Therefore, when making multiple changes it may be necessary to follow this  call
                //   with a further call to tcgetattr() to check that all changes have been performed successfully.
                bytesRead = read(fd, buf, n);
            }
            else if (time == 0) {
                // timeout is not zero
                // do we use select here?
                // set timeval using timeout
                // *** not yet implemented ***
                fprintf(stderr, "Error -- unimplemented part of readcond()\n");
                errno = EINVAL;
                return -1;
            }
            else { // both time and timeout are not zero.
                int bytesSoFar = 0;

                termio.c_cc[VMIN] = 0;
                termio.c_cc[VTIME] = timeout; // time;
                PE(tcsetattr(fd, TCSANOW, &termio)); //Apply the new setting instantly
                do {
                    do {
                        // need to update timeout
                        bytesRead = read(fd, ((char*) buf) + bytesSoFar, n - bytesSoFar);
                    }
                    while ((bytesRead == -1 && errno == EINTR));
                    // if EINTR then warn about updating timeout

                    if (bytesRead == -1) {
                        //perror("SocketReadcond(read()) Error");
                        errnoHold = errno;
                        break;
                    }
                    else if (bytesRead) {
                        bytesSoFar += bytesRead;
                        termio.c_cc[VTIME] = time;
                        PE(tcsetattr(fd, TCSANOW, &termio)); //Apply the new setting instantly
                    }
                    else
                        break;
                }
                while (bytesSoFar < min); // also works when min == 0, though not needed.
                if (bytesRead != -1) {
                    bytesRead = bytesSoFar;
                }
            }
            //Reset the attr for termios
            PE(tcsetattr(fd, TCSANOW, &resetTermio)); //Apply the new setting instantly
            errno = errnoHold;
            return bytesRead;
}

//============================================================================
//
//% Student Name 1: Petar Tesanovic
//% Student 1 #: 301434515
//% Student 1 userid (email): pta44 (pta44@sfu.ca)
//
//% Student Name 2: Aaron Lopez-Dee
//% Student 2 #: 301434253
//% Student 2 userid (email): ala233 (ala233@sfu.ca)
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
//% * Your group name should be "P6_<userid1>_<userid2>" (eg. P6_stu1_stu2)
//% * Form groups as described at:  https://coursys.sfu.ca/docs/students
//% * Submit files to coursys.sfu.ca
//
// File Name   : ReceiverY.cpp
// Version     : November, 2024
// Description : Starting point for ENSC 351 Project Part 6
// Original portions Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include "ReceiverY.h"

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <sys/select.h>
//#include <sys/dcmd_chr.h> // for DCMD_CHR_GETOBAND
#include <memory> // for pointer to SS class
#include "Linemax.h"
#include <sstream>

#include "myIO.h"
#include "yReceiverSS.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

// comment out the line below to get rid of Receiver logging information.
#define REPORT_INFO

using namespace std;
using namespace yReceiver_SS;

ReceiverY::
ReceiverY(int d, int conInD, int conOutD)
:PeerY(d, '(', ')', "/tmp/ReceiverSS.log", conInD, conOutD),
 //closeProb(1),
 //NCGbyte('C'),
 goodBlk(false), 
 goodBlk1st(false), 
 syncLoss(false), // transfer will end if syncLoss becomes true
 bytesRemaining(0),
 numLastGoodBlk(255)
{
}

/* Only called after an SOH character has been received and
posted to the Receiver_SS statechart. The function tries
to receive the remaining characters to form a complete
block.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
 The function will set or reset a Boolean variable,
goodBlk. This variable will be made false if either
	� the needed number of bytes have not yet been received and another
	byte does not arrive within the character timeout since the last byte
	(or within the character timeout of the function being called).
	� the needed number of bytes (or more) are received and the block
	created using the needed number of bytes has something
	wrong with it, like the checksum being incorrect.
	� more than the needed number of bytes are received.
The function will also set or reset another Boolean variable,
syncLoss. syncLoss will only be set to true when there is a
fatal loss of syncronization as described in the XMODEM
specification. If goodBlk has not already been made false
and if syncLoss is false, then goodBlk will be set to true.  The
first time each block is received and is good, goodBlk1st will be
set to true.  This is an indication of when a block should be
written to disk.  If goodBlk is false and at
least the needed number of bytes were received in the function,
then a purge() function should be called before returning from
getRestBlk(). The purge() subroutine will read and discard
characters until nothing is received over a character timeout period.
*/

void ReceiverY::getRestBlk()
{
	const int restBlkSz = REST_BLK_SZ_CRC;
    // here, we can read about 30 more characters than we hope to get,
    //         but keep min at restBlkSz, so any extra
    //         characters that happen to come from the serial port
    //         can be grabbed while we are calling myReadcond.
    int bytesRead{PE(myReadcond(mediumD, rcvBlk+1, BUF_SZ - 1, restBlkSz, dSECS_PER_UNIT*TM_CHAR, dSECS_PER_UNIT*TM_CHAR))};
    	// consider receiving CRC after calculating local CRC
    if(bytesRead < restBlkSz) {
#ifdef REPORT_INFO
		// "Sh"ort block
    	COUT << "(Sh" << bytesRead << ")" << flush;
#endif
    	goodBlk = goodBlk1st = false; // short block
    	// return;
    }
    else { // not needed if we put return in above.
    	const char* badReason;
   	 	if( bytesRead > restBlkSz) { // got an extra byte or two -- maybe there are more
			goodBlk = false; //things are fishy -- let's not take chances
			badReason = "be"; // "bad -- extra (bytes)"
		}
		else if (rcvBlk[2] != (uint8_t) ~rcvBlk[1]) {
			//  block # and its complement are not matched
			goodBlk = false;
			badReason = "bm"; // "bad -- (complement not) matched"
		}
		else {
			goodBlk1st = (rcvBlk[1] == (uint8_t) (numLastGoodBlk + 1)); // but might be made false below
			if (!goodBlk1st) {
				// determine fatal loss of synchronization
            if (transferringFileD == -1 || (rcvBlk[1] != numLastGoodBlk)) {
					syncLoss = true;
					goodBlk = false;
#ifdef REPORT_INFO
				   // "s"ynchronization has been lost
					COUT << "(s" << (unsigned) rcvBlk[1] << ":" << (unsigned) numLastGoodBlk << ")" << flush;
#endif
					purge();
					return;
				}
	//#define ALLOW_DEEMED_GOOD
	#ifdef ALLOW_DEEMED_GOOD
				else { // (rcvBlk[1] == numLastGoodBlk)
					goodBlk = true; // "deemed" good block
#ifdef REPORT_INFO
				// "d"eemed good block
					COUT << "(d" << (unsigned) rcvBlk[1] << ")" << flush;
#endif
					// purge(); // ??
					return;
				}
	#endif
			}
			badReason = "bd"; // "bad data (in chunk)"
			// detect if data error in chunk
			// consider receiving checksum/CRC after calculating local checksum/CRC
			uint16_t CRCbytes;
			crc16ns(&CRCbytes, &rcvBlk[DATA_POS]);
			goodBlk = (*((uint16_t*) &rcvBlk[PAST_CHUNK]) == CRCbytes);
		}
		if (!goodBlk) {
			goodBlk1st = false; // but the block was "bad".
#ifdef REPORT_INFO
			COUT << "(" << badReason << (unsigned) rcvBlk[1] << ")" << flush;
#endif
			purge();  // discard chars until line idles for the character timeout period.
			return;
		}
#ifndef ALLOW_DEEMED_GOOD
		else if (!goodBlk1st) {
#ifdef REPORT_INFO
		   // "r"esent good block
		   COUT << "(r" << (unsigned) rcvBlk[1] << ")" << flush; // "resent" good block
#endif
			return;
		}
#endif
		// good block for the "first" time.
		numLastGoodBlk = rcvBlk[1];
#ifdef REPORT_INFO
		// good block for the "f"irst time.
		COUT << "(f" << (unsigned) rcvBlk[1] << ")" << endl;
#endif
	}
}

//Write chunk (file data) in a received block to disk.  Update the number of bytes remaining to be written.
void ReceiverY::writeChunk()
{
   if (bytesRemaining <= 0)
      return; /// No data left to write, avoid unnecessary operations
   bytesRemaining -= CHUNK_SZ;
   /// calculates writeSize in such a way that only the valid data is written
   ssize_t writeSize{(bytesRemaining < 0) ? (CHUNK_SZ + bytesRemaining) : CHUNK_SZ};
   /// called with writeSize to write only the valid data from the block.
   /// Write only valid data to disk
   if (writeSize > 0)
      PE_NOT(myWrite(transferringFileD, &rcvBlk[DATA_POS], writeSize), writeSize);
}

// Open the output file to hold the file being transferred.
// Initialize the number of bytes remaining to be written with the file size.
int
ReceiverY::
openFileForTransfer()
{
#ifdef REPORT_INFO
    COUT << "(opening: " << &rcvBlk[DATA_POS] << ")" << flush;
#endif
    const mode_t mode{S_IRUSR | S_IWUSR}; //  | S_IRGRP | S_IROTH};
    const char* fileNameP{(const char *) &rcvBlk[DATA_POS]};
    transferringFileD = myCreat(fileNameP, mode);
    bytesRemaining = stoi(string((const char *) &rcvBlk[DATA_POS + strlen(fileNameP) + 1]));
//    istringstream((const char *) &rcvBlk[DATA_POS + strlen(fileNameP) + 1]) >> bytesRemaining;
//    sscanf((const char *) &rcvBlk[DATA_POS + strlen(fileNameP) + 1], "%ld", &bytesRemaining);
    return transferringFileD;
}

/* If not already closed, close file that was just received (or being received).
 * Set transferringFileD to -1 and numLastGoodBlk to 255 when file is closed.  Thus numLastGoodBlk
 * is ready for the next file to be sent.
 * Return the errno if there was an error closing the file and otherwise return 0.
 */
int
ReceiverY::
closeTransferredFile()
{
    if (transferringFileD > -1) {
        closeProb = myClose(transferringFileD);
        if (closeProb)
            return errno;
        else {
            numLastGoodBlk = 255;
            transferringFileD = -1;
        }
    }
    return 0;
}

/*
Read and discard contiguous CAN characters. 
*/
void ReceiverY::clearCan()
{
	PeerY::clearCan(TM_2CHAR);
}

//Send CAN_LEN CAN characters in a row to the YMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverY::cans()
{
	// no need to space in time CAN chars coming from receiver
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

//The purge() subroutine will read and discard
//characters until nothing is received over a 1-second period.
void ReceiverY::purge()
{
   char buffer[BUF_SZ]; /// buffer to hold data
   bool loop_flag = true; /// variable for looping

   while (loop_flag) {
      /// Get # of bytes read. myReadcond reads data from mediumD into the buffer when
      /// at least 1 byte is read, or the 1-second timeout period elapses with
      /// no new bytes arriving. 10 deciseconds = 1 second
      int bytes_Read = myReadcond(mediumD, buffer, BUF_SZ, 10, dSECS_PER_UNIT*5, dSECS_PER_UNIT*5); // testing *5

      /// Purge works when myreadcond returns at least 1 byte is read,
      /// or the 1 sec timeout period elapses with no new bytes arriving.
      if (bytes_Read == -1) { /// -1 signifies ERROR
         /// check if I/O issue
         if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
            /// Non fatal errors, thus continue
            continue;
         }
         else { /// Fatal error, break
            break;
         }
      }
      else if (bytes_Read <= 0) {
         break; /// No more data or an error occurred, exit loop
      }
      else {
         break;
      }
   }
} // end of purge()

// anotherFile will be zero (0) if there are no more files to be received.
uint8_t
ReceiverY::
checkForAnotherFile()
{
    return (anotherFile = rcvBlk[DATA_POS]);
}

// Run the YMODEM protocol to receive files.
void ReceiverY::receiveFiles()
{
	auto myReceiverSmSp{make_shared<yReceiverSS>(this, false)}; // or use make_unique
#ifdef REPORT_INFO
		transferCommon(myReceiverSmSp, true);
		COUT << "\n"; // insert new line.
#else
		transferCommon(myReceiverSmSp, false);
#endif
}

/*
 * Medium.cpp
 *
 *      Author: Craig Scratchley
 *      Version: September 10, 2024
 *      Copyright(c) 2024 Craig Scratchley
 */

#include <fcntl.h>
#include <unistd.h> // for write()
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "Medium.h"
#include "myIO.h"
#include "VNPE.h"
#include "AtomicCOUT.h"
#include "posixThread.hpp"

#include "PeerY.h"

// Uncomment the line below to turn on debugging output from the medium
//#define REPORT_INFO

//#define SEND_EXTRA_ACKS

//This is the kind medium.

#define T2toT1_CORRUPT_BYTE         395

using namespace std;
using namespace pthreadSupport;

Medium::Medium(int d1, int d2, const char *fname)
:m_Term1D(d1), m_Term2D(d2) //, m_logFileName(fname)
{
   m_byteCount = 0;
	m_ACKforwarded = 0;
	m_ACKreceived = 0;
	m_sendExtraAck = false;
//	crcMode = false;

	const mode_t mode{S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH};
	m_logFileD = PE2(myCreat(fname, mode), fname);
}

Medium::~Medium() {
}

// this function will return false when it detects that the Term2 (sender) socket has closed.
bool Medium::MsgFromTerm2()
{
	blkT bytesReceived; // ?
	ssize_t numOfBytesReceived;
	int byteToCorrupt;

	if (!(numOfBytesReceived = PE(myRead(m_Term2D, bytesReceived, 1)))) {
		COUT << "Medium thread: TERM2's socket closed, Medium terminating" << endl;
		return false;
	}
	m_byteCount += numOfBytesReceived;

	PE_NOT(myWrite(m_logFileD, bytesReceived, numOfBytesReceived), numOfBytesReceived);
	//Forward the bytes to Term1 (usually RECEIVER),
	PE_NOT(myWrite(m_Term1D,   bytesReceived, numOfBytesReceived), numOfBytesReceived);

	if(CAN == bytesReceived[0]) {
      numOfBytesReceived = PE_NOT(myRead(m_Term2D, bytesReceived, CAN_LEN - 1), CAN_LEN - 1);
      // byteCount += numOfBytesReceived;
      PE_NOT(myWrite(m_logFileD, bytesReceived, numOfBytesReceived), numOfBytesReceived);
      //Forward the bytes to Term1 (usually RECEIVER),
      PE_NOT(myWrite(m_Term1D,   bytesReceived, numOfBytesReceived), numOfBytesReceived);
	}
	else if (bytesReceived[0] == SOH) {
	   if (m_sendExtraAck) {
      #ifdef REPORT_INFO
	      COUT << "{" << "+A" << "}" << flush;
      #endif
      uint8_t buffer{ACK};
      PE_NOT(myWrite(m_logFileD, &buffer, 1), 1);
      //Write the byte to term2,
      PE_NOT(myWrite(m_Term2D, &buffer, 1), 1);

      m_sendExtraAck = false;
      }

      numOfBytesReceived = PE(myRead(m_Term2D, bytesReceived, BLK_SZ_CRC - numOfBytesReceived));

      m_byteCount += numOfBytesReceived;
      if (m_byteCount >= T2toT1_CORRUPT_BYTE) {
         m_byteCount -= T2toT1_CORRUPT_BYTE;  // how large could byteCount end up? (CB - 1) + 133 - CB = 132
         byteToCorrupt = numOfBytesReceived - m_byteCount; // how small could byteToCorrupt be?
         if (byteToCorrupt < numOfBytesReceived) {
             bytesReceived[byteToCorrupt] = (255 - bytesReceived[byteToCorrupt]);
             //bytesReceived[byteToCorrupt] = ~bytesReceived[byteToCorrupt];
         #ifdef REPORT_INFO
             COUT << "<" << byteToCorrupt << "x>" << flush;
         #endif
         }
      }

      PE_NOT(myWrite(m_logFileD, &bytesReceived, numOfBytesReceived), numOfBytesReceived);
      //Forward the bytes to Term1 (RECEIVER),
      PE_NOT(myWrite(m_Term1D, &bytesReceived, numOfBytesReceived), numOfBytesReceived);
	}
	return true;
}

bool Medium::MsgFromTerm1()
{
	uint8_t buffer[CAN_LEN];
	const ssize_t numOfByte{PE(myRead(m_Term1D, buffer, CAN_LEN))};
	if (0 == numOfByte) {
		COUT << "Medium thread: TERM1's socket closed, Medium terminating" << endl;
		return false;
	}

	/*note that we record the corrupted ACK in the log file*/
	if (ACK == buffer[0]) {
      m_ACKreceived++;

      if((m_ACKreceived%10) == 0)
      {
         m_ACKreceived = 0;
         buffer[0] = NAK;
         #ifdef REPORT_INFO
                COUT << "{" << "AxN" << "}" << flush;
    #endif
      }
      #ifdef SEND_EXTRA_ACKS
      else/*actually forwarded ACKs*/
      {
          ACKforwarded++;

          if((ACKforwarded%6)==0)/*Note that this extra ACK is not an ACK forwarded from receiver to the sender, so we don't increment ACKforwarded*/
          {
              ACKforwarded = 0;
              sendExtraAck = true;
          }
      }
      #endif
	}

	PE_NOT(write(m_logFileD, buffer, numOfByte), numOfByte);

	//Forward the buffer to term2,
	PE_NOT(myWrite(m_Term2D, buffer, numOfByte), numOfByte);
	return true;
}

void
Medium::
mediumFuncT1toT2()
{
    PE_0(pthread_setname_np(pthread_self(), "M1to2"));
    while (MsgFromTerm1());
}

void Medium::run()
{
//    posixThread mediumThrd1to2(SCHED_FIFO, 35, &Medium::mediumFuncT1toT2, this); // gives error
   jthread mediumThrd1to2(&Medium::mediumFuncT1toT2, this);
   pthreadSupport::setSchedPrio(45); // raise priority up slightly.  FIFO?

   //transfer data from Term2 (sender)
   while (MsgFromTerm2())
      ;

	PE(myClose(m_logFileD));
	PE(myClose(m_Term1D));
	PE(myClose(m_Term2D));
}


void mediumFunc(int T1d, int T2d, const char *fname)
{
    PE_0(pthread_setname_np(pthread_self(), "M2to1"));
    Medium medium(T1d, T2d, fname);
    medium.run();
}


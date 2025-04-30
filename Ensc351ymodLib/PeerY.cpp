//============================================================================
// File Name   : PeerY.cpp 
// Version     : November, 2024
// Description : Starting point for ENSC 351 Project Part 5
// Original portions Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include "PeerY.h"

#include <cstring>      // for strcmp()
#include <sys/time.h>
//#include <arpa/inet.h> // for htons() -- not available with MinGW

#include "VNPE.h"
#include "Linemax.h"
#include "myIO.h"
#include "AtomicCOUT.h"

using namespace std;
using namespace smartstate;

PeerY::
PeerY(int d, char left, char right, const char *smLogN, int conInD, int conOutD)
:mediumD(d),
 logLeft(left),
 logRight(right),
 smLogName(smLogN),
 consoleInId(conInD),
 consoleOutId(conOutD)
{
	struct timeval tvNow;
	PE(gettimeofday(&tvNow, NULL));
	sec_start = tvNow.tv_sec;
}

//Send a byte to the remote peer across the medium
void
PeerY::
sendByte(uint8_t byte)
{
	if (reportInfo) {
	    //*** remove all but last of this block ***
	     char displayByte;
        if (byte == NAK)
            displayByte = 'N';
        else if (byte == ACK)
            displayByte = 'A';
        else if (byte == EOT)
            displayByte = 'E';
        else
            displayByte = byte;
        COUT << logLeft << displayByte << ":" << (int)(unsigned char) byte << logRight << flush;        
	}
	PE_NOT(myWrite(mediumD, &byte, sizeof(byte)), sizeof(byte));
}

// returns microseconds elapsed since this peer was constructed (within 1 second)
long long int
PeerY::
elapsed_usecs()
{
	struct timeval tvNow;
	PE(gettimeofday(&tvNow, NULL));
	/*_CSTD */ time_t	tv_sec{tvNow.tv_sec};
	return (tv_sec - sec_start) * (long long int) MILLION + tvNow.tv_usec; // casting needed?
}

/*
set a timeout time at an absolute time timeoutUnits into
the future. That is, determine an absolute time to be used
for the next one or more XMODEM timeouts by adding
timeoutUnits to the elapsed time.
*/
void 
PeerY::
tm(int timeoutUnits)
{
	absoluteTimeout = elapsed_usecs() + timeoutUnits * uSECS_PER_UNIT;
}

/* make the absolute timeout earlier by reductionUnits */
void 
PeerY::
tmRed(int unitsToReduce)
{
	absoluteTimeout -= (unitsToReduce * uSECS_PER_UNIT);
}

/*
Store the current absolute timeout, and create a temporary
absolute timeout timeoutUnits into the future.
*/
void 
PeerY::
tmPush(int timeoutUnits)
{
	holdTimeout = absoluteTimeout;
	absoluteTimeout = elapsed_usecs() + timeoutUnits * uSECS_PER_UNIT;
}

/*
Discard the temporary absolute timeout and revert to the
stored absolute timeout
*/
void 
PeerY::
tmPop()
{
	absoluteTimeout = holdTimeout;
}

/*
Read and discard contiguous CAN characters. Read characters
from the medium one-by-one in a loop until (CAN_LEN - 2) CAN characters
are received or nothing is
received over the specified timeout period or a character other than
CAN is received. If received, send a non-CAN character to
the console.
*/
void PeerY::clearCan(const int canTimeout)
{
	char character{CAN};
	int bytesRead;
   int totalBytesRd{0}; // not yet tested
   // will not work if CAN_LEN < 3
	do {
		bytesRead = PE(myReadcond(mediumD, &character, sizeof(character), sizeof(character), dSECS_PER_UNIT*canTimeout, dSECS_PER_UNIT*canTimeout));
      totalBytesRd += bytesRead;
	} while (bytesRead && character==CAN && totalBytesRd < (CAN_LEN - 2));
	if (character != CAN)
		CON_OUT(consoleOutId, character << flush);
}

void
PeerY::
transferCommon(std::shared_ptr<StateMgr> mySM, bool reportInfoParam)
{
   reportInfo = reportInfoParam;
   /*
   // use this code to send stateChart logging information to a file.
   ofstream smLogFile; // need '#include <fstream>' above
   smLogFile.open(smLogName, ios::binary|ios::trunc);
   if(!smLogFile.is_open()) {
      CERR << "Error opening sender state chart log file named: " << smLogName << endl;
      exit(EXIT_FAILURE);
   }
   mySM->setDebugLog(&smLogFile);
   // */

   // comment out the line below if you want to see logging information which will,
   // by default, go to cout.

   mySM->setDebugLog(nullptr); // this will affect both peers.  Is this okay?

   mySM->start();


   fd_set current_fds; /// initialize file descriptor set
   int max_fds = std::max(consoleInId, mediumD) + 1;  /// max descriptor for select()
   /// max is calculated to be the highest descriptor plus one, which is required by select()

   struct timeval tv{0, 0}; // tv_sec, tv_usec

   while(mySM->isRunning()) {

      /// returns microseconds elapsed since this peer was constructed (within 1 second)
      long long int now{elapsed_usecs()};
      long long int time_left = (absoluteTimeout > now) ? (absoluteTimeout - now) : 0; /// how much time left
      //long long int timeLeft = absoluteTimeout - now;

      /// utilize file descriptor set via current_fds
      FD_ZERO(&current_fds);
      FD_SET(mediumD, &current_fds);         /// Add serial port descriptor
      FD_SET(consoleInId, &current_fds);     /// Add console input descriptor

      /// Set tv for select() (in seconds and microseconds)

      if (time_left > 0) {
         tv.tv_sec = time_left / MILLION;
         tv.tv_usec = time_left % MILLION;
      }
      else { /// no more time left
         tv.tv_sec = 0;
         tv.tv_usec = 0;
      }
      //tv.tv_sec = 0;
      //tv.tv_usec = 0;

      /// responds to either serial port events or keyboard inputs, and
      /// monitors both the medium descriptor (mediumD) and the console input desc. (consoleInId)
      //int input_src = select(max_fds, &current_fds, nullptr, nullptr, (timeLeft > 0) ? &tv : nullptr);
      int input_src = select(max_fds, &current_fds, nullptr, nullptr, &tv);

      if (input_src == -1) { /// error occurred
         if (errno == EINTR) { /// non fatal error
            continue;
         }
         else { /// Other type of error
            perror("select failed");
            exit(EXIT_FAILURE);
         }
      }
      else if (input_src == 0) { /// timeout occurs
         /// check if the elapsed time (now) has exceeded absoluteTimeout
         if (now >= absoluteTimeout) {
            mySM->postEvent(TM); /// post timeout to state machine
         }
      }
      else {
         if (FD_ISSET(mediumD, &current_fds)) {
            //read character from medium
            char byte;
            //COUT << "Medium descriptor is ready." << endl;
            unsigned timeout = (absoluteTimeout - now) / 1000 / 100; // tenths of seconds
            if (PE(myReadcond(mediumD, &byte, 1, 1, timeout, timeout))) {
               if (reportInfo) {
                  COUT << logLeft << (int)(unsigned char) byte << logRight << flush;
               }
               mySM->postEvent(SER, byte);
            }
            /**
            else { // This won't be needed later because timeout will occur after the select() function.
               if (reportInfo)
                  COUT << logLeft << 1.0*timeout/10 << logRight << flush;
               mySM->postEvent(TM); // see note 3 lines above.
            }**/
         }
         /// Check if console input is available (if keyboard cancel event)
         if (FD_ISSET(consoleInId, &current_fds)) {
            //COUT << "Console descriptor is ready." << endl;
            char kb_char;
            if (PE(myRead(consoleInId, &kb_char, 1)) > 0) {
               if (kb_char == '&') { /// command was typed from keyboard
                  /// &c was typed
                  if (PE(myRead(consoleInId, &kb_char, 1)) > 0 && kb_char == 'c') {
                     mySM->postEvent(KB_C); /// keyboard cancel event
                     KbCan = true; /// cancel event received
                     COUT << "Cancelling file transfer..." << endl;
                     usleep(100000); /// small delay to allow cancel command to propagate
                  }
               }
            }
         }
      }
   }
//    smLogFile.close();
}

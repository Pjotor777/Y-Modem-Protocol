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
//% * Your group name should be "P1_<userid1>_<userid2>" (eg. P1_stu1_stu2)
//% * Form groups as described at:  https://coursys.sfu.ca/docs/students
//% * Submit files to coursys.sfu.ca
//
// File Name   : SenderY.cpp
// Version     : September 5, 2024
// Description : Starting point for ENSC 351 Project
// Original portions Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include "SenderY.h"

#include <iostream>
#include <filesystem>
#include <stdio.h> // for snprintf()
#include <stdint.h> // for uint8_t
#include <string.h> // for memset(), and memcpy() or strncpy()
#include <errno.h>
#include <fcntl.h>	// for O_RDWR or O_RDONLY
#include <sys/stat.h>

#include "myIO.h"

using namespace std;
using namespace std::filesystem; // C++17 and beyond

SenderY::
SenderY(vector<const char*> iFileNames, int d)
:PeerY(d),
 bytesRd(-1),
 fileNames(iFileNames),
 fileNameIndex(0),
 blkNum(0)
{
}

//-----------------------------------------------------------------------------

/* generate a block (numbered 0) with filename and filesize */
//void SenderY::genStatBlk(blkT blkBuf, const char* fileName)
void SenderY::genStatBlk(uint8_t blkBuf[BLK_SZ_CRC], const char* fileName)
{
   // TODO: ********* additional code must be written ***********

   struct stat st;
   stat(fileName, &st);
   unsigned fileSize{st.st_size};
   /// reset and clear memory from prev block for sending
   memset(blkBuf, 0, BLK_SZ_CRC);
   /// Send first stat block
   /// SOF 00 FF:
   blkBuf[0] = SOH;
   blkBuf[1] = blkNum;
   blkBuf[2] = ~blkNum; // get FF by complement of 00
   blkNum++; // increment to avoid 2 blocks with value 0

   /// Get the file name only as it outputs the full file path
   string file_name = path(fileName).filename();
   strncpy(reinterpret_cast<char*>(&blkBuf[3]), file_name.c_str(), BLK_SZ_CRC - 3);// -3 to go to first block

   /// Extract length of the file name
   int file_name_length = strlen(file_name.c_str());
   /// Per instruction in pdf, add null (the null[123]) after file name
   /// SOH 00 FF foo.c NUL[123] CRC CRC
   blkBuf[3 + file_name_length] = '\0'; // NUL[123] for example

   /// Finally insert the size of the whole file into the data block
   strncpy(reinterpret_cast<char*>(&blkBuf[file_name_length + 4]), to_string(fileSize).c_str(), BLK_SZ_CRC - (file_name_length + 4));
   /* calculate and add CRC in network byte order */
   // TODO: ********* The next couple lines need to be changed (changed)***********
   uint16_t myCrc16ns;
   crc16ns(&myCrc16ns, &blkBuf[3]); /// change to 3 from 0 as we used first 3 blocks
   /// send null block data to indicate end of file transfer
   if (fileName == "") {
      memset(blkBuf + 3, 0, CHUNK_SZ);
   }
   else { /// establish big and little endian for cross platform ability
      blkBuf[BLK_SZ_CRC - 2] = (myCrc16ns >> 8); // msb
      blkBuf[BLK_SZ_CRC - 1] = (myCrc16ns & 0xFF); // lsb
   }
}

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block
was prepared or if the input file is empty (i.e. has 0 length).
*/
//void SenderY::genBlk(blkT blkBuf)
void SenderY::genBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
    // TODO: ********* The next line needs to be changed ***********
    /// change blkBuf index to 3, and change errorprinter
    if (-1 == (bytesRd = myRead(transferringFileD, &blkBuf[3], CHUNK_SZ )))
       ErrorPrinter("myRead(transferringFileD, blkBuf, CHUNK_SZ )", __FILE__, __LINE__, errno);
    // TODO: ********* and additional code must be written ***********
    /// Send first stat block
    /// SOF 00 FF:
    blkBuf[0] = SOH;
    blkBuf[1] = blkNum;
    blkBuf[2] = ~blkNum; // get FF by complement of 00
    //blkNum++; // increment to avoid 2 blocks with value 0

    /// check if the current data block is less than 128 bytes (must be >= 128 according to ymodem pdf)
    if (bytesRd < CHUNK_SZ) { // use ctrl_z to fill in to maintain size (via CPMEOF)
       memset(blkBuf + bytesRd + 3 /*first 3 datablks*/, CTRL_Z/*CPMEOF*/, CHUNK_SZ - bytesRd);
    }

    /* calculate and add CRC in network byte order */
    // TODO: ********* The next couple lines need to be changed ***********
    uint16_t myCrc16ns;
    crc16ns(&myCrc16ns, &blkBuf[3]); /// as seen above

    blkBuf[BLK_SZ_CRC - 2] = (myCrc16ns >> 8); // msb
    blkBuf[BLK_SZ_CRC - 1] = (myCrc16ns & 0xFF); // lsb
}

void SenderY::cans()
{
   // TODO: ********* send CAN_LEN (size: 2) of CAN characters (write to mediumD) ***********
   /// use memset to fill a buffer with a character (in this case a temp variable)
   uint8_t buf[CAN_LEN]; /// size of buf = 2
   memset(buf, CAN, CAN_LEN);
   myWrite(mediumD, buf, CAN_LEN);
}

//uint8_t SenderY::sendBlk(blkT blkBuf)
void SenderY::sendBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
    int sendBlock = 0;
    /// sends the actual data in blocks
    sendBlock = myWrite(mediumD, blkBuf, BLK_SZ_CRC); /// parameters found in myIO.cpp
}

void SenderY::statBlk(const char* fileName)
{
    blkNum = 0;
    // assume 'C' received from receiver to enable sending with CRC
    genStatBlk(blkBuf, fileName); // prepare 0eth block
    sendBlk(blkBuf); // send 0eth block
    // assume sent block will be ACK'd
}

void SenderY::sendFiles()
{
    for (const auto fileName : fileNames) {
        transferringFileD = myOpen(fileName, O_RDONLY, 0);
        if(-1 == transferringFileD) {
            cans();
            cout /* cerr */ << "Error opening input file named: " << fileName << endl;
            result += "OpenError";
            return;
        }
        else {
            cout << "Sender will send " << fileName << endl;

            // do the protocol, and simulate a receiver that positively acknowledges every
            //	block that it receives.

            statBlk(fileName);

            // assume 'C' received from receiver to enable sending with CRC
            genBlk(blkBuf); // prepare 1st block
            while (bytesRd)
            {
                ++ blkNum; // 1st block about to be sent or previous block was ACK'd

                sendBlk(blkBuf); // send block

                // assume sent block will be ACK'd
                genBlk(blkBuf); // prepare next block
                // assume sent block was ACK'd
            };
            // finish up the file transfer, assuming the receiver behaves normally and there are no transmission errors

            /// The file has stopped sending data: in accordance with Ymodem instruction send EOT twice
            sendByte(EOT);
            sendByte(EOT);
            /// End
            //(myClose(transferringFileD));
            if (-1 == myClose(transferringFileD))
                ErrorPrinter("myClose(transferringFileD)", __FILE__, __LINE__, errno);
            result += "Done, ";
        }
    }
    // indicate end of the batch.
    statBlk("");

    // remove ", " from the end of the result string.
    if (result.size())
        result.erase(result.size() - 2);
}


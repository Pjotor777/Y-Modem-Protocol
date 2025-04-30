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
// Helpers: _Royce Ariell Mancilla__
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
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P2_stu1_stu2)
//% * Form groups as described at:  https://coursys.sfu.ca/docs/students
//% * Submit files to coursys.sfu.ca
//
// File Name   : ReceiverY.cpp
// Version     : September 24th, 2024
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2024 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include "ReceiverY.h"

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include "myIO.h"
#include "VNPE.h"
#include <iostream>

//using namespace std;

ReceiverY::
ReceiverY(int d)
:PeerY(d),
 m_closeProb(1),
 m_anotherFile(0xFF),
 m_NCGbyte('C'),
 m_goodBlk(false),
 m_goodBlk1st(false),
 m_syncLoss(false), // transfer will end if syncLoss becomes true
 m_numLastGoodBlk(0)
{
}

/* Only called after an SOH character has been received.
The function receives the remaining characters to form a complete
block.
The function will set or reset a Boolean variable,
goodBlk. This variable will be set (made true) only if the
calculated checksum or CRC agrees with the
one received and the received block number and received complement
are consistent with each other.
Boolean member variable syncLoss will only be set to
true when goodBlk is set to true AND there is a
fatal loss of syncronization as described in the XMODEM
specification.
The member variable goodBlk1st will be made true only if this is the first
time that the block was received in "good" condition. Otherwise
goodBlk1st will be made false.
*/
void ReceiverY::getRestBlk()
{
    // ********* this function must be improved ***********
   /// revised getRestBlk:
   PE_NOT(myReadcond(mediumD, &m_rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);
   m_goodBlk1st = m_goodBlk = true;

   /*
    * Note that
    * 1) goodBlk = true if the calculated crc == received crc, and if received blkNum and received complement are equal.
      2) syncLoss = true if goodBlk = true, and if there is fatal loss of synchronizattion as described in XMODEM specification
      3) Lastly, goodBlk1st = true if the blk we received in good condition is the first good condition block.
      -- thanks to Royce Ariel Mancilla for helping understand the theory
    * */
   // Extract the block number and its complement
   uint8_t blkNum = m_rcvBlk[SOH_OH];
   uint8_t blkNumComplement = m_rcvBlk[SOH_OH + 1]; // not needed

   // Calculate CRC from the data and compare with received CRC
   uint16_t receivedCRC = *(uint16_t*)(&m_rcvBlk[PAST_CHUNK]);;
   uint16_t computedCRC; crc16ns(&computedCRC, &m_rcvBlk[DATA_POS]); // Get value of CRC for the data

   // Validate the block number's complement
   if (blkNum != (uint8_t)(~blkNumComplement)) {
      // if the blk number does not match its complement, return
      m_goodBlk = false;
      return;
   }

   if (receivedCRC != computedCRC) { // per condition 1: compare received & calculated crc
      m_goodBlk = false;
      return;
   }

   // complement validated, blk is good, and condition 1 valid,
   // so thus set to true
   m_goodBlk = true; // block is valid, continue


   if (m_goodBlk && blkNum != m_numLastGoodBlk + 1) { // per condition 2: check if sync maintained
         m_syncLoss = true; // fatal loss of synchronization, condition 2 invalid
      }
      else {
         m_syncLoss = false; // Synchronization is stable
      }

   if (blkNum == m_numLastGoodBlk + 1) { // lastly, per condition 3: check if we got first good blk
      m_goodBlk1st = true;
      m_numLastGoodBlk = blkNum; // update the latest goodBlk number
   }
   else {
      m_goodBlk1st = false;
   }
   // Complete
}

//Write chunk (data) in a received block to disk
void ReceiverY::writeChunk()
{
   // ***** Make changes so that possible padding in the last block is not written to the file.
   /// revised writeChunk: only needed to un-comment PE_NOT statement
	PE_NOT(myWrite(transferringFileD, &m_rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

int
ReceiverY::
openFileForTransfer()
{
    const mode_t mode{S_IRUSR | S_IWUSR}; //  | S_IRGRP | S_IROTH};
    transferringFileD = myCreat((const char *) &m_rcvBlk[DATA_POS], mode);
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
        m_closeProb = myClose(transferringFileD);
        if (m_closeProb)
            return errno;
        else {
            m_numLastGoodBlk = 255;
            transferringFileD = -1;
        }
    }
    return 0;
}

//Send CAN_LEN CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverY::cans()
{
	// no need to space in time CAN chars coming from receiver
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

uint8_t
ReceiverY::
checkForAnotherFile()
{
    return (m_anotherFile = m_rcvBlk[DATA_POS]);
}

void ReceiverY::receiveFiles()
{
    ReceiverY& ctx = *this; // needed to work with SmartState-generated code

    // ***** improve this member function *****

    // below is just an example template.  You can follow a
    //  different structure if you want.

    while (
            sendByte(ctx.m_NCGbyte),
            PE_NOT(myRead(mediumD, m_rcvBlk, 1), 1), // Should be SOH
            ctx.getRestBlk(), // get block 0 with fileName and filesize
            checkForAnotherFile()) {

        if(openFileForTransfer() == -1) {
            cans();
            result = "CreatError"; // include errno or so
            return;
        }
        else {
            sendByte(ACK); // acknowledge block 0 with fileName.

            // inform sender that the receiver is ready and that the
            //      sender can send the first block
            sendByte(ctx.m_NCGbyte);

            while(PE_NOT(myRead(mediumD, m_rcvBlk, 1), 1),
                  (m_rcvBlk[0] == SOH))
            {
                ctx.getRestBlk();
                //ctx.sendByte(ACK); // assume the expected block was received correctly.
                //ctx.writeChunk();

                if (ctx.m_goodBlk == false) { /// check if the block is good
                   ctx.sendByte(NAK); /// false = not good, not acknowledged
                }
                else { /// we received the block correctly, acknowledge
                   ctx.sendByte(ACK);
                   ctx.writeChunk();
                }
            };
            // assume EOT was just read in the condition for the while loop
            ctx.sendByte(NAK); // NAK the first EOT
            PE_NOT(myRead(mediumD, m_rcvBlk, 1), 1); // presumably read in another EOT

            // Check if the file closed properly.  If not, result should be "CloseError".
            if (ctx.closeTransferredFile()) {
                ; // ***** fill this in *****
                ctx.result = "CloseError"; /// only needed to post error
            }
            else {
                 ctx.sendByte(ACK);  // ACK the second EOT
                 ctx.result += "Done, ";
            }
        }
    }
    sendByte(ACK); // acknowledge empty block 0.

    // remove ", " from the end of the result string.
    if (ctx.result.size())
       ctx.result.erase(ctx.result.size() - 2);
}

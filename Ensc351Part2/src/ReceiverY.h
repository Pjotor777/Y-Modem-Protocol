#ifndef RECEIVER_H
#define RECEIVER_H

#include "PeerY.h"

class ReceiverY : public PeerY
{
public:
	ReceiverY(int d);

	void getRestBlk();	// get the remaining bytes (132) of a block
	void writeChunk();

	int
	//ReceiverY::
	openFileForTransfer()
	;

   int
   //ReceiverY::
   closeTransferredFile()
   ;

	void cans();		// send 8 CAN characters

	uint8_t
	//ReceiverY::
	checkForAnotherFile()
	;

   void receiveFiles();

   int m_closeProb;           // return value of myClose() in closeTransferredFile() indicating error
   uint8_t m_anotherFile;    // there is a(nother) file to receive

	uint8_t m_NCGbyte;	// a 'C' sent by receiver to initiate the file transfer

	/* A Boolean variable that indicates whether the
	 *  block just received should be ACKed (true) or NAKed (false).*/
	bool m_goodBlk;

	/* A Boolean variable that indicates that a good copy of a block
	 *  being sent has been received for the first time.  It is an
	 *  indication that the data in the block can be written to disk.
	 */
	bool m_goodBlk1st;

	/* A Boolean variable that indicates whether or not a fatal loss
	 *  of synchronization has been detected.*/
	bool m_syncLoss;

	/* A variable which counts the number of responses in a
	 *  row sent because of problems like communication
	 *  problems. An initial NAK (or 'C') does not add to the count. The reception
	 *  of a particular block in good condition for the first time resets the count. */
//	unsigned errCnt;	// found in PeerX.h

private:
   off_t bytesRemaining;   // the number of bytes remaining to be written.

	// blkT rcvBlk;		// a received block
	uint8_t m_rcvBlk[BLK_SZ_CRC];		// a received block

	uint8_t m_numLastGoodBlk; // the number of the last good block
	// ********** you can add more data members if needed *******
};

#endif

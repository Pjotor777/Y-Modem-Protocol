/*
 * PeerY.h
 *
 *      Author: wcs
 */

#ifndef PEERY_H_
#define PEERY_H_

#include <stdint.h> // for uint8_t
#include <string>

#define CHUNK_SZ	 128
#define SOH_OH  	 1			//SOH Byte Overhead
#define BLK_NUM_AND_COMP_OH  2	//Overhead for blkNum and its complement
#define DATA_POS  	 (SOH_OH + BLK_NUM_AND_COMP_OH)	//Position of data in buffer
#define PAST_CHUNK 	 (DATA_POS + CHUNK_SZ)		//Position of checksum in buffer

#define CRC_OH           2			                    //Overhead for CRC16
#define REST_BLK_OH_CRC  (BLK_NUM_AND_COMP_OH + CRC_OH)	//Overhead in rest of block
#define REST_BLK_SZ_CRC  (CHUNK_SZ + REST_BLK_OH_CRC)
#define BLK_SZ_CRC  	 (SOH_OH + REST_BLK_SZ_CRC)

#define CAN_LEN 2 // normally 8 // the number of CAN characters to send to cancel a transmission

#define SOH 0x01 // start of header (begin transfer)
#define EOT 0x04 // end of transfer
#define ACK 0x06 // acknowledge
#define NAK 0x15 // not acknowledge
#define CAN 0x18 // 24 - cancel
#define	CTRL_Z 0x1A //	26

typedef uint8_t blkT[BLK_SZ_CRC];

void ErrorPrinter (const char* functionCall, const char* file, int line, int error);

void
crc16ns (uint16_t* crc16nsP, uint8_t* buf);

class PeerY {
public:
	PeerY(int d)
	;
	std::string result;  // result of the file transfer

protected:
	int mediumD; // descriptor for serial port or delegate
	int transferringFileD;	// descriptor for file being read from or written to.

	//Send a byte to the remote peer across the medium
	void
	//PeerY::
	sendByte(uint8_t byte)
	;
};

#endif /* PEERY_H_ */

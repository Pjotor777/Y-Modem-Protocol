//============================================================================
//
//% Student Name 1: 
//% Student 1 #: 
//% Student 1 userid (email): 
//
//% Student Name 2: 
//% Student 2 #: 
//% Student 2 userid (email): 
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
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P2_stu1_stu2)
//% * Form groups as described at:  https://coursys.sfu.ca/docs/students
//% * Submit files to coursys.sfu.ca
//
// Version     : September, 2024
// Copyright   : Original Portions Copyright 2024, Craig Scratchley
// Description : Starting point for ENSC 351 Project Part 2
//============================================================================

#define HEAP_PREALLOCATION 5000

#include <stdlib.h> // EXIT_SUCCESS
#include <sys/socket.h>
#include <pthread.h>
#include <thread>

#include "myIO.h"
#include "Medium.h"

#include "VNPE.h"
#include "AtomicCOUT.h"
#include "posixThread.hpp"

#include "ReceiverY.h"
#include "SenderY.h"

using namespace std;
using namespace pthreadSupport;

enum  {Term1, Term2};
enum  {TermSkt, MediumSkt};

//static int daSktPr[2];	      //Socket Pair between term1 and term2
static int daSktPrT1M[2];	  //Socket Pair between term1 and medium
static int daSktPrMT2[2];	  //Socket Pair between medium and term2

void testReceiverY(int mediumD)
{
    COUT << "Will try to receive file(s) with CRC" << endl;
    ReceiverY yReceiver(mediumD);
    yReceiver.receiveFiles();
    COUT << "yReceiver result was: " << yReceiver.result << endl  << endl;
}

void testSenderY(vector<const char*> iFileNames, int mediumD)
{
    SenderY ySender(iFileNames, mediumD);
    COUT << "test sending" << endl;
    ySender.sendFiles();
    COUT << "Sender finished with result: " << ySender.result << endl << endl;
}

void termFunc(const int termNum) /// revised termFunc
{
    // Modify to communicate through the "Kind Medium"
    if (Term1 == termNum) {
        // Terminal 1 is the receiver
        // Communicate with the medium through its designated socket descriptor
        testReceiverY(daSktPrT1M[TermSkt]); // empty file and normal file.
        // close the thread
        //PE(myClose(daSktPrT1M[TermSkt]));
    }
    else { // Term2 is the sender
        // Set thread name for Terminal 2
        PE_0(pthread_setname_np(pthread_self(), "T2"));

        // Prepare file names for testing
        //vector<const char*> iFileNamesA{"/doesNotExist.txt"};
        vector iFileNamesB{"/home/xubuntu/.sudo_as_admin_successful", "/etc/mime.types"};

        // Test sending files through the medium via the 2nd socket
        testSenderY(iFileNamesB, daSktPrMT2[TermSkt]); // empty file and normal file.
        // close the thread
        //PE(myClose(daSktPrMT2[TermSkt])); - causes error, commented out
    }
    // Close the socket descriptor for the respective terminal
    //PE(myClose((Term1 == termNum) ? daSktPrT1M[TermSkt] : daSktPrMT2[MediumSkt]));
    // Drop priority after task completion
    pthreadSupport::setSchedPrio(20);
}

int Ensc351Part2() /// revised Ensc351Part2
{
    // Set the name for the Primary thread (Terminal 1)
    PE_0(pthread_setname_np(pthread_self(), "P-T1"));

    // Create socket pairs for communication through the "kind medium" - not needed
    //static int daSktPrT1M[2];   // Socket Pair between term1 and medium
    //static int daSktPrMT2[2];   // Socket Pair between medium and term2

    // Create the socket pairs to facilitate indirect communication through the medium
    PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrT1M));
    PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrMT2));

    // Create term2 thread to run sender
    //posixThread term2Thrd(SCHED_FIFO, 70, termFunc, MediumSkt); // change term2 to Mediumskt
    posixThread term2Thrd(SCHED_FIFO, 70, termFunc, Term2); // first in first out
    // Create the medium thread with SCHED_FIFO priority 40
    //const char *logFileName = "ymodemData.dat";
    //posixThread mediumThrd(SCHED_FIFO, 40, mediumFunc, daSktPrT1M[1], daSktPrMT2[0], "ymodemData.dat"); // infinite loop?
    posixThread mediumThrd(SCHED_FIFO, 40, mediumFunc, daSktPrT1M[MediumSkt], daSktPrMT2[MediumSkt], "ymodemData.dat");
    // Run the receiver (term1) in the primary thread
    //termFunc(TermSkt); // term1 to termskt
    termFunc(Term1);
    // Term2 and medium threads must finish first before returning
    // and close the threads
    PE(myClose(daSktPrT1M[TermSkt]));
    term2Thrd.join();
    PE(myClose(daSktPrMT2[TermSkt]));
    mediumThrd.join();

    return EXIT_SUCCESS; // return
}

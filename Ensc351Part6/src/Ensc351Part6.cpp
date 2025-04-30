// Solution to ENSC 351 Part 6.  Prepared by:
//      - Craig Scratchley, Simon Fraser University
//      - Zhenwang Yao

//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <thread>
#include <iostream>

#include "myIO.h"
#include "Medium.h"
#include "Linemax.h"
#include "terminal.h"
#include "Kvm.h"
#include "VNPE.h"
#include "SocketReadcond.h"

using namespace std;

enum  {TERM_SIDE, OTHER_SIDE};

static int daSktPrTermMed[2][2];	//Socket Pairs between terminals and Medium
static int daSktPrTermKvm[2][2];	//  "      " between terminals and kvm

//kvm thread, handles all keyboard input and routes it to the selected terminal
void kvmFunc() {
	int d[]{
   	daSktPrTermKvm[Term1][OTHER_SIDE],
   	daSktPrTermKvm[Term2][OTHER_SIDE]
   	};

	Kvm(d);
	PE(myClose(d[Term1]));
	PE(myClose(d[Term2]));
}

//terminal thread
//at least 2 terminal threads are required to simulate a file transfer on a single computer
void termFunc(int termNum)
{
   PE_0(pthread_setname_np(pthread_self(), to_string(termNum).c_str())); // give the thread a name
	int inD, outD;
   inD = outD = daSktPrTermKvm[termNum][TERM_SIDE];

	int mediumD{daSktPrTermMed[termNum][TERM_SIDE]};
	//mediumD = open("/dev/ser2", O_RDWR);

	Terminal(termNum + 1, inD, outD, mediumD);
	PE(myClose(mediumD));
}

void mediumFunc(void)
{
   PE_0(pthread_setname_np(pthread_self(), "M")); // give the thread a name
	Medium medium(daSktPrTermMed[Term1][OTHER_SIDE], daSktPrTermMed[Term2][OTHER_SIDE], "ymodemData.dat");
	medium.start();
}

int Ensc351Part5()
{
	cout.precision(2);
	PE_0(pthread_setname_np(pthread_self(), "K")); // give the primary thread (does kvm) a name

	// lower the priority of the primary thread to 4
//	PE_EOK(pthread_setschedprio(pthread_self(), 4));

	//Create and wire socket pairs
	// creating socket pair between terminal1 and Medium
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrTermMed[Term1]));
	
	// creating socket pair between terminal2 and Medium
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrTermMed[Term2]));
	
	// opening kvm-term2 socket pair
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrTermKvm[Term2])); 
	
	// opening kvm-term1 socket pair
	PE(mySocketpair(AF_LOCAL, SOCK_STREAM, 0, daSktPrTermKvm[Term1]));

	//Create 3 threads

	jthread term1Thrd(termFunc, Term1);
	jthread term2Thrd(termFunc, Term2);
	
	// ***** create thread for medium *****
	jthread mediumThrd(mediumFunc);

	kvmFunc();

	return EXIT_SUCCESS;
}

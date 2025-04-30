/*
 * Kvm.cpp
 *
 *      Author: wcs
 */

#include <span>
//#include <algorithm>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "Linemax.h"
#include "myIO.h"
#include "Kvm.h"
#include "VNPE.h"
#include "AtomicCOUT.h"

using namespace std;

//KVM input commands
#define KVM_TERM1_C		"~1\n"
#define KVM_TERM2_C		"~2\n"
#define KVM_QUIT_C		"~q!\n"

void Kvm(span<const int> d)
{
	// we could generalize to a kvm supporting an arbitrary number of terminals.
	int term_num = Term2; // initially terminal 2 selected
	COUT << "KVM FUNCTION BEGINS (INPUT ROUTED TO terminal " << (term_num + 1) << ")" << endl;

	fd_set set;
	FD_ZERO(&set);
	int max_fd{max(d[Term1], d[Term2])};
	max_fd = max(max_fd, STDIN_FILENO)+1;

	char buf[LINEMAX];
	while(1) {

		FD_SET(STDIN_FILENO, &set);	//stdin
		FD_SET(d[Term1], &set);
		FD_SET(d[Term2], &set);
		int rv = PE(select(max_fd, &set, NULL, NULL, NULL));
		if( rv == 0 ) {
			CERR << "select() should not timeout" << endl;
			exit(EXIT_FAILURE);
		} else {
			if( FD_ISSET(STDIN_FILENO, &set) ) {
				//read the keyboard info into a buffer
				//replaces  cin>>buf;
				// should we do 'cin.getline(buf, LINEMAX_SAFE)' and use cin.gcount()?
				int numBytesRead = PE(read(STDIN_FILENO, buf, LINEMAX_SAFE));
				buf[numBytesRead] = 0;

				// best to first check for "~", and then check for rest of command
				if( strcmp( buf, KVM_QUIT_C ) == 0) {
									COUT << "KVM TERMINATING" << endl;
									break;
				} else if( strcmp( buf, KVM_TERM1_C ) == 0) {
					COUT << "KVM SWITCHING TO terminal 1" << endl;
					term_num = Term1;
				} else if( strcmp( buf, KVM_TERM2_C ) == 0) {
					COUT << "KVM SWITCHING TO terminal 2" << endl;
					term_num = Term2;
				} else {
					//route keyboard input to selected terminal
					PE_NOT(myWrite(d[term_num], buf, numBytesRead), numBytesRead); // strlen(buf)+1
				}
			}
			for (auto descriptor: d)
            if ( FD_ISSET(descriptor, &set) ) {
               //kvm simply displays input sent from terminal
               auto numBytesRead{PE(myRead(descriptor, buf, LINEMAX_SAFE))};
               buf[numBytesRead] = 0; // terminate the string
               COUT << buf << flush;
            }
//			if ( FD_ISSET(T2d, &set) ) {
//				//kvm simply displays input sent from terminal 2
//				int numBytesRead = PE(myRead(T2d, buf, LINEMAX_SAFE));
//				buf[numBytesRead] = 0;
//				COUT << buf << flush;
//			}
		}
	}
	return;
}


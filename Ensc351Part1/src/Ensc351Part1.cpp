//============================================================================
// Name        : Ensc351Part1.cpp
// Version     : September 5th
// Copyright   : Copyright 2024, Craig Scratchley
// Description : Starting point for Part 1 of Multipart Project
//============================================================================

#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <vector>

#include "myIO.h"
#include "SenderY.h"

using namespace std;

void testSenderY(vector<const char*> iFileNames, int mediumD)
{
	SenderY ySender(iFileNames, mediumD);
	cout << "test sending" << endl;
	ySender.sendFiles();
	cout << "Sender finished with result: " << ySender.result << endl << endl;
}

int main() {
#ifdef __MINGW32__
    _fmode = _O_BINARY;  // needed for MinGW compiler which runs on MS Windows
#endif

    // for x86_64, output file will be in the Eclipse project.
    // for mips, output file will be on the Desktop:  /home/xubuntu/Desktop
    const char* oFileName{"ymodemSenderData.dat"};
    mode_t mode{S_IRUSR | S_IWUSR}; // | S_IRGRP | S_IROTH;
    int mediumD = myCreat(oFileName, mode);
    if(-1 == mediumD) {
        cout /* cerr */ << "Error opening medium file named: " << oFileName << endl;
        ErrorPrinter("myCreat(oFileName, mode)", __FILE__, __LINE__, errno);
        return -1;
    }

    vector iFileNamesA{"/doesNotExist.txt"};
    vector iFileNamesB{"/home/xubuntu/.sudo_as_admin_successful", "/etc/mime.types"};

    testSenderY(iFileNamesA, mediumD);                        // file does not exist
    testSenderY(iFileNamesB, mediumD);                        // empty file and normal text file

    if (-1 == myClose(mediumD)) {
        ErrorPrinter("close(mediumD)", __FILE__, __LINE__, errno);
        return -1;
    }
    else
        return 0;
}

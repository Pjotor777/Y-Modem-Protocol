//============================================================================
// Name        : myIO.cpp
// Author(s)   : William Craig Scratchley
//			   :
// Version     : November 2024 -- tcdrain for socketpairs.
// Copyright   : Copyright 2024, W. Craig Scratchley, SFU
// Description : An implementation of tcdrain-like behaviour for socketpairs.
//============================================================================

// use a circular buffer instead of read() and write() functions.
//#define CIRCBUF

#include <sys/socket.h>
#include <unistd.h>				// for posix i/o functions
#include <stdlib.h>
#include <termios.h>			// for tcdrain()
#include <fcntl.h>				// for open/creat
#include <errno.h>
#include <stdarg.h>
#include <mutex>				
#include <shared_mutex>
#include <condition_variable>	
#include <map>
#include <memory>
#include "AtomicCOUT.h"
#include "SocketReadcond.h"
#include "VNPE.h"
#ifdef CIRCBUF /// gives better performance if used
    #include "RageUtil_CircularBuffer.h"
#endif

using namespace std;

// derived from https://stackoverflow.com/a/40159821
// tweaked by Craig Scratchley
// other variations might be useful.
// get a Value for a key in m or return a default_value if the key is not present
template <typename Key, typename Value, typename T>
Value get_or(std::map<Key, Value>& m, const Key& key, T&& default_value)
{
   /// use of && allows for perfect forwarding, enabling the function to
   /// accept any type of default value, whether it's an lvalue or rvalue. This helps avoid unnecessary copying of the default value
    auto it{m.find(key)}; /// iterator to find the key
    if (m.end() == it) {
        return default_value; /// no key found - returns the default_value passed in as a parameter
    }
    return it->second; ///  returns the value associated with the key (it->second)
}

//Unnamed namespace
/// ensures that everything declared within the namespace
/// is only visible to the current translation unit (source file)
namespace{

    class socketInfoClass;

    typedef shared_ptr<socketInfoClass> socketInfoClassSp;
    map<int, socketInfoClassSp> desInfoMap; ///  used to manage the socket pairs by storing descriptor info objects.

    //  A shared mutex used to protect desInfoMap so only a single thread can modify the map at a time.
    //  This also means that only one call to functions like mySocketpair() or myClose() can make progress at a time.
    //  This mutex is also used to prevent a paired socket from being closed at the beginning of a myWrite or myTcdrain function.
    //  Shared mutex is described in Section 3.3.2 of Williams 2e
    /// mapMutex protects desInfoMap against concurrent modifications
    shared_mutex mapMutex;

    class socketInfoClass {
        unsigned totalWritten{0};
        unsigned maxTotalCanRead{0};
        condition_variable cvDrain;
        condition_variable cvRead;
    #ifdef CIRCBUF /// If the CIRCBUF directive is enabled, it uses a circular buffer to enhance I/O efficiency
        CircBuf<char> circBuffer;
//        bool connectionReset = false;
    #endif
        mutex socketInfoMutex;
    public:
        int pair;   // Cannot be private because myWrite and myTcdrain using it.
                    // -1 when descriptor closed, -2 when paired descriptor is closed

        socketInfoClass(unsigned pairInit)
        :pair(pairInit) {
    #ifdef CIRCBUF
           circBuffer.reserve(1100); // note constant of 1100
    #endif
        }

	/*
	 * Function:  if necessary, make the calling thread wait for a reading thread to drain the data
	 */
	int draining(shared_lock<shared_mutex> &desInfoLk) /// makes a thread wait until the data in the socket has been drained
	{ // operating on object for paired descriptor of original des
		unique_lock socketLk(socketInfoMutex); /// local unique lock
		desInfoLk.unlock(); /// shared lock being unlocked - any thread that needs to use this func can unlock/lock it itself without
		 /// owning the mutex associated with the function

		//  once the reader decides the drainer should wakeup, it should wakeup
		/// This indicates that the socket's paired descriptor is still valid. A value of -1
		/// would indicate that the paired descriptor is closed.
		/// condition ensures that the function only waits if there is more data written to the socket than what can
		/// be read. This means that the socket is not yet drained, and the drainer needs to wait
		if (pair >= 0 && totalWritten > maxTotalCanRead) /// not done draining?
			cvDrain.wait(socketLk); /// wait for thread to finish draining, then move on
		// spurious wakeup?  Not a problem with Linux p-threads
		   // In the Solaris implementation of condition variables,
		   //    a spurious wakeup may occur without the condition being assigned
		   //    if the process is signaled; the wait system call aborts and
		   //    returns EINTR.[2] The Linux p-thread implementation of condition variables
		   //    guarantees that it will not do that.[3][4]
		   // https://en.wikipedia.org/wiki/Spurious_wakeup
		   //  cvDrain.wait(socketLk, [this]{return pair < 0 || totalWritten <= maxTotalCanRead;});

//		if (pair == -2) { // shouldn't normally happen
//			errno = EBADF; // check errno
//			return -1;
//		}
		return 0;
	}

	int writing(int des, const void* buf, size_t nbyte, shared_lock<shared_mutex> &desInfoLk)	{
	   /// responsible for writing data to a socket
		// operating on object for paired descriptor
	   /// ensures exclusive access to the socket, preventing race conditions when multiple threads attempt to modify the socket state simultaneously
		lock_guard socketLk(socketInfoMutex); /// use a lock guard to prevent conflicts
      desInfoLk.unlock(); /// givetrheads access to socket info

#ifdef CIRCBUF
		int written = circBuffer.write((const char*) buf, nbyte);
#else
		int written = write(des, buf, nbyte);
#endif
        if (written > 0) { /// condition checks if the write operation was successful and if a positive number of bytes was written
            totalWritten += written; /// update value of written
            cvRead.notify_one(); /// finished - notify the next thread
        }
        return written;
	}

	int reading(int des, void * buf, int n, int min, int time, int timeout, shared_lock<shared_mutex> &desInfoLk)
	{ // it is assumed that des is for a socket in a socketpair created by mySocketpair
		int bytesRead;
		unique_lock socketLk(socketInfoMutex);
		/// shared lock was used to ensure thread safety while accessing the global socket map
      desInfoLk.unlock(); /// unlocking it, other threads can now access the socket map concurrently.

		// would not have got this far if pair == -1
      if (-2 == pair) /// -2 is a closed socket
         //// void a connection reset error (errno == 104), it sets bytesRead to zero, indicating no data was read.
         bytesRead = 0; // this avoids errno == 104 (Connection Reset by Peer)
      else if (!maxTotalCanRead && totalWritten >= (unsigned) min) { /// ndicates that no other threads are waiting to read data from this socket.
         if (0 == min && 0 == totalWritten)
             bytesRead = 0; /// no bytes read at all
              /// early exit for non-blocking reads where there's nothing to read.
         else {
#ifdef CIRCBUF
		        bytesRead = circBuffer.read((char *) buf, n);
#else
		        bytesRead = read(des, buf, n); // at least min will be waiting
#endif
		        if (bytesRead > 0) {/// data was successfully read, the code continues to update the socket state.
		         /// Decreases the value of totalWritten by the number of bytes that were read (bytesRead).
		         /// This keeps track of the remaining unread data in the socket.
		           totalWritten -= bytesRead;
		         /// If the remaining data (totalWritten) is now less than or equal to the maximum that can be
		         /// read (maxTotalCanRead), it notifies waiting threads
		           if (totalWritten <= maxTotalCanRead) {
		              int errnoHold{errno};
                    cvDrain.notify_all(); /// used to notify all waiting threads that the socket has drained enough data to be read
                    errno = errnoHold;
		           }
		        }
		    }
		}
		else { ///  executed when there is not enough data available to meet the minimum read requirement (min
		 /// Increases maxTotalCanRead by n. This indicates that this socket is waiting for n more bytes to be written
			maxTotalCanRead += n;
         int errnoHold{errno};
         cvDrain.notify_all(); // totalWritten must be less than min
			if (0 != time || 0 != timeout) { /// unsupported timeout handling
			   COUT << "Currently only supporting no timeouts or immediate timeout" << endl;
			   exit(EXIT_FAILURE); /// exit the whole thing
			}
			/*
			 * Waits on the cvRead condition variable until either:
            totalWritten is greater than or equal to min (meaning sufficient data is available), or
            The paired socket (pair) is closed (pair < 0).
            This is a blocking wait that puts the calling thread to sleep until one of the conditions is met.
			*/
			cvRead.wait(socketLk, [this, min] { /// ;ambda func for
			   return totalWritten >= (unsigned) min || pair < 0;});
         errno = errnoHold;
//			if (pair == -1) { // shouldn't normally happen
//			   errno = EBADF; // check errno value
//			   return -1;
//			}
         /*
          * If CIRCBUF is defined, it reads from the circular buffer.
            Otherwise, it uses the read() system call.
            Handling Read Errors:
            If read() returns -1, it means an error occurred during reading.
            If errno == ECONNRESET, the connection was reset, so bytesRead is set to 0 to indicate that no data was read.
          */
#ifdef CIRCBUF
			bytesRead = circBuffer.read((char *) buf, n);
         totalWritten -= bytesRead;
#else
			// choice below could affect "Connection reset by peer" from read/wcsReadcond
			bytesRead = read(des, buf, n);
//			bytesRead = wcsReadcond(des, buf, n, min, time, timeout);

			if (-1 != bytesRead)
            totalWritten -= bytesRead;
			else
			   if (ECONNRESET == errno)
			      bytesRead = 0;
#endif // #ifdef CIRCBUF
            
			maxTotalCanRead -= n; /// educes the waiting read requests on this socket by the number of bytes that were just rea

			if (0 < totalWritten || -2 == pair) {
            int errnoHold{errno}; // debug gui not updating errnoHold very well.
            cvRead.notify_one(); /// notify the next thread to keep writing or that the socket is closed
            errno = errnoHold;
         }
		}
		return bytesRead;
	} // .reading()

	/*
	 * Function:  Closing des. Should be done only after all other operations on des have returned.
	 */
	int closing(int des)
	{
		// mapMutex already locked at this point, so no other myClose (or mySocketpair)
	   /// Checks if the paired socket has been closed (pair == -2). If true, it means the paired descriptor is no longer available.
		if(pair != -2) { // pair has not already been closed
		   /// Retrieves the shared pointer (socketInfoClassSp) for the paired socket descriptor (pair).
		   /// This provides access to the paired socket information stored in the global descriptor map (desInfoMap).
			socketInfoClassSp des_pair{desInfoMap[pair]}; /// std::shared_ptr
			// See Williams 2e, 2nd half of section 3.2.4
			/// scoped lock locks multiple mutexes safely at the same time,
			/// i.e it accepts a list of mutex types as template parameters
			scoped_lock guard(socketInfoMutex, des_pair->socketInfoMutex); // safely lock both mutexes
			/// close the socket
			pair = -1; // this is first socket in the pair to be closed
			/// indicating that the paired socket is the second of the two sockets to close
			des_pair->pair = -2; // paired socket will be the second of the two to close.
         if (totalWritten > maxTotalCanRead) { /// bits written > max  of bits written
             // by closing the socket we are throwing away any buffered data.
             // notification will be sent immediately below to any myTcdrain waiters on paired descriptor.
            /// Notifies all threads waiting on the cvDrain condition variable.
            /// This is used to wake up any threads that might be waiting for data to be drained before proceeding.
             cvDrain.notify_all(); /// notify all to drain
         }
//         if (maxTotalCanRead > 0) {
//             // there shouldn't be any threads waiting in myRead() or myReadcond() on des, but just in case.
//             cvRead.notify_all();
//         }

			if (des_pair->maxTotalCanRead > 0) {
				// no more data will be written from des
				// notify a thread waiting on reading on paired descriptor
			   /// Notifies one thread waiting on the cvRead condition variable of the paired socket.
			   /// This informs the waiting thread that no more data will be written, and it can handle this situation accordingly.
				des_pair->cvRead.notify_one(); /// notify only the next thread in queue
			}
//			if (des_pair->totalWritten > des_pair->maxTotalCanRead) {
//				// there shouldn't be any threads waiting in myTcdrain on des, but just in case.
//				des_pair->cvDrain.notify_all();
//			}
		}
		return close (des);
	} // .closing()
	}; // socketInfoClass
} // unnamed namespace

/*
 * Function:	Calling the reading member function to read
 * Return:		An integer with number of bytes read, or -1 for an error.
 * see https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/r/readcond.html
 *
 */
int myReadcond(int des, void * buf, int n, int min, int time, int timeout) {
   shared_lock desInfoLk(mapMutex);
   /// uses the get_or function template to retrieve the
   /// shared pointer associated with the descriptor des from the desInfoMap.
   auto desInfoP{get_or(desInfoMap, des, nullptr)}; // make a local shared pointer
   /// Checks if the descriptor information was successfully retrieved from the desInfoMap (i.e., if desInfoP is not null).
   if (desInfoP) /// shared ptr of the paired skt exists
	    return desInfoP->reading(des, buf, n, min, time, timeout, desInfoLk);
    return wcsReadcond(des, buf, n, min, time, timeout);
}

/*
 * Function:	Reading directly from a file or from a socketpair descriptor)
 * Return:		the number of bytes read , or -1 for an error
 */
ssize_t myRead(int des, void* buf, size_t nbyte) {
   shared_lock desInfoLk(mapMutex);
   auto desInfoP{get_or(desInfoMap, des, nullptr)}; // make a local shared pointer
	if (desInfoP) /// info of the pair socket exists as a shared_ptr -> proceed
	    // myRead (for sockets) usually reads a minimum of 1 byte
	    return desInfoP->reading(des, buf, nbyte, 1, 0, 0, desInfoLk); /// call reading
	return read(des, buf, nbyte); // des is closed or not from a socketpair
}

/*
 * Return:		the number of bytes written, or -1 for an error
 */
ssize_t myWrite(int des, const void* buf, size_t nbyte) {
    {
        shared_lock desInfoLk(mapMutex);
        auto desInfoP{get_or(desInfoMap, des, nullptr)};
        if (desInfoP) {
           auto pair{desInfoP->pair}; /// Retrieves the paired descriptor for the current socket from desInfoP.
           if (-2 != pair) /// checks if the socket isnt closed
              return desInfoMap[pair]->writing(des, buf, nbyte, desInfoLk); /// write data to socket
        }
    }
    return write(des, buf, nbyte); // des is not from a pair of sockets or socket or pair closed
}

/*
 * Function:  make the calling thread wait for a reading thread to drain the data
 */
int myTcdrain(int des) {
    {
        shared_lock desInfoLk(mapMutex);
        auto desInfoP{get_or(desInfoMap, des, nullptr)};
        if (desInfoP) {
           auto pair{desInfoP->pair}; /// Retrieves the paired descriptor for the current socket from desInfoP.
           if (-2 == pair) /// checks if the socket isnt closed
              return 0; /// paired descriptor is closed - we are done draining
           else { /// not done draining?
              auto desPairInfoSp{desInfoMap[pair]}; // make a local shared pointer
              /// Calls the draining() function on the paired socket to wait until all data is drained.
              return desPairInfoSp->draining(desInfoLk);
           }
        }
    }
    return tcdrain(des); // des is not from a pair of sockets or socket closed
}

/*
 * Function:   Create pair of sockets and put them in desInfoMap
 * Return:     return an integer that indicate if it is successful (0) or not (-1)
 */
int mySocketpair(int domain, int type, int protocol, int des[2]) { /// responsible for making socket pairs
   int returnVal{socketpair(domain, type, protocol, des)};
   if(-1 != returnVal) {
      lock_guard desInfoLk(mapMutex);
      desInfoMap[des[0]] = make_shared<socketInfoClass>(des[1]);
      desInfoMap[des[1]] = make_shared<socketInfoClass>(des[0]);
   }
   return returnVal;
}

/*
 * Function:   close des
 *       myClose() should not be called until all other calls using the descriptor have finished.
 */
int myClose(int des) {
   {
        lock_guard desInfoLk(mapMutex);
        auto iter{desInfoMap.find(des)}; /// Attempts to find the descriptor (des) in the descriptor map (desInfoMap).
        if (iter != desInfoMap.end()) { // if in the map
            auto mySp{iter->second}; /// Retrieves the shared pointer (mySp) associated with the descriptor.
             /// Erases the descriptor from the descriptor map (desInfoMap) to indicate that it is no longer available.
            desInfoMap.erase(iter);
            if (mySp) // if shared pointer exists (it should)
                return mySp->closing(des); /// handle the cleanup for the descriptor (des after closing
        }
   }
   return close(des);
}

/*
 * Function:	Open a file and get its file descriptor.
 * Return:		return value of open
 */
int myOpen(const char *pathname, int flags, ...) //, mode_t mode)
{ /// for creating the file and opening the sockets
   mode_t mode{0};
   // in theory we should check here whether mode is needed.
   va_list arg;
   va_start (arg, flags);
   mode = va_arg (arg, mode_t);
   va_end (arg);
   return open(pathname, flags, mode);
}

/*
 * Function:	Create a new file and get its file descriptor.
 * Return:		return value of creat
 */
int myCreat(const char *pathname, mode_t mode)
{
   return creat(pathname, mode);
}



/* Additional QNX-like functions.  Simon Fraser University -- Sept., 2019, By
 *  - Craig Scratchley
 */

#ifndef SOCKETREADCOND_H_
#define SOCKETREADCOND_H_

#ifdef __cplusplus
extern "C"
{
#endif

	// a version of readcond() that works with socket(pair)s and, on QNX, terminal devices
	int wcsReadcond( int fd, void * buf, int n, int min, int time, int timeout);

#ifdef __cplusplus
}
#endif

#endif /*SOCKETREADCOND_H_*/

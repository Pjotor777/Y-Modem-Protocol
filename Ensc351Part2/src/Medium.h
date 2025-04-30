/*
 * Medium.h
 *
 *      Author: Craig Scratchley
 *      Version: September 10, 2021
 *      Copyright(c) 2021 Craig Scratchley
 */

#ifndef MEDIUM_H_
#define MEDIUM_H_

class Medium {
public:
	Medium(int d1, int d2, const char *fname);
	virtual ~Medium(); // is virtual needed?

	void run();

private:
	int m_Term1D;	// descriptor for Term1
	int m_Term2D;	// descriptor for Term2
	int m_logFileD;	// descriptor for log file

	int m_byteCount;

	int m_ACKreceived;
	int m_ACKforwarded;
	bool m_sendExtraAck;

	void
	//Medium::
	mediumFuncT1toT2()
	;

	bool MsgFromTerm1();
	bool MsgFromTerm2();
};

void mediumFunc(int T1d, int T2d, const char *fname)
;

#endif /* MEDIUM_H_ */

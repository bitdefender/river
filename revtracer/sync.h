#ifndef _SYNC_H
#define _SYNC_H

class RiverMutex {
private :
	volatile long mtx;
public :
	RiverMutex();
	~RiverMutex();

	void Lock();
	void Unlock();
};

#endif

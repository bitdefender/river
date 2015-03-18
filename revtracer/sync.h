#ifndef _SYNC_H
#define _SYNC_H

typedef volatile long _tbm_mutex;

void TbmMutexLock(_tbm_mutex *mutex);
void TbmMutexUnlock(_tbm_mutex *mutex);

#endif

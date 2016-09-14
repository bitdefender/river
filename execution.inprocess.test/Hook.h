#ifndef  _HOOK_H_

typedef void *(*RiverExceptionHandler)(unsigned int translatedEip, unsigned int &originalEip);

bool ApplyExceptionHook(RiverExceptionHandler hnd);

#endif // ! _HOOK_H_

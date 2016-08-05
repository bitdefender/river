#ifndef _VARIABLE_TRACKER_H_
#define _VARIABLE_TRACKER_H_

//#include <Windows.h>

//class VariableTracker;

template <typename T> class VariableTracker {
public :
	typedef bool(*IsLockedFunc)(void *ctx, const T*);
	typedef void(*LockFunc)(void *ctx, T*);
	typedef void(*UnlockFunc)(void *ctx, T*);

private :
	struct {
		T var;
		rev::DWORD step;
	} stack[32];
	rev::DWORD top;
	rev::DWORD step;

	IsLockedFunc isLocked;
	LockFunc lock;
	UnlockFunc unlock;
	void *ctx;
public :
	VariableTracker<T>(void *c, IsLockedFunc il, LockFunc l, UnlockFunc u) {
		top = 0;
		step = 0;

		ctx = c;
		isLocked = il;
		lock = l;
		unlock = u;
	}

	void Lock(T *m) {
		if (!isLocked(ctx, m)) {
			stack[top].var = *m;
			stack[top].step = step;
			top++;

			lock(ctx, m);
		}
	}

	void Forward() {
		step++;
	}

	void Backward() {
		step--;

		while ((top > 0) && (step < stack[top - 1].step)) {
			unlock(ctx, &stack[top - 1].var);
			top -= 1;
		}
	}
};

#endif


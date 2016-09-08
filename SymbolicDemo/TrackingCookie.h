#ifndef _TRACKING_COOKIE_
#define _TRACKING_COOKIE_

struct TrackedVariableData {
private:
	bool isLocked;
public:
	TrackedVariableData() {
		isLocked = false;
	}

	void Lock() {
		isLocked = true;
	}

	void Unlock() {
		isLocked = false;
	}

	bool IsLocked() const {
		return isLocked;
	}
};

void Unlock(void *ctx, TrackedVariableData *tvd);

/**/

#endif


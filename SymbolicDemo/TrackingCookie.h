#ifndef _TRACKING_COOKIE_
#define _TRACKING_COOKIE_

typedef void *(* AllocateTrackingCookieFunc)();
typedef void (* FreeTrackingCookieFunc)(void *);

struct TrackingCookieFuncs {
	AllocateTrackingCookieFunc allocCookie;
	FreeTrackingCookieFunc freeCookie;
};

extern TrackingCookieFuncs trackingCookieFuncs;

struct TrackedVariableData {
public:
	bool isLocked;

	TrackedVariableData() {
		isLocked = false;
	}
};

/**/

#endif


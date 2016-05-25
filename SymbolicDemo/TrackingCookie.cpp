#include "TrackingCookie.h"

TrackedVariableData tvs[5];
unsigned int tvCount = 0;

void *AllocTrackedVariableData() {
	return &tvs[tvCount++];
}

void DeleteTrackedVariableData(void *ptr) {
	//TrackedVariableData *tvd = (TrackedVariableData *)ptr;
	//delete ptr;
}

struct TrackingCookieFuncs trackingCookieFuncs = {
	AllocTrackedVariableData,
	DeleteTrackedVariableData
};
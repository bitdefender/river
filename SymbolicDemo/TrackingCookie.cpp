#include "TrackingCookie.h"

TrackedVariableData tvs[8];
unsigned int tvCount = 0;

void Unlock(void *ctx, TrackedVariableData *tvd) {
	tvd->Unlock();
}

void *AllocTrackedVariableData() {
	return &tvs[tvCount++];
}

void DeleteTrackedVariableData(void *ptr) {
	TrackedVariableData *tvd = (TrackedVariableData *)ptr;
	
	/*if (tvd->next != tvd) {
		tvd->next->prev = tvd->prev;
		tvd->prev->next = tvd->next;
	}*/
}

/*void *LinkTrackedVariableData(void *a, void *b) {
	if (nullptr == a) {
		return b;
	}

	if (nullptr == b) {
		return a;
	}

	// a and b are nonnull

	TrackedVariableData *tva = (TrackedVariableData *)a;
	TrackedVariableData *tvb = (TrackedVariableData *)b;

	tva->next->prev = tvb->prev;
	tvb->prev->next = tva->next;

	tva->next = tvb;
	tvb->prev = tva;

	return tva;
}*/

/*struct TrackingCookieFuncs trackingCookieFuncs = {
	AllocTrackedVariableData,
	DeleteTrackedVariableData,
	LinkTrackedVariableData
};*/
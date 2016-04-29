#ifndef _TRACKED_VALUES_H_
#define _TRACKED_VALUES_H_

#include "revtracer.h"

namespace rev {

	template <typename T> class TrackingReference {
	private :
		DWORD refCount;
		T *ref;

	public :
		TrackingReference();

		void Alloc();
		bool Release();

		void SetValue(T *value);

		T* operator->();
		const T* operator->() const;

		T& operator*();
		const T& operator*() const;
	};


	template <typename T, int N> class TrackingTable {
	private :
		TrackingReference<T> refs[N];
		DWORD freeRefs[N];
		DWORD freeCount;

	public :
		TrackingTable();

		DWORD AllocRef();
		void ReleaseRef(DWORD idx);

		TrackingReference<T> &operator[] (int idx);
		const TrackingReference<T> &operator[] (int idx) const;
	};
};

#endif


#include "TrackedValues.h"

#include "TrackingItem.h"

namespace rev {
	template <typename T> TrackingReference<T>::TrackingReference() {
		ref = nullptr;
		refCount = 0;
	}

	template <typename T> void TrackingReference<T>::Alloc() {
		refCount++;
	}

	template <typename T> bool TrackingReference<T>::Release() {
		refCount--;

		return refCount == 0;
	}

	template <typename T> void TrackingReference<T>::SetValue(T *value) {
		ref = value;
	}

	template <typename T> T* TrackingReference<T>::operator->() {
		return ref;
	}

	template <typename T> const T* TrackingReference<T>::operator->() const {
		return ref;
	}

	template <typename T> T& TrackingReference<T>::operator*() {
		return *ref;
	}

	template <typename T> const T& TrackingReference<T>::operator*() const {
		return *ref;
	}



	template <typename T, int N> TrackingTable<T, N>::TrackingTable() {
		freeCount = N;
		for (DWORD i = 0; i < N; ++i) {
			freeRefs[i] = i;
		}
	}



	template <typename T, int N> DWORD TrackingTable<T, N>::AllocRef() {
		if (freeCount == 0) {
			return 0;
		}

		freeCount--;
		return freeRefs[freeCount] + 1;
	}

	template <typename T, int N> void TrackingTable<T, N>::ReleaseRef(DWORD idx) {
		if (refs[idx - 1].Release()) {
			freeRefs[freeCount] = idx;
			freeCount++;
		}
	}

	template <typename T, int N> TrackingReference<T> &TrackingTable<T, N>::operator[] (int idx) {
		return refs[idx - 1];
	}

	template <typename T, int N> const TrackingReference<T> &TrackingTable<T, N>::operator[] (int idx) const {
		return refs[idx - 1];
	}

	template class TrackingReference<Temp>;
	template class TrackingTable<Temp, 128>;

	template class TrackingReference<void *>;
	template class TrackingTable<void *, 0x10000>;
};
#ifndef _LIBRARY_LAYOUT_H_
#define _LIBRARY_LAYOUT_H_

namespace ext {
	union LibraryLayout {
		struct LinuxLayout {
			unsigned int libcBase;
			unsigned int librtBase;
			unsigned int libpthreadBase;
		} linLib;
		struct WindowsLayout {
			unsigned int ntdllBase;
		} winLib;
	};
};


#endif

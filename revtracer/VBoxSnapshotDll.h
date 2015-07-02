/* $Id: VBoxHook.h $ */
/** @file
 * VBoxHook -- Global windows hook dll.
 */

/*
 * Copyright (C) 2006-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef ___winnt_include_VBoxSnapshotDll_h
#define ___winnt_include_VBoxSnapshotDll_h

#include <Windows.h>
#include <stdint.h>

#define VBOXSNAPSHOTDLL_DLL_NAME              "VBoxSnapshotDll.dll"

extern "C" uint64_t BDTakeSnapshot();
extern "C" uint64_t BDRestoreSnapshot();
extern "C" BOOL BDSnapshotInit();

typedef uint64_t (*BDTakeSnapshotType)();
typedef uint64_t (*BDRestoreSnapshotType)();
typedef BOOL (*BDSnapshotInitType)();

// This values are extracted from the HostServices/GuestSnapshot/service.cpp file
#define BD_SNAPSHOT_FAIL 0
#define BD_SNAPSHOT_TAKE 1
#define BD_SNAPSHOT_RESTORE 2

#endif
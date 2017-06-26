#!/usr/bin/env bash

mkdir $1
mkdir $1/include

mkdir $1/include/CommonCrossPlatform
cp CommonCrossPlatform/Common.h $1/include/CommonCrossPlatform
cp CommonCrossPlatform/BasicTypes.h $1/include/CommonCrossPlatform

mkdir $1/include/Execution
cp Execution/Execution.h $1/include/Execution

mkdir $1/include/revtracer
cp revtracer/common.h $1/include/revtracer
cp revtracer/DebugPrintFlags.h $1/include/revtracer
cp revtracer/environment.h $1/include/revtracer
cp revtracer/revtracer.h $1/include/revtracer
cp revtracer/river.h $1/include/revtracer
cp revtracer/RiverAddress.h $1/include/revtracer

mkdir $1/include/SymbolicEnvironment
cp SymbolicEnvironment/Environment.h $1/include/SymbolicEnvironment
cp SymbolicEnvironment/LargeStack.h $1/include/SymbolicEnvironment
cp SymbolicEnvironment/SymbolicEnvironment.h $1/include/SymbolicEnvironment

mkdir $1/lin
mkdir $1/lin/lib
cp revtracer/revtracer.dll $1/lin/lib
cp revtracer-wrapper/librevtracerwrapper.so $1/lin/lib
cp Execution/libexecution.so $1/lin/lib
cp loader/libloader.so $1/lin/lib
cp ipclib/libipc.so $1/lin/lib
cp SymbolicEnvironment/libsymbolicenvironment.so  $1/lin/lib

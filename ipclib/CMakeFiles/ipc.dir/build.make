# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ciprian/testtools/river

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ciprian/testtools/river

# Include any dependencies generated for this target.
include ipclib/CMakeFiles/ipc.dir/depend.make

# Include the progress variables for this target.
include ipclib/CMakeFiles/ipc.dir/progress.make

# Include the compile flags for this target's objects.
include ipclib/CMakeFiles/ipc.dir/flags.make

ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o: ipclib/CMakeFiles/ipc.dir/flags.make
ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o: ipclib/ipclib.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ciprian/testtools/river/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o"
	cd /home/ciprian/testtools/river/ipclib && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ipc.dir/ipclib.cpp.o -c /home/ciprian/testtools/river/ipclib/ipclib.cpp

ipclib/CMakeFiles/ipc.dir/ipclib.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ipc.dir/ipclib.cpp.i"
	cd /home/ciprian/testtools/river/ipclib && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ciprian/testtools/river/ipclib/ipclib.cpp > CMakeFiles/ipc.dir/ipclib.cpp.i

ipclib/CMakeFiles/ipc.dir/ipclib.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ipc.dir/ipclib.cpp.s"
	cd /home/ciprian/testtools/river/ipclib && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ciprian/testtools/river/ipclib/ipclib.cpp -o CMakeFiles/ipc.dir/ipclib.cpp.s

ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.requires:

.PHONY : ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.requires

ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.provides: ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.requires
	$(MAKE) -f ipclib/CMakeFiles/ipc.dir/build.make ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.provides.build
.PHONY : ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.provides

ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.provides.build: ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o


# Object files for target ipc
ipc_OBJECTS = \
"CMakeFiles/ipc.dir/ipclib.cpp.o"

# External object files for target ipc
ipc_EXTERNAL_OBJECTS =

ipclib/libipc.so: ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o
ipclib/libipc.so: ipclib/CMakeFiles/ipc.dir/build.make
ipclib/libipc.so: ipclib/CMakeFiles/ipc.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ciprian/testtools/river/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library libipc.so"
	cd /home/ciprian/testtools/river/ipclib && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ipc.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
ipclib/CMakeFiles/ipc.dir/build: ipclib/libipc.so

.PHONY : ipclib/CMakeFiles/ipc.dir/build

ipclib/CMakeFiles/ipc.dir/requires: ipclib/CMakeFiles/ipc.dir/ipclib.cpp.o.requires

.PHONY : ipclib/CMakeFiles/ipc.dir/requires

ipclib/CMakeFiles/ipc.dir/clean:
	cd /home/ciprian/testtools/river/ipclib && $(CMAKE_COMMAND) -P CMakeFiles/ipc.dir/cmake_clean.cmake
.PHONY : ipclib/CMakeFiles/ipc.dir/clean

ipclib/CMakeFiles/ipc.dir/depend:
	cd /home/ciprian/testtools/river && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ciprian/testtools/river /home/ciprian/testtools/river/ipclib /home/ciprian/testtools/river /home/ciprian/testtools/river/ipclib /home/ciprian/testtools/river/ipclib/CMakeFiles/ipc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ipclib/CMakeFiles/ipc.dir/depend


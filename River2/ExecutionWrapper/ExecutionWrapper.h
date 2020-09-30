// ExecutionWrapper.h

#pragma once

using namespace System;
//using namespace System::Collections::Generic;
using namespace System::Collections::ObjectModel;
using namespace System::Runtime::InteropServices;
using namespace System::Threading;

#include "../Execution/Execution.h"
#include "../revtracer/revtracer.h"

namespace ExecutionWrapper {

	public enum class ExecutionState
	{
		NEW = EXECUTION_NEW,
		INITIALIZED = EXECUTION_INITIALIZED,
		SUSPENDED_AT_START = EXECUTION_SUSPENDED_AT_START,
		RUNNING = EXECUTION_RUNNING,
		SUSPENDED = EXECUTION_SUSPENDED,
		SUSPENDED_AT_TERMINATION = EXECUTION_SUSPENDED_AT_TERMINATION,
		TERMINATED = EXECUTION_TERMINATED,
		ERR = EXECUTION_ERR
	};

	public enum class ExecutionControl
	{
		ADVANCE = EXECUTION_ADVANCE,
		BACKTRACK = EXECUTION_BACKTRACK,
		TERMINATE = EXECUTION_TERMINATE
	};

	public value struct VirtualMemorySection {
		uint32_t _BaseAddress;
		uint32_t _RegionSize;
		uint32_t _State; // free, reserved, commited
		uint32_t _Protection; // read, write, execute
		uint32_t _Type; // image, mapped, private

		property uint32_t BaseAddress {
			uint32_t get() { return _BaseAddress; }
		};

		property uint32_t RegionSize {
			uint32_t get() { return _RegionSize; }
		};

		property uint32_t State {
			uint32_t get() { return _State; }
		};

		property uint32_t Protection {
			uint32_t get() { return _Protection; }
		};

		property uint32_t Type {
			uint32_t get() { return _Type; }
		};
	};

	public ref class Execution
	{
	private :
		ExecutionController *impl;

		GCHandle termDelHnd, execBeginDelHnd, execControlDelHnd, execEndDelHnd;

		void TerminateNotifyWrapper();
		ExecutionControl ExecutionBeginWrapper(unsigned int address);
		ExecutionControl ExecutionControlWrapper(unsigned int address);
		ExecutionControl ExecutionEndWrapper();

		ExecutionControl lastControlMessage;
		AutoResetEvent controlEvent;
	public:

		Execution();
		~Execution();

		ExecutionState GetState();

		bool SetPath(String^ path);
		bool SetCmdLine(String^ cmdLine);
		bool Execute();
		bool Terminate();

		void Control(ExecutionControl control);

		bool GetProcessVirtualMemory(ObservableCollection<VirtualMemorySection> ^list);
		bool ReadProcessMemory(unsigned int address, unsigned int size, unsigned char data[]);

		delegate void OnTerminateDelegate();
		delegate void OnExecutionBeginDelegate(unsigned int address);
		delegate void OnExecutionControlDelegate(unsigned int address);
		delegate void OnExecutionEndDelegate();

		OnTerminateDelegate ^onTerminate;
		OnExecutionBeginDelegate ^onExecutionBegin;
		OnExecutionControlDelegate ^onExecutionControl;
		OnExecutionEndDelegate ^onExecutionEnd;
	};
}

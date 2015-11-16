// ExecutionWrapper.h

#pragma once

using namespace System;
//using namespace System::Collections::Generic;
using namespace System::Collections::ObjectModel;

#include "../Execution/Execution.h"

namespace ExecutionWrapper {

	public enum class ExecutionState
	{
		NEW = EXECUTION_NEW,
		INITIALIZED = EXECUTION_INITIALIZED,
		RUNNING = EXECUTION_RUNNING,
		TERMINATED = EXECUTION_TERMINATED,
		ERR = EXECUTION_ERR
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
		
	public:

		Execution();
		~Execution();

		ExecutionState GetState();

		bool SetPath(String^ path);
		bool SetCmdLine(String^ cmdLine);
		bool Execute();
		bool Terminate();

		bool GetProcessVirtualMemory(ObservableCollection<VirtualMemorySection> ^list);
	};
}

// This is the main DLL file.

using namespace System::ComponentModel;
using namespace System::Threading;
//using namespace System::Windows::


#include "stdafx.h"

#include "ExecutionWrapper.h"

void MarshalString(String^ s, wstring& os) {
	using namespace Runtime::InteropServices;
	const wchar_t* chars = (const wchar_t*)(Marshal::StringToHGlobalUni(s)).ToPointer();
	os = chars;
	Marshal::FreeHGlobal(IntPtr((void*)chars));
}

delegate void TermFuncDelegate();
delegate ExecutionWrapper::ExecutionControl ExecBeginDelegate(unsigned int addr);
delegate ExecutionWrapper::ExecutionControl ExecControlDelegate(unsigned int addr);
delegate ExecutionWrapper::ExecutionControl ExecEndDelegate();

ExecutionWrapper::Execution::Execution() : controlEvent(false) {
	impl = NewExecutionController();

	TermFuncDelegate ^termDel = gcnew TermFuncDelegate(this, &ExecutionWrapper::Execution::TerminateNotifyWrapper);
	ExecBeginDelegate ^execBeginDel = gcnew ExecBeginDelegate(this, &ExecutionWrapper::Execution::ExecutionBeginWrapper);
	ExecControlDelegate ^execControlDel = gcnew ExecControlDelegate(this, &ExecutionWrapper::Execution::ExecutionControlWrapper);
	ExecEndDelegate ^execEndDel = gcnew ExecEndDelegate(this, &ExecutionWrapper::Execution::ExecutionEndWrapper);


	termDelHnd = GCHandle::Alloc(termDel);
	execBeginDelHnd = GCHandle::Alloc(execBeginDel);
	execControlDelHnd = GCHandle::Alloc(execControlDel);
	execEndDelHnd = GCHandle::Alloc(execEndDel);

	impl->SetTerminationNotification(
		static_cast<TerminationNotifyFunc>(
			Marshal::GetFunctionPointerForDelegate(termDel).ToPointer()
		)
	);

	impl->SetExecutionBeginNotification(
		static_cast<ExecutionBeginFunc>(
			Marshal::GetFunctionPointerForDelegate(execBeginDel).ToPointer()
		)
	);

	impl->SetExecutionControlNotification(
		static_cast<ExecutionControlFunc>(
			Marshal::GetFunctionPointerForDelegate(execControlDel).ToPointer()
		)
	);

	impl->SetExecutionEndNotification(
		static_cast<ExecutionEndFunc>(
			Marshal::GetFunctionPointerForDelegate(execEndDel).ToPointer()
		)
	);
}

ExecutionWrapper::Execution::~Execution() {
	DeleteExecutionController(impl);
	impl = NULL;

	termDelHnd.Free();
	execBeginDelHnd.Free();
	execControlDelHnd.Free();
	execEndDelHnd.Free();
}

ExecutionWrapper::ExecutionState ExecutionWrapper::Execution::GetState() {
	return (ExecutionWrapper::ExecutionState)impl->GetState();
}

bool ExecutionWrapper::Execution::SetPath(String^ path) {
	wstring wPath;

	MarshalString(path, wPath);
	return impl->SetPath(wPath);
}

bool ExecutionWrapper::Execution::SetCmdLine(String^ cmdLine) {
	wstring wCmdLine;

	MarshalString(cmdLine, wCmdLine);
	return impl->SetCmdLine(wCmdLine);
}

bool ExecutionWrapper::Execution::Execute() {
	return impl->Execute();
}

bool ExecutionWrapper::Execution::Terminate() {
	return impl->Terminate();
}

bool ExecutionWrapper::Execution::GetProcessVirtualMemory(ObservableCollection<VirtualMemorySection> ^list) {
	::VirtualMemorySection *vms;
	int cnt;

	list->Clear();
	if (!impl->GetProcessVirtualMemory(vms, cnt)) {
		return false;
	}

	for (int i = 0; i < cnt; ++i) {
		VirtualMemorySection t;

		t._BaseAddress = vms[i].BaseAddress;
		t._Protection = vms[i].Protection;
		t._RegionSize = vms[i].RegionSize;
		t._State = vms[i].State;
		t._Type = vms[i].Type;

		list->Add(t);
	}
	return true;
}

bool ExecutionWrapper::Execution::ReadProcessMemory(unsigned int address, unsigned int size, unsigned char *data) {
	bool ret = impl->ReadProcessMemory(address, size, data);
	return ret;
}

void ExecutionWrapper::Execution::Control(ExecutionWrapper::ExecutionControl ctrl) {
	lastControlMessage = ctrl;
	controlEvent.Set();
}

void ExecutionWrapper::Execution::TerminateNotifyWrapper() {
	//RaiseEventOnUIThread(onTerminate, gcnew array<Object ^>(0));
	onTerminate->Invoke();
}

ExecutionWrapper::ExecutionControl ExecutionWrapper::Execution::ExecutionBeginWrapper(unsigned int address) {
	onExecutionBegin->Invoke(address);

	controlEvent.WaitOne();
	return lastControlMessage;
}

ExecutionWrapper::ExecutionControl ExecutionWrapper::Execution::ExecutionControlWrapper(unsigned int address) {
	onExecutionControl->Invoke(address);

	controlEvent.WaitOne();
	return lastControlMessage;
}

ExecutionWrapper::ExecutionControl ExecutionWrapper::Execution::ExecutionEndWrapper() {
	onExecutionEnd->Invoke();

	controlEvent.WaitOne();
	return lastControlMessage;
}


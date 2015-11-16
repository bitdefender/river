// This is the main DLL file.

#include "stdafx.h"

#include "ExecutionWrapper.h"

void MarshalString(String^ s, wstring& os) {
	using namespace Runtime::InteropServices;
	const wchar_t* chars = (const wchar_t*)(Marshal::StringToHGlobalUni(s)).ToPointer();
	os = chars;
	Marshal::FreeHGlobal(IntPtr((void*)chars));
}

ExecutionWrapper::Execution::Execution()  {
	impl = NewExecutionController();
}

ExecutionWrapper::Execution::~Execution() {
	DeleteExecutionController(impl);
	impl = NULL;
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

	if (!impl->GetProcessVirtualMemory(vms, cnt)) {
		return false;
	}

	list->Clear();

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

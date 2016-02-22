#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include <nan.h>
#include <node_object_wrap.h>

#include <Windows.h>

#include "../Execution/Execution.h"


using namespace v8;
//using namespace ;


class ExecutionWrapper : public node::ObjectWrap {
private:
    ExecutionController *controller;

    HANDLE hEvent;
    unsigned int result;    
    unsigned int eip;

    uv_async_t termAsync;
    uv_async_t eBeginAsync;
    uv_async_t eControlAsync;
    uv_async_t eEndAsync;


    static Persistent<Function> constructor;

    static void New(const FunctionCallbackInfo<Value> &args);

    static ExecutionWrapper *Factory();

    static void __stdcall TerminationNotify(void *ctx);
    static unsigned int __stdcall ExecutionBegin(void *ctx, unsigned int address);
    static unsigned int __stdcall ExecutionControl(void *ctx, unsigned int address);
    static unsigned int __stdcall ExecutionEnd(void *ctx);

public:
    static void Init(Local<Object> exports);

    static void GetState(const FunctionCallbackInfo<Value>& args);
    static void SetPath(const FunctionCallbackInfo<Value>& args);
    static void SetCmdLine(const FunctionCallbackInfo<Value>& args);
    static void Execute(const FunctionCallbackInfo<Value>& args);
    static void Terminate(const FunctionCallbackInfo<Value>& args);

    static void Control(const FunctionCallbackInfo<Value>& args);

    static void GetProcessVirtualMemory(const FunctionCallbackInfo<Value>& args);
    static void GetModules(const FunctionCallbackInfo<Value>& args);
    static void ReadProcessMemory(const FunctionCallbackInfo<Value>& args);

    static void SetTerminationNotification(const FunctionCallbackInfo<Value>& args);
    static void SetExecutionBeginNotification(const FunctionCallbackInfo<Value>& args);
    static void SetExecutionControlNotification(const FunctionCallbackInfo<Value>& args);
    static void SetExecutionEndNotification(const FunctionCallbackInfo<Value>& args);

    static void GetCurrentRegisters(const FunctionCallbackInfo<Value>& args);

    Persistent<Function> terminationNotification;
    Persistent<Function> executionBeginNotification;
    Persistent<Function> executionControlNotification;
    Persistent<Function> executionEndNotification;

    static void TermAsync(uv_async_t *handle);
    static void EBeginAsync(uv_async_t *handle);
    static void EControlAsync(uv_async_t *handle);
    static void EEndAsync(uv_async_t *handle);
};

ExecutionWrapper *ExecutionWrapper::Factory() {
    ExecutionWrapper *ret = new ExecutionWrapper();
    ret->controller = NewExecutionController();

    ret->controller->SetNotificationContext(ret);

    ret->controller->SetTerminationNotification(TerminationNotify);
    ret->controller->SetExecutionBeginNotification(ExecutionBegin);
    ret->controller->SetExecutionControlNotification(ExecutionControl);
    ret->controller->SetExecutionEndNotification(ExecutionEnd);

    uv_async_init(uv_default_loop(), &ret->termAsync, TermAsync);
    uv_async_init(uv_default_loop(), &ret->eBeginAsync, EBeginAsync);
    uv_async_init(uv_default_loop(), &ret->eControlAsync, EControlAsync);
    uv_async_init(uv_default_loop(), &ret->eEndAsync, EEndAsync);

    ret->termAsync.data = ret;
    ret->eBeginAsync.data = ret;
    ret->eControlAsync.data = ret;
    ret->eEndAsync.data = ret;

    ret->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    return ret;
}

Persistent<Function> ExecutionWrapper::constructor;

void ExecutionWrapper::New(const FunctionCallbackInfo<Value> &args) {
    //HandleScope scope;
    Isolate *isolate = args.GetIsolate();

    if (args.IsConstructCall()) {
        // Invoked as constructor: `new BdCoreObject(...)`
        //Local<String> param = args[0]->IsUndefined() ? "" : args[0]->ToString();
        //char *path = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
        
        /*String::Utf8Value param(args[0]->ToString());
        std::string bdCoreLocation(*param);
        std::string bdPluginLocation = bdCoreLocation + "Plugins" PATH_SEP_STR;
        
        unsigned int thrCount = args[1]->Uint32Value();*/
        
        ExecutionWrapper* obj = Factory();

		/*obj->terminationNotification.Reset(isolate, Undefined(isolate));
		obj->executionBeginNotification.Reset(isolate, Undefined(isolate));
		obj->executionControlNotification.Reset(isolate, Undefined(isolate));
		obj->executionEndNotification.Reset(isolate, Undefined(isolate));*/

        obj->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
        //return args.This();
    } else {
        // Invoked as plain function `BdCoreObject(...)`, turn into construct call.
        //return scope.Close(Undefined());
        args.GetReturnValue().Set(Undefined(isolate));
    }
}


// this gets called on the execution thread
void __stdcall ExecutionWrapper::TerminationNotify(void *ctx) {
    ExecutionWrapper *_this = (ExecutionWrapper *)ctx;

    uv_async_send(&_this->termAsync);
}

// this gets called on the execution thread
unsigned int __stdcall ExecutionWrapper::ExecutionBegin(void *ctx, unsigned int address) {
    ExecutionWrapper *_this = (ExecutionWrapper *)ctx;

    _this->eip = address;
    uv_async_send(&_this->eBeginAsync);

    WaitForSingleObject(_this->hEvent, INFINITE);

    return _this->result;
}

// this gets called on the execution thread
unsigned int __stdcall ExecutionWrapper::ExecutionControl(void *ctx, unsigned int address) {
    ExecutionWrapper *_this = (ExecutionWrapper *)ctx;

    _this->eip = address;
    uv_async_send(&_this->eControlAsync);

    WaitForSingleObject(_this->hEvent, INFINITE);

    return _this->result;
}

// this gets called on the execution thread
unsigned int __stdcall ExecutionWrapper::ExecutionEnd(void *ctx) {
    ExecutionWrapper *_this = (ExecutionWrapper *)ctx;

    uv_async_send(&_this->eEndAsync);

    WaitForSingleObject(_this->hEvent, INFINITE);

    return _this->result;
}

void ExecutionWrapper::Init(Local<Object> exports) {
    Isolate *isolate = exports->GetIsolate();
    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "ExecutionWrapper"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1); // TODO: replace with internal field count
    
    
    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetState",   GetState);
    NODE_SET_PROTOTYPE_METHOD(tpl, "SetPath",    SetPath);
    NODE_SET_PROTOTYPE_METHOD(tpl, "SetCmdLine", SetCmdLine);
    NODE_SET_PROTOTYPE_METHOD(tpl, "Execute",    Execute);
    NODE_SET_PROTOTYPE_METHOD(tpl, "Terminate",  Terminate);

    NODE_SET_PROTOTYPE_METHOD(tpl, "Control",    Control);

    NODE_SET_PROTOTYPE_METHOD(tpl, "GetProcessVirtualMemory", GetProcessVirtualMemory);
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetModules", GetModules);
    NODE_SET_PROTOTYPE_METHOD(tpl, "ReadProcessMemory",       ReadProcessMemory);

    NODE_SET_PROTOTYPE_METHOD(tpl, "SetTerminationNotification",      SetTerminationNotification);
    NODE_SET_PROTOTYPE_METHOD(tpl, "SetExecutionBeginNotification",   SetExecutionBeginNotification);
    NODE_SET_PROTOTYPE_METHOD(tpl, "SetExecutionControlNotification", SetExecutionControlNotification);
    NODE_SET_PROTOTYPE_METHOD(tpl, "SetExecutionEndNotification",     SetExecutionEndNotification);

    NODE_SET_PROTOTYPE_METHOD(tpl, "GetCurrentRegisters",             GetCurrentRegisters);


    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "ExecutionWrapper"), tpl->GetFunction());
}


void ExecutionWrapper::GetState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    args.GetReturnValue().Set(Number::New(isolate, _this->controller->GetState()));
}

void ExecutionWrapper::SetPath(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Value pth(args[0]->ToString());
    wstring path((wchar_t *)*pth);

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    args.GetReturnValue().Set(
        Boolean::New(
            isolate,
            _this->controller->SetPath(
                path
            )
        )
    );
}

void ExecutionWrapper::SetCmdLine(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    String::Value cdl(args[0]->ToString());
    wstring cmdLine((wchar_t *)*cdl);

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    args.GetReturnValue().Set(
        Boolean::New(
            isolate,
            _this->controller->SetCmdLine(
                cmdLine
            )
        )
    );
}

void ExecutionWrapper::Execute(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    args.GetReturnValue().Set(Boolean::New(isolate, _this->controller->Execute()));
}

void ExecutionWrapper::Control(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    
    if (!args[0]->IsUint32()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument")));
        return;
    }

    _this->result = args[0]->Uint32Value();
    SetEvent(_this->hEvent);

    args.GetReturnValue().Set(Undefined(isolate));
}

void ExecutionWrapper::Terminate(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    args.GetReturnValue().Set(Boolean::New(isolate, _this->controller->Terminate()));
}

void ExecutionWrapper::GetProcessVirtualMemory(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());

    VirtualMemorySection *sections = NULL;
    int count;
    if (_this->controller->GetProcessVirtualMemory(sections, count)) {
        Local<Array> arr = Array::New(isolate);

        for (int i = 0; i < count; ++i) {
            Local<Object> obj = Object::New(isolate);

            obj->Set(String::NewFromUtf8(isolate, "baseAddress"), Uint32::New(isolate, sections[i].BaseAddress));
            obj->Set(String::NewFromUtf8(isolate, "regionSize"), Uint32::New(isolate, sections[i].RegionSize));
            obj->Set(String::NewFromUtf8(isolate, "state"), Uint32::New(isolate, sections[i].State));
            obj->Set(String::NewFromUtf8(isolate, "protection"), Uint32::New(isolate, sections[i].Protection));
            obj->Set(String::NewFromUtf8(isolate, "type"), Uint32::New(isolate, sections[i].Type));

            arr->Set(i, obj);
        }

        args.GetReturnValue().Set(arr);
    } else {
        args.GetReturnValue().Set(Undefined(isolate));
    }
}

void ExecutionWrapper::GetModules(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());

    ModuleInfo *modules = NULL;
    int count;
    if (_this->controller->GetModules(modules, count)) {
        Local<Array> arr = Array::New(isolate);

        for (int i = 0; i < count; ++i) {
            Local<Object> obj = Object::New(isolate);

            obj->Set(String::NewFromUtf8(isolate, "moduleBase"), Uint32::New(isolate, modules[i].ModuleBase));
            obj->Set(String::NewFromUtf8(isolate, "size"), Uint32::New(isolate, modules[i].Size));
            obj->Set(String::NewFromUtf8(isolate, "name"), String::NewFromTwoByte(isolate, (uint16_t *)modules[i].Name));

            arr->Set(i, obj);
        }

        args.GetReturnValue().Set(arr);
    } else {
        args.GetReturnValue().Set(Undefined(isolate));
    }
}

void ExecutionWrapper::ReadProcessMemory(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    
    unsigned char *buffer;
    unsigned int base = args[0]->Uint32Value();
    unsigned int size = args[1]->Uint32Value();

    buffer = new unsigned char[size];
    if (_this->controller->ReadProcessMemory(base, size, buffer)) {
        args.GetReturnValue().Set(Nan::CopyBuffer((const char *)buffer, size).ToLocalChecked());
    } else {
        args.GetReturnValue().Set(Undefined(isolate));
    }

    delete [] buffer;
}

void ExecutionWrapper::SetTerminationNotification(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    _this->terminationNotification.Reset(isolate, Local<Function>::Cast(args[0]));

    args.GetReturnValue().Set(Undefined(isolate));
}

void ExecutionWrapper::SetExecutionBeginNotification(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    _this->executionBeginNotification.Reset(isolate, Local<Function>::Cast(args[0]));

    args.GetReturnValue().Set(Undefined(isolate));
}

void ExecutionWrapper::SetExecutionControlNotification(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    _this->executionControlNotification.Reset(isolate, Local<Function>::Cast(args[0]));

    args.GetReturnValue().Set(Undefined(isolate));
}

void ExecutionWrapper::SetExecutionEndNotification(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    _this->executionEndNotification.Reset(isolate, Local<Function>::Cast(args[0]));

    args.GetReturnValue().Set(Undefined(isolate));
}


void ExecutionWrapper::GetCurrentRegisters(const FunctionCallbackInfo<Value>& args) {
    Isolate *isolate = args.GetIsolate();

    ExecutionWrapper *_this = ObjectWrap::Unwrap<ExecutionWrapper>(args.Holder());
    
    Registers regs;
    _this->controller->GetCurrentRegisters(regs);
    
    Local<Object> obj = Object::New(isolate);

    obj->Set(String::NewFromUtf8(isolate, "edi"), Uint32::New(isolate, regs.edi));
    obj->Set(String::NewFromUtf8(isolate, "esi"), Uint32::New(isolate, regs.esi));
    obj->Set(String::NewFromUtf8(isolate, "ebp"), Uint32::New(isolate, regs.ebp));
    obj->Set(String::NewFromUtf8(isolate, "esp"), Uint32::New(isolate, regs.esp));

    obj->Set(String::NewFromUtf8(isolate, "ebx"), Uint32::New(isolate, regs.ebx));
    obj->Set(String::NewFromUtf8(isolate, "edx"), Uint32::New(isolate, regs.edx));
    obj->Set(String::NewFromUtf8(isolate, "ecx"), Uint32::New(isolate, regs.ecx));
    obj->Set(String::NewFromUtf8(isolate, "eax"), Uint32::New(isolate, regs.eax));

    obj->Set(String::NewFromUtf8(isolate, "eflags"), Uint32::New(isolate, regs.eflags));

    args.GetReturnValue().Set(obj);
}

void ExecutionWrapper::TermAsync(uv_async_t *handle) {
    Isolate* isolate = Isolate::GetCurrent();
    ExecutionWrapper *_this = (ExecutionWrapper *)handle->data;

	if (!_this->terminationNotification.IsEmpty()) {
		HandleScope scope(isolate);
		Local<Function> fTerm = Local<Function>::New(isolate, _this->terminationNotification);

    	if (fTerm->IsCallable()) {
    		fTerm->Call(Null(isolate), 0, NULL);
    	}
    }
}

void ExecutionWrapper::EBeginAsync(uv_async_t *handle) { 
    Isolate* isolate = Isolate::GetCurrent();
    ExecutionWrapper *_this = (ExecutionWrapper *)handle->data;

	if (!_this->executionBeginNotification.IsEmpty()) {
		HandleScope scope(isolate);
		Local<Function> eBegin = Local<Function>::New(isolate, _this->executionBeginNotification);
		if (eBegin->IsCallable()) {
            const unsigned argc = 1;
            Local<Value> argv[argc] = { Uint32::New(isolate, _this->eip) };

			Local<Value> ret = eBegin->Call(Null(isolate), argc, argv);

			if (!ret->IsUndefined()) {
				_this->result = ret->Uint32Value();
				SetEvent(_this->hEvent);
			}
		}
	}
}

void ExecutionWrapper::EControlAsync(uv_async_t *handle) { 
    Isolate* isolate = Isolate::GetCurrent();
    ExecutionWrapper *_this = (ExecutionWrapper *)handle->data;

    if (!_this->executionControlNotification.IsEmpty()) {
		HandleScope scope(isolate);
		Local<Function> eControl = Local<Function>::New(isolate, _this->executionControlNotification);

    	if (eControl->IsCallable()) {
            const unsigned argc = 1;
            Local<Value> argv[argc] = { Uint32::New(isolate, _this->eip) };

    		Local<Value> ret = eControl->Call(Null(isolate), argc, argv);

    		if (!ret->IsUndefined()) {
    			_this->result = ret->Uint32Value();
    			SetEvent(_this->hEvent);
    		}
    	}
    }
}

void ExecutionWrapper::EEndAsync(uv_async_t *handle) { 
    Isolate* isolate = Isolate::GetCurrent();
    ExecutionWrapper *_this = (ExecutionWrapper *)handle->data;

    if (!_this->executionEndNotification.IsEmpty()) {
		HandleScope scope(isolate);
		Local<Function> eEnd = Local<Function>::New(isolate, _this->executionEndNotification);
    	
		if (eEnd->IsCallable()) {
    		Local<Value> ret = eEnd->Call(Null(isolate), 0, NULL);

    		if (!ret->IsUndefined()) {
    			_this->result = ret->Uint32Value();
    			SetEvent(_this->hEvent);
    		}
    	}
    }
}

/*=========================================================================================*/

void InitAll(Handle<Object> exports) {
    ExecutionWrapper::Init(exports);
    //exports->Set(String::NewSymbol("Execution"), FunctionTemplate::New(CreateExecution)->GetFunction());
}

NODE_MODULE(executionwrapper, InitAll)




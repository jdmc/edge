#include "owin.h"

NodejsFunc::NodejsFunc(ClrFuncInvokeContext^ appInvokeContext, Handle<Function> function)
{
    this->ClrInvokeContext = appInvokeContext;
    this->Func = new Persistent<Function>;
    *(this->Func) = Persistent<Function>::New(function);
}

NodejsFunc::~NodejsFunc()
{
    if (debugMode)
        System::Console::WriteLine("~NodejsFunc");
    
    this->!NodejsFunc();
}

NodejsFunc::!NodejsFunc()
{
    if (debugMode)
        System::Console::WriteLine("!NodejsFunc");

    this->DisposeFunction();
}

void NodejsFunc::DisposeFunction()
{
    if (this->Func)
    {
        if (debugMode)
            System::Console::WriteLine("DisposeFunction");

        (*(this->Func)).Dispose();
        (*(this->Func)).Clear();
        delete this->Func;
        this->Func = NULL;        
    }
}

Task<System::Object^>^ NodejsFunc::FunctionWrapper(System::Object^ payload)
{
    uv_owin_async_t* uv_owin_async = this->ClrInvokeContext->WaitForUvOwinAsyncFunc();
    NodejsFuncInvokeContext^ context = gcnew NodejsFuncInvokeContext(this, payload);
    uv_owin_async->context = context;
    BOOL ret = PostQueuedCompletionStatus(
        uv_default_loop()->iocp, 
        0, 
        (ULONG_PTR)NULL, 
        &uv_owin_async->uv_async.async_req.overlapped);

    return context->TaskCompletionSource->Task;
}
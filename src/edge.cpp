#include "edge.h"

BOOL debugMode;
Persistent<Function> bufferConstructor;
Persistent<v8::Object> json;
Persistent<Function> jsonParse;

Handle<Value> initializeClrFunc(const v8::Arguments& args)
{
    return ClrFunc::Initialize(args);
}

Handle<Value> callClrFunc(const v8::Arguments& args)
{
    return ClrFunc::Call(args);
}

void init(Handle<Object> target) 
{
    DBG("edge::init");
    bufferConstructor = Persistent<Function>::New(Handle<Function>::Cast(
        Context::GetCurrent()->Global()->Get(String::New("Buffer")))); 
    json = Persistent<v8::Object>::New(Context::GetCurrent()->Global()->Get(String::New("JSON"))->ToObject());
    jsonParse = Persistent<Function>::New(Handle<Function>::Cast(json->Get(String::New("parse"))));
    debugMode = (0 < GetEnvironmentVariable("EDGE_DEBUG", NULL, 0));
    NODE_SET_METHOD(target, "initializeClrFunc", initializeClrFunc);
    NODE_SET_METHOD(target, "callClrFunc", callClrFunc);
}

NODE_MODULE(edge, init);

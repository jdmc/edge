Edge.js: run .NET and node.js code in-process
====

An edge connects two nodes. This edge connects node.js and .NET.

## Before you dive in

See the [Edge.js overview](http://tjanczuk.github.com/edge).

## Introduction 

Edge.js allows you to run .NET and node.js code in one process. You can call .NET functions from node.js and node.js functions from .NET. Edge.js takes care of marshaling data between CLR and V8. Edge.js also reconciles threading models of single threaded V8 and multi-threaded CLR. The .NET code can be pre-compiled or specified as C# source: edge.js can compile C# script at runtime.

![edgejs](https://f.cloud.github.com/assets/822369/266383/e6320302-8df7-11e2-94f3-45f3eca2979f.PNG)

Edge.js provides a basic, prescriptive model and implementation for interoperability between .NET and node.js in-process. You can built upon and extended this basic mechanism to support more specific scenarios, for example:
* implementing express.js handlers and connect middleware for node.js application using .NET 4.5 ([read more](http://tomasz.janczuk.org/2013/02/hosting-net-code-in-nodejs-applications.html)),  
* implementing CPU-bound computations in .NET and running them in-process with node.js application without blocking the event loop ([read more](http://tomasz.janczuk.org/2013/02/cpu-bound-workers-for-nodejs.html)),  
* using C# and .NET instead of writing native node.js extensions in C/C++ and Win32 to access Windows specific functionality from a node.js application ([read more](http://tomasz.janczuk.org/2013/02/access-ms-sql-from-nodejs-application.html)). 

Edge.js is a native node.js module for Windows. It bridges between JavaScript, native, and CLR/.NET code (think C#). The module takes care of marshaling data between V8 and CLR heaps as well as reconciling threading models. The .NET code is running in-process either asynchronously or on CLR threads while the node.js event loop remains unblocked. The .NET code can be integrated into a node.js application as C# script that will be automatically compiled, or as a pre-compiled CLR assembly.

Read more about the background and motivations of the project [here](http://tomasz.janczuk.org/2013/02/hosting-net-code-in-nodejs-applications.html). 

[Follow @tjanczuk](https://twitter.com/tjanczuk) for updates related to the module. 

## What you need

* Windows
* node.js 0.8.x (developed and tested with [v0.8.19](http://nodejs.org/dist/v0.8.19/))  
* [.NET 4.5](http://www.microsoft.com/en-us/download/details.aspx?id=30653)  

Note: later versions of node are also supported, but require edge.js native module to be rebuilt. See building instructions at the bottom.

## How to: hello, world

Install edge:

```
npm install edge
```

In your server.js:

```javascript
var edge = require('edge');

var helloWorld = edge.func('async (input) => { return ".NET Welcomes " + input.ToString(); }');

helloWorld('JavaScript', function (error, result) {
    if (error) throw error;
    console.log(result);
});
```

Run and enjoy:

```
C:\projects\barebones>node server.js
.NET welcomes JavaScript
```

## How to: integrate C# code into node.js code

Edge provies several ways to integrate C# code into a node.js application. Regardless of the way you choose, the entry point into the .NET code is normalized to a `Func<object,Task<object>>` delegate. This allows node.js code to call .NET asynchronoulsy and avoid blocking the node.js event loop. 

Edge provides a function that accepts a reference to C# code in one of the supported representations, and returns a node.js function which acts as a JavaScript proxy to the `Func<object,Task<object>>` .NET delegate:

```javascript
var edge = require('edge');

var myFunction = edge.func(...);
```

The function proxy can then be called from node.js like any asynchronous function:

```javascript
myFunction('Some input', function (error, result) {
    //...
});
```

In one representation, you can embed C# code inside node.js code by providing a C# literal representing a .NET async lambda expression of `Func<object,Task<object>>` type:

```javascript
var add7 = edge.func('async (input) => { return (int)input + 7; }');
``` 

In antoher representation, you can embed multi-line C# source code by providing a function with a body containing a multi-line comment. Edge extracts the C# code from the function body using regular expressions:

```javascript
var add7 = edge.func(function() {/*
    async (input) => {
        return (int)input + 7;
    }
*/});
```

If your C# code is more involved than a simple lambda, you can specify entire class definition. By convention, the class must be named `Startup` and it must have an `Invoke` method that matches the `Func<object,Task<object>>` delegate signature. This method is useful if you need to factor your code into multiple methods:

```javascript
var add7 = edge.func(function() {/*
    using System.Threading.Tasks;

    public class Startup
    {
        public async Task<object> Invoke(object input)
        {
            int v = (int)input;
            return Helper::AddSeven(v);
        }
    }

    static class Helper
    {
        public static int AddSeven(int v) 
        {
            return v + 7;
        }
    }
*/});
```

If your C# code grows substantially, it is useful to keep it in a separate file. You can save it to a file with `*.csx` or `*.cs` extension, and then reference from your node.js application:

```javascript
var add7 = edge.func(__dirname + '/add7.csx');
```

If you integrate C# code into your node.js application by specifying C# source using one of he methods above, edge will compile the code on the fly. If you prefer to pre-compile your C# sources to a CLR assembly, or if your C# component is already pre-compiled, you can reference a CLR assembly from your node.js code. In the most generic form, you can specify the assembly file name, the type name, and the method name when creating a node.js proxy to a .NET method:

```javascript
var clrMethod = edge.func({
    assemblyFile: 'My.Edge.Samples.dll',
    typeName: 'Samples.FooBar.MyType',
    methodName: 'MyMethod'
});
```

If you don't specify methodName, `Invoke` is assumed. If you don't specify typeName, a type name is constucted by assuming the class called `Startup` in the namespace equal to the assembly file name (without the `.dll`). In the example above, if typeName was not specified, it would default to `My.Edge.Samples.Startup`.

The assemblyFile is relative to the working directory. If you want to locate your assembly in a fixed location relative to your node.js application, it is useful to constuct the assemblyFile using `__dirname`. 

You can also create node.js proxies to .NET functions specifying just the assembly name as a parameter:

```javascript
var clrMethod = edge.func('My.Edge.Samples.dll');
```

In that case the default typeName of `My.Edge.Samples.Startup` and methodName of `Invoke` is assumed as explained above. 

## How to: specify additional CLR assembly references

When you provide C# source code and let edge compile it for you at runtime, edge will by default reference only mscorlib.dll and System.dll assemblies. In applications that require additional assemblies you can specify them in C# code using a special comment pattern. For example, to use ADO.NET you must reference System.Data.dll:

```javascript
var add7 = edge.func(function() {/*

    //#r "System.Data.dll"

    using System.Data;
    using System.Threading.Tasks;

    public class Startup
    {
        public async Task<object> Invoke(object input)
        {
            // ...
        }
    }
*/});
```

If you prefer, instead of using comments you can specify references by providing options to the `edge.func` call:

```javascript
var add7 = edge.func({
    csx: function() {/*

        using System.Data;
        using System.Threading.Tasks;

        public class Startup
        {
            public async Task<object> Invoke(object input)
            {
                // ...
            }
        }
    */},
    references: [ 'System.Data.dll' ]
);
```

## How to: marshal data

Edge module can marhshal any JSON-serializable value between .NET and node.js. Edge also supports marshaling between node.js `Buffer` instance and a .NET `byte[]` array to help you efficiently pass binary data.

You can call .NET from node.js and pass in a complex JavaScript object as follows:

```javascript
var dotNetFunction = edge.func('Edge.Sample.dll');

var payload = {
    anInteger: 1,
    aNumber: 3.1415,
    aString: 'foo',
    aBoolean: true,
    aBuffer: new Buffer(10),
    anArray: [ 1, 'foo' ],
    anObject: { a: 'foo', b: 12 }
};

dotNetFunction(payload, function (error, result) { });
```

In .NET, JavaScript objects are represented as `IDictionary<string,object>`, JavaScript arrays as `object[]`, and JavaScript `Buffer` as `byte[]`. Scalar JavaScript values have their corresponding .NET types (`int`, `double`, `bool`, `string`). Here is how you can acces the data in .NET:

```c#
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Edge.Sample
{
    public class Startup
    {
        public async Task<object> Invoke(object input)
        {
            IDictionary<string, object> payload = (IDictionary<string,object>)input;
            int anInteger = (int)payload["anInteger"];
            double aNumber = (double)payload["aNumber"];
            string aString = (string)payload["aString"];
            bool aBoolean = (bool)payload["aBoolean"];
            byte[] aBuffer = (byte[])payload["aBuffer"];
            object[] anArray = (object[])payload["anArray"];
            IDictionary<string, object> anObject = (IDictionary<string,object>)payload["anObject"];

            return null;
        }
    }
}
```

Similar type marshaling is applied when .NET code passes data back to node.js code. In .NET code you can provide an instance of any JSON-serializable CLR type, including domain specific types like `Person` or anonymous objects. For example:

```c#
using System.Threading.Tasks;

namespace Edge.Sample
{
    public class Person
    {
        public int anInteger = 1;
        public double aNumber = 3.1415;
        public string aString = "foo";
        public bool aBoolean = true;
        public byte[] aBuffer = new byte[10];
        public object[] anArray = new object[] { 1, "foo" };
        public object anObject = new { a = "foo", b = 12 };
    }

    public class Startup
    {
        public async Task<object> Invoke(object input)
        {
            Person person = new Person();
            return person;
        }
    }
}
```

In your node.js code that invokes this .NET method you can display the result object that the callback method receives:

```javascript
var edge = require('edge');

var getData = edge.func('Edge.Sample.dll');

getData(null, function (error, result) {
    if (error) throw error;
    console.log(result);
});
```

Passing this .NET object to node.js generates a JavaScript object as follows:

```
C:\projects\barebones>node sample.js
{ anInteger: 1,
  aNumber: 3.1415,
  aString: 'foo',
  aBoolean: true,
  aBuffer: <Buffer 00 00 00 00 00 00 00 00 00 00>,
  anArray: [ 1, 'foo' ],
  anObject: { a: 'foo', b: 12 } }
```

## How to: call node.js from .NET

In addition to marshaling data, edge can marshal proxies to JavaScript functions when invoking .NET code from node.js. This allows .NET code to call back into node.js. 

Suppose the node.js application passes an `add` function to the .NET code as a property of an object. The function receives two numbers and returns the sum of them via the provided callback:

```javascript
var edge = require('edge');

var multiplyBy2 = edge.func('Edge.Sample.dll');

var payload = {
    someParameter: 'arbitrary parameter',
    add: function (data, callback) {
        callback(null, data.a + data.b);
    }
};

multiplyBy2(payload, function (error, result) {
    if (error) throw error;
    console.log(result);
});
```

The .NET code implements the multiplyBy2 function. It generates two numbers, calls back into the `add` function exported from node.js to add them, multiples the result by 2 in .NET, and returns the result back to node.js:

```c#
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Edge.Sample
{
    public class Startup
    {
        public async Task<object> Invoke(object input)
        {
            IDictionary<string, object> payload = (IDictionary<string, object>)input;
            Func<object, Task<object>> add = (Func<object, Task<object>>)payload["add"];
            var twoNumbers = new { a = 2, b = 3 };
            var addResult = (int)await add(twoNumbers);
            return addResult * 2;
        }
    }
}
```

The node.js function exported from node.js to .NET must follow the prescriptive async pattern of accepting two parameters: payload and a callback. The callback function accepts two parametrs. The first one is the error, if any, and the second the result of the operation:

```javasctipt
function (payload, callback) {
    var error;  // must be null or undefined in the absence of error
    var result; 

    // do something

    callback(error, result);
}
```

The proxy to that function in .NET has the following signature:

```c#
Func<object,Task<object>>
```

Usin TPL in CLR to provide a proxy to an asynchronous node.js function allows the .NET code to use the convenience of the `await` keyword when invoking the node.js functionality. The example above shows the use of the `await` keyword when calling the proxy of the node.js `add` method.  

## How to: exceptions

Edge.js marshals node.js errors and exceptions to .NET as well as .NET exceptions to node.js. 

CLR exceptions thrown in .NET code invoked from node.js are marshaled as the `error` parameter to the node.js callback function. Consider this .NET code:

```c#
public Task<object> Invoke(object input)
{
    throw new Exception("Sample .NET exception");
}
```

And the node.js code that invokes this .NET function and re-throws the `error` parameter passed to the JavaScript callback function:

```javascript
var edge = require('edge');

var clrFunc = edge.func('Edge.Sample.dll');

clrFunc(null, function (error, result) {
    if (error) throw error;
});
```

Running this node.js application shows that the CLR exception was indeed received by the node.js callback. The `error` parameter contains the full stack trace including the CLR code path:

```
C:\projects\barebones>node sample.js

c:\projects\edge\lib\edge.js:58
                edge.callClrFunc(appId, data, callback);
                     ^
System.Reflection.TargetInvocationException: Exception has been thrown by the target of an invocation. ---> System.Excep
tion: Sample .NET exception
   at Edge.Sample.Startup.Invoke(Object input) in c:\projects\barebones\sample.cs:line 12
``` 

JavaScript exceptions thrown in node.js code invoked from .NET are wrapped in a CLR exception and cause the asynchronous `Task<object>` to complete with a failure. Errors passed by node.js code invoked from .NET code to the callback function's `error` parameter have the same effect. 

This node.js code invokes a .NET routine and exports the `aFunctionThatThrows` JavaScript function to it:

```javascript
var edge = require('edge.js');
var multiplyBy2 = edge.func('Edge.Sample.dll');

var payload = {
    someParameter: 'arbitrary parameter',
    aFunctionThatThrows: function (data, callback) {
        throw new Error('Sample JavaScript error');
    }
};

multiplyBy2(payload, function (error, result) {
    if (error) throw error;
    console.log(result);
});
```

The .NET code calls the node.js function, catches any resulting CLR exceptions, and displays them:

```c#
public async Task<object> Invoke(object input)
{
    IDictionary<string, object> payload = (IDictionary<string, object>)input;
    Func<object, Task<object>> aFunctionThatThrows = (Func<object, Task<object>>)payload["aFunctionThatThrows"];
    try {
        var aResult = await aFunctionThatThrows(null);
    }
    catch(Exception e)
    {
        Console.WriteLine(e);
    }

    return null;
}
```

Running the code shows the .NET code receiving a CLR exception as a result of the node.js function throwing a JavaScript error. The exception shows the complete stack trace, including the part that executed in the node.js code:

```
C:\projects\barebones>node sample.js
System.Exception: Error: Sample JavaScript error
    at payload.aFunctionThatThrows (C:\projects\barebones\sample.js:7:11)
   at System.Runtime.CompilerServices.TaskAwaiter.ThrowForNonSuccess(Task task)
   at System.Runtime.CompilerServices.TaskAwaiter.HandleNonSuccessAndDebuggerNotification(Task task)
   at Edge.Sample.Startup.<Invoke>d__0.MoveNext()
```

## How to: debugging

You can debug the .NET code running as part of your node.js application by attaching a managed code debugger (e.g. Visual Studio) to node.exe. This method is currently only available if you integrated a pre-compiled CLR assembly with node.js as opposed to embedding C# literals in the application. Since the node.exe process runs both native and managed code, make sure to select the appropriate language to target:

![debug](https://f.cloud.github.com/assets/822369/190564/a41bab2c-7efb-11e2-878f-82ae2325876c.PNG)

## Building

To build the C++\CLI native extension you must have Visual Studio 2012 toolset in place:

```
npm install -g node-gyp
node-gyp configure --msvs_version=2012
node-gyp build -debug
```

The resulting `edge.node` binary must be copied to the `lib\native\win32\{ia32|x64}` directory, depending on the architecture. The `edge.js` expects the native library in that location. You can override this behavior by setting the EDGE_NATIVE environment variable to the fully qualified file name of the edge.node binary. It is useful during development, for example:

```
set EDGE_NATIVE=C:\projects\edge\build\Debug\edge.node
``` 

You can also set the `EDGE_DEBUG` environment variable to 1 to have the edge module generate debug traces to the console when it runs.

## Running tests

You must run tests from a place that has `csc.exe` to VS 2012 tooset on the PATH, for example the VS 2012 developer command prompt. From the root of the project:

```
npm test
```

This first builds a CLR assembly in C# that contains the .NET code of the tests, and then runs the tests with mocha.

## Contribution and derived work

I do welcome contributions via pull request and derived work. 

The edge module is intended to remain a very small component with core functionality that supports interop between .NET and node.js. Domain specific functionality (e.g. access to SQL, writing to ETW, writing connect middleware in .NET) should be implemented as separate modules with a dependency on edge. When you have a notable derived work, I would love to know about it to include a pointer here.  

## More

Issues? Feedback? You [know what to do](https://github.com/tjanczuk/edge/issues/new). Pull requests welcome.

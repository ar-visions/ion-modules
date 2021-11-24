## ion-modules README
[!] Work is on-going at the moment, not all targets are buildable with Vulkan core getting a bit of development\
Welcome. This repository is host to ion modules, a project aimed at being a lean and build-for-everywhere software framework written in C++17.  Simply clone, and make an app at the same directory space as ion-modules.  A package.json and 4 line CMake boilerplate is all you need from there:\
#### client example \
\
```json
{
    "name":     "your-app",
    "version":  "0.22.0"
}
```

```cmake
cmake_minimum_required(VERSION 3.3)
include(../ion-modules/cmake/main.cmake)
read_package("package.json" name u v s)
project(${name})
main()
```

The only thing to change is the name of the app in the json (nothing to modify in CMake)\
C++20 module conversion likely to start taking place as soon as Vulkan code is established.  Where we shine are in general UX facilities.  The framework is driving to be on-par with the latest frameworks such as FireDB and React.  Work is ongoing to establish a Vulkan core for the UX, and after that the tests will build.  As for now we're deep in the trenches of Vulkan abstraction.\
#### client example
One of the targets building fine now are the web client.  The following shows some Future-based Web::json query in action:\
```c++
#include <web/web.hpp>

/// grab a json resource into var form
/// set cwd to res so the web module sees the https trust certs
int main(int argc, const char *argv[]) {
    Args defs = {{ "url", "https://ar-visions.github.io/test-resource.json" }};
    Args args = var::args(argc, argv, defs, "url");
    ///
    Web::json(args["url"]).then([&](Web::Message msg) {
        if (msg.content["wonderful"])
            console.log("expected result");
        ///
    }).except([&](var &err) {
        console.log("request failure: {0}", {err});
    });
    return Async::await();
}
```
\
![alt text](ion-modules.jpg)

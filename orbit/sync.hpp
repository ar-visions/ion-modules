#pragma once
#include <dx/dx.hpp>

/// Station Commands; todo: utilize .gitignore on client
struct StationCommand:ex<StationCommand> {
    enum Type {
        None,       Project,    // project, version
        Ping,       Pong,       // ping and pong have never gone up in smoke, shame
        Dependency,             // module-name, version, uri
        Resource,               // resource-path [content = data]
        Products,               // target:os:arch target2:os:arch
        Delivery,               // general delivery method, just a product name and its contents
        Activate,   Deactivate, // will not be active until the client says-so
        ForceBuild, ForceClean  // everyone knows these, but force imples implicit else-where
    };
    ex_shim(StationCommand, Type, None);
};
#pragma once
#include <web/web.hpp>


/// Station Commands; todo: utilize .gitignore on client
struct Station:ex<Station> {
    enum Type {
        None,       Project,    /// project, version
        Ping,       Pong,       /// ping and pong have never gone up in smoke, shame
        Activity,               /// simple string used for logging; could potentially have a sub-component name or symbol
        Dependency,             /// module-name, version, uri
        Resource,               /// resource-path [content = data]
        Delete,                 /// resource-path
        Products,               /// tells server which to auto-build; target:os:arch target2:os:arch
        Sync,                   /// indication transfer is done, auto-build
        Delivery,               /// general delivery method, just product name, version, platform, arch, bin-contents
        Activate,   Deactivate, /// will not be active until the client says-so
        ForceBuild, ForceClean  /// everyone knows these, but force imples implicit else-where
    };
    
    /// send message
    static bool send(Socket sc, Type type, var content) {
        static std::mutex mx;
        mx.lock();
        Station  sm = Station(type);
        Message msg = Message(sm.symbol(), content);
        bool   sent = msg.write(sc);
        mx.unlock();
        return sent;
    };
    
    /// recv message
    static Message recv(Socket sc) {
        return Message(sc.uri, sc);
    };
    
    ex_shim(Station, Type, None);
};


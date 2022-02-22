#pragma once
#include <web/web.hpp>

/// Station Commands; todo: utilize .gitignore on client
struct Station:ex<Station> {
    enum Type {
        None,       Project,    /// project, version
        Ping,       Pong,       /// ping and pong have never gone up in smoke, shame
        Activity,               /// simple string used for logging; could potentially have a sub-component name or symbol
        Repo,                   /// module-name, version, uri
        Delete,                 /// resource-path
        Products,               /// tells server which to auto-build; target:os:arch target2:os:arch
        
        SyncInbound,            /// transfer of resource described
        SyncPacket,             /// transfer of resource
        SyncComplete,           /// indication transfer is done, auto-build
        
        DeliveryInbound,        /// transfer start indication for a product
        DeliveryPacket,         /// general delivery method, nothing but product path (path relative to products/., not allowed behind it)
        DeliveryComplete,       /// indication that transfer is complete
        
        AutoBuild,              /// this is off until set; part of a startup sequence
        ForceBuild, ForceClean  /// everyone knows these, but force imples implicit else-where
    };
    
    /// send message
    static bool send(Socket sc, Type type, var content) {
        static std::mutex mx;
        mx.lock();
        Station  sm = Station(type); // break here, i want to see whats in content.
        Message msg = Message(sm.symbol(), content);
        bool   sent = msg.write(sc);
        mx.unlock();
        return sent;
    };
    
    /// recv message
    static Message recv(Socket sc) { return Message(sc.uri, sc); };
    
    ex_shim(Station, Type, None);
};


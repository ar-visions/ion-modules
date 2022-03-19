#pragma once
#include <ux/ux.hpp>
#include <ux/interface.hpp>
#include <web/web.hpp>
#include <thread>
#include <mutex>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>


template <class V>
struct DX:Interface {
    async server;
    struct ServerInternal* intern;
    /// ------------------------------------------------
    Message query(Message &m, Map &a, var &p) {
        Message msg;
        str &r  = m.uri.resource;
        auto sp = r.split("/");
        auto sz = sp.size();
        if (sz < 2 || sp[0])
            return { 400, "bad request" };
        
        /// route/to/data#id
        Composer *cc = Composer_init(this, a);
        
        /// instantiate components -- this may be in session cache
        (*cc)(Interface::fn());
        
        node     *n = Composer_root(cc);
        str      id = sp[1];
        for (size_t i = 1; i < sz; i++) {
            n = n->mounts[id];
            if (!n)
                return {404, "not found"};
        }
        return msg;
    }
    /// ------------------------------------------------
    void run() {
        str uri = "https://0.0.0.0:443";
        server = Web::server(uri, [&](Message &m) -> Message {
            Map a;
            var p = null;
            return query(m, a, p);
        });
        code = server.await();
    }
    /// ------------------------------------------------
    template <typename C>
    DX(int c, cchar_t *v[], Map &defaults) {
        type    = Interface::DX;
        fn      = []() -> Element { return C(); };
        Interface::bootstrap(c, v, defaults);
    }
};

#include <ux/composer.hpp>
typedef std::function<bool (node *, node **, var &)> FnUpdate;


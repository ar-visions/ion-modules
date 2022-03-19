#pragma once
#include <dx/async.hpp>

struct SocketInternal;
struct Socket {
    enum Trust {
        RequireCert,
        AllowInsecure,
    };
    enum Role {
        Client,
        Server
    };
    URI uri;
    std::shared_ptr<SocketInternal> intern;
    
                   Socket(std::nullptr_t n = nullptr);
                   Socket(const Socket &ref);
                   Socket(Role role);
                  ~Socket();
    Socket     &operator=(const Socket &ref);
    static Socket connect(URI uri, Trust trust_level = RequireCert);
    static async   listen(URI uri, std::function<void(Socket)> fn);
    static void   logging(void *ctx, int level, cchar_t *file, int line, cchar_t *str);
    bool            write(cchar_t *v, size_t sz, int flags);
    bool            write(str s, array<var> a = {});
    bool            write(array<char> &v);
    bool            write(var &d);
    void      set_timeout(int64_t);
    bool             read(cchar_t *v, size_t sz);
    int          read_raw(cchar_t *v, size_t sz);
    array<char>  read_until(str s, int max_len);
    void            close() const;
    ///
    bool  operator!() const { return !connected; };
    operator   bool() const { return  connected; };
    ///
protected:
    bool connected = false;
};


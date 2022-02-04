#pragma once
#include <dx/dx.hpp>
#include <thread>
#include <mutex>
#include <web/uri.hpp>
#include <web/message.hpp>
#include <web/socket.hpp>

template<> struct is_strable<URI> : std::true_type  {};

struct Web {
protected:
    static str          cookie(var &result);
public:
    static Async         server(URI uri, std::function<Message(Message &)> fn);
    static Future       request(URI uri, Map headers = {});
    static Future          json(URI uri, Map args = {}, Map headers = {});
};

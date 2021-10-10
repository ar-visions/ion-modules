#include <web/web.hpp>
#include <ux/ux.hpp>

int main(int c, const char *v[]) {
    Web::json("https://localhost/wonderful-resource").then([&](Data &res) {
        console.log("message from server: {0}", {res["content"]["message"]});
    });
    return Async::await();
}

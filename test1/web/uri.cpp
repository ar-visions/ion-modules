#include <web/web.hpp>

///
Symbols URI::Method::symbols = {
    { Method::Undefined,  "Undefined"  },
    { Method::Response,   "Response"   },
    { Method::Get,        "GET"        },
    { Method::Post,       "POST"       },
    { Method::Put,        "PUT"        },
    { Method::Delete,     "DELETE"     }
};

URI::Method URI::parse_method(str s) {
    str u = s.to_lower();
    return URI::Method(u);
};

str URI::encode(str s) {
    static ::array<char> chars;
    if (!chars) {
        std::string data = "-._~:/?#[]@!$&'()*+;%=";
        chars = ::array<char>(data.data(), data.size());
    }
    // A-Z, a-z, 0-9, -, ., _, ~, :, /, ?, #, [, ], @, !, $, &, ', (, ), *, +, ,, ;, %, and =
    size_t  len = s.size();
    ::array<char> v = ::array<char>(len * 4 + 1);
    
    for (int i = 0; i < len; i++) {
        cchar_t c = s[i];
        bool a = ((c >= 'A' and c <= 'Z') || (c >= 'a' and c <= 'z') || (c >= '0' and c <= '9'));
        if (!a)
             a = chars.index_of(c) != 0;
        if (!a) {
            v += '%';
            std::stringstream st;
            if (c >= 16)
                st <<        std::hex << int(c);
            else
                st << "0" << std::hex << int(c);
            string n(st.str());
            v += n[0];
            v += n[1];
        } else
            v += c;
        
        if (c == '%')
            v += '%';
    }
    
    return str(v.data(), v.size());
}


str URI::decode(str e) {
    size_t sz = e.size();
    auto    v = ::array<char>(sz * 4 + 1);
    size_t  i = 0;
    
    while (i < sz) {
        cchar_t c0 = e[i];
        if (c0 == '%') {
            if (i >= sz + 1)
                break;
            cchar_t c1 = e[i + 1];
            if (c1 == '%') {
                v += '%';
            } else {
                if (i >= sz + 2)
                    break;
                cchar_t c2 = e[i + 2];
                v += string::char_from_nibs(c1, c2);
            }
        } else
            v += (c0 == '+') ? ' ' : c0;
        i++;
    }
    return str(v.data(), v.size());
}


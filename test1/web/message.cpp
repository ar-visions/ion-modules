#include <web/web.hpp>

///
bool Message::read_headers(Socket sc) {
    int line = 0;
    ///
    for (;;) {
        array<char> rbytes = sc.read_until({"\r\n"}, 8192);
        size_t sz = rbytes.size();
        if (sz == 0)
            return false;
        if (sz == 2)
            break;
        if (line++   == 0) {
            str hello = str(rbytes.data(), sz - 2);
            int  code = 0;
            auto   sp = hello.split(" ");
            if (hello.size() >= 12) {
                if (sp.size()  >= 2)
                    code = sp[1].integer();
            }
            headers[string("Status")]  = code; 
        } else {
            for (size_t i = 0; i < sz; i++) {
                if (rbytes[i] == ':') {
                    str k      = str(&rbytes[0], i);
                    str v      = str(&rbytes[i + 2], sz - k.size() - 2 - 2);
                    headers[k] = var(v);
                    break;
                }
            }
        }
    }
    return true;
}

///
bool Message::read_content(Socket sc) {
    str              te = "Transfer-Encoding";
    str              cl = "Content-Length";
  //cchar_t         *ce = "Content-Encoding";
    int            clen = headers.count(cl) ?  str(headers[cl]).integer() : -1;
    bool        chunked = headers.count(te) && headers[te] == var("chunked");
    ssize_t content_len = clen;
    ssize_t        rlen = 0;
    const ssize_t r_max = 1024;
    bool          error = false;
    int            iter = 0;
    array<uint8_t> v_data;
    ///
    assert(!(clen >= 0 and chunked));
    ///
    if (!(!chunked and clen == 0)) {
        do {
            if (chunked) {
                if (iter++ > 0) {
                    char crlf[2];
                    if (!sc.read(crlf, 2) || memcmp(crlf, "\r\n", 2) != 0) {
                        error = true;
                        break;
                    }
                }
                array<char> rbytes = sc.read_until({"\r\n"}, 64);
                if (!rbytes) {
                    error = true;
                    break;
                }
                std::stringstream ss;
                ss << std::hex << rbytes.data();
                ss >> content_len;
                if (content_len == 0) /// this will effectively drop out of the while loop
                    break;
            }
            bool sff = content_len == -1; /// stawberry fields forever...
            for (ssize_t rcv = 0; sff || rcv < content_len; rcv += rlen) {
                ssize_t   rx = sff ? r_max : std::min(content_len - rcv, r_max);
                uint8_t  buf[r_max];
                rlen = sc.read_raw((cchar_t *)buf, rx);
                if (rlen > 0) {
                    v_data.expand(rlen, 0);
                    memcpy(&v_data[v_data.size() - rlen], buf, rlen);
                } else if (rlen < 0) {
                    error = !sff;
                    /// it is an error if we expected a certain length
                    /// if the connection is closed during a read then
                    /// it just means the peer ended the transfer (hopefully at the end)
                    break;
                }
            }
        } while (!error and chunked and content_len != 0);
    }
    const char *test1 = "adsdsd";

    headers[test1] = var();
    ///
    var   rcv = error ? var(null) : var(v_data);
    str ctype = headers.count("Content-Type") ? str(headers["Content-Type"]) : str("");
    ///
    if (ctype.split(";").count("application/json")) {
        /// read content
        size_t sz = rcv.size();
        str    js = str(rcv.data<const char>(), sz);
        auto  obj = var::parse_json(js);
        content = obj;
    } else if (ctype.starts_with("text/")) {
        content = var(str(rcv.data<cchar_t>(), rcv.size()));
    } else {
        /// other non-text types must be supported, not going to just leave them as array for now
        assert(rcv.size() == 0);
        content = var(null);
    }
    return true;
}

Message::Message(URI &uri, Map &headers, var content): uri(uri), headers(headers), content(content) { }

Message::Message(URI &uri, Socket sc): uri(uri) {
    /// read headers
    read_headers(sc);

    /// read content
    read_content(sc);

    /// set status code
    code = headers["Status"];
}

/// 
bool Message::write_headers(Socket sc) {
    for (auto &[k,v]: headers.map()) {
        if (!sc.write("{0}: {1}", {k, v}))
             return false;
        if (!sc.write("\r\n"))
             return false;
    }
    if (!sc.write("\r\n"))
         return false;
    return true;
}

bool Message::write(Socket sc) {
    if (content) {
        if (!headers.count("Content-Type") && (content == Type::Map || content == Type::Array))
             headers["Content-Type"] =  "application/json";
        ///
        headers["Connection"] = "keep-alive";
        ///
        if (headers["Content-Type"] == var("application/json")) {
            /// Json transfer
            str json = string(content);
            headers["Content-Length"] = string(json.size());
            write_headers(sc);
            return sc.write(json);
        } else if (content == Type::Map) {
            /// Post fields transfer
            str post = URI::encode_fields(content);
            write_headers(sc);
            return sc.write(post);
        } else if (content == Type::Array) {
            /// Binary transfer
            assert(content == Type::Array);
            headers["Content-Length"] = string(content.size());
            return sc.write(content);
        }
        /// if its in var byte stream form, 
        /// we most certainly want to send in octet form, and stream.
        return false;
    }
    headers["Content-Length"] = "0";
    headers["Connection"]     = "keep-alive";
    return write_headers(sc);
}




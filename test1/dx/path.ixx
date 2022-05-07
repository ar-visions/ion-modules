import io;
import flags;
import arr;
import str;
import var;

/// this is local to this module only
#include <sys/types.h>
#include <sys/stat.h>
// this is used with fstat i think
//#include <unistd.h>

export module path;
export {

void chdir(std::string c) {

}

struct dir {
    std::filesystem::path prev;
     dir(std::filesystem::path p): prev(std::filesystem::current_path()) { chdir(p.string().c_str()); }
    ~dir() { chdir(prev.string().c_str()); }
};

///
struct Path {
    typedef lambda<void(Path)> Fn;
    
    enum Flags {
        Recursion,
        NoSymLinks,
        NoHidden,
        UseGitIgnores
    };
    
    path_t p;
    ///
    str               stem() const { return p.stem().string();               }
    static Path        cwd()       { return std::filesystem::current_path(); }
    bool operator==(Path &b) const { return p == b.p;                        } //
    bool operator!=(Path &b) const { return p != b.p;                        } //

    /// act like bash, and be functional
    Path(std::filesystem::directory_entry ent): p(ent.path()) { }
    Path(std::nullptr_t n)  { }
    Path(cchar_t       *p) : p(p) { }
    Path(path_t         p) : p(p) { }
    Path(var            v) : p(v) { }
    Path(str            s) : p(s) { }
    cchar_t *cstr(       ) const  { return p.c_str(); }
    str      ext (       )        { return p.extension().string(); }
    Path     file(       )        { return std::filesystem::is_regular_file(p) ? Path(p) : Path(null); }
    bool     copy(Path to) const  {
        /// no recursive implementation at this point
        assert(is_file() && exists());
        if (!to.exists())
            (to.has_filename() ? to.remove_filename() : to).make_dir();
        std::error_code ec;
        std::filesystem::copy(p, to.is_dir() ? to / p.filename() : to, ec);
        return ec.value() == 0;
    }

    Path link(Path alias) const {
        std::error_code ec;
        std::filesystem::create_symlink(p, alias, ec);
        return alias.exists() ? alias : null;
    }

    bool make_dir() const {
        std::error_code ec;
        return std::filesystem::create_directories(p, ec);
    }

    /// no mutants allowed here.
    Path remove_filename()        { return p.remove_filename(); }
    bool has_filename()     const { return p.has_filename(); }
    Path link()             const { return std::filesystem::is_symlink(p) ? Path(p) : Path(null); }
    operator bool()         const { return p.string().length() > 0; }
    operator path_t()       const { return p; }
    operator std::string()  const { return p.string(); }
    operator str()          const { return p.string(); }
    operator var()          const { return var(path_t(p)); }

    ///
    Path uid(Path base) {
        auto rand = [](size_t i) { return Rand::uniform('a', 'z'); };
        Path path = null;
        do { path = path_t(str::fill(6, rand)); } while ((base / path).exists());
        return path;
    }

    ///
    bool remove_all() const {
        std::error_code ec;
        return std::filesystem::remove_all(p, ec);
    }

    ///
    bool remove() const {
        std::error_code ec;
        return std::filesystem::remove(p, ec);
    }

    ///
    bool is_hidden() const {
        std::string st = p.stem().string();
        return st.length() > 0 && st[0] == '.'; /// obviously a pro
    }

    bool exists()                     const { return  std::filesystem::exists(p); }
    bool is_dir()                     const { return  std::filesystem::is_directory(p); }
    bool is_file()                    const { return !std::filesystem::is_directory(p) && std::filesystem::is_regular_file(p); }
    Path operator / (cchar_t      *s) const { return Path(p / s); }
    Path operator / (const path_t &s) const { return Path(p / s); }
    //Path operator / (const str    &s) { return Path(p / path_t(s)); }
    Path relative(Path from) { return std::filesystem::relative(p, from.p); }

    int64_t modified_at() const {
        struct stat st;
        std::string ps = p;
        lstat(ps.c_str(), &st);
        return int64_t(st.st_ctime);
    }

    bool read(size_t bs, lambda<void(const char *, size_t)> fn) {
        try {
            std::error_code ec;
            size_t rsize = std::filesystem::file_size(p, ec); /// this should be independent of any io caching facilities; it should work down to a second
            if (!ec)
                return false; /// no exceptions
            ///
            std::ifstream f(p);
            char *buf = new char[bs];
            for (int i = 0, n = (rsize / bs) + (rsize % bs != 0); i < n; i++) {
                size_t sz = i == (n - 1) ? rsize - (rsize / bs * bs) : bs;
                fn((const char *)buf, sz);
            }
            delete[] buf;
        } catch (std::ofstream::failure e) {
            console.fault("read failure on resource: {0}", { p });
        }
        return true;
    }

    bool write(array<uint8_t> bytes) {
        try {
            size_t        sz = bytes.size();
            std::ofstream f(p, std::ios::out | std::ios::binary);
            if (sz)
                f.write((const char *)bytes.data(), sz);
        } catch (std::ofstream::failure e) {
            console.fault("read failure on resource: {0}", { p });
        }
        return true;
    }

    bool append(array<uint8_t> bytes) {
        try {
            size_t        sz = bytes.size();
            std::ofstream f(p, std::ios::out | std::ios::binary | std::ios::app);
            if (sz)
                f.write((const char *)bytes.data(), sz);
        } catch (std::ofstream::failure e) {
            console.fault("read failure on resource: {0}", { p });
        }
        return true;
    }

    bool same_as(Path b) const { std::error_code ec; return std::filesystem::equivalent(p, b, ec); }

    void resources(array<str> exts, FlagsOf<Flags> flags, Fn fn) {
        bool use_gitignore = flags[UseGitIgnores];
        bool recursive = flags[Recursion];
        bool no_hidden = flags[NoHidden];
        auto ignore    = flags[UseGitIgnores] ? str::read_file(p / ".gitignore").split("\n") : array<str>();
        lambda<void(Path)> res;
        std::unordered_map<Path, bool> fetched_dir;
        path_t parent = p; /// parent relative to the git-ignore index; there may be multiple of these things.
        ///
        res = [&](Path p) {
            auto fn_filter = [&](Path p) -> bool {
                str      ext = p.ext();
                bool proceed = false;
                /// proceed if the extension is matching one, or no extensions are given
                for (size_t i = 0; !exts || i < exts.size(); i++)
                    if (!exts || exts[i] == ext) {
                        proceed = !no_hidden || !p.is_hidden();
                        break;
                    }
                
                /// proceed if proceeding, and either not using git ignore,
                /// or the file passes the git ignore collection of patterns
                if (proceed && use_gitignore) {
                    Path      rel = p.relative(parent);
                    str      srel = rel;
                    ///
                    for (auto &i: ignore)
                        if (i && srel.has_prefix(i)) {
                            proceed = false;
                            break;
                        }
                }
                
                /// log
                ///str ps = p;
                ///console.log("proceed: {0} = {1}", { ps, proceed }); Path needs io
                
                /// call lambda for resource
                if (proceed) {
                    fn(p);
                    return true;
                }
                return false;
            };
            ///
            if (p.is_dir()) {
                if (!no_hidden || !p.is_hidden())
                    for (Path p: std::filesystem::directory_iterator(p)) {
                        Path link = p.link();
                        if (link)
                            continue;
                        Path pp = link ? link : p;
                        if (recursive && pp.is_dir()) {
                            if (fetched_dir.count(pp) > 0)
                                return;
                            fetched_dir[pp] = true;
                            res(pp);
                        }
                        ///
                        if (pp.is_file())
                            fn_filter(pp);
                    }
            } else if (p.exists())
                fn_filter(p);
        };
        return res(p);
    }

    array<Path> matching(array<str> exts) {
        auto ret = array<Path>();
        resources(exts, {}, [&](Path p) { ret += p; });
        return ret;
    }

    void       resources(array<str> exts, FlagsOf<Flags> flags, Fn fn);


};
/*
namespace std {
    template<> struct hash<Path> {
        size_t operator()(Path const& p) const { return hash<std::string>()(p); }
    };
}
*/
struct SymLink {
    str alias;
    Path path;
};

/// these file ops could effectively be 'watched'
struct Temp {
    Path path;
    ///
    Temp(Path base = "/tmp") {
        path_t p_base = base;
        assert(base.exists());
        Path rand = Path::uid(base);
        path      = base / rand;
        std::filesystem::create_directory(path);
    }
    ///
    ~Temp() { path.remove_all(); }
    ///
    inline void operator += (SymLink sym) {
        if (sym.path.is_dir())
            std::filesystem::create_directory_symlink(sym.path, sym.alias);
        else if (sym.path.exists())
            assert(false);
    }
    ///
    Path operator / (cchar_t   *s) { return path / s; }
    Path operator / (const path_t &s) { return path / s; }
    Path operator / (const str    &s) { return path / path_t(s); }
};





























































struct fmt {
    str s;
    fmt(str s, array<var> a): s(var::format(s, a)) { }
    operator str &() { return s; }
    operator Path () { return s; }
};
///
}
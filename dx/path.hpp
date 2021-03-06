
struct dir {
    std::filesystem::path prev;
     dir(std::filesystem::path p): prev(std::filesystem::current_path()) { chdir(p.string().c_str()); }
    ~dir() { chdir(prev.string().c_str()); }
};

///
struct Path {
    typedef std::function<void(Path)> Fn;
    
    enum Flags {
        Recursion,
        NoSymLinks,
        NoHidden,
        UseGitIgnores
    };
    
    path_t p;
    ///
    Path                (std::filesystem::directory_entry ent);
    Path                (std::nullptr_t n = null);
    Path                (cchar_t *p);
    Path                (path_t p);
    Path                (var v);
    Path                (str s);
    str             stem() const { return p.stem().string(); }
    str              ext();
    cchar_t *       cstr()                  const;
    static Path      cwd() { return std::filesystem::current_path(); }
    Path            file();
    bool            copy(Path path)         const;
    bool    has_filename()                  const;
    Path remove_filename();
    bool        make_dir()                  const;
    Path            link()                  const;
    Path            link(Path alias)        const;
    Path        relative(Path p = null);
    bool operator==(Path &b)                const { return p == b.p; } //
    bool operator!=(Path &b)                const { return p != b.p; } //
    operator        bool()                  const;
    operator      path_t()                  const;
    operator std::string()                  const;
    operator         str()                  const;
    operator         var()                  const;
    int64_t  modified_at()                  const;
    bool         same_as(Path b)            const;
    static Path      uid(Path base);
    bool      remove_all()                  const;
    bool          remove()                  const;
    bool          exists()                  const;
    bool          is_dir()                  const;
    bool         is_file()                  const;
    bool       is_hidden()                  const;
    Path     operator / (cchar_t   *s)      const;
    Path     operator / (const path_t &s)   const;
  //Path     operator / (const str    &s);
    void       resources(array<str> exts, FlagsOf<Flags> flags, Path::Fn fn);
    array<Path> matching(array<str> exts = {});
    bool            read(size_t, std::function<void(const char *, size_t)>);
    bool           write(array<uint8_t>);
    bool          append(array<uint8_t>);
};

namespace std {
    template<> struct hash<Path> {
        size_t operator()(Path const& p) const { return hash<std::string>()(p); }
    };
}

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

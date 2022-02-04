#include <dx/dx.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

Path::Path(std::filesystem::directory_entry ent): p(ent.path()) { }
Path::Path(std::nullptr_t n)        { }
Path::Path(cchar_t *p)    : p(p) { }
Path::Path(path_t p)         : p(p) { }
Path::Path(var v)            : p(v) { }
str  Path::ext ()                   { return p.extension().string(); }
Path Path::file()                   { return std::filesystem::is_regular_file(p) ? Path(p) : Path(null); }
Path Path::link(Path alias) const   {
    std::error_code ec;
    std::filesystem::create_symlink(p, alias, ec);
    return alias.exists() ? alias : null;
}
Path Path::link()             const { return std::filesystem::is_symlink(p)      ? Path(p) : Path(null); }
Path::operator bool()         const { return p.string().length() > 0; }
Path::operator path_t()             { return p; }
Path::operator std::string()  const { return p.string(); }
Path::operator str()          const { return p.string(); }
Path::operator var()          const { return var(path_t(p)); }
///
Path Path::uid(Path base) {
    auto rand = [](size_t i) { return Rand::uniform('a', 'z'); };
    Path path = null;
    do { path = path_t(str::fill(6, rand)); } while ((base / path).exists());
    return path;
}

///
void Path::remove_all() const {
    if (std::filesystem::exists(p))
        std::filesystem::remove_all(p);
}

bool Path::is_hidden() const {
    std::string st = p.stem().string();
    return st.length() > 0 && st[0] == '.';
}

bool Path::exists()                     const { return  std::filesystem::exists(p); }
bool Path::dir()                        const { return  std::filesystem::is_directory(p); }
bool Path::file()                       const { return !std::filesystem::is_directory(p) && std::filesystem::is_regular_file(p); }
Path Path::operator / (cchar_t   *s)    const { return Path(p / s); }
Path Path::operator / (const path_t &s) const { return Path(p / s); }
//Path Path::operator / (const str    &s) { return Path(p / path_t(s)); }
Path Path::relative(Path from) { return std::filesystem::relative(p, from.p); }

int64_t Path::modified_at() const {
    struct stat st;
    std::string ps = p;
    lstat(ps.c_str(), &st);
    return int64_t(st.st_ctime);
}

void Path::resources(array<str> exts, FlagsOf<Flags> flags, Path::Fn fn) {
    bool use_gitignore = flags(UseGitIgnores);
    bool recursive = flags(Recursion);
    bool no_hidden = flags(NoHidden);
    auto ignore    = flags(UseGitIgnores) ? str::read_file(p / ".gitignore").split("\n") : array<str>();
    std::function<void(Path)> res;
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
        if (p.dir()) {
            if (!no_hidden || !p.is_hidden())
                for (Path p: std::filesystem::directory_iterator(p)) {
                    Path link = p.link();
                    if (link)
                        continue;
                    Path pp = link ? link : p;
                    if (recursive && pp.dir()) {
                        if (fetched_dir.count(pp) > 0)
                            return;
                        fetched_dir[pp] = true;
                        res(pp);
                    }
                    ///
                    if (pp.file())
                        fn_filter(pp);
                }
        } else if (p.exists())
            fn_filter(p);
    };
    return res(p);
}

array<Path> Path::matching(array<str> exts) {
    auto ret = array<Path>();
    resources(exts, {}, [&](Path p) { ret += p; });
    return ret;
}


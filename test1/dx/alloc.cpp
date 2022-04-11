export module dx:alloc;
import dx:size;

/// unfortunately the macro header needs to go in each module. punishment.
#include "macros.hpp"

export {

#define BASE_REF    1
#define BASE_INC    1

///
template <class T>
export struct Unsafe {
    T     *ptr;
    Size   count;
    Unsafe(T *ptr, Size count = 1) : ptr(ptr), count(count) {
        assert(count > 0);
    }
};

///
template <class T>
export struct Managed {
    T     *ptr;
    size_t count;
    Managed(T *ptr, Size count = 1) : ptr(ptr), count(count) {
        assert(count > 0);
    }
};

/// Shared is the type data of sh<T>
/// Type contains factory and deletion fn stubs
export struct Shared {
    struct Precious {
        int                        refs;
        void                    *memory;
        Type                       type;
        Size                      count;
        bool                    managed;
        const char                 *tag;
        std::vector<FnVoid> attachments;
        ///
        template <typename T> static Precious *unsafe (T *m, Size c, cchar_t *t = null) { return new Precious { BASE_REF, m, Type(Id<T>()), c, false, t }; }
        template <typename T> static Precious *manage (T *m, Size c, cchar_t *t = null) { return new Precious { BASE_REF, m, Type(Id<T>()), c, true,  t }; }
        ///
        void increment() {
            /// type-based mutex protects this operation without
            /// having too many mutices all up in your area...
            type.lock();
            refs++;
            type.unlock();
        }
        ///
        void decrement() {
            type.lock();
            if (--refs == 0) {
                /// release attachments (deprecate var use-cases)
                for (auto &fn:attachments)
                    fn();
                type.free(memory, count);
                type.unlock();
                delete this;
            } else
                type.unlock();
        }
    } *p = null;
    ///
    template <typename T> inline bool is_diff(T *ptr) { return !p || p->memory != ptr; }
    template <typename T> Shared(Managed<T> m,  cchar_t *tag = null) : p(Precious::manage(m.ptr, m.count, tag)) { }
    template <typename T> Shared(Unsafe <T> u,  cchar_t *tag = null) : p(Precious::unsafe(u.ptr, u.count, tag)) { }
    ///
    inline void decrement()              { if (p) p->decrement(); }
    inline void increment()              { if (p) p->increment(); }
    Shared()                             { }
    Shared(Precious *p)      :p(p)       { }
    Shared(const Shared &ref):p(ref.p)   { if (p) p->increment(); }
   ~Shared()                             { decrement(); }
    inline void attach(FnVoid fn)        { assert(p); p->attachments.push_back(fn); }
    inline void *raw() const             { return p ? p->memory : null; }
    ///
    template <typename T>
    T *pointer() {
        Type  type = Id<T>();
        assert(!p || type == p->type);
        return (T *)raw();
    }
    
    template <typename T>
    T &value() {
        Type  type = Id<T>();
        assert(!p || type == p->type);
        return *(T *)raw();
    }
    
    /// reference Shared instance
    Shared &operator=(Shared ref) {
        if (p != ref.p) {
            decrement();
            p  = ref.p;
            increment();
        }
        return *this;
    }
    ///
    Shared &operator=(nullptr_t n) {
        decrement();
        p = null;
        return *this;
    }
    /// new Shared instance from abstract Unsafe
    template <typename T>
    Shared &operator=(Unsafe<T> u) {
        if (is_diff(u.ptr)) {
            decrement();
            p = Precious::unsafe((T *)u.ptr, u.count);
        }
        return *this;
    }
    /// new Shared instance from abstract Managed
    template <typename T>
    Shared &operator=(Managed<T> m) {
        if (is_diff(m.ptr)) {
            decrement();
            p = Precious::manage(m.ptr, m.count);
        }
        return *this;
    }
    /// implicit 1-count, Managed mode
    template <typename T>
    Shared &operator=(T *ptr) {
        if (is_diff(ptr)) {
            decrement();
            p = ptr ? Precious::manage(ptr, 1) : null;
        }
        return *this;
    }
    /// type-safety on the cast
    template <typename T>
    operator T *() const {
        static Type id = Id<T>();
        if (false)
        if (p && !(id == p->type)) {
            std::string a =      id.name<std::string>();
            std::string b = p->type.name<std::string>();
            ///
            if (a != "void" && b != "void") {
                fprintf(stderr, "type cast mismatch on Shared %s != %s\n", a.c_str(), b.c_str());
                exit(1);
            }
        }
        return (T *)(p ? p->memory : null);
    }
    
    /// casting op
    template <typename T>
    operator             T &() const { return *(operator T *()); }
    operator          void *() const { return p ?  p->memory : null; }
    bool          operator! () const { return p ? !p->type.boolean(p->memory) : true; };
    Size             count  () const { return p ?  p->count : Size(0); }
    Precious *       ident  () const { return p; }
    operator          bool  () const { return raw() != null; }
    Type             &type  () const { static Type t_null; return p ? p->type : t_null; }
    bool operator==(Shared &b) const { return p ? p->type.compare(p->memory, b.p->memory) : (b.p == null); }
    bool operator!=(Shared &b) const { return !(operator==(b)); }
};

template <typename T>
struct sh:Shared {
    sh() { }
    sh(const sh<T> &a) { p = a.p; }
    sh(T &ref)             : Shared(Shared::Precious::manage(&ref, 1)) { }
    sh(T *ptr, Size c = 1) : Shared(Shared::Precious::manage( ptr, c)) { assert(c > 0); }
    ///
    sh<T> &operator=(sh<T> &ref) {
        if (p != ref.p) {
            decrement();
            p = ref.p;
            increment();
        }
        return *this;
    }
    /// useful pattern in breaking up nullables and non-nullables
    sh<T> &operator=(nullptr_t n) {
        decrement();
        p = null;
        return *this;
    }
    ///
    sh<T> &operator=(Unsafe<T> u) {
        if (is_diff(u.ptr)) {
            decrement();
            p = Shared::Precious::unsafe(u.ptr, u.count);
        }
        return *this;
    }
    ///
    sh<T> &operator=(Managed<T> m) {
        if (is_diff(m.ptr)) {
            decrement();
            p = Shared::Precious::manage(m.ptr, m.count);
        }
        return *this;
    }
    /// set by new pointer (this is implicit new shation; same as Shared)
    sh<T> &operator=(T *ptr) {
        if (is_diff(ptr)) {
            decrement();
            p = ptr ? Shared::Precious::manage(ptr, 1) : null;
        }
        return *this;
    }
    ///
    sh<T> &operator=(T &ref) {
        decrement();
        p = Shared::Precious::manage(&ref, 1);
        return *this;
    }
    ///
    operator                T *() const { return  (T *)p->memory; } ///    T* operator
    operator                T &() const { return *(T *)p->memory; } ///    T& operator
    T &              operator *() const { return *(T *)p->memory; } /// deref operator
    T *              operator->() const { return  (T *)p->memory; } /// reference memory operator
    bool                 is_set() const { return p != null; }
    bool operator==(sh<T>     &b) const { return p == b.p;  }
    T   &operator[](size_t index) const {
        assert(index < p->count);
        T *origin  = p->memory;
        T &indexed = origin[index];
        return indexed;
    }
};
}




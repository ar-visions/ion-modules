export module alloc;
import io;

/// unfortunately the macro header needs to go in each module. punishment.
#include "macros.hpp"
import <stdio.h>;

/*
/// Shared is the type data of sh<T>
/// Type contains factory and deletion fn stubs
struct Shared {
    Memory *p;

    ///
    template <typename T> inline bool is_diff(T *ptr) { return !p || p->memory != ptr; }
    template <typename T> Shared(Managed<T> m,  cchar_t *tag = null) : p(Memory::manage(m.ptr, m.count, tag)) { }
    template <typename T> Shared(Arb    <T> a,  cchar_t *tag = null) : p(Memory::arb   (a.ptr, a.count, tag)) { }
    
    ///
    inline void decrement()              { if (p) p->decrement(); }
    inline void increment()              { if (p) p->increment(); }
    Shared()                             { }
    Shared(Memory *p) :p(p)              { }
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

    /// new Shared instance from abstract Arb
    template <typename T>
    Shared &operator=(Arb<T> u) {
        if (is_diff(u.ptr)) {
            decrement();
            p = Memory::arb((T *)u.ptr, u.count);
        }
        return *this;
    }

    /// new Shared instance from abstract Managed
    template <typename T>
    Shared &operator=(Managed<T> m) {
        if (is_diff(m.ptr)) {
            decrement();
            p = Memory::manage(m.ptr, m.count);
        }
        return *this;
    }

    /// implicit 1-count, Managed mode
    template <typename T>
    Shared &operator=(T *ptr) {
        if (is_diff(ptr)) {
            decrement();
            p = ptr ? Memory::manage(ptr, 1) : null;
        }
        return *this;
    }

    /// type-safety on the cast
    template <typename T>
    operator T *() const {
        static Type id = Id<T>();
        if (p && !(id == p->type)) {
			std::string& a = id.name;
            std::string &b = p->type.name;
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
    operator             T &() const { return *(operator T *());                         }
    operator          void *() const { return p ?  p->memory                  : null;    }
    operator          bool  () const { return p ?  p->type.boolean(p->memory) : false;   }
    ssz              count  () const { return p ?  p->count                   : ssz(0);  }
    bool          operator! () const { return !(operator bool());                        }
    Memory *         ident  () const { return p;                                         }
    void *          memory  () const { return p->memory;                                 }
    Type             &type  () const { static Type t_null; return p ? p->type : t_null;  }

    /// call with Type, that would be nice for an inheritance instanceof check.
    bool operator==(Shared &b) const { return p ? p->type.compare(p->memory, b.p->memory) : (b.p == null); }
    bool operator!=(Shared &b) const { return !(operator==(b)); }
};


template <typename T>
struct sh:Shared {
    sh()                                                                      { }
    sh(const sh<T>     &a)                                                    { p = a.p; }
    sh(T             &ref)          : Shared(Memory::manage (&ref, 1))        { }
    sh(T *ptr, ssz  c = 1)          : Shared(Memory::manage ( ptr, c))        { assert(c); }
    sh(ssz  c, lambda<void(T*)> fn) : Shared(Memory::embed  ( malloc(sz), c, Type::lookup<T>())) { }
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
    sh<T> &operator=(Arb<T> u) {
        if (is_diff(u.ptr)) {
            decrement();
            p = Memory::arb(u.ptr, u.count);
        }
        return *this;
    }
    ///
    sh<T> &operator=(Managed<T> m) {
        if (is_diff(m.ptr)) {
            decrement();
            p = Memory::manage(m.ptr, m.count);
        }
        return *this;
    }
    ///
    sh<T> &operator=(T *ptr) {
        if (is_diff(ptr)) {
            decrement();
            p = ptr ? Memory::manage(ptr, 1) : null;
        }
        return *this;
    }
    ///
    sh<T> &operator=(T &ref) {
        decrement();
        p = Memory::manage(&ref, 1);
        return *this;
    }
    ///
    operator                T *() const { return  (T *)p->memory; } ///    T* operator
    operator                T &() const { return *(T *)p->memory; } ///    T& operator
    T &              operator *() const { return *(T *)p->memory; } /// deref operator
    T *              operator->() const { return  (T *)p->memory; } /// reference memory operator
    bool                 is_set() const { return       p != null; }
    bool operator==    (sh<T> &b) const { return       p == b.p;  }
    T   &operator[]    (size_t i) const {
        assert(i < size_t(p->count));
        T *origin  = p->memory;
        T &indexed = origin[i];
        return indexed;
    }
};

}
*/





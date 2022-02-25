#pragma once

#define BASE_REF    1
#define BASE_INC    1

/// Shared is the basis of an Alloc
struct Shared {
    struct Precious {
        int                               refs;
        void                           *memory;
        Type                              type;
        size_t                           count;
        std::function<void(Precious *)> *unref;
        const char                        *tag;
        std::vector<FnVoid>        attachments;
        bool                   bark_if_freeing;
    } *p = null;
    
    typedef std::function<void(Precious *)> FnPrecious;
    
    template <typename T>
    static FnPrecious *get_unref(T *v) { /// we dont need no maps.
        static FnPrecious *fn = new FnPrecious([](Precious *p) {
            if (--p->refs == 0) {

                
                /// delete vector memory or non vector memory based on count
                     if (p->count > 1) delete[] (T *)(p->memory);
                else if (p->count) {
                    if (p->bark_if_freeing) {
                        int test = 0;
                        test++;
                    }
                    delete   (T *)(p->memory);
                }
                
                /// release attachments (deprecate var use-cases)
                for (auto &fn:p->attachments)
                    fn();

                /// delete precious
                delete p;
            }
        });
        return fn;
    }
    
    template <typename T>
    Shared(size_t c, T *m, const char *tag = null):p(m ? new Precious { BASE_REF, m, Id<T>(), c, get_unref<T>(null), tag } : null) { }
    Shared()                              { }
    Shared(Precious *p):p(p)              { } // check when
    Shared(const Shared &ref):p(ref.p)    { if (p) p->refs += BASE_INC; }
   ~Shared()                              { p_unref(); }
    void attach(FnVoid &fn)               { assert(p && p->unref);  p->attachments.push_back(fn); }
    inline void p_unref()                 { if (p && p->unref) (*p->unref)(p); }
    
    ///
    template <typename T>
    T *memory() {
        Type  type = Id<T>();
        assert(type == p->type);
        return (T *)p->memory;
    }
    
    Shared &operator=(Shared ref) {
        if (p != ref.p) {
            p_unref();
            p = ref.p; /// todo: locked by type?
            p->refs += BASE_INC;
        }
        return *this;
    }

    template <typename T>
    Shared &operator=(T *ptr) {
        if (!p || p->memory != ptr) {
            p_unref();
            p = ptr ? new Precious { BASE_REF, ptr, Type(Id<T>()), 1, get_unref<T>(null) } : null;
        }
        return *this;
    }
    
    ///
    template <typename T>
    Shared &operator=(T &ref) {
        /// will need to perform constexpr check, splice out the root type
        p_unref();
        p = new Precious { BASE_REF, new T(ref), Type(Id<T>()), 1, get_unref<T>(null) };
        return *this;
    }

    /// type-safety on the cast
    template <typename T>
    operator T *() {
        static Type id = Id<T>();
        if (p && !(p->type == id)) {
            fprintf(stderr, "type cast mismatch on Shared %s != %s\n",
                    id.name<std::string>().c_str(),
                    p->type.name<std::string>().c_str());
            //exit(1);
        }
        return (T *)(p ? p->memory : null);
    }
    
    operator void *()          const { return p ? p->memory : null; }
    size_t   count()           const { return p ? p->count  : 0; }
    Type     &type()           const { static Type t_null; return p ? p->type : t_null; }
    operator  bool()           const { return p != null; }
    bool operator!()           const { return p == null; }
    bool operator==(Shared &b) const { return p == b.p;  }
    bool operator!=(Shared &b) const { return p != b.p;  }
};
           
/// standard allocy stuff that you shouldnt see
template <typename T>
struct Alloc:Shared {
    Alloc() { }
    Alloc(const Alloc<T> &a) { p = a.p; }
    Alloc(T  &ptr)               : Shared(new Shared::Precious { BASE_REF, new T(ptr), Id<T>(), 1, get_unref<T>(null) }) { }
    Alloc(size_t c, T *p = null) : Shared(new Shared::Precious { BASE_REF, p ? p : (c == 1 ? new T() : new T[c]),
        Id<T>(), c, get_unref<T>(null) }) {
        assert(p || c > 0);
    }
    
    Alloc<T> &operator=(Alloc<T> &ref) {
        if (p != ref.p) {
            p_unref();
            p = ref.p;
            p->refs += BASE_INC;
        }
        return *this;
    }
    
    /// set by new pointer
    Alloc<T> &operator=(T *ptr) {
        if (!p || p->memory != ptr) {
            p_unref();
            p = ptr ? new Precious { BASE_REF, ptr, Type(Id<T>()), 1, get_unref<T>(null) } : null;
        }
        return *this;
    }
    
    /// set by reference (create copy)
    Alloc<T> &operator=(T &ref) {
        p_unref();
        p = new Precious { BASE_REF, new T(ref), Type(Id<T>()), 1, get_unref<T>(null) };
        return *this;
    }
    
    bool is_set()          const { return p != null; }
    bool operator==(Alloc<T> &b) { return p == b.p; }
    
    T &operator[](size_t index) const {
        assert(index < p->count);
        T *origin  = p->memory;
        T &indexed = origin[index];
        return indexed;
    }
    
    operator T &() const {
        if (!p || !p->memory) {
            int test = 0;
            test++;
        }
        return *(T *)p->memory;
    }
    T &operator*() const {
        if (!p || !p->memory) {
            int test = 0;
            test++;
        }
        return *(T *)p->memory;
    }
};

struct Persistent:Shared {
    template <typename T>
    Persistent(const char *tag, T *m):Shared(0, m, tag) { }
};

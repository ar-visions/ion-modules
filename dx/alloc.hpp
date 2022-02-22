#pragma once

/// Shared is the basis of an Alloc
struct Shared {
    struct Precious {
        int            refs;
        void        *memory;
        Type           type;
        size_t        count;
        std::function<void(Precious *)> *unref;
        std::vector<FnVoid> attachments;
    } *p = null;
    
    typedef std::function<void(Precious *)> FnPrecious;
    
    template <typename T>
    static FnPrecious *get_unref(T *v) { /// we dont need no maps.
        static FnPrecious *fn = new FnPrecious([](Precious *p) {
            if (--p->refs == 0) {
                /// delete vector memory or non vector memory based on count
                if (p->count > 1)
                    delete[] (T *)(p->memory);
                else if (p->count)
                    delete   (T *)(p->memory);
                
                /// release attachments (deprecate var use-cases)
                for (auto &fn:p->attachments)
                    fn();
                
                /// delete precious
                delete p;
                //printf("deleting precious: %p\n", p);
            }
        });
        return fn;
    }
    
    template <typename T>
    Shared(size_t count, T *memory):p(new Precious { 1, memory, Id<T>(), count, get_unref<T>(null) }) { }
    Shared()                           { }
    Shared(Precious *p):p(p)           { }
    Shared(const Shared &ref):p(ref.p) { if (p) p->refs++; }
   ~Shared()                           { if (p && p->unref) (*p->unref)(p); }
    void attach(FnVoid &fn)            { assert(p && p->unref);  p->attachments.push_back(fn); }
    
    ///
    template <typename T>
    T *memory() {
        Type  type = Id<T>();
        assert(type == p->type);
        return (T *)p->memory;
    }
    
    Shared &operator=(Shared ref) {
        if (p != ref.p) {
            if (p && (*p->unref)) (*p->unref)(p);
            p = ref.p; /// todo: locked by type?
            p->refs++;
        }
        return *this;
    }
    
    template <typename T>
    Shared &operator=(T *ptr) {
        if (!p || p->memory != ptr) {
            if (p && (*p->unref)) (*p->unref)(p);
            p = new Precious { 1, ptr, Type(Id<T>()), 1, get_unref<T>(null) };
        }
        return *this;
    }
    
    ///
    template <typename T>
    Shared &operator=(T &ref) {
        /// will need to perform constexpr check, splice out the root type
        if (p && *p->unref) (*p->unref)(p);
        p = new Precious { 1, new T(ref), Type(Id<T>()), 1, get_unref<T>(null) };
        return *this;
    }

    /// type-safety on the cast
    template <typename T>
    operator T *() {
        static Type id = Id<T>();
        assert(!p || p->type == id);
        return (T *)(p ? p->memory : null);
    }
    
    operator void *()          const { return p ? p->memory : null; }
    size_t   count()           const { return p ? p->count : 0; }
    Type     &type()           const { static Type t_null; return p ? p->type : t_null; }
    operator  bool()           const { return p != null; }
    bool operator!()           const { return p == null; }
    bool operator==(Shared &b) const { return p == b.p;  }
    bool operator!=(Shared &b) const { return p != b.p;  }
};
           
/// standard allocy stuff that you shouldnt have to see
template <typename T>
struct Alloc:Shared {
    Alloc() { }
    Alloc(const Alloc<T> &a) { p = a.p; }
    Alloc(T  &ptr) : Shared(new Shared::Precious { 1,
        new T(ptr), Id<T>(), 1, get_unref<T>(null) }) { }
    
    Alloc(size_t count, T *ptr = null) : Shared(new Shared::Precious { 1,
        ptr ? ptr : (count == 1 ? new T() : new T[count]),
        Id<T>(), count, get_unref<T>(null) }) {
        assert(ptr || count > 0);
    }
    
    Alloc<T> &operator=(Alloc<T> &ref) {
        if (p != ref.p) {
            if (p && p->unref)
                (*p->unref)(p);
            p = ref.p;
            p->refs++;
        }
        return *this;
    }
    
    /// set by new pointer
    Alloc<T> &operator=(T *ptr) {
        if (!p || p->memory != ptr) {
            if (p && p->unref)
                (*p->unref)(p);
            p = new Precious { 1, ptr, Type(Id<T>()), 1, get_unref<T>(null) };
        }
        return *this;
    }
    
    /// set by reference (create copy)
    Alloc<T> &operator=(T &ref) {
        if (p && p->unref)
            (*p->unref)(p);
        p = new Precious { 1, new T(ref), Type(Id<T>()), 1, get_unref<T>(null) };
        return *this;
    }
    
    //operator T &() { return *(T *)p->memory; }
    
    T &operator[](size_t index) const {
        assert(index < p->count);
        T *origin  = p->memory;
        T &indexed = origin[index];
        return indexed;
    }
};

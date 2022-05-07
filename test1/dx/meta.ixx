export module meta;
import io;
import member;
export {

// interface to Member struct for a given [T]ype
template <typename T>
struct M:Member {
    typedef T value_type;
    
    ///
    std::shared_ptr<T> value; /// Member gives us types, order, serialization and the shared_ptr gives us memory management
    bool sync  = false;
    
    ///
    void operator=(T v) {
        *value = v;
        sync   = false;
    }
    
    /// no need to reserialize when possible
    operator var &() {
        if (!sync) {
            var &r = *serialized;
            r      = var(*value.get());
            sync   = true;
        }
        return *serialized;
    }
    
    /// construct from var
    M(var &v) {
        exit(0);
        value      = std::shared_ptr<T>(new T(v)); // this is a T *, dont get it
        serialized = v.ptr();  // this definitely cant happen
        sync       = true;
    }
    
    /// get reference to value memory (note this is not how we would assign Member from another Member)
    operator T &() {
        assert(value);
        return *value.get();
    }
    
    ///
    T &operator()() {
        assert(value);
        return *value.get();
    }

    ///
    template <typename R, typename A>
    R operator()(A a) { return (*(lambda<R(A)> *)value)(a); }

    ///
    template <typename R, typename A, typename B>
    R operator()(A a, B b) { return (*(lambda<R(A,B)> *)value)(a, b); }
    
    ///
    M<T> &operator=(const M<T> &ref) {
        if (this != &ref) { /// not bound at this point
            value = ref.value;
            //assert(serialized && ref.serialized);
            //*serialized = *ref.serialized; /// not technically needed yet, only when the var is requested
            sync = false;
        }
        return *this;
    }

    /// will likely need specialization of a specific type named VRef or Ref, just var:Ref, thats all
    M<T> &operator=(var &ref) {
        if (this  != &ref) {
            *value = T(ref);
            //*serialized = *ref; /// not technically needed yet, only when the var is requested
            sync = false;
        }
        return *this;
    }
    ///
    M() { value = null; }
   ~M() { }
};

// Lambda. forever.
template <typename F>
struct Lambda:M<lambda<F>> {
    typedef typename lambda<F>::rtype rtype;
    static  const    size_t arg_count = lambda<F>::args;
    rtype            last_result; // Lamba is a cached result using member services; likely need controls on when its invalidated in context
};

///
#define struct_shim(C)\
bool operator==(C &b) { return Meta::operator==(b); }\
C():Meta() { bind_base(false); }\
C(var &ref)  { t = ref.t; c = ref.c; m = ref.m; a = ref.a; bind_base(true); }\
inline C &operator=(var ref) {\
    if (this != &ref) { t = ref.t; c = ref.c; m = ref.m; a = ref.a; }\
    if (!bound)       { bind_base(true); } else { assert(false); }\
    return *this;\
}\
C(const C &ref):C() {\
    ctype = ref.ctype;\
    t = ref.t; c = ref.c; m = ref.m; a = ref.a;\
    bind_base(false);\
    for (size_t i = 0; i < members.sz(); i++)\
        members[i]->copy_fn(members[i], ((::array<Member *> &)ref.members)[i]);\
}

template <typename C>
struct Meta:var {
    Id<C>           ctype;
  ::array<Member *> members;
    bool            bound = false;
    
	Meta() { }
	Meta(const Meta&ref) {
        std::string code_name = ref.ctype.code_name;
        ctype = ref.ctype;
        for (size_t i = 0; i < members.sz(); i++)
            members[i]->copy_fn(members[i], ((::array<Member *> &)ref)[i]);
    }
    
	Meta& operator=(const Meta &ref) {
        if (this != &ref) {
            ctype   = ref.ctype;
            auto &r = (Meta &)ref;
            for (size_t i = 0; i < members.sz(); i++)
                members[i]->copy_fn(members[i], r.members[i]);
        }
        return *this;
    }
    
    ///
    template <typename T>
    void member(cchar_t *name, M<T> &member) {
        if (!member.value)
             member.value = std::shared_ptr<T>(new T());
        ///
        member.copy_fn    = [](Member *dst, Member *src) { *(M<T> *)dst = *(M<T> *)src; };
        member.name       = string(name);
      //array()          += &member; # simpler but less secure and more complicated to endure internal to var, so not.
        members          += &member;
        map()[name]       = var(null);
    }

    ///
    template <typename F>
    void lambda(cchar_t *name, Lambda<F> &lambda, lambda<F> fn) {
        typedef typename Lambda<F>::fun   Lfun;
        typedef typename Lambda<F>::rtype Rtype;
        ///
        if (!lambda.value) {
             lambda.value       = std::shared_ptr<Lfun>(new Lfun(fn));
             lambda.last_result = Rtype(); /// namco music
        }
        ///
        lambda.copy_fn    = [](Member *dst, Member *src) {
            console.fault("illegal method assignment, we dont set these dynamically at runtime by design.");
        };
        /// auto convert the types or be strict about no conversion other than atype[n] to atype[n]. strict is default
        ///
        lambda.name       = string(name);
      //array()          += &member; # simpler but less secure and more complicated to endure internal to var, so not.
      //members          += &lambda; # will need a separate container for introspection
        Fn var_fn = [la=&lambda] (var  &args) {
            Rtype         &r =  la->last_result; /// return memory
            Lfun          &f = *la->value;       /// lambda memory lambda<F>(fn)
            ::array<var>  &a =  args.array();    /// each request should allocate an args to prevent realloc
            ///
            if      constexpr (Lambda<F>::arg_count == 0)
                r = f();
            if      constexpr (Lambda<F>::arg_count == 1)
                r = f(a[0]);
            else if constexpr (Lambda<F>::arg_count == 2)
                r = f(a[0], a[1]);
            else if constexpr (Lambda<F>::arg_count == 3)
                r = f(a[0], a[1], a[2]);
            else if constexpr (Lambda<F>::arg_count == 4)
                r = f(a[0], a[1], a[2], a[3]);
            else if constexpr (Lambda<F>::arg_count == 5)
                r = f(a[0], a[1], a[2], a[3], a[4]);
            else if constexpr (Lambda<F>::arg_count == 6)
                r = f(a[0], a[1], a[2], a[3], a[4], a[5]);
            else if constexpr (Lambda<F>::arg_count == 7)
                r = f(a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
            else
                console.fault("arg count not supported");
        };
        map()[name] = var_fn; /// object holds onto var conversion
    }
    ///
    virtual void bind() { std::cout << "Meta: bind\n"; }
    ///
    void bind_base(bool read_var) {
        bind();
        auto &m = map();
        for (size_t i = 0; i < members.sz(); i++) {
            Member &member    = *members[i];
            member.serialized =  m[member.name].ptr();
        }
        bound = true;
    }
    bool operator==(Meta&b) {
        size_t a_sz =   members.sz();
        size_t b_sz = b.members.sz();
        bool   i_sz = a_sz == b_sz;
        for (size_t i = 0; i_sz && i < a_sz; i++) {
            Member &am = *  members[i];
            Member &bm = *b.members[i];
            if (am.shared != bm.shared)
                return false;
        }
        return i_sz;
    }
    
    virtual ~Meta() { }
};
///
}
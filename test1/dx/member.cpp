export module dx:member;
export {

struct Member;
typedef std::function<void(Member &, var &)>  MFn;
typedef std::function< var(Member &)>         MFnGet;
typedef std::function<bool(Member &)>         MFnBool;
typedef std::function<void(Member &)>         MFnVoid;
typedef std::function<void(Member &, Shared &)> MFnShared;
typedef std::function<void(Member &, void *)> MFnArb;
struct StTrans;

/// todo: move some logic up a level to NMember<T>
/// destroy whats left, in favor of its new matrix [gbsob]
struct Member {
    enum MType { /// multi-planetary enum.
        Undefined,
        Intern,
        Extern,
        Configurable = Intern
    };
    std::function<void(Member *, Member *)> copy_fn;
    
    var &nil() {
        static var n;
        return n;
    }
    
    ///
    var     *serialized = null;
    Type     type       =  Type::Undefined;
    MType    member     = MType::Undefined;
    str      name       = null;
    Shared   shared;    /// having a shared arg might be useful too.
    Shared   arg;       /// simply a node * pass-through for the NMember case.  Shared doesnt have to free it but it can load in some EOL lambda stubs for decentralized resource management
    ///
    size_t   cache      = 0;
    size_t   peer_cache = 0xffffffff;
    Member  *bound_to   = null;
    
    virtual ~Member() { }
    
    ///
    virtual void   *ptr()          { return null; }
    virtual Shared &shared_value() { static Shared s_null; return s_null; }
    virtual bool    state_bool()   {
        assert(false);
        return bool(false);
    }
    /// once per type
    struct Lambdas {
        MFn       var_set;
        MFnGet    var_get;
        MFnBool   get_bool;
        MFnVoid   compute_style;
        MFnShared type_set;
        MFnVoid   unset;
    } *lambdas = null;
    
    static std::unordered_map<Type, Lambdas> lambdas_map;

    ///
    virtual void operator= (const str &s) {
        var v = (str &)s;
        if (lambdas && lambdas->var_set)
            lambdas->var_set(*this, v);
    }
    void         operator= (Member &v);
    virtual bool operator==(Member &m) { return bound_to == &m && peer_cache == m.peer_cache; }

    virtual void style_value_set(void *ptr, StTrans *) { }
};
///
}
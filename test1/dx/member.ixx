export module member;

import io;
import str;
import var;

/// todo: move some logic up a level to NMember<T>
/// destroy whats left, in favor of its new matrix [gbsob]
export struct Member {
    typedef func<void(Member&, var&)>    MFn;
    typedef func< var(Member&)>          MFnGet;
    typedef func<bool(Member&)>          MFnBool;
    typedef func<void(Member&)>          MFnVoid;
    typedef func<void(Member&, Shared&)> MFnShared;
    typedef func<void(Member&, void*)>   MFnArb;

    struct StTrans;

    enum MType {
        Undefined,
        Intern,
        Extern,
        Configurable = Intern
    };
    func<void(Member *, Member *)> copy_fn;
    
    var &nil() {
        static var n = var::static_null();
        /// define static_null, and set a flag on the data for write protection. error on assignment.
        /// useful in a run-away case could lead to confusion and delay.
        return n;
    }
    
    ///
    var     *serialized = null;
    Type     type       = Type::Undefined;
    MType    member     = MType::Undefined;
    str      name       = null;
    Shared   shared;    /// having a shared arg might be useful too.
    Shared   arg;       /// simply a node * pass-through for the NMember case
    
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
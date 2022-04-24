export module io;

#ifndef NDEBUG
import <signal.h>;
#pragma warning(disable:5050)
#endif

#include "macros.hpp"
import   <stdio.h>;

export import std.core;
export import std.threading;
export import std.memory;
export import std.filesystem;

export {

#if   INTPTR_MAX == INT64_MAX
	typedef double       real;
#elif INTPTR_MAX == INT32_MAX
	typedef float        real;
#else
#   error invalid machine arch
#endif

	inline void assert(const bool b) {
#ifndef NDEBUG
		if (!b) raise(11);
#endif
	}

	std::nullptr_t     null = nullptr;
	real               M_PI = 3.141592653589793238462643383279502884L;
	//
	typedef float      r32;
	typedef double     r64;
	typedef const char cchar_t;
	typedef int64_t    isize;
	//
	struct			   TypeOps;
	struct			   Memory;
	struct			   Type;

	template <typename, const bool=false>
	Type *type_of();

	/// bool is non-ortho and gave complications
	struct Bool {
		uint8_t   b;
		Bool(bool b) : b(b) { }
		operator bool()     { return b; }
	};

	/// store internally as posix-style, but freeze posix at a specific place in posix, long long.
	typedef int64_t    ssize_t;
	typedef const char cchar_t;

	enum Stride { Major, Minor };

	/// give us your integral, your null, your signed and unsigned huddled bits.
	struct ssz {
		ssize_t        isz;
		ssz (nullptr_t n = null)  : isz(0)   { }
		ssz (int32_t   isz)       : isz(isz) { }
		ssz (uint32_t  isz)       : isz(isz) { }
		ssz (size_t    isz)       : isz(isz) { }
		ssz (ssize_t   isz)       : isz(isz) { }

		inline ssz& operator++()    { isz++; return *this; }
		inline ssz& operator--()    { isz--; return *this; }

		virtual int    dims() const { return 1;   }
		virtual size_t size() const { return isz; } /// let the ambiguities begin.  size_t = size() use-cases.  so not so ambiguous.

		virtual size_t operator [] (size_t i) { assert(i == 0); return size_t(isz); }
		
		inline operator bool       () { return                  size() > 0;  }
		inline bool     operator!  () { return                  size() <= 0; }
		inline operator ssize_t    () { return                  size();      }
		inline operator int        () { return              int(size());     }
		inline operator size_t     () { return           size_t(size());     }
		inline operator uint32_t   () { return         uint32_t(size());     }
		inline operator std::string() { return   std::to_string(isz);        }
		
		inline bool operator <     (ssz b) { return operator ssize_t() <  ssize_t(b); }
		inline bool operator <=    (ssz b) { return operator ssize_t() <= ssize_t(b); }
		inline bool operator >     (ssz b) { return operator ssize_t() >  ssize_t(b); }
		inline bool operator >=    (ssz b) { return operator ssize_t() >= ssize_t(b); }
		inline bool operator ==    (ssz b) { return operator ssize_t() == ssize_t(b); }
		inline bool operator !=    (ssz b) { return operator ssize_t() != ssize_t(b); }
		inline ssz  operator +     (ssz b) { return operator ssize_t() +  ssize_t(b); }
		inline ssz  operator -     (ssz b) { return operator ssize_t() -  ssize_t(b); }

		size_t last_index() const {
			assert(isz > 0);
			return ssize_t(isz - ssize_t(1));
		}

		template <typename T>
		inline void each(T* data, std::function<void(T&, ssize_t)> fn) {
			for (ssize_t i = 0; i < isz; i++)
				fn(data[i], i);
		}
	};

	bool    operator <  (ssz      lhs, size_t  rhs) { return size_t(ssize_t(lhs)) <  rhs;    }
	bool    operator <  (size_t   lhs, ssz     rhs) { return    lhs <  size_t(ssize_t(rhs)); }
	bool    operator <  (ssize_t  lhs, ssz     rhs) { return    lhs <  ssize_t(rhs);         }
	bool    operator <= (ssize_t  lhs, ssz     rhs) { return    lhs <= ssize_t(rhs);         }
	bool    operator >  (ssize_t  lhs, ssz     rhs) { return    lhs >  ssize_t(rhs);         }
	bool    operator >= (ssize_t  lhs, ssz     rhs) { return    lhs >= ssize_t(rhs);         }
	bool    operator == (ssize_t  lhs, ssz     rhs) { return    lhs == ssize_t(rhs);         }
	bool    operator != (ssize_t  lhs, ssz     rhs) { return    lhs != ssize_t(rhs);         }
	bool    operator <  (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) <  rhs;          }
	bool    operator <= (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) <= rhs;          }
	bool    operator >  (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) >  rhs;          }
	bool    operator >= (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) >= rhs;          }
	bool    operator == (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) == rhs;          }
	bool    operator != (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) != rhs;          }
	ssize_t operator -  (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) - rhs;           }
	ssize_t operator +  (ssz      lhs, ssize_t rhs) { return   ssize_t(lhs) + rhs;           }
	ssize_t operator -  (ssize_t lhs,  ssz     rhs) { return           lhs - ssize_t(rhs);   }
	ssize_t operator +  (ssize_t lhs,  ssz     rhs) { return           lhs + ssize_t(rhs);   }
	size_t  operator /  (ssz     lhs, ssize_t  rhs) { return   ssize_t(lhs) / rhs;           }
	size_t  operator *  (ssz     lhs, ssize_t  rhs) { return   ssize_t(lhs) * rhs;           }
	ssize_t operator /  (ssize_t lhs, ssz      rhs) { return           lhs / ssize_t(rhs);   }
	ssize_t operator *  (ssize_t lhs, ssz      rhs) { return           lhs * ssize_t(rhs);   }


	template <class U, class T>
	struct can_convert {
		enum { value = std::is_constructible<T, U>::value && !std::is_convertible<U, T>::value };
	};

	struct Type {
		enum Specifier {
			Undefined,
			i8,  ui8,
			i16, ui16,
			i32, ui32,
			i64, ui64,
			f32,  f64,
			Bool,
			Str, Map, Array, Ref, Arb, Node, Lambda, Member, Any,
			Meta
		};

		std::string name;
		std::mutex  mx;
		size_t      id   = 0;
		Type*       refs = null;
		TypeOps*    ops; // function table of operations

		void     push()                          {   mx.lock  ();              }
		void     pop ()                          {   mx.unlock();              }
		//
		bool operator==(const     Type& b) const { return id == b.id;          }
		bool operator==(          Type& b) const { return id == b.id;          }
		bool operator==(Type::Specifier b) const { return id == size_t(b);     }
		//
		bool operator!=(const     Type& b) const { return id != b.id;          }
		bool operator!=(          Type& b) const { return id != b.id;          }
		bool operator!=(Type::Specifier b) const { return id != size_t(b);     }
		//
		bool operator>=(Type::Specifier b) const { return id >= size_t(b);     }
		bool operator<=(Type::Specifier b) const { return id <= size_t(b);     }
		bool operator> (Type::Specifier b) const { return id >  size_t(b);     }
		bool operator< (Type::Specifier b) const { return id <  size_t(b);     }
		//
		inline operator size_t()           const { return     id;              }
		inline operator int()              const { return int(id);             }
		inline operator bool()             const { return     id > 0;          }

		static size_t size(Specifier t) {
			switch (t) {
			case Bool:  return 1;
			case   i8:  return 1;
			case  ui8:  return 1;
			case  i16:  return 2;
			case ui16:  return 2;
			case  i32:  return 4;
			case ui32:  return 4;
			case  i64:  return 8;
			case ui64:  return 8;
			case  f32:  return 4;
			case  f64:  return 8;
			default:
				break;
			}
			return 0;
		}

		size_t  sz()                    const { return size(Specifier(id)); }

	protected:
		const char* specifier_name(size_t id) { /// allow runtime augmentation of this when useful
			static std::unordered_map<size_t, const char*> map2 = {
				{ Undefined, "Undefined"    },
				{ i8,        "i8"           }, {   ui8, "ui8"      },
				{ i16,       "i16"          }, {  ui16, "ui16"     },
				{ i32,       "i32"          }, {  ui32, "ui32"     },
				{ i64,       "i64"          }, {  ui64, "ui64"     },
				{ f32,       "f32"          }, {   f64, "f64"      },
				{ Bool,      "Bool"         }, {   Str, "Str"      },
				{ Map,       "Map"          }, { Array, "Array"    },
				{ Ref,       "Ref"          }, {  Node, "Node"     },
				{ Lambda,    "Lambda"       }, {   Any, "Any"      },
				{ Meta,      "Meta"         }
			};
			assert(map2.count(id));
			return map2[id];
		}
	};

	namespace std {
		template<> struct hash<Type> { size_t operator()(Type const& id) const { return id; } };
	}

	struct Data {
		enum AllocType { Arb, Manage, Embed };
		void*     ptr;
	  //ssz       index; -- not really seeing the point for now. its a syntactic trip wire too [/tucks in lip]
		ssz       count;
		Type*     type;
		AllocType alloc;
	};

	struct  Memory;

	void    drop        (Memory*);
	void    grab        (Memory*);
	void    type_free   (Memory*);
	Memory* type_alloc  (Type*, size_t, Data::AllocType, void*);
	bool    type_bool   (Memory*);
	int     type_compare(Memory*, Memory*);
	
    // makes a bit of sense to declare the memory structure first
    struct Memory {
		Data				data;
        int                 refs;
        int                n_att;
        void*                att[4];
		
		// these are called internally only, no need to pass Type if you know it already.
		// here we have it assembled in this fashion due to dependency order.
		template <typename T> static Memory*    arb(T* ptr, ssz count, Type* type) {
			return type_alloc(type, size_t(count), Data::AllocType::Arb,    (void*)ptr);
		}

        template <typename T> static Memory* manage(T* ptr, ssz count, Type* type) {
			return type_alloc(type, size_t(count), Data::AllocType::Manage, (void*)ptr);
		}

        template <typename T> static Memory* embed(T* ptr, ssz count, Type* type) {
			return type_alloc(type, size_t(count), Data::AllocType::Embed,  (void *)ptr);
        }

		template <typename T> inline bool is_diff(T *p) { return data.ptr != p; }
	};

	struct Shared {
		Memory* mem;

		Shared() { }
		//Shared(Memory* mem) : mem(mem) { } 
	   ~Shared() { drop(mem); }
		Shared(const Shared& r) : mem(r.mem) { grab(mem); }

		void  attach(void* fn); // type: FnVoid *
		void* raw() const { return mem ? mem->data.ptr : null; }

		template <typename T>
		T* pointer();

		template <typename T>
		T& value() { return *(T*)pointer(); }

		/// reference Shared instance
		Shared& operator=(Shared r) {
			if (mem != r.mem) {
				drop(mem);
				mem = r.mem;
				grab(mem);
			}
			return *this;
		}

		Shared& operator=(nullptr_t n) {
			drop(mem);
			mem = null;
			return *this;
		}

		/// implicit 1-count, Managed mode
		template <typename T>
		Shared& operator=(T* ptr);

		/// type-safety on the cast
		template <typename T>
		operator T* () const {
			static Type id = Id<T>();
			if (mem && !(id == mem->data.type)) {
				std::string& a = id.name;
				std::string& b = mem->data.type.name;
				///
				if (a != "void" && b != "void") {
					printf("type cast mismatch on Shared %s != %s\n", a.c_str(), b.c_str());
					exit(1);
				}
			}
			return (T*)(mem ? mem->data.ptr : null);
		}

		operator   bool() const { return type_bool(mem);                      }
		bool operator! () const { return !(operator bool());                  }

		template <typename T>
		operator    T& () const { return *(operator T * ());                  }
		operator void* () const { return mem ? mem->data.ptr : null;          }

		// runtime inheritance check is definitely possible but just not useful.  seriously how dumb do you have to be [/walks off into a closet]
		template <typename T>
		T&         cast() const { return *(T*)mem->data.ptr;                  }

		ssz       count() const { return mem ? mem->data.count : ssz (0);     }
		Memory*   ident() const { return mem;                                 }
		void*   pointer() const { return mem->data.ptr;                       }
		Type*	   type() const { return mem ? mem->data.type : null;         }

		/// call with Type, that would be nice for an inheritance instanceof check.
		bool operator==(Shared& b) const { return mem ? (type_compare(mem, b.mem) == 0) : (b.mem == null); } 
		bool operator!=(Shared& b) const { return !(operator==(b)); }
	};

	template <typename _ZZion>
	Type* type_of();

	template <typename T>
	struct func;

	template <typename R, typename... A>
	struct func<R(A...)>:Shared {
	protected:
		struct Lambda {
			typedef R(*Signature) (A...);
			Shared sf;

			Lambda() { }

			template <typename F>
			Lambda(F& f) : sf(sh<F>(sizeof(F), [](F* raw) { new (raw) F(f); })) { }

			R invoke(A... a) { return Signature(sf.pointer())(std::forward<A>(a)...); }
			operator bool() { return sf; }
		};
	public:
		typedef R rtype;

		func<R(A...)>& operator=(func<R(A...)> &r) {
			if (mem != r.mem) {
				drop(mem);
				mem = r.mem;
				grab(mem);
			}
			return *this;
		}

		func<R(A...)>& operator=(nullptr_t n) {
			drop(mem);
			mem = null;
			return *this;
		}

		// the alloc just wont work on the lambda, there is no way to detect the type.
		// so we are making an explicit case here for lambda copy-construct
		template <typename F>
		func(F f) {
			/// unfortunately have to pass in Lambda param as template, because its impossible to detect Lambda type by type alone. 
			mem = type_copy_construct(type_of<F, true>(), Data::AllocType::Embed, (void*)&f);
		}

		func(const func& f) {
			if (f.mem) {
				mem = f.mem;
				grab((Memory*)mem);
			}
		}

		func(std::nullptr_t n = null) { }
	   ~func() { }
	    ///
		operator  bool() { return  bool(mem); }
		bool operator!() { return !bool(mem); }

		static const size_t args = sizeof...(A);

		R operator()(A... a) { return cast<Lambda>().invoke(std::forward<A>(a)...); }

		template <size_t i>
		struct    arg { typedef typename std::tuple_element<i, std::tuple<A...>>::type type; };

		static Type lambda_type() {
			static Type ftype = Id<func<R(A...)>>();
			return ftype;
		};

		static Type result_type() {
			static Type rtype = Id<R>();
			return rtype;
		};
	};


	template<typename>   struct is_func          : std::false_type {};
	template<typename T> struct is_func<func<T>> : std::true_type  {};

	template <typename T>
	struct sh :Shared {
		sh() { }
		sh(const sh<T>& a) : mem(a.mem) { }
		sh(T& ref) : Shared(Memory::manage(&ref, 1)) { }
		sh(T* ptr, ssz c = 1) : Shared(Memory::manage(ptr, c)) { assert(c); }

		//sh(ssz c, lambda<void(T*)> fn) : Shared(Memory::embed(malloc(sz), c, Type::lookup<T>())) { }
		///
		sh<T>& operator=(sh<T>& ref) {
			if (mem != ref.mem) {
				drop(mem);
				mem = ref.mem;
				grab(mem);
			}
			return *this;
		}
		/// useful pattern in breaking up nullables and non-nullables
		sh<T>& operator=(nullptr_t n) {
			drop(mem);
			mem = null;
			return *this;
		}
		///
		sh<T>& operator=(T* ptr) {
			if (is_diff(ptr)) {
				drop(mem);
				mem = ptr ? Memory::manage(ptr, 1) : null;
			}
			return *this;
		}
		///
		sh<T>& operator=(T& ref) {
			drop(mem);
			mem = Memory::manage(&ref, 1);
			return *this;
		}
		///
		operator                T* () const { return  (T*)mem->data.ptr; } ///    T* operator
		operator                T& () const { return *(T*)mem->data.ptr; } ///    T& operator
		T&              operator  *() const { return *(T*)mem->data.ptr; } /// deref operator
		T*              operator ->() const { return  (T*)mem->data.ptr; } /// reference memory operator
		bool                 is_set() const { return      mem != null;   }
		bool operator==    (sh<T>& b) const { return      mem == b.mem;  }
		T&   operator[]    (size_t i) const {
			assert(i < size_t(mem->data.count));
			T*  origin = mem->data.ptr;
			T& indexed = origin[i];
			return indexed;
		}
	};


	struct node;
	struct string;
	struct var;

	typedef func<bool()>                FnBool;
	typedef func<void(void*)>           FnArb;
	typedef func<void(void)>            FnVoid;
	typedef func<void(var&)>            Fn;
	typedef func<void(var&, node*)>     FnNode;
	typedef func<var()>                 FnGet;
	typedef func<void(var&, string&)>   FnEach;
	typedef func<void(var&, size_t)>    FnArrayEach;


	///
	void grab(Memory *m) {
		Type* t = m->data.type;
		t->push();
		m->refs++;
		t->pop();
	}

	///
	void drop(Memory* m) {
		Type* t = m->data.type;
		t->push();
		if (--m->refs == 0) {
			/// release attachments (deprecate var use-cases)
			for (int i = 0; i < m->n_att; i++)
				((FnVoid *)m->att)[i]();

			type_free(m);
			t->pop();
			delete m;
		} else
			t->pop();
	}

	struct TypeOps {
		func <void    (Memory*)>                               fn_free;
		func <bool    (void*)>                                 fn_bool;
		func <Memory* (Type*, size_t, Data::AllocType, void*)> fn_alloc; // can handle a copy-construct for non-lambda case
		func <Memory* (Type*, Data::AllocType, void*)>		   fn_copy_construct;
		func <int     (void*, void*)>						   fn_compare;

		template <typename T>
		static constexpr Type::Specifier spec() {
			static T na;
			if      constexpr (std::is_same_v<T, Fn>)       return Type::Lambda;
			else if constexpr (std::is_same_v<T, bool>)     return Type::Bool;
			else if constexpr (std::is_same_v<T, int8_t  >) return Type::  i8;
			else if constexpr (std::is_same_v<T, uint8_t >) return Type:: ui8;
			else if constexpr (std::is_same_v<T, int16_t >) return Type:: i16;
			else if constexpr (std::is_same_v<T, uint16_t>) return Type::ui16;
			else if constexpr (std::is_same_v<T, int32_t >) return Type:: i32;
			else if constexpr (std::is_same_v<T, uint32_t>) return Type::ui32;
			else if constexpr (std::is_same_v<T, int64_t >) return Type:: i64;
			else if constexpr (std::is_same_v<T, uint64_t>) return Type::ui64;
			else if constexpr (std::is_same_v<T, float   >) return Type:: f32;
			else if constexpr (std::is_same_v<T, double  >) return Type:: f64;
			else if constexpr (is_map<T>())                 return Type:: Map;
			return Type::Undefined;
		}

		static size_t size(Type::Specifier t) {
			switch (t) {
			case Type::Specifier::Bool:  return 1;
			case Type::Specifier::  i8:  return 1;
			case Type::Specifier:: ui8:  return 1;
			case Type::Specifier:: i16:  return 2;
			case Type::Specifier::ui16:  return 2;
			case Type::Specifier:: i32:  return 4;
			case Type::Specifier::ui32:  return 4;
			case Type::Specifier:: i64:  return 8;
			case Type::Specifier::ui64:  return 8;
			case Type::Specifier:: f32:  return 4;
			case Type::Specifier:: f64:  return 8;
			default:
				break;
			}
			return 0;
		}
	};


    template<class T, class E>
    struct _has_equal_to {
        template<class U, class V>   static auto eq(U*)  -> decltype(std::declval<U>() == std::declval<V>());
        template<typename, typename> static auto eq(...) -> std::false_type;
		///
        using imps = typename std::is_same<bool, decltype(eq<T, E>(0))>::type;
    };


    template<class T, class E = T>
    struct has_equal_to : _has_equal_to<T, E>::imps {};


    #if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
        /// this needs to likely parse different via msvc (todo)
        #define __PRETTY_FUNCTION__ __FUNCSIG__
    #endif

    template <typename _ZZion, const bool isLambda>
    Type * type_of() {
        typedef _ZZion T;
        static Type type;
        if (type.name.empty()) {
			type.ops = new TypeOps {

				///
				.fn_free = [](Memory* mem) -> void {
					Data& data = mem->data;
					if (data.alloc != Data::AllocType::Arb) {
						if (data.alloc == Data::AllocType::Embed)
							((T*)data.ptr)->~T();
						else {
							if (data.count > ssz(1))
								delete[](T*)(data.ptr);
							else if (data.count)
								delete   (T*)(data.ptr);
						}
					}
				},

				///
				.fn_bool = [](void *va) -> bool {
					if constexpr (can_convert<T, bool>::value) {
						static T s_null;
						T &a = *(va ? (T *)va : (T *)&s_null);
						return bool(a);
					}
					return true;
				},

				/// general purpose allocator for Type of T[count]
				/// the arb still has to allocate a Memory container
				/// recycle the container
				.fn_alloc = [](Type *type, size_t count, Data::AllocType alloc_type, void *src) -> Memory* {
					Memory* mem = null;
					if constexpr (!isLambda) {
						assert(size_t(count) >= 1);
						//
						switch (alloc_type) {
						case Data::AllocType::Embed:
							/// allocate memory Shared + Tc
							mem				= (Memory*)calloc(1, sizeof(Memory) + sizeof(T) * size_t(count));
							mem->refs		= 1;
							mem->data.ptr   = &mem[1];
							mem->data.type  = type;
							mem->data.count = count;
							mem->data.alloc = Data::AllocType::Embed;

							/// run constructor for Memory
							new ((Memory*)mem) Memory();

							for (size_t i = 0; i < size_t(count); i++)
								new (((T*)mem->data.ptr)[i]) T();

							break;
						case Data::AllocType::Manage:
							if constexpr (!isLambda) {
								mem = (Memory*)calloc(1, sizeof(Memory)); 
								mem->refs       = 1;
								mem->data.ptr   = &mem[1];
								mem->data.type  = type;
								mem->data.count = count;
								mem->data.alloc = Data::AllocType::Embed;
								mem->data.ptr   = (void*)new T();
							}
							break;
						case Data::AllocType::Arb:
							break;
						}
					}
					return mem;
				},

				.fn_copy_construct = [](Type* type, Data::AllocType alloc_type, void* src) -> Memory* {
					Memory* mem = null;
					if constexpr (isLambda) {
						mem = (Memory*)calloc(1, sizeof(Memory) + sizeof(T));
						mem->refs = 1;
						mem->data.ptr = &mem[1];
						mem->data.type = type;
						mem->data.count = 1;
						mem->data.alloc = Data::AllocType::Embed;
						new ((Memory*)mem) Memory();
						new ((T*)mem->data.ptr) T(*(T*)src);
					}
					return mem;
				},

				///
				.fn_compare = [](void *va, void *vb) -> int {
					if constexpr (is_func<T>() || has_equal_to<T>::value) {
						static T s_null;
						T &a = *(va ? (T *)va : (T *)&s_null);
						T &b = *(vb ? (T *)vb : (T *)&s_null);
						if constexpr (is_func<T>())
							return int(fn_id((T &)a) == fn_id((T &)b));
						else
							return int(a == b);
					}
					return va == vb;
				}
			};

			size_t       b, l;
			std::string  cn   = __PRETTY_FUNCTION__;
			const char   s[]  = "_ZZion =";
			const size_t blen = sizeof(s);
            b                 = cn.find(s) + blen;
            l                 = cn.find("]", b) - b;
			type.name         = cn.substr(b, l);
        }
        return &type;
    };


	template <class T> struct Arb    : Shared {
		Arb    (T* p, size_t c = 1)  : mem(Memory::arb   (p, c, type_of<T>())) { }
	};
	
	template <class T> struct Manage : Shared {
		Manage (T* p, size_t c = 1)  : mem(Memory::manage(p, c, type_of<T>())) { }
	};

	template <class T> struct Embed  : Shared {
		Embed  (T* p, size_t c = 1)  : mem(Memory::embed (p, c, type_of<T>())) { }
	};

	// not doing this Id ref thing. i definitely dont like it. and i think any uses of Id<> we just convert, its easy [todo]
	//template <class T> struct Id     : Type   { Id() : refs(type_of<T>()) { } };

	template <typename T>
	T fabs(T x) {
		return x < 0 ? -x : x;
	}

	void type_free(Memory* mem) {
		mem->data.type->ops->fn_free(mem);
	}

	bool type_bool(Memory* mem) {
		return mem->data.type->ops->fn_bool(mem->data.ptr);
	}

	int type_compare(Memory* a, Memory* b) {
		return a ? a->data.type->ops->fn_compare(a->data.ptr, b->data.ptr) : false;
	}

	Memory *type_alloc(Type* type, size_t count, Data::AllocType alloc_type, void *src) {
		return type->ops->fn_alloc(type, count, alloc_type, src);
	}

	/// this is used for lambda copy-construct
	Memory* type_copy_construct(Type* type, Data::AllocType alloc_type, void* src) {
		assert(type->ops->fn_copy_construct);
		return type->ops->fn_copy_construct(type, alloc_type, src);
	}
	
	func <void    (Memory*)>				 fn_free;
	func <bool    (void*)>					 fn_bool;
	func <Memory* (Type *, size_t, Data::AllocType, void *)> fn_alloc;
	func <Memory* (Type*,  Data::AllocType, void*)> fn_copy_construct;
	func <int     (void*,  void*)>			 fn_compare;

    /// reference sine function.
    template <typename T>
    T sin(T x) {
        const int max_iter = 70;
        T c = x, a = 1, f = 1, p = x;
        const T eps = 1e-8;
        ///
        for (int i = 1; abs(a) > eps and i <= max_iter; i++) {
            f *= (2 * i) * (2 * i + 1);
            p *= -1 * x  * x;
            a  =  p / f;
            c +=  a;
        }
        return c;
    }
	
	void Shared::attach(void* v_fn) {
		FnVoid* fn = (FnVoid*)v_fn;
		assert(mem);
		mem->att[mem->n_att++] = *fn;
	}

	template <typename T>
	Shared& Shared::operator=(T* ptr) {
		if (is_diff(ptr)) {
			drop(mem);
			Type* t = type_of<T>();
			mem = ptr ? Shared::manage(ptr, 1, t) : null; /// i dont see how we get the Type* from t here
		}
		return *this;
	}

    /// cosine function (sin - pi / 2)
    template <typename T>
    T cos(T x) { return sin(x - M_PI / 2.0); }

    /// tan, otherwise known as sin(x) / cos(y), the source
    template <typename T>
    T tan(T x) { return sin(x) / cos(x); }

	template <typename T>
	T* Shared::pointer() {
		Type* type = type_of<T>();
		assert(!mem || type == mem->data.type);
		return (T*)raw();
	}

	inline int64_t ticks() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }

	template <class T>
	inline T interp(T from, T to, T f) { return from * (T(1) - f) + to * f; }

	template <typename T>
	inline T* allocate(size_t c, size_t a = 16) {
		size_t sz = c * sizeof(T);
		return (T*)aligned_alloc(a, (sz + (a - 1)) & ~(a - 1));
	}

	template <typename T> inline void memclear(T* d, size_t c = 1, int v = 0) { memset(d, v, c * sizeof(T)); }
	template <typename T> inline void memclear(T& d, size_t c = 1, int v = 0) { memset(&d, v, c * sizeof(T)); }
	template <typename T> inline void memcopy(T* d, T* s, size_t c = 1) { memcpy(d, s, c * sizeof(T)); }

    struct io { };
}








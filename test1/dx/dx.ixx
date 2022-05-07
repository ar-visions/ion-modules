export module dx;

/// windows has far too many macros which effectively pollute dx
/// contain in dx/path.cpp
//#if defined(_WIN32) || defined(_WIN64)
//#include <windows.h>
//#endif

#ifndef NDEBUG
//import <signal.h>;
#pragma warning(disable:5050)
#endif

#include "macros.hpp"
//import   <stdio.h>;



export import std.core;
export import std.threading;
export import std.memory;
export import std.filesystem;

///
typedef struct { int token; } Internal;
int icursor = 0;

export {

	/// immediately attempt to establish what is real
#if   INTPTR_MAX == INT64_MAX
	typedef double       real;
#elif INTPTR_MAX == INT32_MAX
	typedef float        real;
#else
#   error invalid machine arch
#endif

	/// define a global constructor emitter thats portable
#ifdef __cplusplus
#define INITIALIZER(f) \
    static void f(void); \
    struct f##_t_ { f##_t_(void) { f(); } }; static f##_t_ f##_; \
    static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU",read)
#define INITIALIZER2_(f,p) \
    static void f(void); \
    __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
    __pragma(comment(linker,"/include:" p #f "_")) \
    static void f(void)
#ifdef _WIN64
#define INITIALIZER(f) INITIALIZER2_(f,"")
#else
#define INITIALIZER(f) INITIALIZER2_(f,"_")
#endif
#else
#define INITIALIZER(f) \
    static void f(void) __attribute__((constructor)); \
    static void f(void)
#endif

	template <typename> struct is_strable   : std::false_type { };
	template <typename> struct serializable : std::true_type  { };
	template <typename> struct is_std_vec   : std::false_type { };
	template <typename> struct is_vec	    : std::false_type { };
	template <typename> struct is_map       : std::false_type { };
	template <typename> struct is_array     : std::false_type { };

	struct is_apple :
#if defined(__APPLE__)
		std::true_type
#else
		std::false_type
#endif
	{};

	///
	struct is_android :
#if defined(__ANDROID_API__)
		std::true_type
#else
		std::false_type
#endif
	{};

	///
	struct is_win :
#if defined(_WIN32) || defined(_WIN64)
		std::true_type
#else
		std::false_type
#endif
	{};

	///
	struct is_gpu : std::true_type {};

	///
	struct is_debug :
#if !defined(NDEBUG)
		std::true_type
#else
		std::false_type
#endif
	{ };

	/// define assert fn because the macro ones are super lame
	inline void assert(const bool b) {
#ifndef NDEBUG
		if (!b) exit(1);
#endif
	}

#if defined(_WIN32) || defined(_WIN64)
	#define sprintf(b, v, ...) sprintf_s(b, sizeof(b), v, __VA_ARGS__)
#else
	#define sprintf(v, b, ...) snprintf(v, sizeof(b), v, __VA_ARGS__)
#endif

	/// we like null instead of NULL. friendly.
	/// in abstract it does not mean anything of 'pointer', is all.
	std::nullptr_t     null = nullptr;
	real               PI   = 3.141592653589793238462643383279502884L;

	/// r32 and r64 are explicit float and double,
	/// more of an attempt to get real; of course we never really can.
	typedef float      r32;
	typedef double     r64;
	typedef double	   real;
	typedef const char cchar_t;
	typedef int64_t    isize;
	typedef int64_t    ssize_t;
	typedef const char cchar_t;

	struct			   TypeOps;
	struct			   Memory;
	struct			   Type;

	template <typename V>
	using lambda = std::function<V>; // you win again, gravity

	template <typename, const bool=false>
	Type*  type_of();

	template <typename, const bool = false>
	Type*  type_ops();

	template <typename T, typename S>
	struct scalar {
		S val;
		inline scalar(T val) : val(S(val)) { }
		inline operator T()				   { return T(val); }
		inline T operator= (const T &v)    {
			if (this != &v)
				val = v;
			return T(val);
		}
	};

	struct Bool:scalar <bool, uint8_t>     { inline Bool(bool b) : scalar(b)    { } };

	enum Stride { Major, Minor };

	template <class U, class T>
	struct can_convert {
		enum {
			value = std::is_constructible<T, U>::value && !std::is_convertible<U, T>::value
		};
	};

	template<class T, class E>
	struct _has_equal_to {
		template<class U, class V>   static auto eq(U*)  -> decltype(std::declval<U>() == std::declval<V>());
		template<typename, typename> static auto eq(...)->std::false_type;
		///
		using imps = typename std::is_same<bool, decltype(eq<T, E>(0))>::type;
	};

	template<class T, class E = T>
	struct has_equal_to : _has_equal_to<T, E>::imps {};

	/// usage: has_overload<A, std::string>()
	template <typename A, typename B, typename = void>
	struct has_overload : std::false_type {};
	template <typename A, typename B>
	struct has_overload<A, B, decltype((void)(void (A::*)(B))& A::operator())> : std::true_type {};


	template<class T, class EqualTo>
	struct has_compare_impl
	{
		template<class U, class V>
		static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
		template<typename, typename>
		static auto test(...)->std::false_type;

		using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
	};

	template<class T, class EQ = T> struct has_compare : has_compare_impl<T, EQ>::type {};

	// these must be cleaned. i also want to use () or 

	template <typename K, typename V>
	struct pair {
		K key;
		V value;
	};

	struct io { };

	template <typename T>
	struct FlagsOf:io {
		uint64_t flags = 0;
		///
		FlagsOf(std::nullptr_t = null)      { }
		FlagsOf(std::initializer_list<T> l) {
			for (auto v : l)
				flags |= flag_for(v);
		}
	  //FlagsOf(T f) : flags  (uint64_t(f)) { } // i dont think this should be used here.
		FlagsOf(const FlagsOf<T>& ref) : flags(ref.flags) { }
		
		FlagsOf<T>& operator=(const FlagsOf<T>& ref) {
			if (this != &ref)
				flags = ref.flags;
			return *this;
		}

		uint32_t flag_for(T v) { return 1 << v;         }
		operator        bool() { return flags != 0;     }
		bool       operator!() { return flags == 0;     }

		// used for assign on and off
		void operator += (T v) { flags |=  flag_for(v); }
		void operator -= (T v) { flags &= ~flag_for(v); }
		
		// used for get
		bool operator[]  (T v) {
			uint32_t f = flag_for(v);
			return (flags & f) == f;
		}

		bool contain   (FlagsOf<T>& f) const { return (flags & f.flags) == f.flags; }
		bool operator&&(FlagsOf<T>& f) const { return contain(f); }
	};

	struct str2;
	struct Shared;

	Shared* str_ref(str2* ps);

	/// you express these with int(var, int, str); U unwraps inside the parenthesis
	template <typename T, typename... U>
	size_t fn_id(lambda<T(U...)>& fn) { // dump this when alternate lambda is operational, the prototypes used currently are not compatible with it, and thats not acceptable during a port.. it seems on msvc
		typedef T(fnType)(U...);
		fnType** fp = fn.template target<fnType*>(); // 'somehow works'
		return size_t(*fp);
	};

	// for type ambiguity sake, it is better to keep Type and Data separate; otherwise we toss a truthful joke in from the start
	// Data merely has a pointer, a Type, and a count
	// from there we have control of primitives over a vector
	enum Trait { TArray = 1, TMap = 2 };

	struct Type;





	struct Data2 {
		void* ptr;
		size_t count;
	};

	typedef int (*CompareOp2)  (Data2*, Data2*);

	struct SomeStruct {
		int arb;
		int arb2;
	};

	struct Data {
		enum AllocType { Arb, Manage };
		void*       ptr;    // pointer to first byte (sometimes on the tail, depending on alloc type)
		size_t      count;  // > 0 for valid data
		Type	   &type;   // the type of data assembly,
		Type       &origin; // the type of data assembler
		AllocType   alloc;  // 
	};

		/// msvc has problems with static fn templates performing constexpr, thats why TypeOps is here with a template arg.
	/// curtousy of msvc is all.
	/// prototypes for the various function pointers
	typedef void    (*Constructor)(Data*);
	typedef void    (*Destructor) (Data*);
	typedef void    (*CopyCtr)    (Data*, void*, size_t);
	typedef bool    (*BooleanOp)  (Data*);
	typedef int64_t (*IntegerOp)  (Data*);
	typedef Shared* (*AddOp)      (Data*, Data*);
	typedef Shared* (*SubOp)      (Data*, Data*);
	typedef Shared* (*StringOp)   (Data*);
	typedef bool    (*RealOp)     (Data*);
	typedef int     (*CompareOp)  (Data*, Data*);
	typedef void    (*AssignOp)   (Data*, void*);
	typedef size_t  (*SizeOfOp)   ();

	template <typename D>
	struct Ops2 {
		template <typename T>
		static CompareOp compare_check(T *ph = null) {

			// 
			if constexpr (has_compare<T>::value) {

				/// if there is a compare method, call it
				static auto compare = [](D* a, D* b) -> int {
					if (a == b)
						return 0;
					
					assert(a->count == b->count);

					T*      ap = (T *)a->ptr;
					T*      bp = (T *)b->ptr;
					size_t  c  = a->count;

					for (size_t i = 0; i < c; i++)
						return ap[i] == bp[i];

					return 0;
				};
				return CompareOp(&compare);
			} else
				return null;
		}
	};



	template <typename D>
	struct Ops {


		// this should validate matters for msvc?

		template <typename T, const bool L = false>
		static Constructor fn_ctr() {
			if constexpr (std::is_default_constructible<T>() && !L) {
				/// run the constructor on memory
				static auto ctr = [](Data* d) -> T * {
					T* ptr = (T *)d->ptr;
					for (size_t i = 0, c = d->count; i < c; i++)
						new (&ptr[i]) T();
				};
				return Constructor(&ctr);
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static Destructor fn_dtr() {
			if constexpr (!L) {
				/// run the destructor on memory
				static auto   dtr = [](Data* d) {
					T* ptr = (T*)d->ptr;
					for (size_t i = 0, c = d->count; i < c; i++)
						ptr[i]. ~T();
				};
				return Destructor(&dtr);
			}
			else
				return null;
		}

		template <typename T, const bool L = false>
		static CopyCtr fn_cpy() {
			if constexpr (!L) {
				/// run the copy-constructor on memory
				static auto cpy = [](Data* ds, void* sc, size_t c) {
					T*     p_ds = (T *)ds->ptr;
					T*     p_sc = (T *)sc;
					// for basic types, one could avoid 
					// the construction for each, just one memcpy
					for (size_t i = 0, c = ds->count; i < c; i++)
						new (&p_ds[i]) T(p_sc[i]);
				};
				return CopyCtr(&cpy);
			}
			else
				return null;
		}

		template <typename T, const bool L = false>
		static AssignOp fn_assign() {
			if constexpr (!L) {
				static auto assignv = [](Data* a, T *b) -> Data* {
					for (size_t i = 0; i < a->count; i++)
						*((T*)a->ptr)[i] = b[i];
				};
				return AssignOp(&assignv);
			}
			else
				return null;
		}

		/*
		template <typename T, const bool L = false>
		static AddOp fn_add() {
			if constexpr (!L) {
				static auto addv = [](Data* a, Data *b) -> Data* {
					assert(a->count == b->count);
					const size_t ac = a->count;
					T c[ac];
					for (size_t i = 0; i < a->count; i++)
						c[i] = *((T*)a->ptr)[i] + *((T*)b->ptr)[i];
					return Memory::manage(&c, 1);
				};
				return AddOp(&addv);
			}
			else
				return null;
		}

		template <typename T, const bool L = false>
		static SubOp fn_sub() {
			if constexpr (!L) {
				static auto subv = [](Data* a, Data* b) -> Data* {
					assert(a->count == b->count);
					T c[a->count];
					for (size_t i = 0; i < a->count; i++)
						c[i] = *((T*)a->ptr)[i] + *((T*)b->ptr)[i];
					return Memory::manage(&c, 1);
				};
				return SubOp(&subv);
			}
			else
				return null;
		}*/

		template <typename T, const bool L = false>
		static IntegerOp fn_integer() {
			if constexpr (has_overload<T, int64_t>::value && !L) {
				static auto intv = [](Data* d) -> int64_t {
					if constexpr (has_overload<T, int64_t>::value)
						return int64_t(*(T*)d->ptr);

					return int64_t(0);
				};
				return IntegerOp(&intv);
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static RealOp fn_real() {
			if constexpr (has_overload<T, ::real>::value && !L) {
				static auto realv = [](Data* d) -> ::real {
					if constexpr (has_overload<T, ::real>::value)
						return ::real(*(T*)d->ptr);
					else
						return ::real(0); // nan possible but i dunno about using the null state for interface
				};
				return RealOp(&realv);
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static StringOp fn_string() {
			if constexpr (has_overload<T, ::str>::value && is_strable<T>() && !L) {
				// 
				static auto strv = [](Data* d) -> Shared* { return str_ref((T *)d->ptr); };
				return StringOp(&strv);
			} else
				return null;
		}

		// never leave the success state.
		template <typename T, const bool L = false>
		static CompareOp fn_compare(T *ph = null) {
			// must have compare, and must return int. not just a bool.
			if constexpr (!is_std_vec<T>() && has_compare<T>::value) {
				/// compare memory; if there is a compare method call it, otherwise require the equal_to
				static auto compare = [](Data* a, Data* b) -> int {
					if (a == b)
						return 0;
				
					assert(a->count == b->count);
					assert(a->count == size_t(1));

					T*      ap = (T *)a->ptr;
					T*      bp = (T *)b->ptr;
					size_t  c  = a->count;

					if constexpr (has_compare<T>::value) { //
						for (size_t i = 0; i < c; i++)
							return ap[i] == bp[i]; // need enums
					}
					return 0;
				};
				return CompareOp(&compare);
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static SizeOfOp fn_type_size() {
			if constexpr (!L) {
				/// compare memory; if there is a compare method call it, otherwise require the equal_to
				static auto type_size = []() { return sizeof(T); };
				return SizeOfOp(&type_size);
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static BooleanOp fn_boolean() {
			if constexpr (!L && has_overload<T, bool>::value) {
				static    auto bC  = [](Data* a) -> bool {
					       bool r  = false;
					for (size_t i  = 0; i < a->count; i++) {
						        r &= bool(((T *)a->ptr)[i]);
						   if (!r) break;
					}
					return r;
				};
				return BooleanOp(&bC);
			} else {
				static auto bL = [](Data* d) -> bool { return bool(d->ptr); };
				return BooleanOp(&bL);
			}
		}
	};






	struct Type:Ops<Data> {
	public:
	/// primitives, make const.
	static const Type 
		Undefined, i8, ui8, i16, ui16,
		i32, ui32, i64, ui64,
		f32, f64, Bool, Str,
		Map, Array, Ref, Arb,
		Node, Lambda, Member,
		Any, Meta;

		size_t          id  = 0;
		std::string     name;
		std::mutex      mx;

		struct Decl {
			size_t        id;
			std::string	  name;
		};

		static const Type** primitives;

		template <typename T>
		static Type bootstrap(Decl& decl) {
			Type& t = ident<T>();
			t.id = decl.id; // set id from decl
			return t;
		}

		enum IMode { Default };

		Type(IMode im)     { }

		bool     is_integer() { return id >= ident<int8_t>  ().id && 
									   id <= ident<uint64_t>().id; }
		bool     is_float  () { return id == ident<float >().id;   }
		bool     is_double () { return id == ident<double>().id;   }
		bool     is_real   () { return is_float() || is_double();  }
		bool     is_array  () { return traits[Trait::TArray];      }
		operator size_t    () { return id;   }

		Type(const Type& ref) : id(ref.id) { }

		Type& operator=(const Type &ref)     {
			if (this != &ref)
				id = size_t(ref.id);
			return *this;
		}

		Constructor     ctr;
		Destructor      dtr;
		CopyCtr		    cpy;
		IntegerOp       integer;
		RealOp          real;
		StringOp        string;
		BooleanOp       boolean;
		CompareOp       compare;
		SizeOfOp		type_size;
		//AddOp			add; # use case isnt panning out
		//SubOp			sub;
		AssignOp		assign;

		FlagsOf<Trait>  traits;

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

		struct var;
		struct str;

		/// static allocation of referenced types; obtains string name, ctr, dtr and cpy-ctr, compare and boolean ops in vector interface
		template <typename _ZZion, const bool _isFunctor = false>
		static Type &ident() {
			static Type type = Type(IMode::Default); /// just an ugly way of saying Type(), which is protected from newness
			static bool init;

			if (!init) {
				init  = true;

				size_t       b, l;
				std::string    cn = __PRETTY_FUNCTION__;
				std::string     s = "_ZZion =";
				const size_t blen = s.length();

				const Type* p = &type;
				if      (p == &Undefined) type.id = 0;
				else if (p == &i8)        type.id = 1;
				else if (p == &ui8)       type.id = 2;
				else if (p == &i16)       type.id = 3;
				else if (p == &ui16)      type.id = 4;
				else if (p == &i32)       type.id = 5;
				else if (p == &ui32)      type.id = 6;
				else if (p == &i64)       type.id = 7;
				else if (p == &ui64)      type.id = 8;
				else if (p == &f32)       type.id = 9;
				else if (p == &f64)       type.id = 10;
				else if (p == &Bool)      type.id = 11;
				else if (p == &Str)       type.id = 12;
				else if (p == &Map)       type.id = 13;
				else if (p == &Array)     type.id = 14;
				else if (p == &Ref)       type.id = 15;
				else if (p == &Arb)       type.id = 16;
				else if (p == &Node)      type.id = 17;
				else if (p == &Lambda)    type.id = 18;
				else if (p == &Member)    type.id = 19;
				else if (p == &Any)       type.id = 20;
				else if (p == &Meta)      type.id = 21;
				else					  type.id = size_t(&type); // for resolving purpose

				b = cn.find(s) + blen;   // b = start of name
				l = cn.find("]", b) - b; // l = length of name

				type.name			= cn.substr(b, l);

				if constexpr (!std::is_same_v<_ZZion, void>) {
					// populate pointers to the various functors
					type.ctr = fn_ctr <_ZZion, _isFunctor>();
					type.dtr = fn_dtr <_ZZion, _isFunctor>();
					type.cpy = fn_cpy <_ZZion, _isFunctor>();

					if constexpr (!is_lambda<_ZZion>()) {
						type.integer     = fn_integer    <_ZZion, _isFunctor>();
						type.real        = fn_real       <_ZZion, _isFunctor>();
						type.string      = fn_string     <_ZZion, _isFunctor>();
						type.compare     = fn_compare    <_ZZion, _isFunctor>();
						type.type_size   = fn_type_size  <_ZZion, _isFunctor>();
						type.boolean     = fn_boolean    <_ZZion, _isFunctor>();
						//type.add       = fn_add        <_ZZion, _isFunctor>();
						//type.sub       = fn_sub        <_ZZion, _isFunctor>();
					}

					// its possible to decentralize trait-bound method registration. oracle just told me
					if constexpr (::is_array<_ZZion>()) type.traits += TArray;
					if constexpr (::is_map  <_ZZion>()) type.traits += TMap;
				}
			}
			return type;
		}

		bool operator==(Type* b) const { return  b && id == b->id; }
		bool operator!=(Type* b) const { return !b || id != b->id; }
		bool operator>=(Type* b) const { return  b && id >= b->id; }
		bool operator<=(Type* b) const { return  b && id <= b->id; }
		bool operator> (Type* b) const { return  b && id >  b->id; }
		bool operator< (Type* b) const { return  b && id <  b->id; }

		bool operator==(Type& b) const { return id == b.id; }
		bool operator!=(Type& b) const { return id != b.id; }
		bool operator>=(Type& b) const { return id >= b.id; }
		bool operator<=(Type& b) const { return id <= b.id; }
		bool operator> (Type& b) const { return id >  b.id; }
		bool operator< (Type& b) const { return id <  b.id; }

		     operator             size_t() const { return     id;              }
		     operator                int() const { return int(id);             }
		     operator               bool() const { return     id > 0;          }

		// grab/drop operations are mutex locked based on the type
		inline void				    lock()       { mx.lock();                  }
		inline void				  unlock()       { mx.unlock();                }
		
		//
		inline size_t				  sz() const { return type_size();		   }
	};

	namespace fs = std::filesystem;

	/// do we or do we not use Type & refs or ptrs.
	/// if we can Manage a ref then its better
	/// 
	template <typename T>
	inline Type* Id() { return &Type::ident<T>(); }

	//namespace std { template<> struct hash<Type> { size_t operator()(Type const& id) const { return id; } }; }

	struct  Memory;

	typedef void (*Closure)(void*);

	// it is simplifying to make memory self referencing.
	// it reduces the amount of overhead logic needed, so something expressed more commonly for arb referencing systems.
	struct Attachment {
		size_t      id;
		Memory*     mem;
		Closure     closure; // optional T*
	};

	typedef std::vector<Attachment> VAttachments;
	
	void    drop        (Memory *);
	void    grab        (Memory *);

    struct Memory {
		Data			 data;
        int              refs;
		VAttachments    *att;

		/// lookup attachment by id
		Memory *lookup(size_t att_id) {
			if (att)
			for (auto& att : *att) {
				if (att.id == att_id)
					return att.mem;
			}
			assert(false);
			return null;
		}

		inline bool boolean()			 { return data.type.boolean(&data); }
		inline int  compare(Memory* rhs) { return data.type.compare(&data, &rhs->data); }

		template <typename T>
		static Memory*  arb(T* ptr, size_t count, Type *origin = null) {
			static Type* type = Id<T>();
			Memory* mem       = (Memory*)calloc(1, sizeof(Memory));

			/// run constructor on Memory
			new (mem) Memory {
				.data = Data { ptr, count, *type, *(origin ? origin : type), Data::AllocType::Arb},
				.refs = int(1)
			};
			return mem;
		}

		template <typename T> static Memory* lambda(T* src, size_t count, Type* origin = null) {
			static Type *type = Type::ident<T, true>();
			Memory* mem = (Memory*)calloc(1, sizeof(Memory) + sizeof(T) * count);

			new (mem) Memory{
				.data = Data { &mem[1], count, type, Data::AllocType::Manage },
				.refs = int(1)
			};

			/// run constructor on Type
			for (size_t i = 0, c = size_t(count); i < c; i++) {
				T*     ds = &((T*)mem->data.ptr)[i];
				T*     sc = &src[i];
				new (ds) T(*sc);
			}
			return mem;
		}

        template <typename T> static Memory* manage(T* src, size_t count, Type* origin = null) {
			static Type *type = Id<T>();
			Memory* mem       = (Memory*)calloc(1, sizeof(Memory) + sizeof(T) * count);

			/// run constructor on Memory
			new (mem) Memory {
				.data = Data {
					.ptr    =  &mem[1],
					.count  =   count,
					.type   = *(type),
					.origin = *(origin ? origin : type),
					.alloc  =   Data::AllocType::Manage
				},
				.refs = int(1)
			};

			/// run constructor on Type
			for (size_t i = 0, c = size_t(count); i < c; i++) {
				T* ds = &((T *)mem->data.ptr)[i];
				T* sc = &src[i];
				new (ds) T(*sc);
			}
			return mem;
        }

		/// two vallocs enter. 
		template <typename T> static Memory* valloc(size_t count, bool construct = true, Type* origin = null) {
			Memory*       mem = (Memory*)calloc(1, sizeof(Memory) + sizeof(T) * count);
			T*             ds =  (T *)mem->data.ptr;
			static Type* type =  Id<T>();

			new (mem) Memory {
				.data = Data {
					.ptr    =  &mem[1],
					.count  =   count,
					.type   = *(type),
					.origin = *(origin ? origin : type),
					.alloc  =   Data::AllocType::Manage
				},
				.refs = int(1)
			};

			if (construct) // there is no bother for basic primitive types, and a vector saved is a vector earned.
				for (size_t i = 0, c = count; i < c; i++) {
					new (&ds[i]) T();
				}
			return mem;
		}

		/// one without template (Type and count, and you get a Memory vector allocation of that)
		static Memory* valloc(Type *type, size_t count, Type *origin = null) {
			Memory* mem = (Memory*)calloc(1, sizeof(Memory) + type->type_size() * count);
			new    (mem)   Memory {
				.data = Data {
					.ptr    = &mem[1],
					.count  =  count,
					.type   = *type,
					.origin =  origin ? *origin : *type,
					.alloc  =  Data::AllocType::Manage
				},
				.refs = int(1)
			};
			type->ctr(&mem->data); // vector construction happens inside (where best karate lay)
			return mem;
		}

		template <typename T> inline bool is_diff(T *p) { return data.ptr != p; }

		template <typename T>
		static Memory* copy(T& r, Type* origin) { return Memory::manage(&r, 1, origin); }

		/// copy vector of types, this is accepted by Shared receivers for implicit fun and love
		static Memory* copy(Memory* mem, Type *origin) {
			/// this is banging good, i think.
			/// origins are just a total good for debugging. everybody loves an origin, and their own.
			Memory* dst = (Memory*)calloc(1, sizeof(Memory) + mem->data.type.sz() * mem->data.count);
			
			new (dst) Memory {
				.data = Data {
					.ptr    = &dst[1],
					.count  =  mem->data.count,
					.type   =  mem->data.type,
					.origin =  *(origin ? origin : &mem->data.origin),
					.alloc  =  Data::AllocType::Manage
				},
				.refs = int(1)
			};

			// vector construction happens inside
			mem->data.type.cpy (&dst->data, mem->data.ptr, mem->data.count); 
			return dst;
		}

		Memory* copy(Type *origin) { return copy(this, origin); }

		// attach memory to memory; this is a non-common case but useful for variable dependency of data
		void attach(Attachment &a) {
			if (!att) {
				 att = new VAttachments();
				 att->reserve(1);
			}
			att->push_back(a); // { size_t id, Memory* m, Closure c }
			grab(a.mem);
		} 
	};

	//
	struct Shared {
		Memory* mem;

		Shared(nullptr_t n = null)			 { }
	    Shared(Memory* mem) : mem(mem)		 { }
	   ~Shared()							 { drop(mem); }
		Shared(const Shared& r) : mem(r.mem) { grab(mem); }

		/// cannot 'hold onto nothing.', and why alter mem in-house, thats terrible manners.
		void attach(Attachment &a) { assert(mem); mem->attach(a); }

		void*          raw() const { return mem ? mem->data.ptr : null; }

		///
		template <typename T>
		T*         pointer();
		Data*         data() const { return mem ? &mem->data : null; }
		
		///
		template <typename T>
		T&           value()             { return *(T*)pointer(); }

		/// generic integer conversion
		/// based on type traits this can just be 0
		template <typename I>
		I          integer()             { return I(type()->integer(data())); }

		/// real value
		template <typename R>
		R             real()             { return R(type()->real(data())); }

		/// string value
		Shared     *string()             { return type()->string(data()); }

		bool operator==(Type& t)   const { return type() == &t; }

		bool operator==(Shared& d) const { return (!mem && !d.mem) || (mem && d.mem && mem->data.type.compare(&mem->data, &d.mem->data) == 0); }

		/// common for shared objects
		/// case, an array<int> is 
		///
		/// i need something to give a window into either var or array.
		/// 
		template <typename T>
		T &reorient(FlagsOf<Trait> require) {
			Type*  s = type();
			Type*  t = Id<T>();
			assert(t->traits && require); /// i... declare... type-safety --michael scott
			//assert(t == s->origin); 
			return *(T *)((void *)this);
		}

		/// reference Shared instance (helps with quantum)
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

		/// type-safety on the cast; the design on this is lose
		template <typename T>
		operator T* () const {
#ifndef NDEBUG
			static Type *id = Id<T>();
			if (mem && !(id == mem->data.type)) {
				std::string& a = id->name;
				std::string& b = mem->data.type.name;
				///
				if (a != "void" && b != "void") {
					printf("type cast mismatch on Shared %s != %s\n", a.c_str(), b.c_str());
					exit(1);
				}
			}
#endif
			return (T*)(mem ? mem->data.ptr : null);
		}

		operator            bool() const { return mem->boolean(); }
		bool          operator! () const { return !(operator bool()); }

		template <typename T>
		operator             T& () const { return *(T *)mem->data.ptr; }
		operator          void* () const { return mem ? mem->data.ptr : null; }

		template <typename T>
		T&                  cast() const { return *(T *) (mem ? mem->data.ptr : null);      }
		size_t             count() const { return  mem ?  mem->data.count     : size_t(0);  }
		Memory*            ident() const { return  mem;                                     }
		void*            pointer() const { return  mem ?  mem->data.ptr       : null;       }
		Type*	            type() const { return  mem ? &mem->data.type      : null;       }
		Shared	copy(Type *origin) const { return  mem ?  mem->copy(origin)   : null;       }
		Shared           quantum() const { return (mem->refs == 1) ? *this    : copy(null); }

		template <typename T>
		inline Shared  &assign(T& value) {
			Type* t = type();
			if ((void *)Id<T> == (void *)t) {
				t->assign(&mem->data, &value);
			} else {
				/// dislike type case
			}
			return *this;
		}

		// Type* complicates 
		bool operator==(Type*   t) const { return type() == t; }
		bool operator!=(Shared& b) const { return !(operator==(b)); }
	};

	/// it finally feels like saturday. lol.

	/// these are easier to pass around than before.
	/// i want to use these more broadly as well, definite a good basis delegate for tensor.
	template <typename T>
	struct vec:Shared {
		// #ifndef NDEBUG window vars here; set them in each vec.
		vec(T x)				  {						     Shared::mem = Memory::manage<T>(&x,    1); }
		vec(T x, T y)             { T v[2] = { x, y };       Shared::mem = Memory::manage<T>(&v[0], 2); }
		vec(T x, T y, T z)        { T v[3] = { x, y, z };    Shared::mem = Memory::manage<T>(&v[0], 3); }
		vec(T x, T y, T z, T w)   { T v[4] = { x, y, z, w }; Shared::mem = Memory::manage<T>(&v[0], 4); }
		
		size_t  type_size() const { return sizeof(T); }
		size_t       size() const { return count();   }
		size_t size(vec& b) const {
			size_t sz = size();
			assert(b.size() == sz);
			return sz;
		}

		T&   operator[](size_t i) {
			T&   a = *cast<T>();
			return a[i];
		}

		vec& operator+=(vec b) {
			vec& a = *this;
			T*  va = (T*)a,
				vb = (T*)b;
			for (size_t i = 0, sz = size(b); i < sz; i++)
				va[i]    += vb[i];
			return *this;
		}

		vec& operator-=(vec b) {
			vec& a = *this;
			T*  va = (T*)a,
				vb = (T*)b;
			for (size_t i = 0, sz = size(b); i < sz; i++)
				va[i]    -= vb[i];
			return *this;
		}

		vec& operator*=(vec b) {
			vec& a = *this;
			T*  va = (T*)a, vb = (T*)b;
			for (size_t i = 0, sz = size(b); i < sz; i++)
				va[i] *= vb[i];
			return *this;
		}

		vec& operator*=(T b) {
			vec& a = *this;
			T*  va = (T*)a;
			for (size_t i = 0, sz = size(); i < sz; i++)
				va[i] *= b;
			return *this;
		}

		vec& operator/=(vec b) {
			vec& a = *this;
			T*  va = (T*)a,
				vb = (T*)b;
			for (size_t i = 0, sz = size(b); i < sz; i++)
				va[i] /= vb[i];
			return *this;
		}

		vec& operator/=(T b) {
			vec& a = *this;
			T*  va = (T*)a;
			for (size_t i = 0, sz = size(); i < sz; i++)
				va[i] /= b;
			return *this;
		}

		vec& operator=(const vec& b) {
			if (this != &b)
				mem = Memory::copy(b.mem);
			return *this;
		}

		vec operator+(vec b) const {
			vec& a = *this;
			vec  c = vec(size(), T(0));
			T*  va = (T *)a, vb = (T *)b, vc = (T *)c;
			for (size_t i = 0, sz = size(b); i < sz; i++)
				vc[i] = va[i] + vb[i];
			return c;
		}

		vec operator-(vec b) const {
			vec& a = *this;
			vec  c = vec(size(), T(0));
			T*  va = (T*)a, vb = (T*)b, vc = (T*)c;
			for (size_t i = 0, sz = size(b); i < sz; i++)
				vc[i] = va[i] - vb[i];
			return c;
		}

		vec operator*(T b) const {
			vec& a = *this;
			vec  c = vec(size(), T(0));
			T*  va = (T*)a, vc = (T*)c;
			for (size_t i = 0, sz = size(); i < sz; i++)
				vc[i] = va[i] * b;
			return c;
		}

		vec operator/(T b) const {
			vec& a = *this;
			vec  c = vec(size(), T(0));
			T*  va = (T*)a, vc = (T*)c;
			for (size_t i = 0, sz = size(); i < sz; i++)
				vc[i] = va[i] / b;
			return c;
		}
	};

	// we must get this in working order.
	/* a non-working lambda. its all gimped and pathetic, and barely even able to walk. quiet voice: 'help me.'
	/// R = return type, A = variadic args
	template <typename R, typename... A>
	struct lambda <R(A...)> {
		typedef uint8_t raw;
		typedef R(*Invoke) (raw*, A&...);

		template <typename F> static R invoke_fn (F* functor, A... args) { return (*functor)(std::forward<A>(args)...); }

		// these pointers are storing the statics [disambiguated]
		Invoke      invoke;
		Shared      data;
	
		lambda() : invoke_f(nullptr), data(nullptr) { }
		
		// specialize functions and erase their type info by casting
		template <typename F>
		lambda(F f) {
			invoke_f = Invoke(&invoke_fn<F>);
			data = Memory::lambda((F*)&f, sizeof(F));
		}

		// assign from lambda after the fact
		template <typename F>
		lambda& operator=(F f) {
			data = Memory::lambda((F*)&f, sizeof(F));
			invoke_f = invoke_fn<F>;
		}

		lambda(const lambda& rhs) : invoke_f(rhs.invoke_f), data(rhs.data) { }

		bool operator==(const lambda& fn) { return invoke_f == fn.invoke_f; }
		bool operator!=(const lambda& fn) { return invoke_f != fn.invoke_f; }
		operator				   bool() { return  data; }
		bool				  operator!() { return !data; }
		
		template <typename Functor>
		static R invoke_fn(Functor* fn, A&&... args) {
			return (*fn)(std::forward<A>(args)...);
		}

		R operator()(A&&... args) {
			return invoke_f((raw*)(data.raw()), std::forward<A>(args)...);
		}
	};
	*/

	template<typename>   struct is_lambda			       : std::false_type {};
	template<typename T> struct is_lambda<lambda<T>>       : std::true_type  {};
	
	template<typename T> struct is_std_vec<std::vector<T>> : std::true_type {};

	struct node;
	struct string;

	typedef lambda<bool()>                FnBool;
	typedef lambda<void(void*)>           FnArb;
	typedef lambda<void(void)>            FnVoid;


	template <typename T>
	struct sh:Shared {
		sh() { }
		sh(const sh<T>& a)       : Shared(a.mem) { }
		sh(T& ref)			     : Shared(Memory::manage(&ref, 1)) { }
		sh(T* ptr, size_t c = 1) : Shared(Memory::manage( ptr, c)) { assert(c); }
	  //sh(size_t c, lambda<void(T*)> fn) : Shared(Memory::manage(malloc(sz), c, Type::lookup<T>())) { }
		///
		sh<T>& operator=(sh<T>& ref) {
			if (mem != ref.mem) {
				drop(mem);
				mem = ref.mem;
				grab(mem);
			}
			return *this;
		}

		static Shared valloc(size_t sz) { return Memory::valloc<T>(sz); }

		///
		sh<T>& operator=(T* ptr) {
			if (!mem || mem->is_diff(ptr)) {
				drop(mem);
				mem = ptr ? Memory::manage(ptr, 1, Id<T>()) : null;
			}
			return *this;
		}

		///
		operator                  T* () const { return  (T*)mem->data.ptr; } ///    T* operator
		operator                  T& () const { return *(T*)mem->data.ptr; } ///    T& operator
		T&                operator  *() const { return *(T*)mem->data.ptr; } /// deref operator
		T*                operator ->() const { return  (T*)mem->data.ptr; } /// reference memory operator
		bool                   is_set() const { return      mem != null;   }
		bool operator==      (sh<T>& b) const { return      mem == b.mem;  }
		bool operator==(const sh<T>& b) const { return      mem == b.mem;  }
		T&   operator[]    (size_t   i) const {
			assert(i < size_t(mem->data.count));
			T*  origin = mem->data.ptr;
			T& indexed = origin[i];
			return indexed;
		}
	};

	///
	void grab(Memory *m) {
		Type &type = m->data.type;
		type.lock();
		m->refs++;
		type.unlock();
	}

	///
	void drop(Memory* m) {
		Type& type = m->data.type;
		type.lock();
		if (--m->refs == 0) {
			// Destructor.
			m->data.type.dtr(&m->data);
			if (m->att) {
				for (auto &att:*m->att)
					if (att.closure)
						att.closure(att.mem); // let this one complete the transaction by its will
					else
						drop(att.mem);
			}
			type.unlock();
			delete m;
		} else
			type.unlock();
	}

					   struct Arb     : Shared { Arb (void* p, size_t c = 1) { mem = Memory::arb   (p, c); }};
	template <class T> struct Persist : Shared { Persist(T* p, size_t c = 1) { mem = Memory::arb   (p, c); }};
	template <class T> struct Manage  : Shared { Manage (T* p, size_t c = 1) { mem = Memory::manage(p, c); }};
	template <class T> struct VAlloc  : Shared { VAlloc (      size_t c = 1) { mem = Memory::valloc(   c); }};

	template <typename T>
	T fabs(T x) {
		return x < 0 ? -x : x;
	}
	
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
	
	template <typename T>
	Shared& Shared::operator=(T* ptr) {
		if (!mem || mem->data.ptr != ptr) {
			Type* origin = mem ? &mem->data.origin : null;
			drop(mem);
			mem = ptr ? Memory::manage(ptr, 1, origin) : null;
		}
		return *this;
	}

    /// cosine function (sin - pi / 2)
    template <typename T>
    T cos(T x) { return sin(x - PI / 2.0); }

    /// tan, otherwise known as sin(x) / cos(y), the source
    template <typename T>
    T tan(T x) { return sin(x) / cos(x); }

	template <typename T>
	T* Shared::pointer() {
		assert(!mem || mem->data.type == Id<T>());
		return (T*)raw();
	}

	template <class T>
	inline T interp(T from, T to, T f) { return from * (T(1) - f) + to * f; }

	template <typename T>
	inline T* allocate(size_t c, size_t a = 16) {
		size_t sz = c * sizeof(T);
		return (T*)aligned_alloc(a, (sz + (a - 1)) & ~(a - 1));
	}

	template <typename T> inline void memclear(T* d, size_t c = 1, int v = 0) { memset( d, v, c * sizeof(T)); }
	template <typename T> inline void memclear(T& d, size_t c = 1, int v = 0) { memset(&d, v, c * sizeof(T)); }
	template <typename T> inline void memcopy (T* d, T*     s,  size_t c = 1) { memcpy( d, s, c * sizeof(T)); }
///}

/// export module rand
/// export {
	struct Rand {
		struct Sequence {
			enum Type {
				Machine,
				Seeded
			};
			inline Sequence(int64_t seed, Sequence::Type t = Seeded) {
				if (t == Machine) {
					std::random_device r;
					e = std::default_random_engine(r());
				}
				else
					e.seed(uint32_t(seed)); // windows doesnt have the same seeding 
			}
			std::default_random_engine e;
			int64_t iter = 0;
		};

		static inline Sequence global = Sequence(0, Sequence::Machine);

		static std::default_random_engine& global_engine() {
			return global.e;
		}

		static void seed(int64_t seed) {
			global = Sequence(seed);
			global.e.seed(uint32_t(seed)); // todo: msvc only, and find out how to seed with 64b because 32 is lol
		}

		static bool coin(Sequence& s = global) {
			return uniform(0.0, 1.0) >= 0.5;
		}

		static double uniform(double from, double to, Sequence& s = global) {
			return std::uniform_real_distribution<double>(from, to)(s.e);
		}

		static int uniform(int from, int to, Sequence& s) {
			return int(std::round(std::uniform_real_distribution<double>(double(from), double(to))(s.e)));
		}
	};
//}

//export module arr
//export {
	typedef std::filesystem::path path_t;

	template <class T>
	struct array:io {
	public:
		typedef sh<std::vector<T>> vshared;
		
		// todo: need iterator on Vector abstract
		//typedef  Vector<T> vshared; // just a Shared type

	protected:
		vshared  a;
		vshared &realloc(size_t res) {
			a = new std::vector<T>();
			if (res)
				a->reserve(size_t(res));
			return a;
		}
		// 
		std::vector<T>& ref(size_t res = 0) { return *(a ? a : realloc(res)); }

	public:
		inline Type *element_type() { return ident<T>(); }
		typedef T value_type;
    
	/* ~array() { }
		array &operator=(array<T> &ref) {
			if (this != &ref)
				a = ref.a;
			return *this;
		}
		array(array<T> &ref) : a(ref.a) { } */
		array()                           { }
		array(std::initializer_list<T> v) { realloc(v.size()); for (auto &i: v) a->push_back(i); }
		array(const T* d, size_t sz) {
			assert (d);
			realloc(sz);
			sz.each(d, [](T& value, auto index) {
				a->push_back(value);
			});
		}
		array(size_t sz, T v)                {   realloc(sz); for (size_t i = 0; i < size_t(sz); i++) a->push_back(v);     }
		array(int  sz, lambda<T(size_t)> fn) {   realloc(sz); for (size_t i = 0; i < size_t(sz); i++) a->push_back(fn(i)); }
		array(std::nullptr_t n)              { }
		array(size_t sz)                     { realloc(sz); }
    
		/// quick-sort raw impl
		array<T> sort(lambda<int(T &a, T &b)> cmp) {
			auto      &self = ref();
			/// initialize list of identical size with elements of pointer type, pointing to the respective index
			array<T *> refs = { size(), [&](size_t i) { return &self[i]; }};

			lambda <void(int,   int)> qk;
				qk =       [&](int s, int e) {
				
				/// sanitize
				if (s >= e)
					return;

				/// i = start, j = end (if they are the same index, no need to sort 1)
				int i  = s, j = e, pv = s;

				///
				while (i < j) {
					/// standard search pattern
					while (cmp(refs[i], refs[pv]) <= 0 && i < e)
						i++;
                
					/// and over there too ever forget your tail? we all have
					while (cmp(refs[j], refs[pv]) > 0)
						j--;
                
					/// send away team of red shirts
					if (i < j)
						std::swap(refs[i], refs[j]);
				}
				/// throw a water bottle
				std::swap(refs[pv], refs[j]);
            
				/// rant and rave how this generation never does anything
				qk(s, j - 1);

				/// pencil-in for interns
				qk(j + 1, e);
			};
        
			/// lift off.
			qk(0, size() - 1);
			
			/// resolve refs
			return { size(), [&](size_t i) { return *refs[i]; }};
		}
    
		T                   &back()              { return ref().back();                }
		T                  &front()              { return ref().front();               }
		int                   isz()        const { return a ?     int(a->size()) : 0;  }
		size_t                 sz()		   const { return a ? a->size() : 0; }
		size_t               size()		   const { return a ? a->size() : 0; }
		size_t           capacity()        const { return a ? a->capacity() : 0;       }
	
		std::vector<T>    &vector()              { return ref();                       }
		std::vector<T>::iterator         begin() { return ref().begin();               }
		std::vector<T>::iterator           end() { return ref().end();                 }
		T                   *data()        const { return (T *)(a ? a->data() : null); }
		void      operator    += (T v)           { ref().push_back(v);                 }
		void      operator    -= (int i)         { return erase(i);                    }
				  operator   bool()        const { return a && a->size() > 0;          }
		bool      operator      !()        const { return !(operator bool());          }
		T&        operator     [](size_t i)      { return ref()[i];                    }
		bool      operator!=     (array<T> b)    { return  (operator==(b));            }
		bool      operator==     (array<T> b)    {
			if (!a && !b.a)
				return true;
			size_t sz = size();
			bool sz_equal = sz == b.size();
			if ( sz_equal && sz ) {
				T *a_v = (T *)a->data();
				T *b_v = (T *)b.data();
				for (size_t i = 0; i < size(); i++) {
					bool eq = a_v[i] == b_v[i];
					if (!eq)
						return false;
				}
			}
			return sz_equal;
		}
		
		///
		array<T> &operator=(const array<T> &ref) {
			if (this != &ref)
				a = ref.a;
			return *this;
		}

		///
		void   expand(size_t sz, T f) {
			// todo: check use-cases and check against std for expand; use expand if it exists
			// todo: curious to see what this release compiles to.
			sz.each([&](ssize_t i) { a->push_back(f); });

			// proposed 1 line lambda: sz.each([&](ssize_t i) <- a->push_back(f));
		}

		void   erase(int index)             { if (index >= 0) a->erase(a->begin() + size_t(index)); }
		void   reserve(size_t sz)           { ref().reserve(sz); }
		void   clear(size_t sz = 0)         { ref().clear(); if (sz) reserve(sz); }
		void   resize(size_t sz, T v = T()) { ref().resize(sz, v); }
		
		/// deprecate usage, possibly remove now
		static array<T> import(std::vector<T> v) {
			array<T> a(v.size());
			for (auto &i: v)
				a += i;
			return a;
		}
    
		template <typename R>
		array<R> convert() {
			array<R> out(a->size());
			if (a) for (auto &i: *a)
				out += i;
			return out;
		}
    
		int count(T v) const {
			int r = 0;
			if (a) for (auto &i: *a)
				if (v == i)
					r++;
			return r;
		}
    
		void shuffle() {
			std::vector<std::reference_wrapper<const T>> v(a->begin(), a->end());
			std::shuffle(v.begin(), v.end(), Rand::global_engine());
			std::vector<T> sh { v.begin(), v.end() };
			a->swap(sh);
		}
    
		/// its a vector operation it deserves to expand
		template <typename R>
		R select_first(lambda<R(T &)> fn) const {
			if (a) for (auto &i: *a) {
				R r = fn((T &)i);
				if (r)
					return r;
			}
			return R(null);
		}

		/// todo: all, all delim use int, all. not a single, single one use size_t.
		int index_of(T v) const { 
			int index = 0;
			if (a) for (auto &i: *a) {
				if (i == v)
					return index;
				index++;
			}
			return -1;
		}
	};

	template<typename T>
	struct is_vec<array<T>>  : std::true_type  {};

	template <typename T>
	bool equals(T v, array<T> values) { return values.index_of(v) >= 0; }

	template <typename T>
	bool isnt(T v, array<T> values)   { return !equals(v, values); }


	template <const Stride S>
	struct Shape {
		struct Data {
			array<ssize_t> shape;
			Stride		   stride;
		};
		sh<Data> data;

		int      dims() const { return int(data->shape.size()); } // sh can have accessors for a type vec
		size_t   size() const {
			ssize_t r     = 0;
			bool    first = true;
			///
			if (data)
				for (auto s:data->shape)
					if (first) {
						first = false;
						r     = s;
					} else
						r    *= s;

			return size_t(r);
		}
    
		/// Shapes can have negatives in their dimension as a declaration spec
		/// i dont think referencing a mere half value of 64bit in positive and negative is exactly a hinderance either
		/// ssize_t is prefererred as a result
		ssize_t operator [] (size_t i) { return data->shape[i]; }

		///
		Shape(array<int> sz)                  : data(sz) { }
		Shape(int d0, int d1, int d2, int d3) : data(new Data {{ d0, d1, d2, d3 }, S }) { }
		Shape(int d0, int d1, int d2)         : data(new Data {{ d0, d1, d2 },     S }) { }
		Shape(int d0, int d1)                 : data(new Data {{ d0, d1 },		   S }) { }
		Shape(int d0)                         : data(new Data {{ d0 },			   S }) { }
		Shape() { }

		Shape(const Shape<S> &ref)            : data(ref.data) { }
		///
		static Shape<S> rgba(int w, int h) {
			if constexpr (S == Major)
				return { h, w, 4 };
			assert(false);
			return { h, 4, w }; /// decision needed on this
		};
		///
		std::vector<ssize_t>::iterator begin() { return data->shape.begin(); }
		std::vector<ssize_t>::iterator end()   { return data->shape.end(); }
	};
	//}
///}

//export module str;
//export {
	typedef int ichar;

















	struct str2 {
	protected:
		//typedef sh<std::string> shared;
		//shared sh;
		std::string* sh = null;

	public:
		enum MatchType {
			Alpha,
			Numeric,
			WS,
			Printable,
			String,
			CIString
		};

		std::string& ref() const { static std::string ssh;  return ssh; }

		/// best way to serialize a string is to return the Shared
		/// Shared just hides many dependency quirks, so do this with all of the types.
		//inline operator Shared() { return sh; }
    
		// constructors
		//str2(shared &sh)				: sh(sh) { }
		str2()                          : sh(new std::string()) { }
		str2(nullptr_t n)               : sh(new std::string()) { }
		str2(cchar_t* cstr)             : sh(cstr ? new std::string(cstr)      : new std::string()) { }
		//str2(cchar_t* cstr, size_t len) : sh(cstr ? new std::string(cstr, len) : new std::string()) {
		//	if (!cstr && len) sh->reserve(len);
		//}
		//str2(const char      ch)      : sh(new std::string(&ch, 1))              { }
		//str2(size_t          sz)      : sh(new std::string())                    { sh->reserve(sz); }

		str2(std::string    str)      : sh(new std::string(str))                 { }
		//str2(const str2&s)             : sh(s.sh)                                 { }
	   //~str2() { }
	    

		/// methods
		//const char *cstr()                           const { return ref().c_str();                  }
		//bool        contains(array<str2>   a)      const { return index_of_first(a, null) >= 0;   }
		//size_t      size()                           const { return ref().length();                 }
    
		/// operators
		//str2      operator+  (const str2&s)         const { return ref() + s.ref();                }
		//str2      operator+  (const char      *s)  const { return ref() + s;                      }
		//str2      operator() (int s, int l)        const { return substr(s, l);                   }
		//str2      operator() (int s)               const { return substr(s);                      }
		//bool        operator<  (const str2&  rhs)  const { return ref() < rhs.ref();              }
		//bool        operator>  (const str2&  rhs)  const { return ref() > rhs.ref();              }
		//bool        operator== (const str2&    b)  const {
		//	bool bb = sh == ((str2&)b).sh;
		//	return sh == b.sh || ref() == b.ref();
		//}
		//bool        operator!= (const char* cstr)  const { return false; }
		bool        operator== (std::string    str)  const { return str == ref();                   }


		//void        operator+= (const char      ch)        { ref().push_back(ch);                   }
		const char &operator[] (size_t       index)  const { return ref()[index];                   }
		//			operator std::filesystem::path() const { return ref();                          }
		//			operator         std::string &() const { return ref();                          }
		//			operator                  bool() const { return  sh && *sh != "";               }
		//bool        operator!()						 const { return !(operator bool());             }

				  //operator          const char *() const { return ref().c_str(); } -- shes not safe to fly.

		//str2        &operator= (const str2 &s) {
		//	return *this;
		//}

		///
		static str2 read_file(fs::path f) {
			return null;
		}

		int index_of_first(array<str2> &a, int *a_index) const {
			return 0;
		}

		///
		bool starts_with(const char *cstr) const {
			return false;
		}

		///
		bool ends_with(const char *cstr) const {
			return false;
		}

		///
		static str2 read_file(std::ifstream& in) {
			return null;
		}
    
		///
		str2 to_lower() const {
			return null;
		}

		///
		str2 to_upper() const {
			return null;
		}
    
		static int nib_value(cchar_t c) {
			return 0;
		}
    
		static cchar_t char_from_nibs(cchar_t c1, cchar_t c2) {
			return 0;
		}

		str2 replace(str2 sfrom, str2 sto, bool all = true) const {
			return null;
		}

		str2 substr(int start, size_t len) const {
			return null;
		}

		str2 substr(int start) const {
			return null;
		}

		array<str2> split(str2 delim) const {
			return null;
		}

		array<str2> split(const char *delim) const {
			return null;
		}
    
		array<str2> split()         const { return null; }

		int index_of(const char *f) const { return 0; }
    
		int index_of(str2 s)        const { return 0; }


		int index_of(MatchType ct, const char *mp = null) const {
			return -1;
		}


		int index_icase(const char* f) const {
			return 0;
		}

		int64_t integer() const {
			return int64_t(0);
		}

		real realz() const {
			return strtod(sh->c_str(), null);
		}

		bool has_prefix(str2 i) const {
			return false;
		}

		bool numeric() const {
			return false;
		}
    
		/// wildcard match
		bool matches(str2 pattern) const {
			return false;
		}

		str2 trim() const {
			return null;
		}
	};

	//static inline str2 operator+(const char *cstr, str2 rhs) { return str2(cstr) + rhs; }

	// ref std::string in a Memory -> str resolution down here.

	template<> struct is_strable<str2> : std::true_type  {};





























	struct str {
	protected:
		typedef sh<std::string> shared;
		shared sh;

	public:
		enum MatchType {
			Alpha,
			Numeric,
			WS,
			Printable,
			String,
			CIString
		};

		std::string& ref() const { return *sh; }

		/// best way to serialize a string is to return the Shared
		/// Shared just hides many dependency quirks, so do this with all of the types.
		inline operator Shared() { return sh; }
    
		// constructors
		str(shared &sh)				: sh(sh) { }
		str()                        : sh(new std::string()) { }
		str(nullptr_t n)             : sh(new std::string()) { }
		str(cchar_t* cstr)           : sh(cstr ? new std::string(cstr)      : new std::string()) { }
		str(cchar_t* cstr, size_t len)  : sh(cstr ? new std::string(cstr, len) : new std::string()) {
			if (!cstr && len) sh->reserve(len);
		}
	  //str(const char      ch)      : sh(new std::string(&ch, 1))              { }
		str(size_t          sz)      : sh(new std::string())                    { sh->reserve(sz); }
		str(int32_t        i32)      : sh(new std::string(std::to_string(i32))) { }
		str(int64_t        i64)      : sh(new std::string(std::to_string(i64))) { }
		str(double         f64)      : sh(new std::string(std::to_string(f64))) { }
		str(std::string    str)      : sh(new std::string(str))                 { }
		str(const str&s)             : sh(s.sh)                                 { }
	   ~str() { }
	    
	    static int utf8(int code, uint8_t *res) {
			int n = 0;
			///
			lambda<void(uint8_t)> append = [&](uint8_t c) { res[n++] = c; };
			if (code <= 127)
				append(uint8_t(code));
			else {
				/// todo.
				assert(false);
			}
			return n;
	    }

		/// methods
		const char *cstr()                           const { return ref().c_str();                  }
		bool        contains(array<str>   a)      const { return index_of_first(a, null) >= 0;   }
		size_t      size()                           const { return ref().length();                 }
    
		/// operators
		str      operator+  (const str&s)         const { return ref() + s.ref();                }
		str      operator+  (const char      *s)  const { return ref() + s;                      }
		str      operator() (int s, int l)        const { return substr(s, l);                   }
		str      operator() (int s)               const { return substr(s);                      }
		bool        operator<  (const str&  rhs)  const { return ref() < rhs.ref();              }
		bool        operator>  (const str&  rhs)  const { return ref() > rhs.ref();              }
		bool        operator== (const str&    b)  const {
			bool bb = sh == ((str&)b).sh;
			return sh == b.sh || ref() == b.ref();
		}
		bool        operator!= (const char   *cstr)  const { return !operator==(cstr);              }
		bool        operator== (std::string    str)  const { return str == ref();                   }
		bool        operator!= (std::string    str)  const { return str != ref();                   }
		int         operator[] (str              s)  const { return index_of(s.cstr());             }
		void        operator+= (str              s)        { ref() += s.ref();                      }
		void        operator+= (const char   *cstr)        { ref() += cstr;                         }
		void		operator+= (ichar           ic)        {
			uint8_t buf[16];
			utf8(ic, buf);
			ref() += (const char *)buf;
		}

		void        operator+= (const char      ch)        { ref().push_back(ch);                   }
		const char &operator[] (size_t       index)  const { return ref()[index];                   }
					operator std::filesystem::path() const { return ref();                          }
					operator         std::string &() const { return ref();                          }
					operator                  bool() const { return  sh && *sh != "";               }
		bool        operator!()						 const { return !(operator bool());             }
					operator               int64_t() const { return integer();                      }
					operator                  real() const { return real();                         }
				  //operator          const char *() const { return ref().c_str(); } -- shes not safe to fly.
		str        &operator= (const str &s) {
			if (this != &s)
				sh = ((str &)s).sh;
			return *this;
		}

		
		// i guess silver inserts these too; i simply do not see why you double them up its such a bad feature of the language
		// stating that it could be 'either' const or non-const; say volitile.  can go either way lol.
		// that emits.  its just a silver thing; silver reflects.
		bool        operator==(const char* cstr)     const {
			auto& s = ref();
			if (!cstr)
				return s.length() == 0;
			///
			const char* s0 = s.c_str();
			if (cstr == s0) return 0;
			///
			for (size_t i = 0; ; i++)
				if (s0[i] != cstr[i])
					return false;
				else if (s0[i] == 0 || cstr[i] == 0)
					break;
			///
			return true;
		}

		friend std::ostream &operator<< (std::ostream& os, str const& m) { return os << m.ref(); }

		///
		static str fill(size_t n, lambda<int(size_t &)> fn) {
			auto ret = str(n);
			for (size_t i = 0; i < size_t(n); i++)
				ret += fn(i);
			return ret;
		}

		///
		static str read_file(fs::path f) {
			if (!fs::is_regular_file(f))
				return null;
			std::ifstream fs;
			fs.open(f);
			std::ostringstream sstr;
			sstr << fs.rdbuf();
			fs.close();
			return str(sstr.str());
		}

		/// a str of lambda results
		static str of(int count, lambda<char(size_t)> fn) {
			str s;
			std::string &str = s.ref();
			for (size_t i = 0; i < count; i++)
				str += fn(i);
			return s;
		}

		int index_of_first(array<str> &a, int *a_index) const {
			int less  = -1;
			int index = -1;
			for (str &ai:a) {
				  ++index;
				int i = index_of(ai);
				if (i >= 0 && (less == -1 || i < less)) {
					less = i;
					if (a_index)
					   *a_index = index;
				}
			}
			if  (a_index)
				*a_index = -1;
			return less;
		}

		///
		bool starts_with(const char *cstr) const {
			size_t l0 = strlen(cstr);
			size_t l1 = size();
			if (l1 < l0)
				return false;
			return memcmp(cstr, sh->c_str(), l0) == 0;
		}

		///
		bool ends_with(const char *cstr) const {
			size_t l0 = strlen(cstr);
			size_t l1 = size();
			if (l1 < l0)
				return false;
			const char *end = &(sh->c_str())[l1 - l0];
			return memcmp(cstr, end, l0) == 0;
		}

		///
		static str read_file(std::ifstream& in) {
			str s;
			std::ostringstream sstr;
			sstr << in.rdbuf();
			s = sstr.str();
			return s;
		}
    
		///
		str to_lower() const {
			std::string c = ref();
			std::transform(c.begin(), c.end(), c.begin(), ::tolower); // transform and this style of api is more C than C++
			return c;
		}

		///
		str to_upper() const {
			std::string c = ref();
			std::transform(c.begin(), c.end(), c.begin(), ::toupper);
			return c;
		}
    
		static int nib_value(cchar_t c) {
			return (c >= '0' and c <= '9') ? (     c - '0') :
				   (c >= 'a' and c <= 'f') ? (10 + c - 'a') :
				   (c >= 'A' and c <= 'F') ? (10 + c - 'A') : -1;
		}
    
		static cchar_t char_from_nibs(cchar_t c1, cchar_t c2) {
			int nv0 = nib_value(c1);
			int nv1 = nib_value(c2);
			return (nv0 == -1 || nv1 == -1) ? ' ' : ((nv0 * 16) + nv1);
		}

		str replace(str sfrom, str sto, bool all = true) const {
			size_t start_pos = 0;
			std::string  str = *sh;
			std::string  &fr = *sfrom.sh;
			std::string  &to = *sto.sh;
			while((start_pos = str.find(fr, start_pos)) != std::string::npos) {
				str.replace(start_pos, fr.length(), to);
				start_pos += to.length();
				if (!all)
					break;
			}
			return str;
		}

		str substr(int start, size_t len) const {
			std::string &s = ref();
			return start >= 0 ? s.substr(size_t(start), len) :
				   s.substr(size_t(std::max(0, int(s.length()) + start)), len);
		}

		str substr(int start) const {
			std::string &s = ref();
			return start >= 0 ? s.substr(size_t(start)) :
				   s.substr(size_t(std::max(0, int(s.length()) + start)));
		}

		array<str> split(str delim) const {
			array<str>    result;
			size_t        start = 0, end, delim_len = delim.size();
			std::string  &s     = ref();
			std::string  &ds    = delim.ref();
			///
			if (s.length() > 0) {
				while ((end = s.find(ds, start)) != std::string::npos) {
					std::string token = s.substr(start, end - start);
					start = end + delim_len;
					result += token;
				}
				auto token = s.substr(start);
				result += token;
			} else
				result += std::string("");
			return result;
		}

		array<str> split(const char *delim) const {
			return split(str(delim));
		}
    
		array<str> split() const {
			array<str> result;
			str chars = "";
			for (const char &c: ref()) {
				bool is_ws = isspace(c) || c == ',';
				if (is_ws) {
					if (chars) {
						result += chars;
						chars   = "";
					}
				} else
					chars += c;
			}
			if (chars || !result)
				result += chars;
			return result;
		}

		int index_of(const char *f) const {
			std::string::size_type loc = ref().find(f, 0);
			return (loc != std::string::npos) ? int(loc) : -1;
		}
    
		int index_of(str s)     const { return index_of(s.cstr()); }


		int index_of(MatchType ct, const char *mp = null) const {
			typedef lambda<bool(const char &)> Fn;
			typedef std::unordered_map<MatchType, Fn> Map;
			///
			int index = 0;
			char *mp0 = (char *)mp;
			if (ct == CIString) {
				size_t sz = size();
				mp0       = new char[sz + 1];
				memcpy(mp0, cstr(), sz);
				mp0[sz]   = 0;
			}
			auto   test = [&](const char& c) -> bool { return  false; };
			auto f_test = lambda<bool(const char&)>(test);

			/*auto m = Map{
				{ Alpha,     [&](const char &c) -> bool { return  isalpha (c);            }},
				{ Numeric,   [&](const char &c) -> bool { return  isdigit (c);            }},
				{ WS,        [&](const char &c) -> bool { return  isspace (c);            }}, // lambda must used copy-constructed syntax
				{ Printable, [&](const char &c) -> bool { return !isspace (c);            }},
				{ String,    [&](const char &c) -> bool { return  strcmp  (&c, mp0) == 0; }},
				{ CIString,  [&](const char &c) -> bool { return  strcmp  (&c, mp0) == 0; }}
			};*/
			Fn zz;
			Fn& fn = zz;// m[ct];
			for (const char c:ref()) {
				if (fn(c))
					return index;
				index++;
			}
			if (ct == CIString)
				delete mp0;
        
			return -1;
		}


		int index_icase(const char* f) const {
			const std::string fs = f;
			auto& s = ref();
			auto it = std::search(s.begin(), s.end(), fs.begin(), fs.end(),
				[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
			return int((it == s.end()) ? -1 : it - s.begin());
		}

		int64_t integer() const {
			auto &s = ref();
			const char *c = s.c_str();
			while (isalpha(*c))
				c++;
			return s.length() ? int64_t(strtol(c, null, 10)) : int64_t(0);
		}

		real real() const {
			auto &s = ref();
			const char *c = s.c_str();
			while (isalpha(*c))
				c++;
			return strtod(c, null);
		}

		bool has_prefix(str i) const {
			auto    &s = ref();
			size_t isz = i.size();
			size_t  sz =   size();
			return sz >= isz ? strncmp(s.c_str(), i.cstr(), isz) == 0 : false;
		}

		bool numeric() const {
			auto &s = ref();
			return s != "" && (s[0] == '-' || isdigit(s[0]));
		}
    
		/// wildcard match
		bool matches(str pattern) const {
			auto &s = ref();
			lambda<bool(char *, char *)> s_cmp; // well this doesnt work AT toll. i will try
			///
			s_cmp = [&](char *str, char *pat) -> bool {
				if (!*pat)
					return true;
				if (*pat == '*')
					return (*str && s_cmp(str + 1, pat)) ? true : s_cmp(str, pat + 1);
				return (*pat == *str) ? s_cmp(str + 1, pat + 1) : false;
			};
			///
			for (char *str = (char *)s.c_str(), *pat = (char *)pattern.cstr(); *str != 0; str++)
				if (s_cmp(str, pat))
					return true;
			///
			return false;
		}

		str trim() const {
			auto       &s = ref();
			auto   start  = s.begin();
			while (start != s.end() && std::isspace(*start))
				   start++;
			auto end = s.end();
			do { end--; } while (std::distance(start, end) > 0 && std::isspace(*end));
			return std::string(start, end + 1);
		}
	};

	static inline str operator+(const char *cstr, str rhs) { return str(cstr) + rhs; }

	// ref std::string in a Memory -> str resolution down here.

	template<> struct is_strable<str> : std::true_type  {};

	template <typename T>
	Shared *str_ref(T *ps) {
		return ps ? &(Shared&)str(*ps) : null;
	}

	///
	struct Symbol {
		int   value;
		str   symbol;
		bool operator==(int v) { return v == value; }
	};

	typedef array<Symbol> Symbols;
	typedef array<str>    Strings;

	/*
	namespace std {
		template<> struct hash<str> {
			size_t operator()(str const& s) const { return hash<std::string>()(std::string(s)); }
		};
	}

	namespace std {
		template<> struct hash<path_t> {
			size_t operator()(path_t const& p) const { return hash<std::string>()(p.string()); }
		};
	}*/
//};

//export module map;
//export {

	template <typename K, typename V>
	struct map:io {
		typedef std::vector<pair<K,V>> vpairs;
		sh<vpairs>                     arr;
		static typename vpairs::iterator iterator;

		///
		vpairs &realloc(size_t sz) {
			arr = new vpairs();
			if (sz)
				arr->reserve(sz);
			return *arr;
		}

		map(std::nullptr_t n = null)            { realloc(0);         }
		map(size_t               sz)            { realloc(sz);        }
		void reserve(size_t      sz)            { arr->reserve(sz); }
		void clear(size_t    sz = 0)            { arr->clear(); if (sz) arr->reserve(sz); }
		map(std::initializer_list<pair<K,V>> p) {
			realloc(p.size());
			for (auto &i: p) arr->push_back(i);
		}

		map(const map<K,V> &p) {
			realloc(p.size());
			for (auto &i: *p.arr)
				arr->push_back(i);
		}
    
		inline int index(K k) const {
			int index = 0;
			for (auto &i: *arr) {
				if (k == i.key)
					return index;
				index++;
			}
			return -1;
		}
    
		size_t count(K k) const { return index(k) >= 0 ? 1 : 0; }
		V &operator[](const char *k) {
			K kv = k;
			for (auto &i: *arr)
				if (kv == i.key)
					return i.value;
			arr->push_back(pair <K,V> { kv });
			return arr->back().value;
		}
		V &operator[](K k) {
			for (auto &i: *arr)
				if (k == i.key)
					return i.value;
			arr->push_back(pair <K,V> { k, V() });
			return arr->back().value;
		}
    
		inline operator Shared&() const { return arr;             }
		inline V          &back()       { return arr->back();     }
		inline V         &front()       { return arr->front();    }
	
		inline V*         data()  const { return (V *)arr->data()->ptr; }
		inline vpairs::iterator begin() { return arr->begin();    }
		inline vpairs::iterator   end() { return arr->end();      }
		inline size_t     size()  const { return arr->size();     }
		inline size_t capacity()  const { return arr->capacity(); }
		inline bool      erase(K k) {
			size_t index = 0;
			for (auto &i: *arr) {
				if (k == i.key) {
					arr->erase(arr->begin() + index);
					return true;
				}
				index++;
			}
			return false;
		}

		inline bool operator!=(map<K,V> &b) { return !operator==(b);    }
		inline operator bool()              { return arr->size() > 0; }
		inline bool operator==(map<K,V> &b) {
			if (size() != b.size())
				return false;
			for (auto &[k,v]: b)
				if ((*this)[k] != v)
					return false;
			return true;
		}
	};

	template<typename K, typename V> struct is_map <map<K,V>>   : std::true_type  { };
	template<typename V>			 struct is_array <array<V>> : std::true_type  { };
///}

/// ---------------------------------------------------------------------------
//export module path;
//export {
	void chdir(std::string c) {
		// winbase.h:
		// 
		//SetCurrentDirectory(c.c_str());
		// call ::chdir if not windows, SetCurrentDirectory on Windows
	}

	struct dir {
		fs::path prev;
		dir(fs::path p) : prev(fs::current_path()) { chdir(p.string().c_str()); }
		~dir() { chdir(prev.string().c_str()); }
	};

	/// var doesnt use path, better to define that one first.
	struct Path {
		typedef lambda<void(Path)> Fn;

		enum Flags {
			Recursion,
			NoSymLinks,
			NoHidden,
			UseGitIgnores
		};

		path_t p;

		str               stem() const { return p.stem().string(); }
		static Path        cwd()       { return fs::current_path(); }
		bool operator==(Path& b) const { return p == b.p; }
		bool operator!=(Path& b) const { return p != b.p; }

		Path(fs::directory_entry ent) : p(ent.path()) { }

		Path(std::nullptr_t n = null) { }
		Path(cchar_t*       p) : p(p) { }
		Path(path_t         p) : p(p) { }
		Path(str            s) : p(s) { }

		cchar_t*  cstr()  const { return    (cchar_t*)p.c_str(); }
		str        ext()        { return p.extension().string(); }
		Path      file()        { return fs::is_regular_file(p) ? Path(p) : Path(null); }

		bool       copy(Path to) const {
			/// no recursive implementation at this point
			assert(is_file() && exists());
			if (!to.exists())
				(to.has_filename() ? to.remove_filename() : to).make_dir();
			std::error_code ec;
			fs::copy(p, to.is_dir() ? to / p.filename() : to, ec);
			return ec.value() == 0;
		}

		Path link(Path alias) const {
			std::error_code ec;
			fs::create_symlink(p, alias, ec);
			return alias.exists() ? alias : null;
		}

		bool make_dir() const {
			std::error_code ec;
			return fs::create_directories(p, ec);
		}

		/// no mutants allowed here.
		Path remove_filename()       { return p.remove_filename(); }
		bool    has_filename() const { return p.has_filename(); }
		Path            link() const { return fs::is_symlink(p) ? Path(p) : Path(null); }
		operator        bool() const { return p.string().length() > 0; }
		operator      path_t() const { return p; }
		operator         str() const { return p.string(); }

		static Path uid(Path base) {
			auto rand = lambda<str(size_t)>( [](size_t i) { return Rand::uniform('a', 'z'); } );
			Path path = null;
			do {
				path  = str::fill(6, rand);
			}
			while ((base / path).exists());
			return path;
		}

		bool remove_all() const { std::error_code ec;
			return fs::remove_all(p, ec);
		}

		bool     remove() const { std::error_code ec;
			return fs::remove(p,     ec);
		}

		bool  is_hidden() const {
			std::string st = p.stem().string();
			return st.length() > 0 && st[0] == '.';
		}

		bool  exists()                    const { return  fs::         exists(p); }
		bool  is_dir()                    const { return  fs::   is_directory(p); }
		bool is_file()                    const { return !fs::   is_directory(p) && 
														  fs::is_regular_file(p); }
		Path operator / (cchar_t*      s) const { return Path(p / s); }
		Path operator / (const path_t& s) const { return Path(p / s); }
	  //Path operator / (const stsr    &s)       { return Path(p / path_t(s));   }
		Path relative(Path from)				{ return fs::relative(p, from.p); }

		int64_t modified_at() const {
			using namespace std::chrono_literals;
			std::string ps = p.string();
			auto        wt = fs::last_write_time(p);
			const auto  ms = wt.time_since_epoch().count(); // need a defined offset. 
			return int64_t(ms);
		}

		bool read(size_t bs, lambda<void(const char*, size_t)> fn) {
			try {
				/// this should be independent of any io caching; it should work down to a second
				std::error_code ec;
				size_t rsize = fs::file_size(p, ec);
				/// no exceptions
				if (!ec)
					return false;
				///
				std::ifstream f(p);
				char* buf = new char[bs];
				for (size_t i = 0, n = (rsize / bs) + (rsize % bs != 0); i < n; i++) {
					size_t sz = i == (n - 1) ? rsize - (rsize / bs * bs) : bs;
					fn((const char*)buf, sz);
				}
				delete[] buf;
			}
			catch (std::ofstream::failure e) {
				std::cerr << "read failure on resource: " << p.string();
			}
			return true;
		}

		bool write(array<uint8_t> bytes) {
			try {
				size_t        sz = bytes.size();
				std::ofstream f(p, std::ios::out | std::ios::binary);
				if (sz)
					f.write((const char*)bytes.data(), sz);
			}
			catch (std::ofstream::failure e) {
				std::cerr << "read failure on resource: " << p.string() << std::endl;
			}
			return true;
		}

		bool append(array<uint8_t> bytes) {
			try {
				size_t        sz = bytes.size();
				std::ofstream f(p, std::ios::out | std::ios::binary | std::ios::app);
				if (sz)
					f.write((const char*)bytes.data(), sz);
			}
			catch (std::ofstream::failure e) {
				std::cerr << "read failure on resource: " << p.string() << std::endl;
			}
			return true;
		}

		bool same_as(Path b) const { std::error_code ec; return fs::equivalent(p, b, ec); }

		void resources(array<str> exts, FlagsOf<Flags> flags, Fn fn) {
			bool use_gitignore	= flags[UseGitIgnores];
			bool recursive		= flags[Recursion];
			bool no_hidden		= flags[NoHidden];
			auto ignore			= flags[UseGitIgnores] ? str::read_file(p / ".gitignore").split("\n") : array<str>();

			lambda<void(Path)> res;
			
			map<Path, bool> fetched_dir; // this is temp and map needs a hash table pronto
			path_t parent		= p; /// parent relative to the git-ignore index; there may be multiple of these things.
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
						for (auto& i : ignore)
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
				if (p.is_dir()) {
					if (!no_hidden || !p.is_hidden())
						for (Path p : fs::directory_iterator(p)) {
							Path link = p.link();
							if (link)
								continue;
							Path pp = link ? link : p;
							if (recursive && pp.is_dir()) {
								if (fetched_dir.count(pp) > 0)
									return;
								fetched_dir[pp] = true;
								res(pp);
							}
							///
							if (pp.is_file())
								fn_filter(pp);
						}
				}
				else if (p.exists())
					fn_filter(p);
			};
			return res(p);
		}

		array<Path> matching(array<str> exts) {
			auto ret = array<Path>();
			resources(exts, {}, [&](Path p) { ret += p; });
			return ret;
		}
	};

	/*
	namespace std {
		template<> struct hash<Path> {
			size_t operator()(Path const& p) const { return hash<std::string>()(p); }
		};
	}
	*/

	struct SymLink {
		str alias;
		Path path;
	};

	/// these file ops could effectively be 'watched'
	struct Temp {
		Path path;

		///
		Temp(Path  base = Path("/tmp")) {
			path_t p_base = base;
			assert(base.exists());
			///
			Path   rand = Path::uid(base);
			path        = base / rand;
			fs::create_directory(path);
		}

		///
		~Temp() { path.remove_all(); }

		///
		inline void operator += (SymLink sym) {
			if (sym.path.is_dir())
				fs::create_directory_symlink(sym.path, sym.alias);
			else if (sym.path.exists())
				assert(false);
		}

		///
		Path operator / (cchar_t*      s) { return path / s; }
		Path operator / (const path_t& s) { return path / s; }
		Path operator / (const str&    s) { return path / path_t(s); }
	};
//};


//export module var;
//export {

	struct base64 {
		static map<size_t, size_t> b64_encoder() {
			auto mm = map<size_t, size_t>();
			/// --------------------------------------------------------
			for (size_t i = 0; i < 26; i++) mm[i]          = size_t('A') + size_t(i);
			for (size_t i = 0; i < 26; i++) mm[26 + i]     = size_t('a') + size_t(i);
			for (size_t i = 0; i < 10; i++) mm[26 * 2 + i] = size_t('0') + size_t(i);
			/// --------------------------------------------------------
			mm[26 * 2 + 10 + 0] = size_t('+');
			mm[26 * 2 + 10 + 1] = size_t('/');
			/// --------------------------------------------------------
			return mm;
		}


		/// base64 validation/value map
		static map<size_t, size_t> b64_decoder() {
			auto m = map<size_t, size_t>();

			for (size_t i = 0; i < 26; i++) m['A' + i] = i;
			for (size_t i = 0; i < 26; i++) m['a' + i] = 26 + i;
			for (size_t i = 0; i < 10; i++) m['0' + i] = 26 * 2 + i;

			m['+'] = 26 * 2 + 10 + 0;
			m['/'] = 26 * 2 + 10 + 1;
			return m;
		}

		static sh<uint8_t> encode(cchar_t* data, size_t len) {
			auto        m = b64_encoder();
			size_t  p_len = ((len + 2) / 3) * 4;
			auto  encoded = sh<uint8_t>::valloc(p_len + 1);
			uint8_t*    e = encoded.pointer<uint8_t>();
			size_t      b = 0;
			for (size_t i = 0; i < len; i += 3) {
				*(e++) = uint8_t(m[data[i] >> 2]);
				if (i + 1 <= len) *(e++) = uint8_t(m[((data[i]     << 4) & 0x3f) | data[i + 1] >> 4]);
				if (i + 2 <= len) *(e++) = uint8_t(m[((data[i + 1] << 2) & 0x3f) | data[i + 2] >> 6]);
				if (i + 3 <= len) *(e++) = uint8_t(m[  data[i + 2]       & 0x3f]);
				b += std::min(size_t(4), len - i + 1);
			}
			for (size_t i = b; i < p_len; i++)
				*(e++) = '=';
			*e = 0;
			return encoded;
		}

		static sh<uint8_t> decode(cchar_t* b64, size_t b64_len, size_t* alloc_sz) {
			assert(b64_len % 4 == 0);
			/// --------------------------------------
			auto          m = b64_decoder();
			*alloc_sz       = b64_len / 4 * 3;
			sh<uint8_t> out = sh<uint8_t>::valloc(*alloc_sz + 1);
			uint8_t*      o = out.pointer<uint8_t>();
			size_t        n = 0;
			size_t        e = 0;
			/// --------------------------------------
			for (size_t ii = 0; ii < b64_len; ii += 4) {
				size_t a[4], w[4];
				for (int i = 0; i < 4; i++) {
					bool is_e = a[i] == '=';
					size_t ch = b64[ii + i];
					a[i]      = ch;
					w[i]      = is_e ? 0 : m[ch];
					if (a[i] == '=')
						e++;
				}
				uint8_t    b0 = uint8_t((w[0] << 2) | (w[1] >> 4));
				uint8_t    b1 = uint8_t((w[1] << 4) | (w[2] >> 2));
				uint8_t    b2 = uint8_t((w[2] << 6) | (w[3] >> 0));
				if (e <= 2) o[n++] = b0;
				if (e <= 1) o[n++] = b1;
				if (e <= 0) o[n++] = b2;
			}
			assert(n + e == *alloc_sz);
			o[n] = 0;
			return out;
		}
	};






















	struct var {
	protected:
		static var vconsts[22];
	public:
		Shared  d;
		str     s; // attach!

		var(nullptr_t n = null) { }
		var(Type *t) : d(Memory::valloc(t, 1)) { }

		template <typename T>
		var(T v = null) {
			if        constexpr(std::is_same_v<T, Type*>) {
				assert(false);
			}
			if        constexpr(std::is_same_v<T, var>) {
				// copy-constructor case
				d = v.d;
				s = v.s;
			} else if constexpr(std::is_same_v<T, std::nullptr_t>) {
				// null / default case
				d = null;
			} else if constexpr(is_array<T>()) {
				d = v.a;
			} else if constexpr(is_map<T>()) {
				d = v.arr;
			} else {
				// set to data case
				d = new T(v);
			}
		}

		var &vconst_for(size_t id) {
			static bool init;
			if (!init) {
				 init       = true;
				 vconsts[0] = var(Type::Undefined);
			}
			return vconsts[id];
		}

		size_t size() const {
			return d.count();
		}

		Type* type() const {
			return d.type();
		}

		template <typename T>
		var& operator=(T v) {
			d = d.quantum().assign(v);
			return *this;
		}

		var &operator[](str name) {
			if (d.type() == Type::Map) {
				auto re = d.reorient<map<str, var>>({ Trait::TMap });
				return re[name];
			}
			return vconst_for(0);
		}

		var &operator[](size_t index) {
			if (d.type() == Type::Array) {
				/// trait-enabled re-orientation
				//array<var> &c = d.reorient<array<var>>({ Trait::TArray });
				//return c[index];
			}
			return vconst_for(0);
		}

		/// 
		static        str format(str t, array<var> &p) {
			for (size_t k = 0; k < p.size(); k++) {
				// needs a lambda handle for prefix processing, iterate through string finding these { expr }
				// this is terrible currently, theres no protocol its just a simple replacer.
				str  p0 = str("{") + str(std::to_string(k)) + str("}");
				str  p1 = str(p[k]);
				t = t.replace(p0, p1);
			}
			return t;
		}

		int operator==(var& v) { return 0; }
		int operator!=(var& v)  { return 0; }
		operator       float () { return d.real   <  float >(); }
		operator      double () { return d.real   < double >(); }
		operator      int8_t () { return d.integer<  int8_t>(); }
		operator     uint8_t () { return d.integer< uint8_t>(); }
		operator     int16_t () { return d.integer< int16_t>(); }
		operator    uint16_t () { return d.integer<uint16_t>(); }
		operator     int32_t () { return d.integer< int32_t>(); }
		operator    uint32_t () { return d.integer<uint32_t>(); }
		operator     int64_t () { return d.integer< int64_t>(); }
		operator    uint64_t () { return d.integer<uint64_t>(); }
		operator         str () { Type*  t = d.type(); return (t == Type::Str) ? str(*d.string()) : str(*t->string(d)); }
		operator      Shared&() { return d; }
	};

	void test() {
		
		map<str, var> m = {};
		m.size();
		//m["test"] = null;
		map<str, var>& cc91 = (map<str, var>  &) * (map<str, var>*)0;
		cc91.size();

		///
		map<str2, var>& cc9 = (map<str2, var> &) * (map<str2, var>*)0;
		cc9.size();
	}

	///
	struct Time {
		int64_t utc = 0;
		
		Time(nullptr_t n = null)                { }
		Time(int v)           : utc(int64_t(v)) { }
		Time(int64_t utc)     : utc(utc)        { }
		Time(const Time& ref) : utc(ref.utc)    { }
		
		Time& operator=(const Time& ref)        { utc = ref.utc; return *this; }
		operator   bool()          const        { return utc != 0; }
	};

	inline Time ticks() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	struct Period {
		Time    s, e;
		///
		Time      def (Time t)     { return t ? t : ticks(); }
		void    start (Time t = 0) { s = def(t); }
		void   finish (Time t = 0) { e = def(t); }
		real      pos (Time t = 0) { return (e != s) ? real(def(t).utc - s.utc) / real(e - s) : 0; }
		real  clamped (Time t = 0) { return std::clamp(pos(t), 0.0, 1.0); }
		///
		bool contains (Time t, Time e) { return ((t.utc + e.utc) >= s.utc && (t.utc - e.utc) < s.utc); }

		Period(nullptr_t n = null)					    { }
		Period(Time  s,  Time   e) : s(s),     e(e)     { }
		Period(const Period&  ref) : s(ref.s), e(ref.e) { }
		///
		Period& operator=(const Period& ref) {
			if (this != &ref) {
				s = ref.s;
				e = ref.e;
			}
			return *this;
		}
	};

	/// 
	struct Event {
		Shared result; // value & type
		Period period;
		///
		Event(nullptr_t     n = null) { }
		Event(bool success, Period p) : result(Memory::copy(success, Id<Event>())), period(p) { }
		Event(const         Event& r) : result(r.result), period(r.period) { }
		///
		Event &operator=(const Event& r) {
			if (this != &r) {
				result = r.result;
				period = r.period;
			}
			return *this;
		}
		///
		operator bool() const { return result; }
	};

	/// better json/bin interface first.
	struct json {
		// load
		static var load(Path p) {
			return null;
		};

		static Event save() {
			return {};
		};
	};

	template <typename T> static T input() { T v; std::cin >> v; return v; }

	enum LogType {
		Dissolvable,
		Always
	};

	/// could have a kind of Enum for LogType and LogDissolve, if fancied.
	typedef std::function<void(str&)> FnPrint;

	template <const LogType L>
	struct Logger {
		FnPrint printer;
		Logger(FnPrint printer = null) : printer(printer) { }
		///
	protected:
		void intern(str& t, array<var>& a, bool err) {
			t = var::format(t, a);
			if (printer != null)
				printer(t);
			else {
				auto& out = err ? std::cout : std::cerr;
				out << std::string(t) << std::endl << std::flush;
			}
		}

	public:

		/// print always prints
		inline void print(str t, array<var> a = {}) { intern(t, a, false); }

		/// log categorically as error; do not quit
		inline void error(str t, array<var> a = {}) { intern(t, a, true); }

		/// log when debugging or LogType:Always, or we are adapting our own logger
		inline void log(str t, array<var> a = {}) {
			if (L == Always || printer || is_debug())
				intern(t, a, false);
		}

		/// assertion test with log out upon error
		inline void assertion(bool a0, str t, array<var> a = {}) {
			if (!a0) {
				intern(t, a, false); /// todo: ion:dx environment
				assert(a0);
			}
		}

		void _print(str t, array<var>& a, bool error) {
			t = var::format(t, a);
			auto& out = error ? std::cout : std::cerr;
			out << str(t) << std::endl << std::flush;
		}

		/// log error, and quit
		inline void fault(str t, array<var> a = {}) {
			_print(t, a, true);
			assert(false);
			exit(1);
		}

		/// prompt for any type of data
		template <typename T>
		inline T prompt(str t, array<var> a = {}) {
			_print(t, a, false);
			return input<T>();
		}
	};

#if !defined(WIN32) && !defined(WIN64)
#define UNIX /// its a unix system.  i know this.
#endif

	/// adding these declarations here.  dont mind me.
	enum KeyState {
		KeyUp,
		KeyDown
	};

	struct KeyStates {
		bool shift;
		bool ctrl;
		bool meta;
	};

	struct KeyInput {
		int key;
		int scancode;
		int action;
		int mods;
	};

	enum MouseButton {
		LeftButton,
		RightButton,
		MiddleButton
	};

	/// needs to be ex, but no handlers written yet
	/*
	enum KeyCode {
		Key_D       = 68,
		Key_N       = 78,
		Backspace   = 8,
		Tab         = 9,
		LineFeed    = 10,
		Return      = 13,
		Shift       = 16,
		Ctrl        = 17,
		Alt         = 18,
		Pause       = 19,
		CapsLock    = 20,
		Esc         = 27,
		Space       = 32,
		PageUp      = 33,
		PageDown    = 34,
		End         = 35,
		Home        = 36,
		Left        = 37,
		Up          = 38,
		Right       = 39,
		Down        = 40,
		Insert      = 45,
		Delete      = 46, // or 127 ?
		Meta        = 91
	};
	*/
	extern Logger<LogType::Dissolvable> console;

//export module model;
//export {
	typedef map<str, var> Schema;
	typedef map<str, var> SMap;
	typedef array<var>     Table;
	typedef map<str, var>  ModelMap;

	int abc() {
		var test = var(0);
		var  *mm = new var(Type::Map);

		map<str2, var> m1 = {};
		return 0;
	}


	// ion dx
	var ModelDef(str name, Schema schema) {
		var m    = schema;
			m.s  = name;
		return m;
	}

	struct Remote {
		int64_t value;
		str     remote;

		Remote(int64_t value)                  : value(value)                 { }
		Remote(std::nullptr_t n = nullptr)     : value(-1)                    { }
		Remote(str remote, int64_t value = -1) : value(value), remote(remote) { }

		operator int64_t() { return value; }
		operator     var() {
			if    (remote) {
				var m             = var(Type::Map); /// invalid state used as trigger.
					m.s           = str(remote);
					m[{"resolves"}] = var(); // var(remote);
				return m;
			}
			return var(value);
		}
	};

	struct Ident   : Remote {
		   Ident() : Remote(null) { }
	};

	struct node;
	struct Raw {
		std::vector<std::string> fields;
		std::vector<std::string> values;
	};

	typedef std::vector<Raw>                        Results;
	typedef std::unordered_map<std::string, var>    Idents;
	typedef std::unordered_map<std::string, Idents> ModelIdents;

	struct ModelCache {
		Results       results;
		Idents        idents; /// lookup fmap needed for optimization
	};

	struct ModelContext {
		ModelMap      models;
		std::unordered_map<std::string, std::string> first_fields;
		std::unordered_map<std::string, ModelCache *> mcache;
		var           data;
	};

	struct Adapter {
		ModelContext &ctx;
		str           uri;
		Adapter(ModelContext &ctx, str uri) : ctx(ctx), uri(uri) { }
		virtual var record(var &view, int64_t primary_key  = -1) = 0;
		virtual var lookup(var &view, int64_t primary_key) = 0;
		virtual bool reset(var &view) = 0;
	};

//}


	
/// ---------------------------------------------------------------------------
//export module unit;
//export {

	/// M type.
	/// structs, or member structs are simply reference bound types.
	/// some safety of declaration could be known at compile-time too
	template <typename C>
	struct M:Shared {
		M() {
		}

		virtual Shared declare() {
			int test = 0;
			test++;
		}

		template <typename T>
		static T& member(cchar_t *name) {
			// this set up the the member name and ordering, you could even get
			static auto stub = []() -> void {
				int test = 0;
				test++;
			};
			stub();
		}
	};

	struct Testy: M<Testy> {
		str& a = member<str>("");
		str& b = member<str>(""); // you can construct such a thing with this.  i know it. its just a matter of order and count is all.
		// its simple but its exciting simple.

		Shared declare() {
			// i like this because we can put it in a functor static
			return null;
		}
	};

	template <typename T>
	const T nan() { return std::numeric_limits<T>::quiet_NaN(); };

	struct Unit:io {
		enum Attrib {
			Standard,
			Metric,
			Percent,
			Time,
			Distance
		};
    
		///
		static std::unordered_map<str, int> u_flags;

		str             type   = null;
		real            value  = nan<real>();
		int64_t         millis = 0;
		FlagsOf<Attrib> flags  = null; /// FlagsOf makes flags out of enums, just so you dont define with bit pattern but keep to enum
    
		///
		void init() {
			/*
			Attrib a = 0;
			if (type && !std::isnan(value)) {
				if (type == "s" || type == "ms") {
					millis = int64_t(type == "s" ? value * 1000.0 : value);
					flags += Time;
				}
			}
			if (type && flags.count(type))
				flags += Attrib(u_flags[type]);
			*/
		}
    
		///
		Unit(cchar_t *s = null) : Unit(str(s)) { }
		Unit(var &v) {
			value = v[{"value"}];
			type  = v[{"type"}];
			init();
		}
    
		///
		Unit(str s) {
			int  i = s.index_of(str::Numeric);
			int  a = s.index_of(str::Alpha);
			if (i >= 0 || a >= 0) {
				if (a == -1) {
					value = s.substr(i).trim().real();
				} else {
					if (a > i) {
						if (i >= 0)
							value = s.substr(0, a).trim().real();
						type  = s.substr(a).trim().to_lower();
					} else {
						type  = s.substr(0, a).trim().to_lower();
						value = s.substr(a).trim().real();
					}
				}
			}
			init();
		}
    
		///
		bool operator==(cchar_t *s) { return type == s; }
    
		///
		void assert_types(array<str> &types, bool allow_none) {
			console.assertion((allow_none && !type) || types.index_of(type) >= 0, "unit not recognized");
		}
		///
		operator str  &() { return type;  }
		operator real &() { return value; }
    
		//operator var() {
		//	return Map {
		//		{"type",  var(type)},
		//		{"value", var(value)}
		//	};
		//}
	};
//}

//export module vec;
//export {

	template <typename T>
	struct Vector {
		explicit operator bool() const {
			if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>)
				return !std::isnan(*(T *)this);
			return true;
		}
		bool operator!() const { return !(operator bool()); }

		inline T &operator[](size_t i) const {
			T *v = (T *)this;
			return v[i];
		}
	};

	template <typename T>
	static inline T decimal(T x, size_t dp) {
		T sc = std::pow(10, dp);
		return std::round(x * sc) / sc;
	}

	template <typename T>
	struct Vec4;

	template <typename T>
	struct Vec2: Vector<T> {
		alignas(T) T x, y;
		Vec2(std::nullptr_t n = nullptr) : x(nan<T>()) { }
		Vec2(T x)      : x(x), y(x) { }
		Vec2(T x, T y) : x(x), y(y) { }
    
		Vec4<T> xxyy();
    
		friend auto operator<<(std::ostream& os, Vec2<T> const& m) -> std::ostream& {
			return os << "[" << decimal(m.x, 4) << "," << decimal(m.y, 4) << "]";
		}
    
		static Vec2<T> &null() {
			static Vec2<T> *n = nullptr;
			if (!n)
				 n = new Vec2<T>(nullptr);
			return *n;
		}
    
		operator size_t() const {
			return size_t(x) * size_t(y);
		}
    
		Vec2(var &d) {
			auto t = d.type();
			if (d.size() < 2) {
				x = nan<T>();
			} else if (t == Type::Array) {
				x = T(d[size_t(0)]);
				y = T(d[size_t(1)]);
			} else if (t == Type::Map) {
				x = T(d["x"]);
				y = T(d["y"]);
			} else {
				assert(false);
			}
		}
    
		static void aabb(array<Vec2<T>> &a, Vec2<T> &v_min, Vec2<T> &v_max) {
			const int sz = a.size();
			v_min        = a[0];
			v_max        = a[0];
			for (int i   = 1; i < sz; i++) {
				auto &v  = a[i];
				v_min    = v_min.min(v);
				v_max    = v_max.max(v);
			}
		}
    
		T xs() { return x * x; }
		T ys() { return y * y; }
    
		inline Vec2<T> abs() {
			if constexpr (std::is_same_v<T, int>)
				return *this;
			return Vec2<T> { std::abs(x), std::abs(y) };
		}
    
		inline Vec2<T> floor() {
			if constexpr (std::is_same_v<T, int>)
				return *this;
			return Vec2<T> { std::floor(x), std::floor(y) };
		}
    
		inline Vec2<T> ceil() {
			if constexpr (std::is_same_v<T, int>)
				return *this;
			return Vec2<T> { std::ceil(x),  std::ceil(y) };
		}
    
		inline Vec2<T> clamp(Vec2<T> mn, Vec2<T> mx) {
			return Vec2<T> {
				std::clamp(x, mn.x, mx.x),
				std::clamp(y, mn.y, mx.y)
			};
		}
    
		inline T max() {
			return std::max(x, y);
		}
    
		inline T min() {
			return std::min(x, y);
		}
    
		inline Vec2<T> max(T v) {
			return Vec2<T> {
				std::max(x, v),
				std::max(y, v)
			};
		}
    
		inline Vec2<T> min(T v) {
			return Vec2<T> {
				std::min(x, v),
				std::min(y, v)
			};
		}
    
		inline Vec2<T> max(Vec2<T> v) {
			return Vec2<T> {
				std::max(x, v.x),
				std::max(y, v.y)
			};
		}
    
		inline Vec2<T> min(Vec2<T> v)   {
			return Vec2<T> {
				std::min(x, v.x),
				std::min(y, v.y)
			};
		}
    
		operator var() { return array<T> { x, y }; }
		///
		bool operator== (const Vec2<T> &r) const { return x == r.x && y == r.y; }
		///
		void operator += (Vec2<T>  r)   { x += r.x; y += r.y; }
		void operator -= (Vec2<T>  r)   { x -= r.x; y -= r.y; }
		void operator *= (Vec2<T>  r)   { x *= r.x; y *= r.y; }
		void operator /= (Vec2<T>  r)   { x /= r.x; y /= r.y; }
		///
		void operator *= (T r)          { x *= r;   y *= r; }
		void operator /= (T r)          { x /= r;   y /= r; }
		///
		Vec2<T> operator + (Vec2<T> r)  { return { x + r.x, y + r.y }; }
		Vec2<T> operator - (Vec2<T> r)  { return { x - r.x, y - r.y }; }
		Vec2<T> operator * (Vec2<T> r)  { return { x * r.x, y * r.y }; }
		Vec2<T> operator / (Vec2<T> r)  { return { x / r.x, y / r.y }; }
		///
		Vec2<T> operator * (T r)        { return { x * r, y * r }; }
		Vec2<T> operator / (T r)        { return { x / r, y / r }; }
		///
		operator Vec2<float>()          { return Vec2<float>  {  float(x),  float(y) }; }
		operator Vec2<double>()         { return Vec2<double> { double(x), double(y) }; }
		operator Vec2<int>()            { return Vec2<int>    {    int(x),    int(y) }; }
		///
		static double area(array<Vec2<T>> &p) {
			double area = 0.0;
			const int n = int(p.size());
			int       j = n - 1;
        
			// shoelace formula
			for (int i = 0; i < n; i++) {
				Vec2<T> &pj = p[j];
				Vec2<T> &pi = p[i];
				area += (pj.x + pi.x) * (pj.y - pi.y);
				j = i;
			}
     
			// return absolute value
			return std::abs(area / 2.0);
		}
	};

	template <typename T>
	struct Vec3: Vector<T> {
		alignas(T) T x, y, z;
		Vec3(std::nullptr_t n = nullptr) : x(nan<T>()) { }
		Vec3(T x) : x(x), y(x), z(x) { }
		Vec3(T x, T y, T z) : x(x), y(y), z(z) { }
    
		static Vec3<T> &null() {
			static Vec3<T> *n = nullptr;
			if (!n)
				 n = new Vec3<T>(nullptr);
			return *n;
		}

		bool operator== (const Vec3<T> &r) const { return x == r.x && y == r.y && z == r.z; }

		void operator += (Vec3<T> &r)   { x += r.x; y += r.y; z += r.z; }
		void operator -= (Vec3<T> &r)   { x -= r.x; y -= r.y; z -= r.z; }
		void operator *= (Vec3<T> &r)   { x *= r.x; y *= r.y; z *= r.z; }
		void operator /= (Vec3<T> &r)   { x /= r.x; y /= r.y; z /= r.z; }
    
		void operator *= (T r)          { x *= r;   y *= r;   z *= r.z; }
		void operator /= (T r)          { x /= r;   y /= r;   z /= r.z; }
    
		Vec3<T> operator + (Vec3<T> &r) { return { x + r.x, y + r.y, z + r.z }; }
		Vec3<T> operator - (Vec3<T> &r) { return { x - r.x, y - r.y, z - r.z }; }
		Vec3<T> operator * (Vec3<T> &r) { return { x * r.x, y * r.y, z * r.z }; }
		Vec3<T> operator / (Vec3<T> &r) { return { x / r.x, y / r.y, z / r.z }; }
    
		Vec3<T> operator * (T r)        { return { x * r.x, y * r.y, z * r.z }; }
		Vec3<T> operator / (T r)        { return { x / r.x, y / r.y, z / r.z }; }

		inline Vec2<T> xy()             { return Vec2<T> { x, y }; }
		inline operator Vec3<float>()   { return Vec3<float>  { float(x),  float(y),  float(z)  }; }
		inline operator Vec3<double>()  { return Vec3<double> { double(x), double(y), double(z) }; }
    
		operator var() {
			return std::vector<T> { x, y, z };
		}
    
		Vec3(var &d) {
			x = T(d[size_t(0)]);
			y = T(d[size_t(1)]);
			z = T(d[size_t(2)]);
		}
    
		Vec3<T> &operator=(const std::vector<T> f) {
			assert(f.size() == 3);
			x = f[0];
			y = f[1];
			z = f[2];
			return *this;
		}
    
		T len_sqr() const {
			return x * x + y * y + z * z;
		}
    
		T len() const {
			return sqrt(len_sqr());
		}
    
		Vec3<T> normalize() const {
			T l = len();
			return *this / l;
		}
    
		inline Vec3<T> cross(Vec3<T> b) {
			Vec3<T> &a = *this;
			return { a.y * b.z - a.z * b.y,
					 a.z * b.x - a.x * b.z,
					 a.x * b.y - a.y * b.x };
		}
	};


	template <typename T>
	struct Vec4: Vector<T> {
		alignas(T) T x, y, z, w;
		Vec4(std::nullptr_t n = nullptr) {
			x = nan<T>();
		}
		Vec4(T x)                  : x(x),   y(x),   z(x),   w(x)   { }
		Vec4(T x, T y, T z, T w)   : x(x),   y(y),   z(z),   w(w)   { }
		Vec4(Vec2<T> x, Vec2<T> y) : x(x.x), y(x.y), z(y.x), w(y.y) { }
    
		static Vec4<T> &null() {
			static Vec4<T> *n = nullptr;
			if (!n)
				 n = new Vec4<T>(nullptr);
			return *n;
		}

		bool operator== (const Vec4<T> &r) const { return x == r.x && y == r.y && z == r.z && w == r.w; }

		void operator += (Vec4<T> &r)          { x += r.x; y += r.y; z += r.z; w += r.w; }
		void operator -= (Vec4<T> &r)          { x -= r.x; y -= r.y; z -= r.z; w -= r.w; }
		void operator *= (Vec4<T> &r)          { x *= r.x; y *= r.y; z *= r.z; w *= r.w; }
		void operator /= (Vec4<T> &r)          { x /= r.x; y /= r.y; z /= r.z; w /= r.w; }
    
		inline void operator *= (T r)          { x *= r;   y *= r;   z *= r;   w *= r; }
		inline void operator /= (T r)          { x /= r;   y /= r;   z /= r;   w /= r; }
    
		inline Vec4<T> operator + (Vec4<T>  r) { return { x + r.x, y + r.y, z + r.z, w + r.w }; }
		inline Vec4<T> operator - (Vec4<T>  r) { return { x - r.x, y - r.y, z - r.z, w - r.w }; }
		inline Vec4<T> operator * (Vec4<T>  r) { return { x * r.x, y * r.y, z * r.z, w * r.w }; }
		inline Vec4<T> operator / (Vec4<T>  r) { return { x / r.x, y / r.y, z / r.z, w / r.w }; }
    
		inline Vec4<T> operator * (T v)        { return { x * v, y * v, z * v, w * v }; }
		inline Vec4<T> operator / (T v)        { return { x / v, y / v, z / v, w / v }; }
    
		inline Vec3<T> xyz()                   { return Vec3<T> { x, y, z }; }
    
		inline operator Vec4<float>()          { return Vec4<float>  {  float(x),  float(y),  float(z),  float(w) }; }
		inline operator Vec4<double>()         { return Vec4<double> { double(x), double(y), double(z), double(w) }; }
    
		inline Vec2<T> xy()                    { return Vec2<T> { x, y }; }
		inline Vec2<T> xz()                    { return Vec2<T> { x, z }; }
		inline Vec2<T> yz()                    { return Vec2<T> { y, z }; }
		inline Vec2<T> xw()                    { return Vec2<T> { x, w }; }
		inline Vec2<T> yw()                    { return Vec2<T> { y, w }; }
	};

	template <typename T>
	struct Edge {
		Vec2<T> &a;
		Vec2<T> &b;
    
		/// returns x-value of intersection of two lines
		inline T x(Vec2<T> c, Vec2<T> d) {
			T num = (a.x*b.y - a.y*b.x) * (c.x-d.x) - (a.x-b.x) * (c.x*d.y - c.y*d.x);
			T den = (a.x-b.x) * (c.y-d.y) - (a.y-b.y) * (c.x-d.x);
			return num / den;
		}
    
		/// returns y-value of intersection of two lines
		inline T y(Vec2<T> c, Vec2<T> d) {
			T num = (a.x*b.y - a.y*b.x) * (c.y-d.y) - (a.y-b.y) * (c.x*d.y - c.y*d.x);
			T den = (a.x-b.x) * (c.y-d.y) - (a.y-b.y) * (c.x-d.x);
			return num / den;
		}
    
		/// returns xy-value of intersection of two lines
		inline Vec2<T> xy(Vec2<T> c, Vec2<T> d) {
			return { x(c, d), y(c, d) };
		}
	};

	template <typename T>
	struct Rect: Vector<T> {
		alignas(T) T x, y, w, h;
		Rect(std::nullptr_t n = nullptr) : x(std::numeric_limits<T>::quiet_NaN()) { }
		Rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) { }
		Rect(Vec2<T> p, Vec2<T> s, bool c) {
			if (c)
				*this = { p.x - s.x / 2, p.y - s.y / 2, s.x, s.y };
			else
				*this = { p.x, p.y, s.x, s.y };
		}
		Rect(Vec2<T> p0, Vec2<T> p1) {
			auto x0 = std::min(p0.x, p1.x);
			auto y0 = std::min(p0.y, p0.y);
			auto x1 = std::max(p1.x, p1.x);
			auto y1 = std::max(p1.y, p1.y);
			*this   = { x0, y0, x1 - x0, y1 - y0 };
		}
    
		operator bool() {
			return !std::isnan(x) && !std::isnan(y) && w > 0 && h > 0;
		}
    
		inline Rect<T> offset(T a) const { /// rename to something else
			return { x - a, y - a, w + (a * 2), h + (a * 2) };
		}
    
		static Rect<T> &null() {
			static Rect<T> *n = nullptr;
			if (!n)
				 n = new Rect<T>(nullptr);
			return *n;
		}
    
		Vec2<T> size()   const { return { w, h }; }
		Vec2<T> xy()     const { return { x, y }; }
		Vec2<T> center() const { return { x + w / 2.0, y + h / 2.0 }; }
		bool contains(Vec2<T> p) const {
			return p.x >= x && p.x <= (x + w) &&
				   p.y >= y && p.y <= (y + h);
		}
		bool    operator== (const Rect<T> &r) { return x == r.x && y == r.y && w == r.w && h == r.h; }
		bool    operator!= (const Rect<T> &r) { return !operator==(r); }
		///
		Rect<T> operator + (Rect<T> r)        { return { x + r.x, y + r.y, w + r.w, h + r.h }; }
		Rect<T> operator - (Rect<T> r)        { return { x - r.x, y - r.y, w - r.w, h - r.h }; }
		Rect<T> operator + (Vec2<T> v)        { return { x + v.x, y + v.y, w, h }; }
		Rect<T> operator - (Vec2<T> v)        { return { x - v.x, y - v.y, w, h }; }
    
		///
		Rect<T> operator * (T r) { return { x * r, y * r, w * r, h * r }; }
		Rect<T> operator / (T r) { return { x / r, y / r, w / r, h / r }; }
		operator Rect<int>()     { return {    int(x),    int(y),    int(w),    int(h) }; }
		operator Rect<float>()   { return {  float(x),  float(y),  float(w),  float(h) }; }
		operator Rect<double>()  { return { double(x), double(y), double(w), double(h) }; }
		operator var()           { return ::array<T> { x, y, w, h }; }
		Rect<T>(var &d)          {
			if (d.size()) {
				x = T(d[size_t(0)]);
				y = T(d[size_t(1)]);
				w = T(d[size_t(2)]);
				h = T(d[size_t(3)]);
			} else
				x = std::numeric_limits<double>::quiet_NaN();
		}
    
		array<Vec2<T>> edges() const {
			return {{x,y},{x+w,y},{x+w,y+h},{x,y+h}};
		}
    
		array<Vec2<T>> clip(array<Vec2<T>> &poly) const {
			const Rect<T>  &clip = *this;
			array<Vec2<T>>       p = poly;
			array<Vec2<T>>       e = clip.edges();
			for (int i = 0; i < e.size(); i++) {
				Vec2<T>  &e0 = e[i];
				Vec2<T>  &e1 = e[(i + 1) % e.size()];
				Edge<T> edge = { e0, e1 };
				array<Vec2<T>> cl;
				for (int ii = 0; ii < p.size(); ii++) {
					const Vec2<T> &pi = p[(ii + 0)];
					const Vec2<T> &pk = p[(ii + 1) % p.size()];
					const bool    top = i == 0;
					const bool    bot = i == 2;
					const bool    lft = i == 3;
					if (top || bot) {
						const bool cci = top ? (edge.a.y <= pi.y) : (edge.a.y > pi.y);
						const bool cck = top ? (edge.a.y <= pk.y) : (edge.a.y > pk.y);
						if (cci) {
							cl     += cck ? pk : Vec2<T> { edge.x(pi, pk), edge.a.y };
						} else if (cck) {
							cl     += Vec2<T> { edge.x(pi, pk), edge.a.y };
							cl     += pk;
						}
					} else {
						const bool cci = lft ? (edge.a.x <= pi.x) : (edge.a.x > pi.x);
						const bool cck = lft ? (edge.a.x <= pk.x) : (edge.a.x > pk.x);
						if (cci) {
							cl     += cck ? pk : Vec2<T> { edge.a.x, edge.y(pi, pk) };
						} else if (cck) {
							cl     += Vec2<T> { edge.a.x, edge.y(pi, pk) };
							cl     += pk;
						}
					}
				}
				p = cl;
			}
			return p;
		}
	};

	/// never change:
	typedef Vec2<float>  vec2f;
	typedef Vec2<real>   vec2;
	typedef Vec2<double> vec2d;
	typedef Vec3<float>  vec3f;
	typedef Vec3<real>   vec3;
	typedef Vec3<double> vec3d;
	typedef Vec4<float>  vec4f;
	typedef Vec4<real>   vec4;
	typedef Vec4<double> vec4d;

	typedef Vec2<int>    vec2i;
	typedef Vec3<int>    vec3i;
	typedef Vec4<int>    vec4i;

	typedef Rect<double> rectd;
	typedef Rect<float>  rectf;
	typedef Rect<int>    recti;

	template <typename T>
	static inline T mix(T a, T b, T i) {
		return (a * (1.0 - i)) + (b * i);
	}

	template <typename T>
	static inline Vec2<T> mix(Vec2<T> a, Vec2<T> b, T i) {
		return (a * (1.0 - i)) + (b * i);
	}

	template <typename T>
	static inline Vec3<T> mix(Vec3<T> a, Vec3<T> b, T i) {
		return (a * (1.0 - i)) + (b * i);
	}

	template <typename T>
	static inline Vec4<T> mix(Vec4<T> a, Vec4<T> b, T i) {
		return (a * (1.0 - i)) + (b * i);
	}

	template <typename T>
	static inline Vec4<T> xxyy(Vec2<T> v) { return { v.x, v.x, v.y, v.y }; }

	template <typename T>
	static inline Vec4<T> xxyy(Vec3<T> v) { return { v.x, v.x, v.y, v.y }; }

	template <typename T>
	static inline Vec4<T> xxyy(Vec4<T> v) { return { v.x, v.x, v.y, v.y }; }

	template <typename T>
	static inline Vec4<T> ywyw(Vec4<T> v) { return { v.y, v.x, v.y, v.w }; }

	template <typename T>
	static inline Vec4<T> xyxy(Vec2<T> v) { return { v.x, v.y, v.x, v.y }; }

	template <typename T>
	static inline Vec2<T> fract(Vec2<T> v) { return v - v.floor(); }

	template <typename T>
	static inline Vec3<T> fract(Vec3<T> v) { return v - v.floor(); }

	template <typename T>
	static inline Vec4<T> fract(Vec4<T> v) { return v - v.floor(); }
///}


// if its an io struct it has a map of shared data?..
// this can be reduced a bit.
// except for attachments.  not on shared and not for member identity,
// but it is important to 'attach' for life cycle management, not for
// retrieval because then the streams are crossed and egon gets a
// massive amount of slime to sample
// 
// 
// 	
// should not need to change anything.
//export module member;
//export {
struct Member {
	typedef lambda<void(Member&, var&)>		MFn;
	typedef lambda< var(Member&)>			MFnGet;
	typedef lambda<bool(Member&)>			MFnBool;
	typedef lambda<void(Member&)>			MFnVoid;
	typedef lambda<void(Member&, Shared&)>	MFnShared;
	typedef lambda<void(Member&, void*  )>	MFnArb;

	struct StTrans;

	enum MType {
		Undefined,
		Intern, // your inner workings
		Extern, // your interface
		Configurable = Intern // having a configurable bit is important in many cases
	};

	lambda<void(Member*, Member*)> copy_fn;

	var& nil() {
		static var n = var(null);
		return n;
	}

	///
	var*     serialized = null;
	Type     type       = Type::Undefined;
	MType    member     = MType::Undefined;
	str      name       = null;
	Shared   shared;    /// having a shared arg might be useful too.
	Shared   arg;       /// simply a node * pass-through for the NMember case

	///
	size_t   cache      = 0;
	size_t   peer_cache = 0xffffffff;
	Member*  bound_to   = null;

	virtual ~Member() { }

	///
	virtual void*            ptr() { return null; }
	virtual Shared& shared_value() { static Shared s_null; return s_null; }
	virtual bool      state_bool() {
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
	virtual void operator= (const str& s) {
		var v = (str&)s;
		if (lambdas && lambdas->var_set)
			lambdas->var_set(*this, v);
	}
	void         operator= (Member& v);
	virtual bool operator==(Member& m) { return bound_to == &m && peer_cache == m.peer_cache; }

	virtual void style_value_set(void* ptr, StTrans*) { }
};
//}

//export module build;
//export {
	struct Build {
		str version;

        /*
        /// attach to enum? so map with name, those names app-based or general
        Symbols Build::Role::symbols = {
            { Undefined,        "undefined" },
            { Terminal,         "terminal"  },
            { Mobile,           "mobile"    },
            { Desktop,          "desktop"   },
            { AR,               "ar"        },
            { VR,               "vr"        },
            { Cloud,            "cloud"     } };

        Symbols Build::OS::symbols = {
            { Undefined,        "undefined" },  /// Arb behaviour with this null state
            { macOS,            "macOS"     },  /// Simple binary
            { Linux,            "Linux"     },  /// Simple binary
            { Windows,          "Windows"   },  /// Universal App Packaging (if its actually a thing for compiled apps)
            { iOS,              "iOS"       },  /// Universal App Packaging
            { Android,          "Android"   } }; /// Universal Binaries as done with the bin images in zip

        Symbols Build::Arch::symbols = {
            { Undefined,        "undefined" },
            { x86,              "x86"       },
            { x86_64,           "x86_64"    },
            { Arm32,            "arm32"     },
            { Arm64,            "arm64"     },
            { Mx,               "Mx"        } };
        */
	};
//}

//export import meta; // Component uses a collection of metas, default is just 'members'; they will have a different namespace in context
//export {
	// interface to Member struct for a given [T]ype
	/*
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

	template <typename F>
	struct Lambda:M<lambda<F>> {
		typedef typename lambda<F>::rtype rtype;
		static  const    size_t arg_count = lambda<F>::args;
		rtype            last_result; // Lamba is a cached result using member services; likely need controls on when its invalidated in context
	};

	important to migrate to new lambda that returns a shared result
	*/



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
//}


//export import ex;
//export {
	/// basic enums type needs to be enumerable by enum keyword, serializable, and introspectable
	struct EnumData:io {
		Type        type;
		int         kind;
		EnumData(Type type, int kind): type(type), kind(kind) { }
	};

	template <typename T>
	struct ex:EnumData {
		ex(int kind) : EnumData(Id<T>(), kind) { }
		operator bool() { return kind > 0; }
		operator  var() {
			for (auto &sym: T::symbols)
				if (sym.value == kind)
					return sym.symbol;
			assert(false);
			return null;
		}

		str symbol() {
			for (auto &sym: T::symbols)
				if (sym.value == kind)
					return sym.symbol;
			return null;
		}

		/// depending how Symbol * param, this is an assertion failure on a resolve of symbol to int
		int resolve(std::string s, Symbol **ptr = null) {
			for (auto &sym: T::symbols)
				if (sym.symbol == s) {
					if (ptr)
						*ptr = &sym;
					return sym.value;
				}
			if (ptr)
			   *ptr = null;
			///
			assert(!ptr || T::symbols.size());
			return T::symbols[0].value;
		}
		ex(var &v) { kind = resolve(v); }
	};

	struct Any  { Shared data; }; /// not sure how far we go with Any, or if its fully baked yet

	struct Node   { Shared data; };
	struct Meta   { Shared data; };
	struct Ref    { Shared data; }; // no problem with this.  same data is same data.

	// this is placeholder. ux needs to add its own ref type, and i should break up the types by who uses dx
//};

	void testme() {
		//map<str, var> m = {};
		//CompareOp c0 = Ops<Data2>::fn_compare<pair<str,var>>((pair<str, var> *)null);
	}
}



INITIALIZER(initialize) {
	static bool init = false;
	assert(!init);

	static Type::Decl decls[] =
	{
		{ 0,  "undefined" },
		{ 1,  "i8"        },
		{ 2,  "ui8"       },
		{ 3,  "i16"       },
		{ 4,  "ui16"      },
		{ 5,  "i32"       },
		{ 6,  "ui32"      },
		{ 7,  "i64"       },
		{ 8,  "ui64"      },
		{ 9,  "f32"		  },
		{ 10, "f64"       },
		{ 11, "Bool"      },
		{ 12, "Str"       },
		{ 13, "Map"       },
		{ 14, "Array"     },
		{ 15, "Ref"       },
		{ 16, "Arb"       },
		{ 17, "Node"      },
		{ 18, "Lambda"    },
		{ 19, "Member"    },
		{ 20, "Any"       },
		{ 21, "Meta"      }
	};

	//
	size_t p                   = 0;
	*(Type **)Type::primitives = (Type *)calloc(sizeof(Type), 22); // C++ is champ
	*(Type *)&Type::Undefined  = Type::bootstrap<void>        (decls[0]);  Type::primitives[p++] = &Type::Undefined;
	*(Type *)&Type::  i8       = Type::bootstrap<int8_t>      (decls[1]);  Type::primitives[p++] = &Type::i8;
	*(Type *)&Type:: ui8	   = Type::bootstrap<uint8_t>     (decls[2]);  Type::primitives[p++] = &Type::ui8;
	*(Type *)&Type:: i16       = Type::bootstrap<int16_t>     (decls[3]);  Type::primitives[p++] = &Type::i16;
	*(Type *)&Type::ui16       = Type::bootstrap<uint16_t>    (decls[4]);  Type::primitives[p++] = &Type::ui16;
	*(Type *)&Type:: i32       = Type::bootstrap<int32_t>     (decls[5]);  Type::primitives[p++] = &Type::i32;
	*(Type *)&Type::ui32       = Type::bootstrap<uint32_t>    (decls[6]);  Type::primitives[p++] = &Type::ui32;
	*(Type *)&Type:: i64       = Type::bootstrap<int64_t>     (decls[7]);  Type::primitives[p++] = &Type::i64;
	*(Type *)&Type::ui64       = Type::bootstrap<uint64_t>    (decls[8]);  Type::primitives[p++] = &Type::ui64;
	*(Type *)&Type:: f32       = Type::bootstrap<float  >     (decls[9]);  Type::primitives[p++] = &Type::f32;
	*(Type *)&Type:: f64       = Type::bootstrap<double >     (decls[10]); Type::primitives[p++] = &Type::f64;
	*(Type *)&Type::Bool	   = Type::bootstrap<Bool>        (decls[11]); Type::primitives[p++] = &Type::Bool;
	*(Type *)&Type::Str		   = Type::bootstrap<str>         (decls[12]); Type::primitives[p++] = &Type::Str;
	*(Type *)&Type::Map        = Type::bootstrap<array<var>>  (decls[13]); Type::primitives[p++] = &Type::Array;
	*(Type *)&Type::Array	   = Type::bootstrap<map<str,var>>(decls[14]); Type::primitives[p++] = &Type::Map;
	*(Type *)&Type::Ref	       = Type::bootstrap<Ref>		  (decls[15]); Type::primitives[p++] = &Type::Ref;
	*(Type *)&Type::Arb        = Type::bootstrap<Arb>         (decls[16]); Type::primitives[p++] = &Type::Arb;
	*(Type *)&Type::Node	   = Type::bootstrap<Node>		  (decls[17]); Type::primitives[p++] = &Type::Node;

	*(Type*)&Type::Lambda      = Type::bootstrap<lambda<var(var&)>> (decls[18]); Type::primitives[p++] = &Type::Str;

	*(Type*)&Type::Member	   = Type::bootstrap<Member>	  (decls[19]); Type::primitives[p++] = &Type::Member;
	*(Type*)&Type::Any		   = Type::bootstrap<Any>		  (decls[20]); Type::primitives[p++] = &Type::Any;
	*(Type*)&Type::Meta		   = Type::bootstrap<Meta>		  (decls[21]); Type::primitives[p++] = &Type::Meta;
}
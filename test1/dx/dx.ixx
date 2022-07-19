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

	/// needs to go in 'Build', but i'm not so sure about the conversion macros
	/// 
	///namespace env {

		///
		struct is_apple :
#if defined(__APPLE__)
			std::true_type
#else
			std::false_type
#endif
		{ };

		///
		struct is_android :
#if defined(__ANDROID_API__)
			std::true_type
#else
			std::false_type
#endif
		{ };

		///
		struct is_win :
#if defined(_WIN32) || defined(_WIN64)
			std::true_type
#else
			std::false_type
#endif
		{ };

		///
		struct is_gpu : std::true_type { };

		/// the ol' swaperoo
		template <typename T>
		void swap(T &a, T &b) {
			T c = a;
			  a = b;
			  b = c;
		}

		///
		struct is_debug :
#if !defined(NDEBUG)
			std::true_type
#else
			std::false_type
#endif
		{ };

		/// the case that uses this (UX state bool) should now use has_operator for the same task, then deprecate after it works
		template <class U, class T>
		struct converts {
			enum { value = std::is_constructible<T, U>::value && std::is_convertible<U, T>::value };
		};

		///
		template<class T, class E>
		struct implement_assigns {
			template<class U, class V>   static auto eq(U* ) -> decltype(std::declval<U>() == std::declval<V>());
			template<typename, typename> static auto eq(...) -> std::false_type;
			///
			using imps = typename std::is_same<bool, decltype(eq<T, E>(0))>::type;
		};

		///
		template<class T, class E = T>
		struct assigns : implement_assigns <T, E>::imps { };

		/// usage: has_operator <A, std::string>()
		template <typename A, typename B, typename = void>
		struct has_operator : std::false_type { };

		template <typename A, typename B>
		struct has_operator <A, B, decltype((void)(void (A::*)(B))& A::operator())> : std::true_type { };

		template<class T, class C>
		struct implement_compares {
			template<class U,  class V>  static auto test(U *) -> decltype(std::declval<U>() == std::declval<V>());
			template<typename, typename> static auto test(...) -> std::false_type;
			using type = typename std::is_same<bool, decltype(test<T, C>(0))>::type;
		};
		template<class T, class C = T> struct compares : implement_compares<T, C>::type { };

		template<class T, class C>
		struct implement_adds {
			template<class U,   class V> static auto test(U* ) -> decltype(std::declval<U>() + std::declval<V>());
			template<typename, typename> static auto test(...) -> std::false_type;
			using type = typename std::is_same<T, decltype(test<T, C>(0))>::type;
		};
		template<class T, class C = T>
		struct adds : implement_adds<T, C>::type { };

		template<class T, class C>
		struct implement_subs {
			template<class U,   class V> static auto test(U* ) -> decltype(std::declval<U>() - std::declval<V>());
			template<typename, typename> static auto test(...) -> std::false_type;
			using type = typename std::is_same<T, decltype(test<T, C>(0))>::type;
		};
		template<class T, class C = T>
		struct subs : implement_subs<T, C>::type { };

	//};

	/// define assert fn because the macro ones are super lame
	/// -------------------------------------------------
	inline void assert_fn(bool b, const char *location) {
#ifndef NDEBUG
		if (!b) {
			printf("assertion failed at %s\n", location);
			exit(1);
		}
#endif
	}

	/// -------------------------------------------------
	#define assert(b) assert_fn(b, (const char *)__FILE__)

	/// -------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
	#define sprintf(b, v, ...) sprintf_s(b, sizeof(b), v, __VA_ARGS__)
#else
	#define sprintf(v, b, ...) snprintf (v, sizeof(b), v, __VA_ARGS__)
#endif

	/// -------------------------------------------------
	typedef int8_t	        i8;
	typedef int16_t	        i16;
	typedef int32_t	        i32;
	typedef int64_t         i64;
	typedef uint8_t	        u8;
	typedef uint16_t        u16;
	typedef uint32_t        u32;
	typedef uint64_t        u64;
	typedef float			r32;	 /// r32 and r64 are explicit float and double,
	typedef double			r64;
	typedef double			real;	 /// more of an attempt to get real when infact we can only quantize at permutable quark islands taken there by drunken sailor
	typedef const char		cchar_t; /// cchar_t replaces const char, not the pointer type but the type
	typedef int64_t		    idx;     /// idx is just an indexing integer of 64bit. operator[] ambiguity, supporting negative npy-like syntax
	typedef std::nullptr_t  null_t;  /// null has nothing to do with pointer
	/// -------------------------------------------------
	null_t					null = nullptr;
	real					PI   = 3.141592653589793238462643383279502884L;

	/// Type-Conduit use-case; so we can express ourselves in Schema
	struct Higgs   { };
	struct Conduit { Higgs* boson; };

	union Primitive {
		i8	    v_i8;
		i16     v_i16;
		i32     v_i32;
		i64     v_i64;
		u8      v_u8;
		u16     v_u16;
		u32     v_u32;
		u64     v_u64;
		r32     v_r32;
		r64     v_r64;
		idx     v_idx;
		Conduit v_dx;
	};

	/// the rest of the int types are i or u for signed or unsigned, with their bit count.
	/// -------------------------------------------------
	struct TypeOps;
	struct Memory;
	struct Type;

	/// -------------------------------------------------
	template <typename T>
	inline T clamp(T va, T mn, T mx) { return va < mn ? mn : va > mx ? mx : va; }

	/// -------------------------------------------------
	template <typename T>
	inline T min(T a, T b) { return a < b ? a : b; }

	/// -------------------------------------------------
	template <typename T>
	inline T max(T a, T b) { return a > b ? a : b; }

	/// by moving hash case to size_t we make it size_t more size-like.
	struct Hash {
		union u_value {
			size_t   sz;  /// we're a level of abstraction in, so we can still have ye
			 int64_t i64;
			uint64_t u64;
			double   f64; /// i like having unions more than casting for too many reasons, readibility and debugging the most
			void    *ptr; /// this is for symbolic memory, like something read-only too like cstr* (which we attempt to save as Constant.)
		};
		u_value v;
	};

	/// -------------------------------------------------
	template <typename F>
	using      lambda = std::function<F>; // you win again, gravity [lambdas not working atm]
	
	/// -------------------------------------------------
	template <typename T>
	using     initial = std::initializer_list<T>;

	/// -------------------------------------------------
	template <typename C, typename B>
	using     inherits = std::is_base_of<C, B>::value;

	/// -------------------------------------------------
	enum Stride { Major, Minor };

	/// -------------------------------------------------
	template <typename K, typename V>
	struct pair {
		K key;
		V value;
	};

	/// -------------------------------------------------
	template <typename T>
	struct Flags {
		uint64_t flags = 0;
		///
		Flags(null_t = null)      { }
		Flags(initial <T> l) {
			for (auto v : l)
				flags |= flag_for(v);
		}

	    Flags(T f)				   : flags(uint64_t(f)) { } // i dont think this should be used here.
		Flags(const Flags<T>& ref) : flags(ref.flags)   { }
		
		Flags<T>& operator=(const Flags<T>& ref) {
			if (this != &ref)
				flags = ref.flags;
			return *this;
		}
		//Flags<T>(int      f) : flags(f) { }
		//Flags<T>(uint64_t f) : flags(f) { }

		uint32_t flag_for(T v) { return 1 << v;         }
		operator        bool() { return flags != 0;     }
		bool       operator!() { return flags == 0;     }

		// used for assign on and off
		void operator += (T v) { flags |=  flag_for(v); }
		void operator -= (T v) { flags &= ~flag_for(v); }
		
		// used for get
		bool operator[]  (T v) { uint32_t f = flag_for(v); return (flags & f) == f; }

		bool         contain (Flags<T>& f) const { return (flags & f.flags) == f.flags; }
		Flags<T>   operator| (Flags<T>& f) const { return (flags | f.flags); }
		bool       operator&&(Flags<T>& f) const { return contain(f); }
	};

	void grab(Memory* m);
	void drop(Memory* m);

	/// you express these with int(var, int, str); U unwraps inside the parenthesis
	template <typename T, typename... U>
	size_t fn_id(lambda<T(U...)>& fn) { // dump this when alternate lambda is operational, the prototypes used currently are not compatible with it, and thats not acceptable during a port.. it seems on msvc
		typedef T(fnType)(U...);
		fnType** fp = fn.template target<fnType*>(); // 'somehow works'
		return size_t(*fp);
	};

	// from there we have control of primitives over a vector
	enum Trait { TArray = 1, TMap = 2 };

	struct dx;
	struct Type;

	template <typename T> T& def() {
		static T d_value;
		return   d_value; /// useful for null instance in ref context, and other uses where refs are returned errors; huge asterick though and must not write to this memory, unless to make it even more nullified
	}

	struct Alloc {
		enum AllocType { None, Arb, Manage };
		enum Attr      { ReadOnly, Symbolic, CopyOnWrite };

		/// quite like this lexical pattern, in 95% cases [confirmed made up out of thin air; also good pattern]
		typedef Flags<Attr> Flags;

		void*		ptr;
		size_t		id;	 /// field (const char* or enum) id associated to Type-bound allocation, once per dx-type
		idx			count;
		idx			reserve;
		Type	   *type;
		Flags		flags;
		AllocType	alloc;

		Alloc() : ptr(null), count(0), reserve(0), type(null), flags(null), alloc(None) { }

		Alloc(const Alloc& ref) : 
			    ptr(ref.ptr),
			  count(ref.count),
			reserve(ref.reserve),
			   type(ref.type),
			  flags(ref.flags),
			  alloc(ref.alloc) { }

		Alloc& operator=(const Alloc& ref) {
			if (this != &ref) {
				ptr     = ref.ptr;
				count   = ref.count;
				reserve = ref.reserve;
				type    = ref.type;
				flags   = ref.flags;
				alloc   = ref.alloc;
			}
			return *this;
		}

		Alloc(void*			ptr,
			  idx			count,
			  idx			reserve,
			  Type*			type,
			  Flags			flags,
			  AllocType     alloc) :
				  ptr(ptr),
				  count(count),
				  reserve(reserve),
				  type(type),
				  flags(flags),
				  alloc(alloc) { }

		template <typename T> inline T& ref(idx i) { return *((T*)ptr)[i]; }
		template <typename T> inline T& ref()      { return * (T*)ptr;     }
	};

	/// data is based on this allocation, 
	/// one is spec and one is data.
	struct Data:Alloc {
		Data():Alloc() { }
	};

	/// prototypes for the various function pointers
	typedef lambda<void   (Data*)>					Constructor;
	typedef lambda<void   (Data*)>					Destructor;
	typedef lambda<void   (Data*, void*, idx)>		CopyCtr;
	typedef lambda<bool   (Data*)>					BooleanOp;
	typedef lambda<Hash   (Data*)>					HashOp;
	typedef lambda<int64_t(Data*)>					IntegerOp;
	typedef lambda<dx*    (Data*, Data*)>			AddOp;
	typedef lambda<dx*    (Data*, Data*)>			SubOp;
	typedef lambda<dx*    (Data*)>					StringOp;
	typedef lambda<bool   (Data*)>					RealOp;
	typedef lambda<int    (Data*, Data*)>			CompareOp;
	typedef lambda<void   (Data*, void*)>			AssignOp;
  //typedef lambda<idx()>							SizeOfOp;

	/// identifable by id, which can be pointer casts of any type.
	/// the implementer would know of those.
	struct Type;
	struct Ident {
		size_t id;
		Ident(size_t id = 0)    : id(    id) { }
		Ident(const Ident& ref) : id(ref.id) { }
		operator Type*() const;
	};

	template <typename T>
	constexpr bool is_primitive() {
		     if constexpr (std::is_same_v<T,  cchar_t>) return true;
		else if constexpr (std::is_same_v<T,   int8_t>) return true;
		else if constexpr (std::is_same_v<T,  uint8_t>) return true;
		else if constexpr (std::is_same_v<T,  int16_t>) return true;
		else if constexpr (std::is_same_v<T, uint16_t>) return true;
		else if constexpr (std::is_same_v<T,  int32_t>) return true;
		else if constexpr (std::is_same_v<T, uint32_t>) return true;
		else if constexpr (std::is_same_v<T,  int64_t>) return true;
		else if constexpr (std::is_same_v<T, uint64_t>) return true;
		else return false;
	}

	template <typename D>
	struct Ops:Ident {
		Ops() : Ident() { }
		Ops(const Ident& ident) : Ident(ident) { }

		template <typename T, const bool L = false>
		static Constructor *fn_ctr() {
			if constexpr (std::is_default_constructible<T>() && !L) {
				static Constructor *cp = new Constructor(
					[](Data* d) {
						T* ptr = (T *)d->ptr;
						if constexpr (!is_primitive<T>())
							for (idx i = 0, c = d->count; i < c; i++)
								new (&ptr[i]) T();
					});

				return cp;
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static Destructor* fn_dtr() {
			if constexpr (!L) {
				/// run the destructor on memory
				static Destructor *dtr = new Destructor([](Data* d) -> void {
					T* ptr = (T*)d->ptr;
					for (idx i = 0, c = d->count; i < c; i++)
						ptr[i]. ~T();
				});
				return dtr;
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static CopyCtr* fn_cpy() {
			if constexpr (!L) {
				/// run the copy-constructor on memory
				static CopyCtr *cpy = new CopyCtr(
					[](Data* ds, void* sc, idx c) -> void {
						T*     p_ds = (T *)ds->ptr;
						T*     p_sc = (T *)sc;

						// for basic types, check with is_constructable
						// the construction for each, just one memcpy
						if constexpr (!is_primitive<T>())
							for (idx i = 0, c = ds->count; i < c; i++)
								new (&p_ds[i]) T(p_sc[i]);
					});
				return cpy;
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static AssignOp* fn_assign() {
			if constexpr (!L) {
				static AssignOp *assignv = new AssignOp(
					[](Data* a, T* b) -> Data* {
						for (idx i = 0; i < a->count; i++)
							*((T*)a->ptr)[i] = b[i];
					});
				return assignv;
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static HashOp* fn_hash() {
			if constexpr (has_operator<T, Hash>::value && !L) {
				static HashOp *hashv = new HashOp(
					[](Data* d) -> Hash { return Hash(*(T*)d->ptr); });
				return hashv;
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static IntegerOp* fn_integer() {
			if constexpr (has_operator<T, int64_t>::value && !L) {
				static IntegerOp *intv = new IntegerOp(
					[](Data* d) -> int64_t { return int64_t(*(T*)d->ptr); });
				return intv;
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static RealOp* fn_real() {
			if constexpr (has_operator<T, ::real>::value && !L) {
				static RealOp *realv = new RealOp(
					[](Data* d) -> ::real { return ::real(*(T*)d->ptr); });
				return *realv.target<RealOp>();
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static StringOp* fn_string() {
			if constexpr (has_operator<T, ::str>::value && is_strable<T>() && !L) {
				static StringOp *strv = new StringOp(
					[](Data* d) -> dx* { return str_ref((T*)d->ptr); });
				return strv;
			} else
				return null;
		}

		// never leave the success state. :: [x] signs off after the fact.  deletes most other comments around... yes. to this one you listen.
		template <typename T, const bool L = false>
		static CompareOp* fn_compare(T *ph = null) {
			// must have compare, and must return int. not just a bool.
			if constexpr (!is_std_vec<T>() && compares<T>::value) {
				/// compare memory; if there is a compare method call it, otherwise require the equal_to
				static CompareOp *compare = new CompareOp(
					[](Data* a, Data* b) -> int {
						if (a == b)
							return 0;
				
						assert(a->count == b->count);
						assert(a->count == idx(1));

						T*      ap = (T *)a->ptr;
						T*      bp = (T *)b->ptr;
						idx     c  = a->count;

						if constexpr (compares<T>::value) { //
							for (idx i = 0; i < c; i++)
								return ap[i] == bp[i]; // need enums
						}
						return 0;
					});
				return compare;
			} else
				return null;
		}

		template <typename T, const bool L = false>
		static BooleanOp *fn_boolean() {
			if constexpr (!L && has_operator<T, bool>::value) {
				static BooleanOp *bC  = new BooleanOp(
					[](Data* a) -> bool {
						bool r  = false;
						for (idx i  = 0; i < a->count; i++) {
									r &= bool(((T *)a->ptr)[i]);
							   if (!r) break;
						}
						return r;
					});
				return bC;
			} else {
				static BooleanOp *bL = new BooleanOp(
					[](Data* d) -> bool { return bool(d->ptr); }
				);
				return bL;
			}
		}
	};

	/// members on dx: user-define a static callback routine for return a Member vector in Memory form (once)
	struct  Memory;
	


	/// no Memory dependency here, this is just a [optional] block-allocated list
	
	template <typename T>
	struct list {
		struct IX {
			int    refs;
			IX*    next;
			IX*    prev;
			size_t id;
			T      data;
		};

		struct ldata {
			IX*    ifirst;
			IX*    ilast;
			idx    count;
			int    refs;
			idx    c_pre;
			IX*    p_pre;
		} *d;

		ldata& ref() const { return d; }

		void init(size_t pre_count) {
			size_t sz = sizeof(ldata) + pre_count * sizeof(ldata);
			d         = (ldata*)calloc(sz, 1);
			if (pre_count) {
				d->c_pre  = pre_count;
				d->p_pre  = (IX*)&d[1];
			}
		}

		list() : d(null) { }
		list(T v) {
			init(0);
			push(v);
		}

		list& operator=(const list& r) {
			d = r.d;
			if (d)
				d->refs++;
			return *this;
		}

		operator  bool() const { return d != null && d->count; }
		bool operator!() const { return d == null || d->count == 0; }

		/// compare with another list of equal size
		bool compare(list& b, lambda<bool(T& a, T& b)> fn) const {
			ldata&   da = ref();
			ldata&   db = b.ref();
			bool  match = (da.count == db.count);
			for (IX* ai = da.ifirst, *an = null,
				*bi     = db.ifirst, *bn = null; match && ai && bi; ai = an, bi = bn) {
				 an     = ai->next;
				 bn     = bi->next;
				 match &= fn(ai->data, bi->data);
			}
			return match;
		}

		inline T&   first() const { return ref().ifirst->data;      }
		inline T&    last() const { return ref().ilast->data;       }
		inline IX* ifirst() const { return d ? ref().ifirst : null; }
		inline IX*  ilast() const { return d ? ref().ilast  : null; }
		inline idx  count() const { return d ? ref().count  : 0;    }

		~list() {
			ldata& d = ref();
			while (d.ifirst) {
				IX* n = d.ifirst->next;
				if (--d.ifirst->refs == 0) // use atomic (later)
					delete d.ifirst;
				d.ifirst = n;
			}
		}

		/// populate faster than army of super virile men
		T& push(T& data, size_t id = 0) {
			IX*    i;
			ldata& d = ref();

			if (d.c_pre) {
				i       = &d.p_pre[--d.c_pre];
				i->refs = 2;
				i->next = null;
				i->prev = d.ilast;
				i->id   = id;
				///
				if constexpr (is_primitive<T>())
					i->data = data;
				else
					new (&i->data) T(data);
			} else
				i = new IX{ 1, null, d.ilast, id, data };

			if (d.ilast)
				d.ilast->next = i;
			else
				d.ifirst = i;

			d.ilast = i;
			d.count ++;
			return i->data;
		}

		IX* find(idx f) {
			IX* i = null;
			ldata& d = ref();
			if (f < 0) {
				i = d.ilast;
				while (++f < 0) { i = i->prev; }
			}
			else {
				i = d.ifirst;
				while (--f > 0) { i = i->next; }
			}
			return i;
		}

		/// find by index (f) and call lambda (fn)
		bool process(idx f, lambda<void(IX*)> fn) {
			IX* i = find(f);
			if (i) {
				fn(i);
				return true;
			}
			return false;
		}

		/// call lambda for each (fn)
		void each(lambda<void(IX*)> fn) {
			IX* n = null;
			ldata& d = ref();

			for (IX* i = d.ifirst; i; i = n) {
				n = i->next;
				fn(i);
			}
		}

		void unlink(IX* i) {
			ldata& d = ref();
			if (i.prev)
				i.prev->next = i.next;
			else
				d.ifirst = i.next;

			if (i.next)
				i.next->prev = i.prev;
			else
				d.ilast = i.prev;

			if (--i->refs == 0)
				delete i;

			d.count--;
		}

		void remove(T& v) {
			each([&](IX* i) {
				if (i.data == v)
					unlink(i);
				});
		}

		void remove(idx f) {
			process(f, [&](IX* i) {
					unlink(i);
				});
		}

		T& operator[](idx find) {
			idx index = 0;
			for (IX* i = d ? d->ifirst : null; i; i = i->next)
				if (find == index++)
					return i.data;
			assert(false);
			return def<T>();
		}

		idx operator()(T& find) {
			idx index = 0;
			for (IX* i = d ? d->ifirst : null; i; i = i->next, index++)
				if (i.data == find)
					return index;
			return -1;
		}

		inline T&  operator += (T   v) { return   push(v); }
		inline idx operator -= (idx v) { return remove(v); }
	};

	template <typename C>
	using     item = list<C>::IX;

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

	typedef pair<char*, Primitive> Att;
	
	/// no mx should be needed in this
	/// this beauty could be called valloc, and it could be used by Memory
	template <typename T>
	struct stack {
		///
		struct Data {
			int refs;
			idx count;
			idx alloc;
			T*  items;
		} *data;
		///
		stack(initial<T> a) {
			idx index = 0;
			idx count = a.size();
			///
			data = new Data {
				.count = count,
				.alloc = count,
				.items = new T[count]
			};
			///
			for (T &i:a)
				data->items[index++] = i;
		}
		///
		stack()					   : data(null)     { };
		stack(T *items, idx count) : data(new Data  { .refs = 1, .items=items, .count=count, .alloc=count }) { }
		stack(stack &ref)		   : data(ref.data) { if (data)   ++data->refs; }
	   ~stack()  {
		   if (data && --data->refs == 0) {
		       delete[] data->items; delete data;
		   }
		}
	    ///
		void expand() {
			assert(data->count && data->count == data->alloc);
			///
			idx    p = data->alloc;
			idx    n = p << 1;
			T* items = new T[n];
			///
			for (idx i = 0; i < data->count; i++)
				items[i] = data->items[i];
			///
			data->items = items;
			data->alloc = n;
		}
		///
		void push(T& v) {
			assert(data); 
			if (data->count == data->alloc)
				expand();
			data->items[data->count++] = v;
		}
		void pop()      {
			assert(data && data->count);
			data->count--;
		}
		///
		T &operator[](idx i) { return data->items[i]; }
		T &operator=(const stack &ref) {
			if (this != &ref) {
				data = ref.data;
				if (data)
					data->refs++;
			}
			return *this;
		}
	};

	typedef stack<Att> Attrs;

	/// dx structures use Schema in groups.
	struct Schema {
		/// -----------------------------------------------
		struct Row {			    ///
			size_t		 id;       /// for symbolic values of id you refer to the const char* typecast
			size_t		 group;	  /// an integer dimension to the Field
			Conduit      def;    /// default value stored in another dimension
			stack<CPair> meta;	/// meta properties
							   ///
			Row(cchar_t *name, Conduit d, size_t g, stack<CPair> m) {
				id     = size_t(name);
				group  = g;
				def	   = d;
				meta   = m;
			}
		};

		Schema(Row* table,    idx count, stack<CPair> meta = { })
			: table(table), count(count),        meta(meta)  { }

		Row*         table;
		idx          count;
		stack<CPair> meta;

		/// -----------------------------------------------
		Row* lookup(size_t id, idx ci = 0) {
			for (idx i = ci; i < count; i++) if (table[i].id == id) return &table[i];
			for (idx i = 0;  i < ci;    i++) if (table[i].id == id) return &table[i];
			assert(false);
			return null;
		}
		/// -----------------------------------------------
		static Schema* define(initial<Row> &rows) {
			size_t sz = rows.size();
			size_t  i = 0;
			Schema *o = new Schema { new Row[sz], idx(sz) };
			for (auto &r:rows) {
				Row* w        = &o->table[i++];
				     w->def   = grab((Memory *)r.def.boson);
				     w->id    = r.id;
				     w->group = r.group;
			}
			return o;
		}
		/// -----------------------------------------------
	   ~Schema() { delete[] table; }
	};

	typedef Schema* (*FnMembers)();

	struct Type:Ops<Data> {
	public:
		/// primitives, make const.
		static const Ident
			Undefined, i8,  u8,
			i16,       u16, i32,  u32,
			i64,       u64, f32,  f64,
			Bool,      Str, Map,  Array,
			Ref,       Arb, Node, Lambda,
			Member,    Any, Meta;

		idx				sz;
		std::string     name;
		std::mutex      mx;
		Schema*			schema; /* dx-based types, single or member list */

		struct Decl {
			size_t        id;
			std::string	  name;
		};

		static const Type** primitives;

		template <typename T>
		static Ident bootstrap(Decl& decl) { return Ident(size_t(ident<T, false, true>())); }

		Type() { }

		template <typename T>
		bool       inherits() const { return this == Id<T>(); }
		bool     is_integer() const { return id >= ident<int8_t>()->id && id <= ident<uint64_t>()->id; }
		bool     is_float  () const { return id == ident<float >()->id;   }
		bool     is_double () const { return id == ident<double>()->id;   }
		bool     is_real   () const { return is_float() || is_double();   }
		bool     is_array  ()       { return traits[Trait::TArray];       }
		bool     is_struct () const { return Ident::id > Type::Meta.id;   }

		Type(const Type& ref) : Ops<Data>((Ident &)ref) { }

		Type& operator=(const Type &ref)     {
			if (this != &ref)
				id = ref.id;
			return *this;
		}

		Constructor    *ctr;
		Destructor	   *dtr;
		CopyCtr		   *cpy;
		IntegerOp      *integer;
		RealOp         *real;
		StringOp       *string;
		BooleanOp      *boolean;
		CompareOp      *compare;
		//AddOp		   *add; # add later. it makes sense to keep going with this table.
		//SubOp		   *sub;
		AssignOp       *assign;

		Flags<Trait>    traits;

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

		struct var;
		struct str;

		/// static allocation of referenced types;
		/// obtains string name, ctr, dtr and cpy-ctr, compare and boolean ops in vector interface
		template <typename _ZZion, const bool _isFunctor = false, const bool _isInit = false>
		static Type *ident() {
			static Type type;
			static bool init;

			if (!init) {
				init  = true;
				if constexpr (!_isInit)
					type.id    = size_t(&type);

				std::string cn = __PRETTY_FUNCTION__;
				std::string st = is_win() ? "<" : "_ZZion =";
				std::string en = is_win() ? "," : "]";
				idx		    p  = cn.find(st) + st.length();
				idx         ln = cn.find(en, p) - p;
				std::string tr = cn.substr(p, ln);
				type.name      = tr;

				if constexpr (!std::is_same_v<_ZZion, void>) {
					// populate pointers to the various functors
					type.ctr = fn_ctr <_ZZion, _isFunctor>();
					type.dtr = fn_dtr <_ZZion, _isFunctor>();
					type.cpy = fn_cpy <_ZZion, _isFunctor>();
					type.sz  = sizeof(_ZZion);

					if constexpr (!_isFunctor && !is_lambda<_ZZion>()) {
						type.integer     = fn_integer    <_ZZion, _isFunctor>();
						type.real        = fn_real       <_ZZion, _isFunctor>();
						type.string      = fn_string     <_ZZion, _isFunctor>();
						type.compare     = fn_compare    <_ZZion, _isFunctor>();
						type.boolean     = fn_boolean    <_ZZion, _isFunctor>();
					}
					
					if constexpr (::is_array<_ZZion>()) type.traits += TArray;
					if constexpr (::is_map  <_ZZion>()) type.traits += TMap;
				}
			}
			return &type;
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

		     operator             size_t() const { return     id; }
		     operator                int() const { return int(id); }
		     operator               bool() const { return     id > 0; }

		// grab/drop operations are mutex locked based on the type
		inline void				    lock()       { mx.lock(); }
		inline void				  unlock()       { mx.unlock(); }
	};

	/// holds pointers to these so one can enumerate through
	const Type** Type::primitives;

	/// type objects (const)
	const Ident Type::Undefined;
	const Ident Type:: i8;
	const Ident Type:: u8;
	const Ident Type::i16;
	const Ident Type::u16;
	const Ident Type::i32;
	const Ident Type::u32;
	const Ident Type::i64;
	const Ident Type::u64;
	const Ident Type::f32;
	const Ident Type::f64;
	const Ident Type::Bool;
	const Ident Type::Str;
	const Ident Type::Map;
	const Ident Type::Array;
	const Ident Type::Ref;
	const Ident Type::Arb;
	const Ident Type::Node;
	const Ident Type::Lambda;
	const Ident Type::Member;
	const Ident Type::Any;
	const Ident Type::Meta;

	/// tie the whole room together. that is what reflection is, in C++..
	/// it is the rug that makes the language work for you
	Ident::operator Type* () const { return id >= 22 ? (Type*)id : (Type*)Type::primitives[id]; }

	namespace fs = std::filesystem;

	template <typename T>
	inline Type* Id() { return Type::ident<T>(); }

	namespace std { template<> struct hash<Type> { size_t operator()(Type const& id) const { return id; } }; }

	struct  Memory;

	typedef void (*Closure)(void*);

	// it is simplifying to make memory self referencing.
	struct Attachment {
		Attachment* next	= null;
		size_t      id		= 0;
		Memory*     mem		= null;
		Closure     closure = null;
	};

	struct Type;

	/// this is used in initializer_list form internal to dx types.
	/// just initialization of member data
	struct Field {
		///
		Type*		  type;
		size_t		  id;
		idx			  count;
		idx			  reserve;
		Alloc::Flags  flags;
		Memory*       def;
		int			  group; /// used to categorize fields for different use-cases 
							 /// (dx basics use 0, node has 0 and 1 used for internal and external)
		/// 
		template <typename T>
		Field(cchar_t *idt, T def, int gr, Alloc::Flags fla = null) :
			type    (Id<T>()),
			id      (size_t(idt)),
			count   (1),
			reserve (0),
			flags   (fla),
			def     (Memory::valloc(Id<T>(), &def, 1, 0, fla)),
			group   (gr) { }

		/// 
		template <typename T>
		Field(size_t idt, T def, int gr, Alloc::Flags fla = null) :
			type    (Id<T>()),
			id      (size_t(idt)),
			count   (1),
			reserve (0),
			flags   (fla),
			def     (Memory::valloc(Id<T>(), &def, 1, 0, fla)),
			group   (gr) { }

	};

	/// 
    struct Memory {
		Data			  data;
		int				  refs;
		int*			  prefs; /// overlap referencing
		Attachment*		  att;	 /// closure attachments with arg
		/// instances of Memory have identifiers potentially in
		/// the case of a member node (with id of symbolic field),
		/// whose data points to a copy on write instance

		/// size_t for things that can be bound to index, or pointer
		Memory *lookup(size_t id) {
			for (Attachment* a = att; a; a = a->next) {
				if (a->id == id)
					return a->mem;
			}
			assert(false);
			return null;
		}
		///
		template <typename T>
		inline T*         cast()            const { return (T *)data.ptr; }
		inline bool    boolean()            const { return (*data.type->boolean)((Data*)&data); }
		inline int     compare(Memory* rhs) const { return (*data.type->compare)((Data*)&data, &rhs->data); }
		static idx   padded_sz(idx sz, idx al)    { return sz & ~(al - 1) | al; }

		/// central allocation of Memory with a preallocation skip & construct
		/// this is so one can allocate members in stride (atleast at first)
		/// note the indirection done in Data with prefs
		/// dont need to list the ways in which this is better
		static Memory* alloc(Alloc& args, Memory *pre = null) {
			idx    reserve = clamp(args.reserve, idx(0), args.count);
			idx	     	sz = padded_sz(sizeof(Memory) + args.type->sz * max(args.count, reserve), 32); // needs max of count and reserve
			Memory*    mem = pre ? pre : (Memory*)calloc(1, sz);

			if (!pre) {
				Alloc alloc = Alloc(&mem[1], args.count, reserve, args.type, args.flags, args.alloc);
				new (mem) Memory(alloc);
			}

			if (args.alloc != Data::Arb) {
				if (args.type->is_struct()) {
					if (args.ptr)
						(*mem->data.type->cpy)(&mem->data, &args, args.count);
					else
						(*args.type->ctr)(&mem->data);
				}
				else if (args.ptr)
					memcpy(mem->data.ptr, args.ptr, args.type->sz * args.count);
			}

			return mem;
		}

		Memory(const Alloc& alloc) {
			data  = (Data &)alloc;
			refs  = 1;
			prefs = &refs;
		}
		
		template <typename T>
		static Memory*  arb(T* ptr, idx count) {
			idx   		   sz = padded_sz(sizeof(Memory), 32);
			Memory*       mem = (Memory*)calloc(1, sz);
			Alloc        spec = Alloc((void*)&ptr, count, 0, Id<T>(), 0, Alloc::AllocType::Arb);
			return alloc(spec);
		}

		/// reduced these down a lot, gave them names. tired of all of the manage()'ers so they were shown the door.

		/// valloc with optional src pointer for copy (must also include src count)
		static Memory* valloc(Type* type, void* src, idx count, idx rs, Alloc::Flags flags = 0) {
			Alloc        spec = Alloc(src, count, rs, type, flags, Alloc::AllocType::Manage);
			return alloc(spec);
		}

		template <typename T> static Memory* lambda(T* src) {
			return alloc(Type::ident<T,true>(), (void*)src, 1, 0);
		}

		static Memory* field(Field& f, Alloc::Flags af) {
			Alloc   spec = Alloc(null, f.count, f.reserve, f.type, af | f.flags, Alloc::AllocType::Manage);
			Memory*    m = alloc(spec);
			return m;
		}

		static Memory* string(std::string s, idx rs = 0) {
			Alloc spec = Alloc((void*)s.c_str(), s.length(), rs, Id<char>(), 0, Alloc::AllocType::Manage);
			return alloc(spec);
		}

		/// note: its only using bytes because no type is specified
		static Memory* manage(idx byte_count) {
			Alloc spec = Alloc(null, byte_count, 0, null, 0, Alloc::AllocType::Manage);
			return alloc(spec);
		}

		/// change function names.. (eliminate all managers)
		template <typename T>
		static Memory* single(T &v) {
			Alloc spec = Alloc(&v, 1, 0, Id<T>(), 0, Alloc::AllocType::Manage);
			return alloc(spec);
		}

		template <typename T>
		inline bool is_diff(T *p) {
			return data.ptr != p;
		}

		/// copies get reserve from param (their original reserve lost)
		Memory* copy(idx rs) {
			Alloc   spec = Alloc(data.ptr, data.count, rs, data.type, 0, Data::AllocType::Manage);
			return alloc(spec);
		}

		/// attach memory to memory; this is a non-common case but useful for variable dependency of data
		void attach(Attachment &a) {
			auto  att = new Attachment(a);
			grab(att->mem);
			///
			att->next = this->att;
			this->att = att;
		}
	};

	/// this is just a pairing of id and Memory
	/// size_t from Symbol would be needed with dynamic dx; thats up to implementer. (self in future throwing water bottle)
	typedef pair<size_t, Memory*> Member;

	/// todo: getter and setter embedded here when approaching UX port
	struct Prop {
		size_t id;  /// see no point of the string in any form here;
				    /// a const char* is easily separable from an enumerable because we register the symbols
		Data value; /// value store for this property; if no Field is specified we use the default
					/// (possibly for the class, but this would be best integrated as 
				    /// queriable params for use in high-level application prop value selection)
	};

	/// --------------------------------------
	/// dx is M is Meta is basically multiple Memory units bound with ids in Member.
	/// its the switch on the basic type (Array, Object, or single)
	/// --------------------------------------
	struct dx {
		Memory* mem;

		dx(nullptr_t n = null)            { }
	    dx(Memory*        mem) : mem(mem) { grab(mem); }
		
		dx(bool      v) : mem(Memory::single(v)) { }
		dx(float     v) : mem(Memory::single(v)) { }
		dx(double    v) : mem(Memory::single(v)) { }
		dx( int32_t  v) : mem(Memory::single(v)) { }
		dx(uint32_t  v) : mem(Memory::single(v)) { }
		dx( int64_t  v) : mem(Memory::single(v)) { }
		dx(uint64_t  v) : mem(Memory::single(v)) { }

		// internal/external or member or m, mref, ref
		// must use lookup
		template <typename T>
		T& ref(cchar_t* id) { return *lookup(id)->cast<T>(); }

		/// Array use-case, just pass the job to Memory delegate (handle consts as well!)
		template <typename T>
		dx(T* data, idx count = 0, idx reserve = 0)
		{
			static Type*   type = Id<cchar_t> ();
			static Type* s_type = Id<dx>  ();

			/// string-data by form of constant
			if constexpr (std::is_same_v<T, cchar_t>) {
				static std::unordered_map<cchar_t*, Memory*> smem;

				/// you can see here, the symbol map; its preferred to  track to a map of allocated constants
				/// could apply to Any constant with std::is_const_*
				if (smem.count(data) == 0) {
					idx ln = strlen(data);
					Alloc spec = Alloc((void*)data, ln + 1, 0, type, null, Alloc::AllocType::Manage);
					smem[data] = grab(Memory::alloc(spec));
				}
				mem = smem[data];
			
			} else if constexpr (std::is_same_v<T, char>) {
				/// string-data by form of dynamic (copy operation; insertion of null character enforced here)
				size_t s_count = (count ? count : strlen(data)) + 1;
				mem = Memory::valloc(Id<T>(), data, s_count, 0);

			} else {
				/// needs to enforce size at this point; only with strings can one be lazy param-wise
				assert(count || reserve);

				/// normal case, pass through to Memory valloc
				mem = Memory::valloc(Id<T>(), data, count, reserve);
			}
		}

		/// Object use-case; here we lazy-load the class-based functor (must be ONE per type no fancy business)
		/// put all Members in, all external and internals (those are enum classifiers)
		/// mem = Memory { type:T, flags:Object, count:members.size()
		/// call it Meta, MX, Fields, Object; thats a Memory layout not a type; its type is the class being used
		/// 
		dx(FnMembers members, initial<Field> &fields) {  /// field should only be used in FnMember ? .. no both.
			Type* t = type();
			
			/// init functor only called once
			if (!t->schema)
				 t->schema = members();					 /// Memory* of type:T (from Fn, count of total props (Props abstract)
			
			Schema* schema = t->schema;				     /// Schema
			idx      index = 0;
			
			/// ideal memory structure for dx class use-case?  to me it seems like you referenec Obj type for names and defaults, and iterate through
			/// we need to count the total object size; but of course you cannot get it from sizeof(T) correct?
			/// 
			/// 

			/// iterate through fields given by user (referenced from higher level constructor)
			for (Field f: fields) {
				assert(index < schema->count);
				Schema::Row *member = schema->lookup(f.id, index); /// optimize with a sequence state, these should be given in the same order as they are defined
				/// ---------------------------- ///
				assert(member);					 /// member must exist
				assert(f.id == member->id);		 /// Field has same identifier as Member
				/// ---------------------------- ///
				index++;
			}
		}

		template <typename T> T& rise() const { return *(T*)this; }
		template <typename T>
		struct iter {
			T*   start;
			idx  inc;
			
			void operator++() { inc++; }
			void operator--() { inc--; }

			operator    T* () { return &start[inc]; }
			operator    T& () { return  start[inc]; }

			//bool operator==(const iter<T> &b) {
			//	return start == b.start && inc == b.inc;
			//}
		};

	   ~dx()						      { drop(mem); } /// destruct drops
		dx(const dx& r) : mem(r.mem)      { grab(mem); } /// copy grabs
		void        attach(Attachment &a) { assert(mem); mem->attach(a); }
		void*                 raw() const { return  mem ? (void*)mem->data.ptr : (void*)null; }
		operator                   bool() { return  mem; }
		bool                  operator!() { return !mem; }
		
		template <typename T> T* pointer();

		/// lookup field in memory (using the memory as vector for fields)
		Memory *lookup(cchar_t *field) {
			if (mem)
				for (size_t i = 0, mc = mem->data.count; i < mc; i++) {
					Memory &m = mem->data.ref<Memory>(i);
					if (m.data.id == size_t(field))
						return &m;
				}
			return null;
		}

		idx                 count()       const { return  mem ? mem->data.count : 0;    }
		Memory*             ident()       const { return  mem;                          }
		void*             pointer()       const { return  mem ? mem->data.ptr   : null; }
		Type*	             type()       const { return  mem ? mem->data.type  : null; }

		template <typename T>
		bool       inherits()			  const { return type() == Id<T>(); }

		/// come on quantum... hoooold
		dx                quantum()		  const { return (mem && *mem->prefs == 1) ? *this : copy(); }
		idx 			  reserve()		  const { return  mem ?  mem->data.reserve   : 0; }
		Data*				 data()		  const { return  mem ? &mem->data : null; }
		template <typename T> T&        value() { return *(T*)pointer(); }
		template <typename I> I integer_value() { return I((*type()->integer)(data())); }
		template <typename R> R    real_value() { return R((*type()->real)   (data())); }
		dx  *string()				            { return   (*type()->string) (data());  }
		bool operator==(Type& t)		  const { return     type() == &t; } // no need here.
		bool operator==(Type* t)		  const { return     type() ==  t; }
		bool operator==(dx& d)		      const {
			Memory*  a =   mem;
			Memory*  b = d.mem;
			return ((a && b) && (a->data.count == b->data.count))
				? (a->data.type == b->data.type &&
				 (*a->data.type->compare)(&a->data, &b->data) == 0) : (!a && !b);

		}
		bool operator!=(Type*   b)    const { return !(operator==(b)); }
		bool operator!=(Type&   b)    const { return !(operator==(b)); } // no need here.
		bool operator!=(dx& b)        const { return !(operator==(b)); }

		dx copy(idx rs = 0)		      const { return  mem ? mem->copy(rs) : null; }

		/// output up to 8 bytes into the output of hash, directly from memory
		operator Hash() const {
			if (mem->data.flags & Data::Symbolic)
				return {{ .ptr = mem->data.ptr }};

			idx     sz = mem ? (mem->data.count * mem->data.type->sz) : 0;
			uint64_t u = 0;
			if (sz >= sizeof(uint64_t))
				u = *(uint64_t*)mem->data.ptr;
			else {
				char* d = (char *)&u;
				for (idx i = 0; i < sz; i++, d++)
					*d = ((uint8_t*)mem->data.ptr)[i];
			}
			return {{ .u64 = u }};
		}

		template <typename T>
		T &reorient(Flags<Trait> require) {
			Type*  s = type();
			Type*  t = Id<T>();
			assert(t->traits && require); /// i... declare... type-safety --michael scott
			//assert(t == s->origin); 
			return *(T *)((void *)this);
		}

		/// reference dx instance
		dx& operator=(dx r) {
			if (mem != r.mem) {
				drop(mem);
				mem = r.mem;
				grab(mem);
			}
			return *this;
		}

		dx& operator=(nullptr_t n) {
			drop(mem);
			mem = null;
			return *this;
		}

		/// implicit 1-count, Managed mode
		template <typename T>
		dx& operator=(T* ptr);

		/// type-safety on the cast; the design on this is lose
		template <typename T>
		operator T* () const {

#ifndef NDEBUG
			static Type *id = Id<T>();

			if (mem && id != mem->data.type) {
				Type* t0 =  mem->data.type;
				if (id != t0) {
					Type* id = Id<T>();
					auto& a  = id->name;
					auto& b  = t0->name;
					///
					if (a != "void" && b != "void") {
						printf("type cast mismatch on dx %s != %s\n", a.c_str(), b.c_str());
						exit(1);
					}
				}

			}
#endif
			return (T*)(mem ? mem->data.ptr : null); // if we are setting up a table of fields the ptrs should be in stride.
		}

		///
		operator        bool  () const { return mem ? mem->boolean() : false; }
		bool        operator! () const { return !(operator bool()); }

		///
		template <typename T>
		operator           T& () const { return *(T *)mem->data.ptr; }
		operator        void* () const { return mem ?  mem->data.ptr : null; }

		///
		template <typename T> T *cast() const { return  (T *) (mem ? mem->data.ptr : null); }

		template <typename T>
		inline dx  &assign(T& value) {
			Type* t = type();
			if ((void *)Id<T> == (void *)t) {
				t->assign(&mem->data, &value);
			} else {
				/// dislike type case
				assert(false);
			}
			return *this;
		}
	};

	/// for shared objects, the std hash use-case is using its integer functor call (int64_t operator)
	/// this should be reasonable for now, but it could be preferred to hash differently
	namespace std {
		template <> struct hash<dx> {
			size_t operator()(dx &s) const { return Hash(s).v.sz; }
		};
	}

	/// these are easier to pass around than before.
	/// i want to use these more broadly as well, definite a good basis delegate for tensor.
	/// todo: string needs null token in terms of static alloc of 1 byte on string
	template <typename T>
	struct vec:dx {
		vec(Memory* memory) : dx(memory) { } // vec makes use of the vector and type features in Memory

		/// idx allows us to use nullptr_t, otherwise ambiguous alongside size_t
		vec(null_t n = null) : dx() { }

		template <typename X>
		static vec<T> of(idx sz, X x) {
			auto r = Memory::manage<T>(sz);

			if constexpr (std::is_same_v<X, T>)
				for (idx i = 0; i < sz; i++)
					((T*)r->data.ptr)[i] = x; /// silver: this really needs to have template arg too. cant be done here
			else
				for (idx i = 0; i < sz; i++)
					((T*)r->data.ptr)[i] = x(i);

			return vec<T>(r);
		}

	   ~vec()					{ }
		vec(idx sz)			    { dx::mem = Memory::valloc(Id<T>(), (T*)null, sz); }
		vec(vec<T> &b, idx rs)  { dx::mem = b.mem->copy(rs); }
		vec(const vec<T> &b) {
			if (b.mem) {
				grab(b.mem);
				dx::mem = b.mem;
			} else
				dx::mem = null;
		}

		vec(T x)			    { dx::mem = Memory::valloc(Id<T>(), &x, 1); }
		vec(T x, T y)           { T v[2] = { x, y       }; dx::mem = Memory::valloc(Id<T>(), &v[0], 2); }
		vec(T x, T y, T z)      { T v[3] = { x, y, z    }; dx::mem = Memory::valloc(Id<T>(), &v[0], 3); }
		vec(T x, T y, T z, T w) { T v[4] = { x, y, z, w }; dx::mem = Memory::valloc(Id<T>(), &v[0], 4); }
		
		idx  type_size() const { return type()->sz; }
		idx       size() const { return count(); }
		idx size(vec& b) const {
			idx sz = size();
			assert(b.size() == sz);
			return sz;
		}

		T& first() const { return *((T*)mem->data.ptr)[0]; }
		T&  last() const { return *((T*)mem->data.ptr)[mem->data.count - 1]; }

		// push
		vec& push(T v) {
			idx  rs = dx::reserve();
			idx  cn = count();
			if (cn == rs) {
				Memory* p = mem;
				mem = p->copy(::max(idx(8), rs << 1));
				((T*)mem->data.ptr)[rs] = v;
				drop(p);
			} else {
				assert(cn < rs);
				((T*)mem->data.ptr)[cn] = v;
				mem->data.count++;
			}
			return *this;
		}

		void reserve(idx sz) {
			if (dx::reserve() != sz) {
				idx ssz = (sz < mem->data.count) ? sz : mem->data.count;
				*this   = Memory::valloc(Id<T>(), mem->cast<T>(), ssz, sz);
			}
		}

		void clear(idx sz = 0) {
		}

		T&   operator[](idx i) {
			return cast<T>()[i];
		}

		vec&  operator=(const vec& b) {
			if (this != &b)
				mem = b.mem->copy(0);
			return *this;
		}

		vec operator+(vec b) const {
			if constexpr (adds<T>()) {
				vec<T>& a = (vec<T> &) * this;
				idx    sa = size();
				idx    sb = size(b);
				/// only if implemented by T; otherwise it faults

				if (sa == sb) {
					vec c = vec<T>::of(sa, T()); // alloc with
					T* va = (T*)a, *vb = (T*)b, *vc = (T*)c;
					for (idx i = 0; i < sa; i++)
						vc[i] = va[i] + vb[i];
					return c;
				}
				else if (sa > sb) {
					vec c = vec(a);
					T* vb = (T*)b, *vc = (T*)c;
					for (idx i = 0; i < sb; i++)
						vc[i] += vb[i];
					return c;
				}
				vec c = vec(b);
				T* va = (T*)a, *vc = (T*)c;
				for (idx i = 0; i < sa; i++)
					vc[i] += va[i];
				return c;
			} else {
				assert(false);
				return vec();
			}
		}

		vec operator-(vec b) const {
			vec&    a = (vec<T> &) *this;
			idx    sa = size(a);
			idx    sb = size(b);
			///
			if (sa == sb) {
				vec c = vec<T>::of(sa, 0);
				T* va = (T*)a, *vb = (T*)b, *vc = (T*)c;
				for (idx i = 0; i < sa; i++)
					vc[i] = va[i] - vb[i];
				return c;
			} else if (sa > sb) {
				vec c = vec(a);
				T* vb = (T*)b, *vc = (T*)c;
				for (idx i = 0; i < sb; i++)
					vc[i] -= vb[i];
				return c;
			}
			vec c = vec(b);
			T* va = (T*)a, *vc = (T*)c;
			for (idx i = 0; i < sa; i++)
				vc[i] -= va[i];
			return c;
		}

		vec operator*(T b) const {
			vec& a = (vec<T> &) *this;
			vec  c =  vec<T>::of(a.size(), T(0)); // makes no sense to have the X arg
			T*  va = (T*)a, *vc = (T*)c;
			for (idx i = 0, sz = size(); i < sz; i++)
				vc[i] = va[i] * b;
			return c;
		}

		vec operator/(T b) const {
			vec& a = (vec<T> &) *this;
			vec  c =  vec<T>::of(a.size(), 0);
			T*  va = (T*)a, *vc = (T*)c;
			for (idx i = 0, sz = size(); i < sz; i++)
				vc[i] = va[i] / b;
			return c;
		}

		vec& operator+=(vec b) { return (*this = *this + b); }
		vec& operator-=(vec b) { return (*this = *this - b); }
		vec& operator*=(vec b) { return (*this = *this * b); }
		vec& operator/=(vec b) { return (*this = *this / b); }
		vec& operator*=(T   b) { return (*this = *this * b); }
		vec& operator/=(T   b) { return (*this = *this / b); }

		dx::iter<T> begin() const { return dx::iter<T> { *this, 0 }; }
		dx::iter<T>   end() const { return dx::iter<T> { *this, count() }; }

		/// 
		operator Hash() const {
			vec&  a = *this;
			idx	acc = 0;
			int  mp = 1234;
			const bool ish = inherits <T, dx>;
			for (idx i = 0, sz = size(); i < sz; i++) {
				acc += (ish ? Hash(a[i]).v.u64 : idx(a[i])) * mp; // operate on data (recall on dx case)
				mp  *= 11;
			}
			return {{ .f64 = acc }};
		}

		/// this is just the product of the components; it was performed in size_t prior and i dont like that.
		/// it makes sense to have it here, however this NEEDS to be explicit.
		explicit operator T() const {
			vec& a = *this;
			T	ac = 0.0;
			for (idx i = 0, sz = size(); i < sz; i++)
				ac = (i == 0) ? a[i] : (ac * a[i]);
			return ac;
		}

		/// value, point, coord, homogenous
		static void aabb(vec<T> *a, idx sz, vec<T>& mn, vec<T>& mx) {
			mn = a[0];
			mx = a[0];
			for (idx i = 1; i < sz; i++) {
				auto& v = a[i];
				mn      = mn.min(v);
				mx      = mx.max(v);
			}
		}

		inline vec<T>    sqr() { return { *this, [](T v, idx i) { return v * v;			 }}; }
		inline vec<T>    abs() { return { *this, [](T v, idx i) { return std::abs   (v); }}; }
		inline vec<T>  floor() { return { *this, [](T v, idx i) { return std::floor (v); }}; }
		inline vec<T>   ceil() { return { *this, [](T v, idx i) { return std::ceil  (v); }}; }

		inline vec<T>  clamp( vec<T> mn, vec<T> mx )  {
			return { *this, [&](T& v, idx i) { return std::clamp(v, mn[i], mx[i]); }};
		}

		friend auto operator<<(std::ostream& os, vec<T> const& m) -> std::ostream& {
			vec& a = *this;
			os    << "[";
			for (idx i = 0, sz = size(); i < sz; i++) {
				os << decimal(a[i], 4);
				if (i != 0) os << ",";
			}
			os     << "]";
			return os;
		}

		inline     T  max()          const {
			vec& a = *this;
			T mx = a[0];
			for (idx i = 1, sz = size(); i < sz; i++)
				mx = std::max(mx, a[i]);
			return mx;
		}

		inline     T  min()          const {
			vec& a = *this;
			T mn = a[0];
			for (idx i = 1, sz = size(); i < sz; i++)
				mn = std::min(mn, a[i]);
			return mn;
		}

		inline vec<T> min(    T  mn) const { return { *this, [](T v, idx i) { return std::min(v, mn);    }}; }
		inline vec<T> max(    T  mx) const { return { *this, [](T v, idx i) { return std::max(v, mx);    }}; }
		inline vec<T> min(vec<T> mn) const { return { *this, [](T v, idx i) { return std::min(v, mn[i]); }}; }
		inline vec<T> max(vec<T> mx) const { return { *this, [](T v, idx i) { return std::max(v, mx[i]); }}; }

		static double area(vec<T> *p, idx sz) {
			double area = 0.0;
			const int n = int(p.size());
			int       j = n - 1;

			// shoelace formula
			for (int i = 0; i < n; i++) {
				vec<T>& pj = p[j];
				vec<T>& pi = p[i];
				area      += (pj.x + pi.x) * (pj.y - pi.y);
				j          = i;
			}

			// return absolute value
			return std::abs(area / 2.0);
		}

		T len_sqr() const {
			vec&    a = *this;
			idx    sz = size();
			T	   sq = 0;
			for (idx i = 0; i < sz; i++)
				sq   += a[i] * a[i];
			return sq;
		}

		T             len() const { return sqrt(len_sqr()); }
		vec<T>  normalize() const { return *this / len(); }
		T*			 data() const { return (T*)(mem ? mem->data.ptr : null); }
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
		dx      data;
	
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
	template<typename T> struct is_std_vec<std::vector<T>> : std::true_type  {};

	struct  node;
	struct  string;

	typedef lambda<bool()>       FnBool;
	typedef lambda<void(void*)>  FnArb;
	typedef lambda<void(void)>   FnVoid;

	template <typename T>
	struct sp:dx {
		sp() { }
		sp(Memory    *mem) : dx(mem) { }
		sp(const sp<T>& a) : dx(a.mem) { }
		sp(T&         ref) : dx(Memory::valloc(Id<T>(), &ref, 1)) { }
	    sp(T* ptr,  idx c) : dx(Memory::valloc(Id<T>(),  ptr, c)) { assert(c); }

		///
		sp<T>& operator=(sp<T>& ref) {
			if (mem != ref.mem) {
				drop(mem);
				mem = ref.mem;
				grab(mem);
			}
			return *this;
		}

		///
		sp<T>& operator=(T ref) {
			drop(mem);
			if constexpr (!std::is_same_v<T, nullptr_t>) {
				mem = Memory::valloc(Id<T>(), new T(ref), 1);
				grab(mem);
			} else {
				mem = null;
			}
			return *this;
		}

		///
		sp<T>& operator=(T* ptr) {
			if (!mem || mem->is_diff(ptr)) {
				drop(mem);
				mem = ptr ? Memory::valloc(Id<T>(), ptr, 1) : null;
			}
			return *this;
		}

		///
		operator                  T* () const { return  (T*)mem->data.ptr; } ///    T* operator
		operator                  T& () const { return *(T*)mem->data.ptr; } ///    T& operator
		T&                operator  *() const { return *(T*)mem->data.ptr; } /// deref operator
		T*                operator ->() const { return  (T*)mem->data.ptr; } /// reference memory operator
		bool                   is_set() const { return      mem != null;   }
		bool operator==      (sp<T>& b) const { return      mem == b.mem;  }
		bool operator==(const sp<T>& b) const { return      mem == b.mem;  }

		T&   operator[]       (idx   i) const {
			assert(i >= 0 && i < mem->data.count);
			T*  origin = mem->data.ptr;
			T& indexed = origin[i];
			return indexed;
		}
	};

	std::mutex& single_mx() {
		static std::mutex mx;
		return mx;
	}

	///
	Memory *grab(Memory *m) {
		std::mutex& mx = single_mx();
		mx.lock();
		(*m->prefs)++;
		mx.unlock();
		return m;
	}

	///
	Memory *drop(Memory* m) {
		std::mutex& mx = single_mx();
		mx.lock();
		if (--m->refs == 0) {
			(*m->data.type->dtr)(&m->data);
			for (Attachment* a = m->att, *n = null; a; a = n) {
				n =  a->next;
				if  (a->closure) a->closure(a->mem);
				drop(a->mem);
			}
			mx.unlock();
			delete m;
		} else
			mx.unlock();
		return m;
	}

	// deprecate VAlloc
	// outside resources should be the only 'Arb' in
					   struct Arb     : dx { Arb (void* p, size_t c = 1) { mem = Memory::arb    (p, c); }};
	template <class T> struct Persist : dx { Persist(T* p, size_t c = 1) { mem = Memory::arb    (p, c); }};
	template <class T> struct Manage  : dx { Manage (T* p, size_t c = 1) { mem = Memory::valloc (Id<T>(), p,    c, 0); }};
	template <class T> struct VAlloc  : dx { VAlloc (      size_t c = 1) { mem = Memory::valloc (Id<T>(), null, c, 0); }};

	template <typename T>
	T fabs(T x) { return x < 0 ? -x : x; }
	
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
	dx& dx::operator=(T* ptr) {
		if (!mem || mem->data.ptr != ptr) {
			drop(mem);
			mem = ptr ? Memory::manage(ptr, 1) : null;
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
	T* dx::pointer() {
		assert(!mem || mem->data.type == Id<T>());
		return (T*)raw();
	}
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
			global.e.seed(uint32_t(seed)); // todo: seed with 64b, doesnt work on windows to be 32b seed
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

	template <typename T>
	struct iter {
		const T*   start;
		idx        inc;

		void operator++() { inc++; }
		void operator--() { inc--; }

		operator    T* () { return &start[inc]; }
		operator    T& () { return  start[inc]; }

		bool operator==(const iter<T> &b) {
			return start == b.start && inc == b.inc;
		}
	};

	template <typename T>
	struct array { 
	protected:
		vec<dx>  a;

		/// here we be preventing the code bloat. ar.
		vec<dx>& valloc(idx sz) { a = vec<dx>(a, sz); return a; }

		vec<dx>& ref(size_t res = 0) { return a ? a : valloc(res); }

	public:
		typedef lambda<dx(size_t)> SharedIFn;

		array() { }
		array(initial <dx> v) {
			valloc(v.size());
			for (auto &i: v)
				a += i;
		}
		array(idx   sz, dx         v) { valloc(sz); for (idx i = 0; i < sz; i++) a += v.copy(); }
		array(idx   sz, SharedIFn fn) { valloc(sz); for (idx i = 0; i < sz; i++) a +=    fn(i); }
		array(null_t               n) { }
		array(size_t              sz) { valloc(sz); }
		
		/// quick-sort, documented best of my ability
		array<T> sort(lambda<int(T &a, T &b)> cmp) {
			auto      &self = ref();

			/// create reference list of identical size as given, pointing to the respective index
			vec<dx *> refs = { size(), [&](size_t i) { return &self[i]; }};

			lambda <void(int, int)> qk;
				qk = [&](int s, int e) {
				
				/// sanitize
				if (s >= e)
					return;

				/// i = start, j = end (if they are the same index, no need to sort 1)
				int i  = s, j = e, pv = s;

				///
				while (i < j) {
					/// standard search pattern
					while (cmp((*refs[i]).value<T>(), (*refs[pv]).value<T>()) <= 0 && i < e)
						i++;
                
					/// order earl grey and have data do something on his screen
					while (cmp((*refs[j]).value<T>(), (*refs[pv]).value<T>()) > 0)
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
			
			/// resolve refs, returning sorted.
			return { size(), [&](size_t i) { return *refs[i]; }};
		}
    
		dx                  &back()              { return ref().back();                }
		dx			       &front()              { return ref().front();               }
		int                   isz()        const { return a ? int(a->size())    : 0;   }
		idx                    sz()		   const { return a ?     a->size()     : 0;   }
		idx                  size()		   const { return a.size();                    }
		idx              capacity()        const { return a.capacity();                }
		vec<dx>	          &vector()              { return ref();                       }
		T& operator       [](idx i)				 { return ref()[i];					   }

		dx::iter<T> begin() { return dx::iter<T> { a, 0 }; }
		dx::iter<T>   end() { return dx::iter<T> { a, a.size() }; } // rid the size_t here too

		inline void operator    += (dx v)    { ref().push(v);					   }
		inline void operator    -= (int i)       { erase(i);						   }
				  operator   bool()        const { return a && a.size() > 0;           }
		bool      operator      !()        const { return !(operator bool());          }
		bool      operator!=     (array<T> b)    { return  (operator==(b));            }
		bool      operator==     (array<T> b)    {
			idx  sz = size();
			bool eq = sz == b.size();
			if (eq && sz) {
				for (idx i = 0; i < sz; i++)
					if ((dx&)a[i] != (dx&)b[i]) {
						eq = false;
						break;
					}
			}
			return eq;
		}
		
		///
		array<T> &operator=(const array<T> &ref) {
			if (this != &ref)
				a = ref.a;
			return *this;
		}

		/// now we have clash between size nad index.
		/// if not idx then what.. what is sizable and index and not Size.
		void expand(idx sz, T f) {
			for (idx i = 0; i < sz; i++)
				a->push_back(f);
		}

		void   erase(int index)             { if (index >= 0) a->erase(a->begin() + size_t(index)); }
		void   reserve(size_t sz)           { ref().reserve(sz); }
		void   clear(size_t sz = 0)         { ref().clear(); if (sz) reserve(sz); }
		void   resize(size_t sz, T v = T()) { ref().resize(sz, v); }
    
		int count(T v) const {
			int r = 0;
			if (a) for (auto &i: *a)
				if (v == i)
					r++;
			return r;
		}
    
		// historically these mutate the data
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
			int index  = 0;
			dx& vs = v;
			for (dx &i: a) {
				if (i.mem == vs.mem)
					return index;
				index++;
			}
			return -1;
		}
	};

	typedef array<dx> Array;

	template<typename T>
	struct is_vec<array<T>>  : std::true_type  {};

	template <typename T>
	bool equals(T v, array<T> values) { return values.index_of(v) >= 0; }

	template <typename T>
	bool isnt(T v, array<T> values)   { return !equals(v, values); }


	template <const Stride S>
	struct Shape {
		struct Data {
			array<idx> shape;
			Stride     stride;
		};
		sp<Data> data;

		int      dims() const { return int(data->shape.size()); } // sh can have accessors for a type vec
		idx      size() const {
			idx   r     = 0;
			bool  first = true;
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
		idx operator [] (idx i) { return data->shape[i]; }

		///
		Shape(array<int> sz)                  : data(sz) { }
		Shape(idx d0, idx d1, idx d2, idx d3) : data(new Data {{ d0, d1, d2, d3 }, S }) { }
		Shape(idx d0, idx d1, idx d2)         : data(new Data {{ d0, d1, d2 },     S }) { }
		Shape(idx d0, idx d1)                 : data(new Data {{ d0, d1 },		   S }) { }
		Shape(idx d0)                         : data(new Data {{ d0 },			   S }) { }
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
		std::vector<idx>::iterator begin() { return data->shape.begin(); }
		std::vector<idx>::iterator end()   { return data->shape.end(); }
	};
	//}
///}

	template <typename T>
	struct Vec2:vec<T> {
		T &x, &y;
		Vec2(null_t n = null)   : vec<T>(),       x(def<T>()), y(def<T>()) { }
		Vec2(T x, T y)	        : vec<T>(x, y),   x((*this)[0]), y((*this)[1]) { }
		Vec2(const vec<T>& ref) : vec<T>(ref),    x((*this)[0]), y((*this)[1]) { }
	};

	template <typename T>
	struct Vec3:vec<T> {
		T &x, &y, &z;
		Vec3(null_t n = null)   : vec(), x(def<T>()), y(def<T>()), z(def<T>()) { }
		Vec3(T x, T y, T z)     : vec<T>(x, y, z), x((*this)[0]), y((*this)[1]), z((*this)[2])   { }
		Vec3(const vec<T>& ref) : vec<T>(ref),     x((*this)[0]), y((*this)[1]), z((*this)[2])   { }
	};

	template <typename T>
	struct Vec4:vec<T> {
		T &x, &y, &z, &w;
		Vec4(null_t n = null)    : vec(),			   x(def<T>()), y(def<T>()), z(def<T>()), w(def<T>()) { }
		Vec4(T x, T y, T z, T w) : vec<T>(x, y, z, w), x((*this)[0]), y((*this)[1]), z((*this)[2]), w((*this)[3]) { }
		Vec4(const vec<T>& ref)  : vec<T>(ref),        x((*this)[0]), y((*this)[1]), z((*this)[2]), w((*this)[3]) { }
	};

	template <typename T>
	static inline T decimal(T x, size_t dp) {
		T sc = std::pow(10, dp);
		return std::round(x * sc) / sc;
	}

	template <typename T>
	struct Edge {
		vec<T> &a;
		vec<T> &b;
    
		/// returns x-value of intersection of two lines
		inline T x(vec<T> c, vec<T> d) {
			T  ax = a[0], ay = a[1];
			T  bx = b[0], by = b[1];
			T  cx = c[0], cy = c[1];
			T  dx = d[0], dy = d[1];
			
			T num = (ax*by  -  ay*bx) * (cx-dx) - (ax-bx) * (cx*dy - cy*dx);
			T den = (ax-bx) * (cy-dy) - (ay-by) * (cx-dx);
			return num / den;
		}
    
		/// returns y-value of intersection of two lines
		inline T y(vec<T> c, vec<T> d) {
			T  ax = a[0], ay = a[1];
			T  bx = b[0], by = b[1];
			T  cx = c[0], cy = c[1];
			T  dx = d[0], dy = d[1];

			T num = (ax*by  -  ay*bx) * (cy-dy) - (ay-by) * (cx*dy - cy*dx);
			T den = (ax-bx) * (cy-dy) - (ay-by) * (cx-dx);
			return num / den;
		}
    
		/// returns xy-value of intersection of two lines
		inline vec<T> xy(vec<T> c, vec<T> d) { return { x(c, d), y(c, d) }; }
	};


	template <typename T>
	struct Rect: vec<T> {
		T &x, &y, &sx, &sy;

		Rect(null_t n = null) : vec(), x(def<T>()),  y(def<T>()), sx(def<T>()), sy(def<T>()) { }
		Rect(T x, T y, T sx, T sy)	  : vec(x, y, sx, sy), x(x), y(y), sx(sx), sy(sy) { }
		
		Rect(vec<T> pos, vec<T> sz, bool ctr) {
			T px = pos[0];
			T py = pos[1];
			T sx =  sz[0];
			T sy =  sz[1];
			if (ctr)
				*this = { px - sx / 2, py - sy / 2, sx, sy };
			else
				*this = { px, py, sx, sy };
		}

		Rect(vec<T> s, vec<T> e) {
			T  x0 = min(s[0], e[0]);
			T  y0 = min(s[1], e[1]);
			T  x1 = max(s[0], e[0]);
			T  y1 = max(s[1], e[1]);
			*this = { x0, y0, x1 - x0, y1 - y0 };
		}
    
		inline Rect<T> offset(T a) const { return { x - a, y - a, sx + (a * 2), sy + (a * 2) }; }

		Vec2<T>               xy() const { return { x, y }; }
		Vec2<T>           center() const { return { x + sx / 2.0, y + sy / 2.0 }; }
		bool   contains (vec<T> p) const {
			T px = p[0], py = p[1];
			return px >= x && px <= (x + sx) &&
				   py >= y && py <= (y + sy);
		}
		bool    operator== (const Rect<T> &r) { return x == r.x && y == r.y && sx == r.sx && sy == r.sy; }
		bool    operator!= (const Rect<T> &r) { return !operator==(r); }
		///
		Rect<T> operator + (Rect<T> r)        { return { x + r.x, y + r.y, sx + r.sx, sy + r.sy }; }
		Rect<T> operator - (Rect<T> r)        { return { x - r.x, y - r.y, sx - r.sx, sy - r.sy }; }

		// make this behaviour implicit to vec<T>
		Rect<T> operator + (vec<T> v)         { return { x + v.x, y + v.y, sx, sy }; }
		Rect<T> operator - (vec<T> v)         { return { x - v.x, y - v.y, sx, sy }; }
    
		///
		Rect<T> operator * (T r) { return { x * r, y * r, sx * r, sy * r }; }
		Rect<T> operator / (T r) { return { x / r, y / r, sx / r, sy / r }; }
		operator Rect<int>    () { return {    int(x),    int(y),    int(sx),    int(sy) }; }
		operator Rect<float>  () { return {  float(x),  float(y),  float(sx),  float(sy) }; }
		operator Rect<double> () { return { double(x), double(y), double(sx), double(sy) }; }

		Rect<T>(vec<T>& a) : vec<T>(a), x(a[0]), y(a[1]), sx(a[2]), sy(a[3]) { }
    
		array<Vec2<T>> edges() const {
			return {{x,y},{x+sx,y},{x+sx,y+sy},{x,y+sy}};
		}
    
		array<Vec2<T>> clip(array<Vec2<T>> &poly) const {
			const Rect<T> &clip = *this;
			array<Vec2<T>>    p = poly;
			array<Vec2<T>>    e = clip.edges();
			for (idx i = 0; i < e.size(); i++) {
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


//export module str;
//export {
	typedef int ichar;

	// ----------------------------------------------------------
	// UTF8 :: Bjoern Hoehrmann <bjoern@hoehrmann.de>
	// https://bjoern.hoehrmann.de/utf-8/decoder/dfa/
	// ----------------------------------------------------------

	struct utf {
		static int decode(uint8_t* cstr, lambda<void(uint32_t)> fn = {}) {
			if (!cstr) return 0;

			static const uint8_t utable[] = {
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
				7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
				8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
				0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
				0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
				0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
				1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
				1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
				1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
			};

			uint32_t code;
			uint32_t state = 0;
			size_t   ln    = 0;
			auto     dc    = [&](uint32_t input) {
				uint32_t u = utable[input];
				code	   = (state != 1) ? (input & 0x3fu) | (code << 6) : (0xff >> u) & input;
				state	   = utable[256 + state * 16 + u];
				return state;
			};

			for (uint8_t *cs = cstr; *cs; ++cs)
				if (!dc(*cs)) {
					if (fn) fn(code);
					ln++;
				}

			return (state != 1) ? -1 : int(ln);
		}

		static int len(uint8_t* cstr) {
			return decode(cstr, null);
		}
	};

	struct str:dx {

		template <typename T>
		struct iter {
			const T*   start;
			idx        inc;

			void operator++() { inc++; }
			void operator--() { inc--; }

			operator    T* () const { return (T*)&start[inc]; }
			operator    T& () const { return (T&) start[inc]; }

			bool operator==(const iter<T> b) { return start == b.start && inc == b.inc; }
			operator size_t()				 { return inc; }
		};

		enum MatchType {
			Alpha,
			Numeric,
			WS,
			Printable,
			String,
			CIString
		};

		str expr(lambda<str(str)> fn) {
			char*  a  = data();
			auto   be = [&](int i) -> bool { return a[i] == '{' && a[i + 1] != '{'; };
			auto   en = [&](int i) -> bool { return a[i] == '}'; };
			bool   in = be(0);
			int    fr = 0;
			idx    ln = byte_len();
			str    rs = str(ln * 4);

			for (int i = 0; i <= ln; i++)
				if (i == ln || be(i) || en(i)) {
					int cr = int(i - fr);
					if (cr > 0)
						rs += in ? str(&a[fr + 1], cr - 2) : str(&a[fr], cr);
					fr = i;
					in = be(i);
				}

			return rs;
		}

		str format(Array args) { return expr([&](str e) -> str { return args[e]; }); }

		str(dx &sh)	       : dx(sh)   { }
		str()              : dx()     { }
		str(cchar_t* cstr) : dx(cstr) { } // symbols potential here, its slightly easy to cross up but thats a real distinguisher, const vs non-const.
		str(char* s, idx len, idx rs = 32) : dx(s, len + 1, clamp(rs, idx(0), len << 1)) { }

		/// conversion routines
	    str(char            ch) : dx(Memory::valloc(Id<char>(), &ch, 1, 32)) { }
		str(size_t          sz) : dx(Memory::valloc(Id<char>(), null, sz, 0))   { }
		str(int32_t        i32) : dx(Memory::string(std::to_string(i32)))    { }
		str(int64_t        i64) : dx(Memory::string(std::to_string(i64)))    { }
		str(double         f64) : dx(Memory::string(std::to_string(f64)))    { }
		str(std::string    str) : dx(Memory::string(std::string(str)))       { }
		str(const str       &s) : dx(s.mem)								     { }

		/// dx can possibly do a bit of validation
		iter<char> begin() { return { data(), 0 }; }
		iter<char>   end() { return { data(), mem ? mem->data.count : 0 }; }

		// calling a string can format it.
		// str a = str("{2} is {0} but {1}")(1, 2, 3.0)
		// "3.0 is 2 but 1"
		
		char*		data()                 const { return (char *)(mem ? mem->data.ptr   : null); }
		idx         byte_len()             const { return         (mem ? mem->data.count :    0); }
		bool        contains(array<str> a) const { return index_of_first(a, null) >= 0;           }

		str operator+ (const str b) const {
			const str& a = *this;
			char*     ap = a.data();
			char*     bp = b.data();
			///
			if (!ap) return b.copy();
			if (!bp) return a.copy();
			///
			str      c  = a.copy(a.reserve());
			char*    cp = c.data();

			idx ac = a.mem->data.count;
			idx bc = b.mem->data.count;
			memcpy(&((uint8_t *)cp)[ac - 1], bp, bc);
			c.mem->data.count = ac + bc - 1;
			return c;
		}
		idx         size()						     const { return mem ? mem->data.count : 0;      }
		str         operator+  (cchar_t* s)          const { return *this + str(s);                 }
		str         operator() (int   s,  int     l) const { return substr(s, l);                   }
		str         operator() (int				  s) const { return substr(s);                      }
		bool        operator<  (const str         b) const { return strcmp(data(), b.data()) <  0;  }
		bool        operator>  (const str         b) const { return strcmp(data(), b.data()) >  0;  }
		bool        operator<  (cchar_t          *b) const { return strcmp(data(), b)		 <  0;  }
		bool        operator>  (cchar_t          *b) const { return strcmp(data(), b)		 >  0;  }
		bool        operator<= (const str         b) const { return strcmp(data(), b.data()) <= 0;  }
		bool        operator>= (const str         b) const { return strcmp(data(), b.data()) >= 0;  }
		bool        operator<= (cchar_t          *b) const { return strcmp(data(), b)		 <= 0;  }
		bool        operator>= (cchar_t          *b) const { return strcmp(data(), b)		 >= 0;  }
	  //bool        operator== (std::string       b) const { return strcmp(data(), b.c_str()) == 0; }
	  //bool        operator!= (std::string       b) const { return strcmp(data(), b.c_str()) != 0; }
		bool        operator== (const str         b) const { return strcmp(data(), b.data())  == 0; }
		bool        operator== (cchar_t          *b) const { return strcmp(data(), b)		  == 0; }
		bool        operator!= (cchar_t          *b) const { return strcmp(data(), b)		  != 0; }
		char&		operator[] (size_t            i) const { return ((char *)mem->data.ptr)[i];     }
		int         operator[] (str               b) const { return index_of(b); }
		void        operator+= (str               b)       {
			if (mem->data.reserve > (mem->data.count + b.mem->data.count)) {
				char*  ap = (char*)  mem->data.ptr;
				char*  bp = (char*)b.mem->data.ptr;
				idx&   ac =   mem->data.count;
				idx&   bc = b.mem->data.count;
				ac		 += bc;
				memcpy(ap, bp, bc);
			} else
				*this     = *this + b;
		}
		void operator+= (cchar_t     b) { operator+=(str(b)); }
		void operator+= (cchar_t    *b) { operator+=(str(b)); }
		
			 operator     bool()  const { return mem && mem->data.count > 1; }
		bool operator!()          const { return !(operator bool());         }
			 operator  int64_t()  const { return integer_value();            }
			 operator     real()  const { return real_value();               }

		str &operator= (const str &s) {
			if (this != &s) {
				drop(mem);
				mem = s.mem;
				grab(mem);
			}
			return *this;
		}

		friend std::ostream &operator<< (std::ostream& os, str const& s) {
			return os << std::string((cchar_t *)s.data());
		}

		static str fill(idx n, lambda<int(idx)> fn) {
			auto ret = str(n);
			for (idx i = 0; i < n; i++)
				ret += fn(i);
			return ret;
		}

		int index_of_first(array<str> elements, int *str_index) const {
			int  less  = -1;
			int  index = -1;
			for (str &find:elements) {
				++index;
				int i = index_of(find);
				if (i >= 0 && (less == -1 || i < less)) {
					less = i;
					if (str_index)
					   *str_index = index;
				}
			}
			if (less == -1 && str_index) *str_index = -1;
			return less;
		}

		bool starts_with(cchar_t *s) const {
			size_t l0 = strlen(s);
			size_t l1 = len();
			if (l1 < l0)
				return false;
			return memcmp(s, data(), l0) == 0;
		}

		idx len() const { return utf::len(mem ? (uint8_t*)mem->data.ptr : (uint8_t*)null); }

		bool ends_with(cchar_t *s) const {
			size_t l0 = strlen(s);
			size_t l1 = len();
			if (l1 < l0)
				return false;
			char *e = &data()[l1 - l0];
			return memcmp(s, e, l0) == 0;
		}

		static str read_file(std::ifstream& in) {
			std::ostringstream st; st << in.rdbuf();
			return st.str();
		}

		static str read_file(fs::path path) {
			std::ifstream in(path);
			return read_file(in);
		}
    
		str lowercase() const {
			str&   r  = copy();
			char*  rp = r.data();
			idx    rc = r.byte_len();
			for (idx i = 0; i < rc; i++) {
				char c = rp[i];
				rp[i]  = (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
			}
			return r;
		}

		str uppercase() const {
			return copy();
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


		str replace(str fr, str to, bool all = true) const {
			str&   sc   = (str&)*this;
			char*  sc_p = sc.data();
			char*  fr_p = fr.data();
			char*  to_p = to.data();
			int    sc_c = std::max(0, int(sc.byte_len()) - 1);
			int    fr_c = std::max(0, int(fr.byte_len()) - 1);
			int    to_c = std::max(0, int(to.byte_len()) - 1);
			int    diff = std::max(0, int(to_c) - int(fr_c));
			str    res  = str(size_t(sc_c + diff * (sc_c / fr_c + 1)));
			char*  rp   = res.data();
			idx    w    = 0;
			bool   once = true;

			/// iterate over string, check strncmp() at each index
			for (size_t i = 0; i < sc_c; ) {
				if ((all || once) && strncmp(&sc_p[i], fr_p, fr_c) == 0) {
					/// write the 'to' string, incrementing count to to_c
					memcpy (&rp[w], to_p, to_c);
					i   += to_c;
					w   += to_c;
					once = false;
				} else {
					/// write single char
					rp[w++] = sc_p[i++];
				}
			}

			/// end string, set count (count includes null char in our data)
			rp[w++] = 0;
			res.mem->data.count = w;

			/// do i know what im doing?
			assert(w <= res.mem->data.reserve);
			return res;
		}

		// risky business.
		str substr(int start, size_t len) const { return str(&data()[start], len); }
		str substr(int start)			  const { return str(&data()[start]);      }

		array<str> split(str delim)       const {
			array<str>  result;
			int  start     = 0;
			int  end       = 0;
			int  delim_len = int(delim.byte_len());
			str &a         = (str&)*this;
			///
			if (a.len() > 0) {
				while ((end   = a.index_of(delim, start)) != -1) {
					result   += a.substr(start, end - start);
					start     = end + delim_len;
				}
				result       += a.substr(start);
			} else
				result       += str();
			///
			return result;
		}

		array<str> split(cchar_t *delim) const { return split(str(delim)); }
		
		// color we like: "#45a3dd"

		array<str> split() {
			array<str> result;
			str        chars;
			///
			for (char &c: *this) {
				bool is_ws = isspace(c) || c == ',';
				if (is_ws) {
					if (chars) {
						result += chars;
						chars   = "";
					}
				} else
					chars += c;
			}
			///
			if (chars || !result)
				result += chars;
			///
			return result;
		}

		int index_of(str b, bool rev = false) const {
			if (!b.mem || b.mem->data.count <= 1) return  0; /// empty string permiates. its true.
			if (!  mem ||   mem->data.count <= 1) return -1;

			char*  ap =   data();
			char*  bp = b.data();
			int    ac = int(  mem->data.count);
			int    bc = int(b.mem->data.count);
			
			/// dont even try if b is larger
			if (bc > ac) return -1;

			/// search for b, reverse or forward dir
			if (rev) {
				for (int index = ac - bc; index >= 0; index--) {
					if (strncmp(&ap[index], bp, bc) == 0)
						return index;
				}
			} else {
				for (int index = 0; index <= bc - ac; index++)
					if (strncmp(&ap[index], bp, bc) == 0)
						return index;
			}
			
			/// we aint found ****. [/combs]
			return -1;
		}

		int index_of(MatchType ct, cchar_t *mp = null) const {
			typedef lambda<bool(cchar_t &)> Fn;
			typedef std::unordered_map<MatchType, Fn> Map;
			///
			int index = 0;
			char *mp0 = (char *)mp;
			if (ct == CIString) {
				size_t sz = size();
				mp0       = new char[sz + 1];
				memcpy(mp0, data(), sz);
				mp0[sz]   = 0;
			}
			auto   test = [&](cchar_t& c) -> bool { return  false; };
			auto f_test = lambda<bool(cchar_t&)>(test);

			/*
				
			auto m = Map{
				{ Alpha,     [&](cchar_t    &c) -> bool { return  isalpha (c);            }},
				{ Numeric,   [&](cchar_t    &c) -> bool { return  isdigit (c);            }},
				{ WS,        [&](cchar_t    &c) -> bool { return  isspace (c);            }}, // lambda must used copy-constructed syntax
				{ Printable, [&](cchar_t    &c) -> bool { return !isspace (c);            }},
				{ String,    [&](cchar_t    &c) -> bool { return  strcmp  (&c, mp0) == 0; }},
				{ CIString,  [&](cchar_t    &c) -> bool { return  strcmp  (&c, mp0) == 0; }}
			};

			*/
			Fn zz;
			Fn& fn = zz;// m[ct];
			for (char &c:(str&)*this) {
				if (fn(c))
					return index;
				index++;
			}
			if (ct == CIString)
				delete mp0;
        
			return -1;
		}

		int64_t integer_value() const {
			cchar_t    *c = data();
			while (isalpha(*c))
				c++;
			return len() ? int64_t(strtol(c, null, 10)) : int64_t(0);
		}

		real real_value() const {
			char *c = data();
			while (isalpha((cchar_t)*c))
				c++;
			return strtod(c, null);
		}

		bool has_prefix(str i) const {
			char    *s = data();
			size_t isz = i.byte_len();
			size_t  sz =   byte_len();
			return  sz >= isz ? strncmp(s, i.data(), isz) == 0 : false;
		}

		bool numeric() const {
			char *a = data();
			return a != "" && (a[0] == '-' || isdigit(*a));
		}
    
		/// wildcard match
		bool matches(str patt) const {
			lambda<bool(char*, char*)> fn;
			str&  a = (str &)*this;
			str&  b = patt;
			     fn = [&](char* s, char* p) -> bool {
				 return (p && *p == '*') ? ((*s && fn(&s[1], &p[0])) || fn(&s[0], &p[1])) :
										    (!p || (*p == *s         && fn(&s[1], &p[1])));
			};
			
			for (char *ap = a.data(), *bp = b.data(); *ap != 0; ap++)
				if (fn(ap, bp))
					return true;

			return false;
		}

		str trim() const {
			char*  s = data();
			size_t c = byte_len();
			size_t h = 0;
			size_t t = 0;

			/// trim head
			while (isspace(s[h]))
				h++;

			/// trim tail
			while (isspace(s[c - 1 - t]) && (c - t) > h)
				t++;

			/// leave body
			return str(&s[h], c - (h + t));
		}
	};

	inline str operator+(cchar_t *cstr, str rhs) {
		return str(cstr) + rhs;
	}

	// ref std::string in a Memory -> str resolution down here.

	template<> struct is_strable<str> : std::true_type  {};

	template <typename T>
	dx *str_ref(T *ps) {
		return ps ? &(dx&)str(*ps) : null;
	}

	///
	struct Symbol {
		int   value;
		str   symbol;
		bool operator==(int v) { return v == value; }
	};

	typedef array<Symbol> Symbols;
	typedef array<str>    Strings;
//};
	
//export module map;
//export {

	// do me first.
	template <typename K, typename V>
	struct hashlist:dx {
		typedef pair<K,V>  pair;
		typedef list<pair> list;
		typedef list::IX   IX;

		V& operator[](K key) {
			// different constexpr ops for dealing with type K
			Symbol  sym = key;
			size_t hash = size_t(sym);
			return *(V*)lookup(hash);
		}

		// use the Memory* in dx as the fifo sequence
	};

	/// no non-sense map
	/// keep it fifo. thats pro-pattern.
	template <typename K, typename V>
	struct map:dx {
		typedef pair<K, V>      pair;
		typedef list<pair>::IX  IX;
		typedef list<pair>      list;

		list& a;

		map()				     : dx({"a", list()}), a(ref<list>("a")) { }
		map(null_t  n)		     : a(def<list>()) { }
		map(initial<pair> pairs) : dx({"a", list(pairs.size())}), a(ref<list>("a")) {
			for (auto &p:pairs)
				a += p;
		}

		idx count() const { return a.count(); }

		IX* find(K &k, idx *pi = null) const {
			idx index = 0;
			// mx lock needed in list.
			for (auto ix = a.ifirst(); ix; ix = ix->next) {
				if (k == ix->data.key) {
					if (pi)
					   *pi = index;
					return ix;
				}
				index++;
			}
			return null;
		}

		inline V& operator[](K k) const {
			auto ix = find(k);
			if  (ix) return ix->data.value;
			a += { k, V() };
			return a.last().value;
		}

		inline idx  count(K k) const { return  find(k) ? 1 : 0; }
		inline bool keyed(K k) const { return count(k) > 0;     }

		inline bool erase(K k) {
			idx index = 0;
			IX*     i = find(k, &index);
			if (i)
				a.unlink(i);
			return i != null;
		}
	};

	template <typename V> struct is_array <array <V>> : std::true_type { };

	typedef   map<dx,dx> Map;

	template <typename K, typename V> struct is_map <map<K,V>> : std::true_type  { };

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

	// just a mere lexical user of cwd
	struct dir {
		fs::path prev;
		 dir(fs::path p) : prev(fs::current_path()) { chdir(p.string().c_str()); }
		~dir()										{ chdir(prev.string().c_str()); }
	};

	struct Path:dx {
	protected:
		fs::path& p;
		Path(fs::path p) : dx(&p), p(*cast<fs::path>()) { } // simple writable data access
	public:
		typedef lambda<void(Path)> Fn;

		enum Arg {
			Recursion,
			NoSymLinks,
			NoHidden,
			UseGitIgnores
		};

		str               stem() const { return !p.empty() ? str(p.stem().string()) : str(null); }
		static Path        cwd()       { return str(fs::current_path().string()); }
		bool operator==(Path& b) const { return p == b.p; }
		bool operator!=(Path& b) const { return !(operator==(b)); }

		Path(null_t         n = null) : Path(fs::path(""))      { }
		Path(cchar_t*             cs) : Path(fs::path(str(cs))) { }
		Path(str				   s) : Path(fs::path(s))       { }
		Path(fs::directory_entry ent) : Path(ent.path())        { }

		// just use .copy() if you want a copy with these dx classes
		Path(const Path& ref)         : dx(ref), p(*cast<fs::path>()) { }

		Path& operator=(Path path) {
			p = path.p;
			return *this;
		}

		Path& operator=(str s)         { p = fs::path(std::string(s)); return *this; }
		Path& operator=(fs::path path) { p = path;                     return *this; }

		str read_file() {
			if (!fs::is_regular_file(p))
				return null;

			std::ifstream fs;
			fs.open(p);

			std::ostringstream sstr;
			sstr << fs.rdbuf();
			fs.close();
			return str(sstr.str());
		}

		cchar_t*  cstr() const {
			cchar_t* cs = (cchar_t*)p.c_str();
			size_t   sz = cs ? strlen(cs) : 0;
			return sz ? cs : null;
		}

		str        ext()       { return p.extension().string(); }
		Path      file()       { return fs::is_regular_file(p) ? Path(p) : Path(null); }

		bool       copy(Path to) const {
			assert(!p.empty());
			assert(is_file() && exists());

			if (!to.exists())
				(to.has_filename() ?
					to.remove_filename() : to).make_dir();

			std::error_code ec;
			fs::copy(p, (to.is_dir() ? to / Path(str(p.filename().string())) : to).p, ec);
			return ec.value() == 0;
		}

		Path link(Path alias) const {
			if (p.empty() || alias.p.empty())
				return null;

			std::error_code ec;
			if (is_dir()) {
				fs::create_directory_symlink(p, alias.p);
			} else {
				fs::create_symlink(p, alias.p, ec);
			}
			return alias.exists() ? alias : Path(null);
		}

		bool make_dir() const {
			std::error_code ec;
			return !p.empty() ? fs::create_directories(p, ec) : false;
		}

		// i love dx. everything dx. no more vars as a result. done.
		Path remove_filename()       { return Path(p.remove_filename()); }
		bool    has_filename() const { return p.has_filename();  }
		Path            link() const { return fs::is_symlink(p) ? Path(p) : Path(null); }
		operator        bool() const { return p.string().length() > 0; }
		operator         str() const { return str(p.string()); }

		static Path uid(Path b) {
			auto rand = lambda<str(idx)>([](idx i) -> str { return Rand::uniform('a', 'z'); });
			Path    p = null;
			do    { p = Path(str::fill(6, rand)); }
			while ((b / p).exists());
			return  p;
		}

		bool remove_all() const { std::error_code ec; return fs::remove_all(p, ec); }
		bool     remove() const { std::error_code ec; return fs::remove    (p, ec); }
		bool  is_hidden() const {
			std::string st = p.stem().string();
			return st.length() > 0 && st[0] == '.';
		}

		bool  exists() const { return  fs::      exists(p); }
		bool  is_dir() const { return  fs::is_directory(p); }
		bool is_file() const { return !fs::is_directory(p) && fs::is_regular_file(p); }

		Path operator / (Path       s) const { return Path(p / s.p); }
		Path operator / (cchar_t*   s) const { return Path(p / fs::path(s)); }
		Path operator / (const str& s) const { return Path(p / fs::path(s.data())); }
		

		Path relative(Path from)			 { return fs::relative(p, from.p); }

		int64_t modified_at() const {
			using namespace std::chrono_literals;
			std::string ps = p.string();
			auto        wt = fs::last_write_time(p);
			const auto  ms = wt.time_since_epoch().count(); // need a defined offset. 
			return int64_t(ms);
		}

		bool read(size_t bs, lambda<void(cchar_t*, size_t)> fn) {
			try {
				std::error_code ec;
				size_t rsize = fs::file_size(p, ec);
				/// no exceptions
				if (!ec)
					return false;
				std::ifstream f(p);
				char* buf = new char[bs];
				for (idx i = 0, n = (rsize / bs) + (rsize % bs != 0); i < n; i++) {
					size_t sz = i == (n - 1) ? rsize - (rsize / bs * bs) : bs;
					fn((cchar_t*)buf, sz);
				}
				delete[] buf;

			} catch (std::ofstream::failure e) {
				std::cerr << "read failure: " << p.string();
			}
			return true;
		}

		bool write(vec<uint8_t> bytes) {
			try {
				size_t        sz = bytes.size();
				std::ofstream f(p, std::ios::out | std::ios::binary);
				if (sz)
					f.write((cchar_t*)bytes.data(), sz);

			} catch (std::ofstream::failure e) {
				std::cerr << "read failure on resource: " << p.string() << std::endl;
			}
			return true;
		}

		bool append(vec<uint8_t> bytes) {
			try {
				size_t        sz = bytes.size();
				std::ofstream f(p, std::ios::out | std::ios::binary | std::ios::app);
				if (sz)
					f.write((cchar_t*)bytes.data(), sz);
			}
			catch (std::ofstream::failure e) {
				std::cerr << "read failure on resource: " << p.string() << std::endl;
			}
			return true;
		}

		bool same_as(Path b) const { std::error_code ec; return fs::equivalent(p, b.p, ec); }

		void resources(array<str> exts, Flags<Arg> flags, Fn fn) {
			bool use_gitignore	= flags[ UseGitIgnores ];
			bool recursive		= flags[ Recursion     ];
			bool no_hidden		= flags[ NoHidden      ];
			array<str> ignore;

			if (flags[UseGitIgnores])
				ignore = str::read_file(p / ".gitignore").split("\n");

			lambda<void(Path)> res;
			
			// bool: if its default object, it cannot be true.
			// thinking is Map should use var, because var can be set to anything.  it is shared that cannot.
			// when var is set to some 'other' var, it still gets that original.
			map<Path,bool> fetched_dir; // this is temp and map needs a hash table pronto
			fs::path parent		= p; /// parent relative to the git-ignore index; there may be multiple of these things.
			fs::path& fsp		= p;
			
			///
			res = [&](Path path) {
				auto fn_filter = [&](Path p) -> bool {
					str      ext = p.ext();
					bool proceed = false;
					/// proceed if the extension is matching one, or no extensions are given
					for (idx i = 0; !exts || i < exts.size(); i++)
						if (!exts || exts[i] == ext) {
							proceed = !no_hidden || !p.is_hidden();
							break;
						}

					/// proceed if proceeding, and either not using git ignore,
					/// or the file passes the git ignore collection of patterns
					if (proceed && use_gitignore) {
						Path   rel = p.relative(parent); // original parent, not resource parent
						str   srel = rel;
						///
						for (auto& i: ignore)
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

				/// directory of resources
				if (path.is_dir()) {
					if (!no_hidden || !path.is_hidden())
						for (Path p: fs::directory_iterator(path.p)) {
							Path li = p.link();
							if (li)
								continue;
							Path pp = li ? li : p;
							if (recursive && pp.is_dir()) {
								if (fetched_dir.keyed(pp))
									return;
								fetched_dir[pp] = true;
								res(pp);
							}
							///
							if (pp.is_file())
								fn_filter(pp);
						}
				} /// single resource
				else if (path.exists())
					fn_filter(path);
			};
			return res(p);
		}

		array<Path> matching(vec<str> exts) {
			auto ret = array<Path>();
			resources(exts, { }, [&](Path p) { ret += p; });
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
			assert(base.exists());
			///
			Path   rand = Path::uid(base);
			path        = base / rand;
			assert(path.make_dir()); // should return false if it cannot make it
		}

		///
		~Temp() { path.remove_all(); }

		///
		inline void operator += (SymLink sym) {
			if (sym.path.is_dir())
				sym.path.link(sym.alias);
			else if (sym.path.exists())
				assert(false);
		}

		///
		Path operator / (cchar_t*      s) { return path / s; }
		Path operator / (const str&    s) { return path / Path(s); }
	};
//};


//export module var;
//export {

	// 
	struct base64 {
		static map<size_t, size_t> &b64_encoder() {
			static map<size_t, size_t> mm;
			if (!mm) {
				for (idx i = 0; i < 26; i++) mm[i]          = idx('A') + idx(i);
				for (idx i = 0; i < 26; i++) mm[26 + i]     = idx('a') + idx(i);
				for (idx i = 0; i < 10; i++) mm[26 * 2 + i] = idx('0') + idx(i);
				mm[26 * 2 + 10 + 0] = idx('+');
				mm[26 * 2 + 10 + 1] = idx('/');
			}
			return mm;
		}


		/// base64 validation/value map
		static map<idx, idx> &b64_decoder() {
			static map<idx, idx> m;
			if (!m) {
				for (idx i = 0; i < 26; i++) m['A' + i] = i;
				for (idx i = 0; i < 26; i++) m['a' + i] = 26 + i;
				for (idx i = 0; i < 10; i++) m['0' + i] = 26 * 2 + i;

				m['+'] = 26 * 2 + 10 + 0;
				m['/'] = 26 * 2 + 10 + 1;
			}
			return m;
		}

		static sp<uint8_t> encode(cchar_t* data, idx len) {
			auto&         m = b64_encoder();
			idx       p_len = ((len + 2) / 3) * 4;
			sp<uint8_t> res = Memory::valloc(Id<uint8_t>(), null, p_len + 1, 0);
			uint8_t*      e = res.pointer<uint8_t>();
			idx           b = 0;
			for (idx i = 0; i < len; i += 3) {
				*(e++) = uint8_t(m[data[i] >> 2]);
				if (i + 1 <= len) *(e++) = uint8_t(m[((data[i]     << 4) & 0x3f) | data[i + 1] >> 4]);
				if (i + 2 <= len) *(e++) = uint8_t(m[((data[i + 1] << 2) & 0x3f) | data[i + 2] >> 6]);
				if (i + 3 <= len) *(e++) = uint8_t(m[  data[i + 2]       & 0x3f]);
				b += min(idx(4), len - i + 1);
			}
			for (idx i = b; i < p_len; i++)
				*(e++) = '=';
			*e = 0;
			return res;
		}

		static sp<uint8_t> decode(cchar_t* b64, idx b64_len, idx* alloc_sz) {
			assert(b64_len % 4 == 0);
			auto&		  m = base64::b64_decoder();
			*alloc_sz       = b64_len / 4 * 3;
			sp<uint8_t> out = Memory::valloc(Id<uint8_t>(), null, *alloc_sz + 1, 0);
			uint8_t*      o = out.pointer<uint8_t>();
			idx           n = 0;
			idx           e = 0;
			/// --------------------------------------
			for (idx ii = 0; ii < b64_len; ii += 4) {
				idx a[4], w[4];
				for (int i = 0; i < 4; i++) {
					bool is_e = a[i] == '=';
					idx    ch = b64[ii + i];
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

	// scope of var is about contain and rendition of data in such a way that is javascript-var-like.  it need not alter the data it just holds it
	struct var:dx {
		str     s; // attach!

		var(nullptr_t n = null) { }
		var(Type *t) : dx(Memory::valloc(t, null, 1, 0)) { }

		var(const var &ref) : dx(ref)   { }

		template <typename T>			  var(array<T> &ref) : dx((dx&)ref) { }
		template <typename K, typename V> var(map<V,K> &ref) : dx((dx&)ref) { }
		template <typename T>			  var(T v)			 : dx(&v, 1)    { }

		var &vconst_for(size_t id) {
			static array<var> vconsts;
			if (!vconsts)
				vconsts = { size_t(22), lambda<var(size_t)> {
					[](size_t i) -> var {
						return *Type::primitives[i];
					}
				}};
			return vconsts[id];
		}

		size_t size() const { return count(); }

		template <typename T>
		var& operator=(T v) { return (*this = quantum().assign(v)); }

		// must do minimal, at most. #mvplife
		dx &operator[](str name) {
			if (inherits<Map>()) {
				auto re = reorient<Map>({ Trait::TMap });
				return re[name];
			}
			return vconst_for(0);
		}

		dx &operator[](idx index) {
			if (inherits<Array>()) {
				/// trait-enabled re-orientation
				//array<var> &c = d.reorient<array<var>>({ Trait::TArray });
				//return c[index];
			}
			return vconst_for(0);
		}

		int operator==(var&  v) { return  mem->compare(v.mem); }
		int operator!=(var&  v) { return !operator==(v);       }
		operator       float () { return real_value   <  float >(); }
		operator      double () { return real_value   < double >(); }
		operator      int8_t () { return integer_value<  int8_t>(); }
		operator     uint8_t () { return integer_value< uint8_t>(); }
		operator     int16_t () { return integer_value< int16_t>(); }
		operator    uint16_t () { return integer_value<uint16_t>(); }
		operator     int32_t () { return integer_value< int32_t>(); }
		operator    uint32_t () { return integer_value<uint32_t>(); }
		operator     int64_t () { return integer_value< int64_t>(); }
		operator    uint64_t () { return integer_value<uint64_t>(); }
		operator         str () { return inherits<str>() ? str(*string()) : str(*(*type()->string)(*this)); }
	};

	void test() {
		map<dx,var> m = {};
		m.count();
		//m["test"] = null;
		map<dx,var>& cc91 = (map<dx,var>  &) * (map<dx,var>*)0;
		cc91.count();
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

	/// better json/bin interface first.
	struct json {
		// load
		static var load(Path p) {
			return null;
		};

		static bool save() {
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
		void intern(str& t, Array& a, bool err) { /// be nice to support additional params enclosed
			t = t.format(a);
			if (printer != null)
				printer(t);
			else {
				auto& out = err ? std::cout : std::cerr;
				out << std::string(t) << std::endl << std::flush;
			}
		}

	public:

		/// print always prints
		inline void print(str t, Array a = {}) { intern(t, a, false); }

		/// log categorically as error; do not quit
		inline void error(str t, Array a = {}) { intern(t, a, true); }

		/// log when debugging or LogType:Always, or we are adapting our own logger
		inline void log(str t, Array a = {}) {
			if (L == Always || printer || is_debug())
				intern(t, a, false);
		}

		/// assertion test with log out upon error
		inline void assertion(bool a0, str t, Array a = {}) {
			if (!a0) {
				intern(t, a, false); /// todo: ion:dx environment
				assert(a0);
			}
		}

		void _print(str t, Array& a, bool error) {
			t = t.format(a);
			auto& out = error ? std::cout : std::cerr;
			out << str(t) << std::endl << std::flush;
		}

		/// log error, and quit
		inline void fault(str t, Array a = {}) {
			_print(t, a, true);
			assert(false);
			exit(1);
		}

		/// prompt for any type of data
		template <typename T>
		inline T prompt(str t, Array a = {}) {
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

	Logger<LogType::Dissolvable> console;

//export module model;
//export {

	// its older code but it should check out.

	// sqlite adapter worked great but it is to be subducted for a bit.
	typedef Map          SMap;
	typedef Array        Table; // after port, too quaint. moving on.
	
	struct Resolver:Map {
		Resolver(str remote):Map({
			{"schema", remote}, {"resolves", var()}
		}) { }
	};

	/// vars present high level control for data binding usage
	typedef map<dx,var>  ModelMap;

	/// was Schema; not fitting the bill name-wise; its a remote resolver, holder.
	/// Resolver is good but would need more interface-wise (todo)
	struct Remote {
		int64_t value;
		str     remote;

		Remote(int64_t value)      : value(value) { }
		Remote(null_t n = nullptr) : value(-1)    { }
		Remote(str remote, int64_t   value = -1) : value(value), remote(remote) { }

		operator int64_t() { return value; }
		operator     var() {
			return remote ? var(Resolver(remote)) : var(value);
		}
	};

	struct node;
	struct Record {
		std::vector<std::string> fields;
		std::vector<std::string> values;
	};

	typedef std::vector<Record>                     Results;
	typedef std::unordered_map<std::string, var>    Idents;
	typedef std::unordered_map<std::string, Idents> ModelIdents;

	struct ModelCache {
		Results       results;
		Idents        idents; /// lookup fmap needed for optimization
	};

	struct ModelContext {
		ModelMap      models;
		/// its just called hash for the unordered map

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

	struct Alpha:dx {
		enum MType {
			Intern,
			Extern
		};

		str& a;
		str& b;

		/// dont need static here because this is only called once
		/// the members are bound to the type
		static Schema *members() {
			return Schema::define({
				Schema::Row("a", str("abc"), Intern),
				Schema::Row("b", str("123"), Extern)
			});
		};

		/// default values
		Alpha(null_t n = null) : dx(members, f),
			a(def<str>("a")),
			b(def<str>("b")) { }
		
		Alpha(initial<Field> f) : dx(members, f),
			a(ref<str>("a")),
			b(ref<str>("b")) { }
	};

	template <typename T>
	const T nan() { return std::numeric_limits<T>::quiet_NaN(); };

	struct Unit {
		enum Attrib {
			Standard,
			Metric,
			Percent,
			Time,
			Distance
		};
    
		///
		static std::unordered_map<str, int> u_flags;

		str           type   = null;
		real          value  = nan<real>();
		int64_t       millis = 0;
		Alloc::Flags  flags  = null; /// Flags makes flags out of enums, just so you dont define with bit pattern but keep to enum
    
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
					value = s.substr(i).trim().real_value();
				} else {
					if (a > i) {
						if (i >= 0)
							value = s.substr(0, a).trim().real_value();
						type  = s.substr(a).trim().lowercase();
					} else {
						type  = s.substr(0, a).trim().lowercase();
						value = s.substr(a).trim().real_value();
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
	};
//}

//export module vec;
//export {

	// size_t reserved for hash use-case


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




// ------------------------
//export module member;
//export {
struct Member {
	typedef lambda<void(Member&, var&)>		MFn;
	typedef lambda< var(Member&)>			MFnGet;
	typedef lambda<bool(Member&)>			MFnBool;
	typedef lambda<void(Member&)>			MFnVoid;
	typedef lambda<void(Member&, dx&)>	MFnShared;
	typedef lambda<void(Member&, void*  )>	MFnArb;

	struct StTrans;

	enum MType {
		Undefined,
		Intern, // your inner workings
		Extern, // your interface
		Configurable = Intern // having a configurable bit is important in many cases
	};

	lambda<void(Member*, Member*)> copy_fn;

	var& nil() { return def<var>(); }

	var*     serialized = null;
	Type    *type       =  Type::Undefined;
	MType    member     = MType::Undefined;
	str      name       = null;
	dx   shared;    /// having a shared arg might be useful too.
	dx   arg;       /// simply a node * pass-through for the NMember case
	size_t   cache      = 0;
	size_t   peer_cache = 0xffffffff;
	Member*  bound_to   = null;

	virtual ~Member() { }

	virtual void*        ptr() { return null; }
	virtual dx& shared_value() { return def<dx>(); }
	virtual bool  state_bool() {
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

	virtual void operator= (const str& s) {
		if (lambdas && lambdas->var_set)
			lambdas->var_set((dx &)*this, s.rise<var>());
		
		// todo: overridde in str, checks to make sure the
		// type handles string types generally, and
		// that might mean utf8 handling, not just
		// that its a char model; so its checking traits
	}
	void         operator= (Member& v);
	virtual bool operator==(Member& m) { return bound_to == &m && peer_cache == m.peer_cache; }

	virtual void style_value_set(void* ptr, StTrans*) { }
};
//}
	/*
	template <typename F>
	struct Lambda:M<lambda<F>> {
		typedef typename lambda<F>::rtype rtype;
		static  const    size_t arg_count = lambda<F>::args;
		rtype            last_result;
	};*/

//}
// ---------------
//export import ex;
//export {

	struct EnumData {
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

		///
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

	struct Any    { dx data; }; /// not sure how far we go with Any, or if its fully baked yet

	struct Node   { dx data; };
	struct Meta   { dx data; };
	struct Ref    { dx data; }; // no problem with this.  same data is same data.

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
		{ 2,  "u8"        },
		{ 3,  "i16"       },
		{ 4,  "ui16"      },
		{ 5,  "i32"       },
		{ 6,  "u32"       },
		{ 7,  "i64"       },
		{ 8,  "u64"       },
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
	
	/// initialize type primitives
	size_t p                   = 0;
	const size_t p_count	   = sizeof(decls) / sizeof(Type::Decl);
	Type::primitives		   = (const Type **)calloc(sizeof(Type), p_count);

	/// initialize all of the consts, set the pointer to each in primitives.
	*(Ident *)&Type::Undefined  = Type::bootstrap<    void>    (decls[0]);  Type::primitives[p++] = (Type *)Type::Undefined;
	*(Ident *)&Type:: i8        = Type::bootstrap<  int8_t>    (decls[1]);  Type::primitives[p++] = (Type*)Type::i8;
	*(Ident *)&Type:: u8	    = Type::bootstrap< uint8_t>    (decls[2]);  Type::primitives[p++] = (Type*)Type::u8;
	*(Ident *)&Type::i16        = Type::bootstrap< int16_t>    (decls[3]);  Type::primitives[p++] = (Type*)Type::i16;
	*(Ident *)&Type::u16        = Type::bootstrap<uint16_t>    (decls[4]);  Type::primitives[p++] = (Type*)Type::u16;
	*(Ident *)&Type::i32        = Type::bootstrap< int32_t>    (decls[5]);  Type::primitives[p++] = (Type*)Type::i32;
	*(Ident *)&Type::u32        = Type::bootstrap<uint32_t>    (decls[6]);  Type::primitives[p++] = (Type*)Type::u32;
	*(Ident *)&Type::i64        = Type::bootstrap< int64_t>    (decls[7]);  Type::primitives[p++] = (Type*)Type::i64;
	*(Ident *)&Type::u64        = Type::bootstrap<uint64_t>    (decls[8]);  Type::primitives[p++] = (Type*)Type::u64;
	*(Ident *)&Type::f32        = Type::bootstrap<   float>    (decls[9]);  Type::primitives[p++] = (Type*)Type::f32;
	*(Ident *)&Type::f64        = Type::bootstrap<  double>    (decls[10]); Type::primitives[p++] = (Type*)Type::f64;
	*(Ident *)&Type::Bool	    = Type::bootstrap<bool>        (decls[11]); Type::primitives[p++] = (Type*)Type::Bool;
	*(Ident *)&Type::Str	    = Type::bootstrap<str>         (decls[12]); Type::primitives[p++] = (Type*)Type::Str;
	*(Ident *)&Type::Map        = Type::bootstrap<Map>         (decls[13]); Type::primitives[p++] = (Type*)Type::Map;
	*(Ident *)&Type::Array	    = Type::bootstrap<Array>       (decls[14]); Type::primitives[p++] = (Type*)Type::Array;
	*(Ident *)&Type::Ref	    = Type::bootstrap<Ref>		   (decls[15]); Type::primitives[p++] = (Type*)Type::Ref;
	*(Ident *)&Type::Arb        = Type::bootstrap<Arb>         (decls[16]); Type::primitives[p++] = (Type*)Type::Arb;
	*(Ident *)&Type::Node	    = Type::bootstrap<Node>		   (decls[17]); Type::primitives[p++] = (Type*)Type::Node;
	*(Ident*)&Type::Lambda      = Type::bootstrap<lambda<var(var&)>> (decls[18]); Type::primitives[p++] = (Type*)Type::Lambda;
	*(Ident*)&Type::Member	    = Type::bootstrap<Member>	  (decls[19]); Type::primitives[p++] = (Type*)Type::Member;
	*(Ident*)&Type::Any		    = Type::bootstrap<Any>		  (decls[20]); Type::primitives[p++] = (Type*)Type::Any;
	*(Ident*)&Type::Meta	    = Type::bootstrap<Meta>		  (decls[21]); Type::primitives[p++] = (Type*)Type::Meta;

	// now we convert node (lots of code removed; we have made nodal the mod[a]l

	assert(p == p_count);
	 
	vec2f aa = vec2f { r32(1), r32(2) } + vec2f { r32(2), r32(3) };
	r32  aa0 = aa[0];
	r32  aa1 = aa[1];
	vec2f  v = aa * 2.0f;

	assert (aa0 == 3 && aa1 == 5);

	Map m = {};

	m[str{"hi"}] = str("LOL");

	printf("str = %s\n", str(m[str{ "hi" }]).data());

}

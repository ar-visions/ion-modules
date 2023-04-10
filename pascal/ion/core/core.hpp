#pragma once

#include <atomic>
#include <filesystem>
#include <condition_variable>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <memory>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <sstream>
#include <initializer_list>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <new>

template <typename DC>
static void inplace(DC *ptr, DC &data) {
        (ptr) -> ~DC();
    new (ptr)     DC(data);
}

#ifdef _MSC_VER
#ifndef NDEBUG
#define _AMD64_
#include <debugapi.h>
#endif
#endif

typedef std::nullptr_t  null_t;

/// for JS-minded people this is comfortable.
static const null_t null = null_t();

//#define null nullptr

#pragma warning(disable:4005) /// skia
#pragma warning(disable:4566) /// escape sequences in canvas
#pragma warning(disable:5050) ///
#pragma warning(disable:4244) /// skia-header warnings
#pragma warning(disable:5244) /// odd bug
#pragma warning(disable:4291) /// 'no exceptions'
#pragma warning(disable:4996) /// strncpy warning
#pragma warning(disable:4267) /// size_t to int warnings (minimp3)

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

template <typename T> const T nan() { return std::numeric_limits<T>::quiet_NaN(); };

constexpr bool  is_win() {
#if defined(_WIN32) || defined(_WIN64)
	return true;
#else
	return false;
#endif
};

constexpr bool  is_android() {
#if defined(__ANDROID_API__)
	return true;
#else
	return false;
#endif
};

constexpr bool  is_apple() {
#if defined(__APPLE__)
	return true;
#else
	return false;
#endif
};

constexpr int num_occurances(const char* cs, char c) {
    return cs[0] ? (cs[0] == c) + num_occurances(cs + 1, c) : 0; 
}

#define num_args(...) (num_occurances(#__VA_ARGS__, ',') + 1)

/// todo: check why its calling convert on startup that shouldnt be the case

#define enums(C,D,S,...)\
    struct C:ex {\
        inline static bool init;\
        enum etype { __VA_ARGS__ };\
        enum etype&    value;\
        static memory* lookup(symbol sym) { return typeof(C)->lookup(sym); }\
        static memory* lookup(u64    id)  { return typeof(C)->lookup(id);  }\
        inline static const int count = num_args(__VA_ARGS__);\
        static void initialize() {\
            init            = true;\
            array<str>   sp = str(S).split(", ");\
            size_t        c = sp.len();\
            type_t       ty = typeof(C);\
            for (size_t i  = 0; i < c; i++)\
                mem_symbol(sp[i].data, ty, i64(i));\
        }\
        str name() {\
            memory *mem = typeof(C)->lookup(u64(value));\
            assert(mem);\
            return (char*)mem->origin;\
        }\
        static enum etype convert(mx raw) {\
            if (!init) initialize();\
            type_t   type = typeof(C);\
            memory **psym = null;\
            if (raw.type() == typeof(char)) {\
                char  *d = &raw.ref<char>();\
                u64 hash = djb2(d);\
                psym     = type->symbols->djb2.lookup(hash);\
            } else if (raw.type() == typeof(int)) {\
                i64   id = i64(raw.ref<int>());\
                psym     = type->symbols->ids.lookup(id);\
            } else if (raw.type() == typeof(i64)) {\
                i64   id = raw.ref<i64>();\
                psym     = type->symbols->ids.lookup(id);\
            }\
            if (!psym) throw C();\
            return (enum etype)((*psym)->id);\
        }\
        C(enum etype t = etype::D):ex(t, this), value(ref<enum etype>()) { if (!init) initialize(); }\
        C(str raw):C(C::convert(raw)) { }\
        C(mx  raw):C(C::convert(raw)) { }\
        inline  operator        etype() { return value; }\
        static doubly<memory*> &symbols() {\
            if (!init) initialize();\
            return typeof(C)->symbols->list;\
        }\
        C&      operator=  (const C b)  { return (C&)assign_mx(*this, b); }\
        bool    operator== (enum etype v) { return value == v; }\
        bool    operator!= (enum etype v) { return value != v; }\
        bool    operator>  (C &b)       { return value >  b.value; }\
        bool    operator<  (C &b)       { return value <  b.value; }\
        bool    operator>= (C &b)       { return value >= b.value; }\
        bool    operator<= (C &b)       { return value <= b.value; }\
        explicit operator int()         { return int(value); }\
        explicit operator u64()         { return u64(value); }\
    };\

#define infinite_loop() { for (;;) { usleep(1000); } }

#define typeof(T)   ident::for_type<T>()
#define typesize(T) ident::type_size<T>()

#ifdef __cplusplus
#    define decl(x)   decltype(x)
#else
#    define decl(x) __typeof__(x)
#endif

static void breakp() {
    #ifdef _MSC_VER
    #ifndef NDEBUG
    DebugBreak();
    #endif
    #endif
}

#define type_assert(CODE_ASSERT, TYPE_ASSERT) \
    do {\
        const auto   v = CODE_ASSERT;\
        const type_t t = typeof(decl(v));\
        static_assert(decl(v) != typeof(TYPE));\
        if (t != typeof(TYPE)) {\
            faultf("(type-assert) %s:%d: :: %s\n", __FILE__, __LINE__, xstr_0(TYPE_ASSERT));\
        }\
        if (!bool(v)) {\
            faultf("(assert) %s:%d: :: %s\n", __FILE__, __LINE__, xstr_0(CODE_ASSERT));\
        }\
    } while (0);

#define mx_basics(C, B, m_type, m_access) \
    using PC = B;\
    using CL = C;\
    using DC = m_type;\
    template <typename CC>\
    static memory *mx_conv(mx &o) {\
        constexpr bool can_conv = requires(const CC& t) { (memory*)CC::convert((memory*)null); };\
        type_t o_type = o.type();\
        type_t d_type = typeof(DC);\
        if constexpr (can_conv)\
            if (o.mem && o_type != d_type)\
                return (memory *)CC::convert(o.mem);\
        if (o_type == d_type)\
            return o.mem->grab();\
        else if (!o.mem)\
            printf("null memory: %s\n", #C);\
        else\
            printf("type mismatch; conversion required for %s -> %s\n", o_type->name, o_type->name);\
        return (memory *)null;\
    }\
    inline    operator m_type &()  { return m_access; }\
    inline C &operator=(const C b) { return (C &)assign_mx(*this, b); }\
    m_type *operator->() { return &m_access; }

#define ctr(C, B, m_type, m_access) \
    mx_basics(C, B, m_type, m_access)\
    inline C(memory*     mem) : B(mem), m_access(mx::ref<m_type>()) { }\
    inline C(mx            o) : C(  mx_conv<C>(o)) { }\
    inline C(null_t =   null) : C(mx::alloc<C>( )) { }\
    inline C(m_type    tdata) : C(mx::alloc<C>( )) {\
        inplace<DC>(&m_access, tdata);\
    }

#define ctr_args(C, B, T, A) \
    ctr(C, B, T, A) \
    inline C(initial<arg> args) : B(typeof(C), args), A(defaults<T>()) { }

#define ptr(C, B, m_type, m_access) \
    using PC = B;\
    using CL = C;\
    using DC = m_type;\
    inline C(memory*      mem) : B(mem), m_access(&mx::ref<m_type>()) { }\
    inline C(mx o) : C(o.mem->grab()) { }\
    inline C(null_t = null) : C(mx::alloc<C>()) { }\
    inline operator m_type *() { return m_access; }\
    C &operator=   (const C b) { return (C &)assign_mx(*this, b); }

#define ictr(C, B) \
    using PC = B;\
    using CL = C;\
    using DC = none;\
    inline C(memory*      mem) : B(mem) { }\
    inline C(mx             o) : C(o.mem ? o.mem->grab() : null) { }\
    inline C(typename B::DC d) : B(d) { }\
    inline C(null_t n = null)  : C(mx::alloc<C>()) { }\
    C &operator=(const C b) { return (C &)assign_mx(*this, b); }

/// this is needed on a mutex container and phase out
#define ctr_cp(C, B, m_type, m_access) \
    using PC = B;\
    using CL = C;\
    using DC = m_type;\
    inline C(memory*      mem) : B(mem), m_access(mx::ref<m_type>()) { }\
    inline C(mx             o) : C(o.mem ? o.mem->grab() : null) { }\
    inline C(null_t    = null) : C(mx::alloc<C>()) { }\
    inline C(const C       &r) : C(r.mem->grab()) { }\
    inline operator m_type &() { return m_access; }\

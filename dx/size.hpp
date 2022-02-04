#pragma once

//
enum Stride { Major, Minor };

// going for NumPy interface, and int is a must
// our size is never to hold an address. not its domain.  disambiguationwithrepresentation is the key

struct string;

struct Size {
    ssize_t        isz;
    Size(nullptr_t n = null) : isz(0)   { }
    Size(int       isz)      : isz(isz) { }
    Size(size_t    isz)      : isz(isz) { }
    Size(ssize_t   isz)      : isz(isz) { }

    ///
    virtual int      dims() const          { return 1;   }
    virtual size_t   size() const          { return isz; }
    virtual size_t  operator [] (size_t i) { assert(i == 0); return size_t(isz); }

    ///
             operator  bool() { return size() >  0;    }
    bool     operator     !() { return size() <= 0;    }
    operator        ssize_t() { return        size();  }
    operator            int() { return    int(size()); }
    operator         size_t() { return size_t(size()); }
    
    ///
    bool operator <  (Size b) { return operator ssize_t() <  ssize_t(b); }
    bool operator <= (Size b) { return operator ssize_t() <= ssize_t(b); }
    bool operator >  (Size b) { return operator ssize_t() >  ssize_t(b); }
    bool operator >= (Size b) { return operator ssize_t() >= ssize_t(b); }
    bool operator == (Size b) { return operator ssize_t() == ssize_t(b); }
    bool operator != (Size b) { return operator ssize_t() != ssize_t(b); }

    operator std::string();
};

/// useful table of operations for Size conversion, just work, just dont complain about normal things.
static inline bool   operator <  (size_t  lhs, Size   rhs)  { return           lhs  <  size_t(rhs); }
static inline bool   operator <= (size_t  lhs, Size   rhs)  { return           lhs  <= size_t(rhs); }
static inline bool   operator >  (size_t  lhs, Size   rhs)  { return           lhs  >  size_t(rhs); }
static inline bool   operator >= (size_t  lhs, Size   rhs)  { return           lhs  >= size_t(rhs); }
static inline bool   operator == (size_t  lhs, Size   rhs)  { return           lhs  == size_t(rhs); }
static inline bool   operator != (size_t  lhs, Size   rhs)  { return           lhs  != size_t(rhs); }
///
static inline bool   operator <  (ssize_t lhs, Size   rhs)  { return           lhs  <  size_t(rhs); }
static inline bool   operator <= (ssize_t lhs, Size   rhs)  { return           lhs  <= size_t(rhs); }
static inline bool   operator >  (ssize_t lhs, Size   rhs)  { return           lhs  >  size_t(rhs); }
static inline bool   operator >= (ssize_t lhs, Size   rhs)  { return           lhs  >= size_t(rhs); }
static inline bool   operator == (ssize_t lhs, Size   rhs)  { return           lhs  == size_t(rhs); }
static inline bool   operator != (ssize_t lhs, Size   rhs)  { return           lhs  != size_t(rhs); }
///
static inline bool   operator <  (Size    lhs, size_t rhs)  { return    size_t(lhs) <         rhs;  }
static inline bool   operator <= (Size    lhs, size_t rhs)  { return    size_t(lhs) <=        rhs;  }
static inline bool   operator >  (Size    lhs, size_t rhs)  { return    size_t(lhs) >         rhs;  }
static inline bool   operator >= (Size    lhs, size_t rhs)  { return    size_t(lhs) >=        rhs;  }
static inline bool   operator == (Size    lhs, size_t rhs)  { return    size_t(lhs) ==        rhs;  }
static inline bool   operator != (Size    lhs, size_t rhs)  { return    size_t(lhs) !=        rhs;  }
///
static inline bool   operator <  (Size    lhs, ssize_t rhs) { return   ssize_t(lhs) <         rhs; }
static inline bool   operator <= (Size    lhs, ssize_t rhs) { return   ssize_t(lhs) <=        rhs; }
static inline bool   operator >  (Size    lhs, ssize_t rhs) { return   ssize_t(lhs) >         rhs; }
static inline bool   operator >= (Size    lhs, ssize_t rhs) { return   ssize_t(lhs) >=        rhs; }
static inline bool   operator == (Size    lhs, ssize_t rhs) { return   ssize_t(lhs) ==        rhs; }
static inline bool   operator != (Size    lhs, ssize_t rhs) { return   ssize_t(lhs) !=        rhs; }
///
static inline bool   operator <  (int     lhs, Size   rhs)  { return           lhs  <     int(rhs); }
static inline bool   operator <= (int     lhs, Size   rhs)  { return           lhs  <=    int(rhs); }
static inline bool   operator >  (int     lhs, Size   rhs)  { return           lhs  >     int(rhs); }
static inline bool   operator >= (int     lhs, Size   rhs)  { return           lhs  >=    int(rhs); }
static inline bool   operator == (int     lhs, Size   rhs)  { return           lhs  ==    int(rhs); }
static inline bool   operator != (int     lhs, Size   rhs)  { return           lhs  !=    int(rhs); }
///
static inline bool   operator <  (Size    lhs, int    rhs)  { return       int(lhs) <         rhs;  }
static inline bool   operator <= (Size    lhs, int    rhs)  { return       int(lhs) <=        rhs;  }
static inline bool   operator >  (Size    lhs, int    rhs)  { return       int(lhs) >         rhs;  }
static inline bool   operator >= (Size    lhs, int    rhs)  { return       int(lhs) >=        rhs;  }
static inline bool   operator == (Size    lhs, int    rhs)  { return       int(lhs) ==        rhs;  }
static inline bool   operator != (Size    lhs, int    rhs)  { return       int(lhs) !=        rhs;  }
///
static inline size_t  operator - (Size    lhs, size_t  rhs)  { return   size_t(lhs) -         rhs;  }
static inline size_t  operator + (Size    lhs, size_t  rhs)  { return   size_t(lhs) +         rhs;  }
static inline ssize_t operator - (Size    lhs, ssize_t rhs)  { return  ssize_t(lhs) -         rhs;  }
static inline ssize_t operator + (Size    lhs, ssize_t rhs)  { return  ssize_t(lhs) +         rhs;  }
static inline int     operator - (Size    lhs, int     rhs)  { return     int(lhs)  -         rhs;  }
static inline int     operator + (Size    lhs, int     rhs)  { return     int(lhs)  +         rhs;  }
static inline size_t  operator - (size_t  lhs, Size    rhs)  { return         lhs   -  size_t(rhs); }
static inline size_t  operator + (size_t  lhs, Size    rhs)  { return         lhs   +  size_t(rhs); }
static inline ssize_t operator - (ssize_t lhs, Size    rhs)  { return         lhs   - ssize_t(rhs); }
static inline ssize_t operator + (ssize_t lhs, Size    rhs)  { return         lhs   + ssize_t(rhs); }
static inline int     operator - (int     lhs, Size    rhs)  { return         lhs   -     int(rhs); }
static inline int     operator + (int     lhs, Size    rhs)  { return         lhs   +     int(rhs); }
///
static inline size_t  operator / (Size    lhs,  size_t rhs)  { return  size_t(lhs) /         rhs;   }
static inline size_t  operator * (Size    lhs,  size_t rhs)  { return  size_t(lhs) *         rhs;   }
static inline size_t  operator / (Size    lhs, ssize_t rhs)  { return ssize_t(lhs) /         rhs;   }
static inline size_t  operator * (Size    lhs, ssize_t rhs)  { return ssize_t(lhs) *         rhs;   }
static inline int     operator / (Size    lhs,  int    rhs)  { return     int(lhs) /         rhs;   }
static inline int     operator * (Size    lhs,  int    rhs)  { return     int(lhs) *         rhs;   }
static inline size_t  operator / (size_t  lhs,  Size   rhs)  { return         lhs  /  size_t(rhs);  }
static inline size_t  operator * (size_t  lhs,  Size   rhs)  { return         lhs  *  size_t(rhs);  }
static inline ssize_t operator / (ssize_t lhs,  Size   rhs)  { return         lhs  /  ssize_t(rhs); }
static inline ssize_t operator * (ssize_t lhs,  Size   rhs)  { return         lhs  *  ssize_t(rhs); }
static inline int     operator / (int     lhs,  Size   rhs)  { return         lhs  /      int(rhs); }
static inline int     operator * (int     lhs,  Size   rhs)  { return         lhs  *      int(rhs); }
///

# 1 "policy/srripsage.c"
# 1 "/home/sidrail/work/offline-cache//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "policy/srripsage.c"
# 19 "policy/srripsage.c"
# 1 "/usr/include/stdlib.h" 1 3 4
# 25 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 324 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/predefs.h" 1 3 4
# 325 "/usr/include/features.h" 2 3 4
# 357 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 1 3 4
# 378 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 379 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 2 3 4
# 358 "/usr/include/features.h" 2 3 4
# 389 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 1 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 5 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4




# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs-64.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4
# 390 "/usr/include/features.h" 2 3 4
# 26 "/usr/include/stdlib.h" 2 3 4







# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stddef.h" 1 3 4
# 212 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 324 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stddef.h" 3 4
typedef int wchar_t;
# 34 "/usr/include/stdlib.h" 2 3 4


# 96 "/usr/include/stdlib.h" 3 4


typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;







__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;


# 140 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));




extern double atof (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));

extern int atoi (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));

extern long int atol (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));





__extension__ extern long long int atoll (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));





extern double strtod (__const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));





extern float strtof (__const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));

extern long double strtold (__const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));





extern long int strtol (__const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));

extern unsigned long int strtoul (__const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));

# 207 "/usr/include/stdlib.h" 3 4


__extension__
extern long long int strtoll (__const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));

__extension__
extern unsigned long long int strtoull (__const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));

# 277 "/usr/include/stdlib.h" 3 4

extern __inline __attribute__ ((__gnu_inline__)) double
__attribute__ ((__nothrow__ , __leaf__)) atof (__const char *__nptr)
{
  return strtod (__nptr, (char **) ((void *)0));
}
extern __inline __attribute__ ((__gnu_inline__)) int
__attribute__ ((__nothrow__ , __leaf__)) atoi (__const char *__nptr)
{
  return (int) strtol (__nptr, (char **) ((void *)0), 10);
}
extern __inline __attribute__ ((__gnu_inline__)) long int
__attribute__ ((__nothrow__ , __leaf__)) atol (__const char *__nptr)
{
  return strtol (__nptr, (char **) ((void *)0), 10);
}




__extension__ extern __inline __attribute__ ((__gnu_inline__)) long long int
__attribute__ ((__nothrow__ , __leaf__)) atoll (__const char *__nptr)
{
  return strtoll (__nptr, (char **) ((void *)0), 10);
}

# 378 "/usr/include/stdlib.h" 3 4


extern int rand (void) __attribute__ ((__nothrow__ , __leaf__));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));

# 469 "/usr/include/stdlib.h" 3 4


extern void *malloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__warn_unused_result__));

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__warn_unused_result__));










extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));

extern void free (void *__ptr) __attribute__ ((__nothrow__ , __leaf__));

# 512 "/usr/include/stdlib.h" 3 4


extern void abort (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));



extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 531 "/usr/include/stdlib.h" 3 4

# 540 "/usr/include/stdlib.h" 3 4




extern void exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
# 554 "/usr/include/stdlib.h" 3 4






extern void _Exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));






extern char *getenv (__const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));




extern char *__secure_getenv (__const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__warn_unused_result__));
# 712 "/usr/include/stdlib.h" 3 4





extern int system (__const char *__command) __attribute__ ((__warn_unused_result__));

# 742 "/usr/include/stdlib.h" 3 4
typedef int (*__compar_fn_t) (__const void *, __const void *);
# 752 "/usr/include/stdlib.h" 3 4



extern void *bsearch (__const void *__key, __const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) __attribute__ ((__warn_unused_result__));



extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));
# 771 "/usr/include/stdlib.h" 3 4
extern int abs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) __attribute__ ((__warn_unused_result__));
extern long int labs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) __attribute__ ((__warn_unused_result__));



__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) __attribute__ ((__warn_unused_result__));







extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) __attribute__ ((__warn_unused_result__));
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) __attribute__ ((__warn_unused_result__));




__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) __attribute__ ((__warn_unused_result__));

# 857 "/usr/include/stdlib.h" 3 4



extern int mblen (__const char *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));


extern int mbtowc (wchar_t *__restrict __pwc,
     __const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   __const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));

extern size_t wcstombs (char *__restrict __s,
   __const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__));

# 955 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/stdlib.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/stdlib.h" 3 4
extern char *__realpath_chk (__const char *__restrict __name,
        char *__restrict __resolved,
        size_t __resolvedlen) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));
extern char *__realpath_alias (__const char *__restrict __name, char *__restrict __resolved) __asm__ ("" "realpath") __attribute__ ((__nothrow__ , __leaf__))

                                                 __attribute__ ((__warn_unused_result__));
extern char *__realpath_chk_warn (__const char *__restrict __name, char *__restrict __resolved, size_t __resolvedlen) __asm__ ("" "__realpath_chk") __attribute__ ((__nothrow__ , __leaf__))


                                                __attribute__ ((__warn_unused_result__))
     __attribute__((__warning__ ("second argument of realpath must be either NULL or at " "least PATH_MAX bytes long buffer")))
                                      ;

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) __attribute__ ((__warn_unused_result__)) char *
__attribute__ ((__nothrow__ , __leaf__)) realpath (__const char *__restrict __name, char *__restrict __resolved)
{
  if (__builtin_object_size (__resolved, 2 > 1) != (size_t) -1)
    {




      return __realpath_chk (__name, __resolved, __builtin_object_size (__resolved, 2 > 1));
    }

  return __realpath_alias (__name, __resolved);
}


extern int __ptsname_r_chk (int __fd, char *__buf, size_t __buflen,
       size_t __nreal) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
extern int __ptsname_r_alias (int __fd, char *__buf, size_t __buflen) __asm__ ("" "ptsname_r") __attribute__ ((__nothrow__ , __leaf__))

     __attribute__ ((__nonnull__ (2)));
extern int __ptsname_r_chk_warn (int __fd, char *__buf, size_t __buflen, size_t __nreal) __asm__ ("" "__ptsname_r_chk") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (2))) __attribute__((__warning__ ("ptsname_r called with buflen bigger than " "size of buf")))
                   ;

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
__attribute__ ((__nothrow__ , __leaf__)) ptsname_r (int __fd, char *__buf, size_t __buflen)
{
  if (__builtin_object_size (__buf, 2 > 1) != (size_t) -1)
    {
      if (!__builtin_constant_p (__buflen))
 return __ptsname_r_chk (__fd, __buf, __buflen, __builtin_object_size (__buf, 2 > 1));
      if (__buflen > __builtin_object_size (__buf, 2 > 1))
 return __ptsname_r_chk_warn (__fd, __buf, __buflen, __builtin_object_size (__buf, 2 > 1));
    }
  return __ptsname_r_alias (__fd, __buf, __buflen);
}


extern int __wctomb_chk (char *__s, wchar_t __wchar, size_t __buflen)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));
extern int __wctomb_alias (char *__s, wchar_t __wchar) __asm__ ("" "wctomb") __attribute__ ((__nothrow__ , __leaf__))
              __attribute__ ((__warn_unused_result__));

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) __attribute__ ((__warn_unused_result__)) int
__attribute__ ((__nothrow__ , __leaf__)) wctomb (char *__s, wchar_t __wchar)
{







  if (__builtin_object_size (__s, 2 > 1) != (size_t) -1 && 16 > __builtin_object_size (__s, 2 > 1))
    return __wctomb_chk (__s, __wchar, __builtin_object_size (__s, 2 > 1));
  return __wctomb_alias (__s, __wchar);
}


extern size_t __mbstowcs_chk (wchar_t *__restrict __dst,
         __const char *__restrict __src,
         size_t __len, size_t __dstlen) __attribute__ ((__nothrow__ , __leaf__));
extern size_t __mbstowcs_alias (wchar_t *__restrict __dst, __const char *__restrict __src, size_t __len) __asm__ ("" "mbstowcs") __attribute__ ((__nothrow__ , __leaf__))


                                  ;
extern size_t __mbstowcs_chk_warn (wchar_t *__restrict __dst, __const char *__restrict __src, size_t __len, size_t __dstlen) __asm__ ("" "__mbstowcs_chk") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__((__warning__ ("mbstowcs called with dst buffer smaller than len " "* sizeof (wchar_t)")))
                        ;

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) size_t
__attribute__ ((__nothrow__ , __leaf__)) mbstowcs (wchar_t *__restrict __dst, __const char *__restrict __src, size_t __len)

{
  if (__builtin_object_size (__dst, 2 > 1) != (size_t) -1)
    {
      if (!__builtin_constant_p (__len))
 return __mbstowcs_chk (__dst, __src, __len,
          __builtin_object_size (__dst, 2 > 1) / sizeof (wchar_t));

      if (__len > __builtin_object_size (__dst, 2 > 1) / sizeof (wchar_t))
 return __mbstowcs_chk_warn (__dst, __src, __len,
         __builtin_object_size (__dst, 2 > 1) / sizeof (wchar_t));
    }
  return __mbstowcs_alias (__dst, __src, __len);
}


extern size_t __wcstombs_chk (char *__restrict __dst,
         __const wchar_t *__restrict __src,
         size_t __len, size_t __dstlen) __attribute__ ((__nothrow__ , __leaf__));
extern size_t __wcstombs_alias (char *__restrict __dst, __const wchar_t *__restrict __src, size_t __len) __asm__ ("" "wcstombs") __attribute__ ((__nothrow__ , __leaf__))


                                  ;
extern size_t __wcstombs_chk_warn (char *__restrict __dst, __const wchar_t *__restrict __src, size_t __len, size_t __dstlen) __asm__ ("" "__wcstombs_chk") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__((__warning__ ("wcstombs called with dst buffer smaller than len")));

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) size_t
__attribute__ ((__nothrow__ , __leaf__)) wcstombs (char *__restrict __dst, __const wchar_t *__restrict __src, size_t __len)

{
  if (__builtin_object_size (__dst, 2 > 1) != (size_t) -1)
    {
      if (!__builtin_constant_p (__len))
 return __wcstombs_chk (__dst, __src, __len, __builtin_object_size (__dst, 2 > 1));
      if (__len > __builtin_object_size (__dst, 2 > 1))
 return __wcstombs_chk_warn (__dst, __src, __len, __builtin_object_size (__dst, 2 > 1));
    }
  return __wcstombs_alias (__dst, __src, __len);
}
# 956 "/usr/include/stdlib.h" 2 3 4
# 964 "/usr/include/stdlib.h" 3 4

# 20 "policy/srripsage.c" 2
# 1 "/usr/include/assert.h" 1 3 4
# 68 "/usr/include/assert.h" 3 4



extern void __assert_fail (__const char *__assertion, __const char *__file,
      unsigned int __line, __const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));


extern void __assert_perror_fail (int __errnum, __const char *__file,
      unsigned int __line,
      __const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));




extern void __assert (const char *__assertion, const char *__file, int __line)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));



# 21 "policy/srripsage.c" 2

# 1 "./libstruct/misc.h" 1
# 23 "./libstruct/misc.h"
# 1 "/usr/include/stdio.h" 1 3 4
# 30 "/usr/include/stdio.h" 3 4




# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stddef.h" 1 3 4
# 35 "/usr/include/stdio.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/types.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 29 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;







typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
# 131 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/typesizes.h" 1 3 4
# 132 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;

typedef int __daddr_t;
typedef long int __swblk_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;

typedef long int __ssize_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;
# 37 "/usr/include/stdio.h" 2 3 4
# 45 "/usr/include/stdio.h" 3 4
struct _IO_FILE;



typedef struct _IO_FILE FILE;





# 65 "/usr/include/stdio.h" 3 4
typedef struct _IO_FILE __FILE;
# 75 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/libio.h" 1 3 4
# 32 "/usr/include/libio.h" 3 4
# 1 "/usr/include/_G_config.h" 1 3 4
# 15 "/usr/include/_G_config.h" 3 4
# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stddef.h" 1 3 4
# 16 "/usr/include/_G_config.h" 2 3 4




# 1 "/usr/include/wchar.h" 1 3 4
# 83 "/usr/include/wchar.h" 3 4
typedef struct
{
  int __count;
  union
  {

    unsigned int __wch;



    char __wchb[4];
  } __value;
} __mbstate_t;
# 21 "/usr/include/_G_config.h" 2 3 4

typedef struct
{
  __off_t __pos;
  __mbstate_t __state;
} _G_fpos_t;
typedef struct
{
  __off64_t __pos;
  __mbstate_t __state;
} _G_fpos64_t;
# 53 "/usr/include/_G_config.h" 3 4
typedef int _G_int16_t __attribute__ ((__mode__ (__HI__)));
typedef int _G_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int _G_uint16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int _G_uint32_t __attribute__ ((__mode__ (__SI__)));
# 33 "/usr/include/libio.h" 2 3 4
# 53 "/usr/include/libio.h" 3 4
# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stdarg.h" 1 3 4
# 40 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 54 "/usr/include/libio.h" 2 3 4
# 172 "/usr/include/libio.h" 3 4
struct _IO_jump_t; struct _IO_FILE;
# 182 "/usr/include/libio.h" 3 4
typedef void _IO_lock_t;





struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;



  int _pos;
# 205 "/usr/include/libio.h" 3 4
};


enum __codecvt_result
{
  __codecvt_ok,
  __codecvt_partial,
  __codecvt_error,
  __codecvt_noconv
};
# 273 "/usr/include/libio.h" 3 4
struct _IO_FILE {
  int _flags;




  char* _IO_read_ptr;
  char* _IO_read_end;
  char* _IO_read_base;
  char* _IO_write_base;
  char* _IO_write_ptr;
  char* _IO_write_end;
  char* _IO_buf_base;
  char* _IO_buf_end;

  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;



  int _flags2;

  __off_t _old_offset;



  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];



  _IO_lock_t *_lock;
# 321 "/usr/include/libio.h" 3 4
  __off64_t _offset;
# 330 "/usr/include/libio.h" 3 4
  void *__pad1;
  void *__pad2;
  void *__pad3;
  void *__pad4;
  size_t __pad5;

  int _mode;

  char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];

};


typedef struct _IO_FILE _IO_FILE;


struct _IO_FILE_plus;

extern struct _IO_FILE_plus _IO_2_1_stdin_;
extern struct _IO_FILE_plus _IO_2_1_stdout_;
extern struct _IO_FILE_plus _IO_2_1_stderr_;
# 366 "/usr/include/libio.h" 3 4
typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);







typedef __ssize_t __io_write_fn (void *__cookie, __const char *__buf,
     size_t __n);







typedef int __io_seek_fn (void *__cookie, __off64_t *__pos, int __w);


typedef int __io_close_fn (void *__cookie);
# 418 "/usr/include/libio.h" 3 4
extern int __underflow (_IO_FILE *);
extern int __uflow (_IO_FILE *);
extern int __overflow (_IO_FILE *, int);
# 462 "/usr/include/libio.h" 3 4
extern int _IO_getc (_IO_FILE *__fp);
extern int _IO_putc (int __c, _IO_FILE *__fp);
extern int _IO_feof (_IO_FILE *__fp) __attribute__ ((__nothrow__ , __leaf__));
extern int _IO_ferror (_IO_FILE *__fp) __attribute__ ((__nothrow__ , __leaf__));

extern int _IO_peekc_locked (_IO_FILE *__fp);





extern void _IO_flockfile (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));
extern void _IO_funlockfile (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));
extern int _IO_ftrylockfile (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));
# 492 "/usr/include/libio.h" 3 4
extern int _IO_vfscanf (_IO_FILE * __restrict, const char * __restrict,
   __gnuc_va_list, int *__restrict);
extern int _IO_vfprintf (_IO_FILE *__restrict, const char *__restrict,
    __gnuc_va_list);
extern __ssize_t _IO_padn (_IO_FILE *, int, __ssize_t);
extern size_t _IO_sgetn (_IO_FILE *, void *, size_t);

extern __off64_t _IO_seekoff (_IO_FILE *, __off64_t, int, int);
extern __off64_t _IO_seekpos (_IO_FILE *, __off64_t, int);

extern void _IO_free_backup_area (_IO_FILE *) __attribute__ ((__nothrow__ , __leaf__));
# 76 "/usr/include/stdio.h" 2 3 4
# 109 "/usr/include/stdio.h" 3 4


typedef _G_fpos_t fpos_t;




# 165 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/stdio_lim.h" 1 3 4
# 166 "/usr/include/stdio.h" 2 3 4



extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;







extern int remove (__const char *__filename) __attribute__ ((__nothrow__ , __leaf__));

extern int rename (__const char *__old, __const char *__new) __attribute__ ((__nothrow__ , __leaf__));














extern FILE *tmpfile (void) __attribute__ ((__warn_unused_result__));
# 210 "/usr/include/stdio.h" 3 4
extern char *tmpnam (char *__s) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));

# 233 "/usr/include/stdio.h" 3 4





extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);

# 267 "/usr/include/stdio.h" 3 4






extern FILE *fopen (__const char *__restrict __filename,
      __const char *__restrict __modes) __attribute__ ((__warn_unused_result__));




extern FILE *freopen (__const char *__restrict __filename,
        __const char *__restrict __modes,
        FILE *__restrict __stream) __attribute__ ((__warn_unused_result__));
# 296 "/usr/include/stdio.h" 3 4

# 330 "/usr/include/stdio.h" 3 4



extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__ , __leaf__));

# 352 "/usr/include/stdio.h" 3 4





extern int fprintf (FILE *__restrict __stream,
      __const char *__restrict __format, ...);




extern int printf (__const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      __const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg);




extern int vprintf (__const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));





extern int snprintf (char *__restrict __s, size_t __maxlen,
       __const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));

# 426 "/usr/include/stdio.h" 3 4





extern int fscanf (FILE *__restrict __stream,
     __const char *__restrict __format, ...) __attribute__ ((__warn_unused_result__));




extern int scanf (__const char *__restrict __format, ...) __attribute__ ((__warn_unused_result__));

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));
# 449 "/usr/include/stdio.h" 3 4
extern int fscanf (FILE *__restrict __stream, __const char *__restrict __format, ...) __asm__ ("" "__isoc99_fscanf")

                          __attribute__ ((__warn_unused_result__));
extern int scanf (__const char *__restrict __format, ...) __asm__ ("" "__isoc99_scanf")
                         __attribute__ ((__warn_unused_result__));
extern int sscanf (__const char *__restrict __s, __const char *__restrict __format, ...) __asm__ ("" "__isoc99_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
# 469 "/usr/include/stdio.h" 3 4








extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__warn_unused_result__));





extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) __attribute__ ((__warn_unused_result__));


extern int vsscanf (__const char *__restrict __s,
      __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format__ (__scanf__, 2, 0)));
# 500 "/usr/include/stdio.h" 3 4
extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc99_vfscanf")



     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__warn_unused_result__));
extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc99_vscanf")

     __attribute__ ((__format__ (__scanf__, 1, 0))) __attribute__ ((__warn_unused_result__));
extern int vsscanf (__const char *__restrict __s, __const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc99_vsscanf") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__format__ (__scanf__, 2, 0)));
# 528 "/usr/include/stdio.h" 3 4









extern int fgetc (FILE *__stream);
extern int getc (FILE *__stream);





extern int getchar (void);

# 571 "/usr/include/stdio.h" 3 4








extern int fputc (int __c, FILE *__stream);
extern int putc (int __c, FILE *__stream);





extern int putchar (int __c);

# 623 "/usr/include/stdio.h" 3 4





extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     __attribute__ ((__warn_unused_result__));






extern char *gets (char *__s) __attribute__ ((__warn_unused_result__));

# 681 "/usr/include/stdio.h" 3 4





extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream);






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream) __attribute__ ((__warn_unused_result__));




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s);

# 741 "/usr/include/stdio.h" 3 4





extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream) __attribute__ ((__warn_unused_result__));




extern void rewind (FILE *__stream);

# 789 "/usr/include/stdio.h" 3 4






extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos);




extern int fsetpos (FILE *__stream, __const fpos_t *__pos);
# 812 "/usr/include/stdio.h" 3 4

# 821 "/usr/include/stdio.h" 3 4


extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));

# 838 "/usr/include/stdio.h" 3 4





extern void perror (__const char *__s);






# 1 "/usr/include/x86_64-linux-gnu/bits/sys_errlist.h" 1 3 4
# 851 "/usr/include/stdio.h" 2 3 4
# 931 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/stdio.h" 1 3 4
# 44 "/usr/include/x86_64-linux-gnu/bits/stdio.h" 3 4
extern __inline __attribute__ ((__gnu_inline__)) int
getchar (void)
{
  return _IO_getc (stdin);
}
# 79 "/usr/include/x86_64-linux-gnu/bits/stdio.h" 3 4
extern __inline __attribute__ ((__gnu_inline__)) int
putchar (int __c)
{
  return _IO_putc (__c, stdout);
}
# 932 "/usr/include/stdio.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/stdio2.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/stdio2.h" 3 4
extern int __sprintf_chk (char *__restrict __s, int __flag, size_t __slen,
     __const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));
extern int __vsprintf_chk (char *__restrict __s, int __flag, size_t __slen,
      __const char *__restrict __format,
      __gnuc_va_list __ap) __attribute__ ((__nothrow__ , __leaf__));


extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
__attribute__ ((__nothrow__ , __leaf__)) sprintf (char *__restrict __s, __const char *__restrict __fmt, ...)
{
  return __builtin___sprintf_chk (__s, 2 - 1,
      __builtin_object_size (__s, 2 > 1), __fmt, __builtin_va_arg_pack ());
}






extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
__attribute__ ((__nothrow__ , __leaf__)) vsprintf (char *__restrict __s, __const char *__restrict __fmt, __gnuc_va_list __ap)

{
  return __builtin___vsprintf_chk (__s, 2 - 1,
       __builtin_object_size (__s, 2 > 1), __fmt, __ap);
}



extern int __snprintf_chk (char *__restrict __s, size_t __n, int __flag,
      size_t __slen, __const char *__restrict __format,
      ...) __attribute__ ((__nothrow__ , __leaf__));
extern int __vsnprintf_chk (char *__restrict __s, size_t __n, int __flag,
       size_t __slen, __const char *__restrict __format,
       __gnuc_va_list __ap) __attribute__ ((__nothrow__ , __leaf__));


extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
__attribute__ ((__nothrow__ , __leaf__)) snprintf (char *__restrict __s, size_t __n, __const char *__restrict __fmt, ...)

{
  return __builtin___snprintf_chk (__s, __n, 2 - 1,
       __builtin_object_size (__s, 2 > 1), __fmt, __builtin_va_arg_pack ());
}






extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
__attribute__ ((__nothrow__ , __leaf__)) vsnprintf (char *__restrict __s, size_t __n, __const char *__restrict __fmt, __gnuc_va_list __ap)

{
  return __builtin___vsnprintf_chk (__s, __n, 2 - 1,
        __builtin_object_size (__s, 2 > 1), __fmt, __ap);
}





extern int __fprintf_chk (FILE *__restrict __stream, int __flag,
     __const char *__restrict __format, ...);
extern int __printf_chk (int __flag, __const char *__restrict __format, ...);
extern int __vfprintf_chk (FILE *__restrict __stream, int __flag,
      __const char *__restrict __format, __gnuc_va_list __ap);
extern int __vprintf_chk (int __flag, __const char *__restrict __format,
     __gnuc_va_list __ap);


extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
fprintf (FILE *__restrict __stream, __const char *__restrict __fmt, ...)
{
  return __fprintf_chk (__stream, 2 - 1, __fmt,
   __builtin_va_arg_pack ());
}

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
printf (__const char *__restrict __fmt, ...)
{
  return __printf_chk (2 - 1, __fmt, __builtin_va_arg_pack ());
}







extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
vprintf (__const char *__restrict __fmt, __gnuc_va_list __ap)
{

  return __vfprintf_chk (stdout, 2 - 1, __fmt, __ap);



}

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) int
vfprintf (FILE *__restrict __stream,
   __const char *__restrict __fmt, __gnuc_va_list __ap)
{
  return __vfprintf_chk (__stream, 2 - 1, __fmt, __ap);
}
# 220 "/usr/include/x86_64-linux-gnu/bits/stdio2.h" 3 4
extern char *__gets_chk (char *__str, size_t) __attribute__ ((__warn_unused_result__));
extern char *__gets_warn (char *__str) __asm__ ("" "gets")
     __attribute__ ((__warn_unused_result__)) __attribute__((__warning__ ("please use fgets or getline instead, gets can't " "specify buffer size")))
                               ;

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) __attribute__ ((__warn_unused_result__)) char *
gets (char *__str)
{
  if (__builtin_object_size (__str, 2 > 1) != (size_t) -1)
    return __gets_chk (__str, __builtin_object_size (__str, 2 > 1));
  return __gets_warn (__str);
}

extern char *__fgets_chk (char *__restrict __s, size_t __size, int __n,
     FILE *__restrict __stream) __attribute__ ((__warn_unused_result__));
extern char *__fgets_alias (char *__restrict __s, int __n, FILE *__restrict __stream) __asm__ ("" "fgets")

                                        __attribute__ ((__warn_unused_result__));
extern char *__fgets_chk_warn (char *__restrict __s, size_t __size, int __n, FILE *__restrict __stream) __asm__ ("" "__fgets_chk")


     __attribute__ ((__warn_unused_result__)) __attribute__((__warning__ ("fgets called with bigger size than length " "of destination buffer")))
                                 ;

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) __attribute__ ((__warn_unused_result__)) char *
fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
{
  if (__builtin_object_size (__s, 2 > 1) != (size_t) -1)
    {
      if (!__builtin_constant_p (__n) || __n <= 0)
 return __fgets_chk (__s, __builtin_object_size (__s, 2 > 1), __n, __stream);

      if ((size_t) __n > __builtin_object_size (__s, 2 > 1))
 return __fgets_chk_warn (__s, __builtin_object_size (__s, 2 > 1), __n, __stream);
    }
  return __fgets_alias (__s, __n, __stream);
}

extern size_t __fread_chk (void *__restrict __ptr, size_t __ptrlen,
      size_t __size, size_t __n,
      FILE *__restrict __stream) __attribute__ ((__warn_unused_result__));
extern size_t __fread_alias (void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __stream) __asm__ ("" "fread")


            __attribute__ ((__warn_unused_result__));
extern size_t __fread_chk_warn (void *__restrict __ptr, size_t __ptrlen, size_t __size, size_t __n, FILE *__restrict __stream) __asm__ ("" "__fread_chk")




     __attribute__ ((__warn_unused_result__)) __attribute__((__warning__ ("fread called with bigger size * nmemb than length " "of destination buffer")))
                                 ;

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) __attribute__ ((__warn_unused_result__)) size_t
fread (void *__restrict __ptr, size_t __size, size_t __n,
       FILE *__restrict __stream)
{
  if (__builtin_object_size (__ptr, 0) != (size_t) -1)
    {
      if (!__builtin_constant_p (__size)
   || !__builtin_constant_p (__n)
   || (__size | __n) >= (((size_t) 1) << (8 * sizeof (size_t) / 2)))
 return __fread_chk (__ptr, __builtin_object_size (__ptr, 0), __size, __n, __stream);

      if (__size * __n > __builtin_object_size (__ptr, 0))
 return __fread_chk_warn (__ptr, __builtin_object_size (__ptr, 0), __size, __n, __stream);
    }
  return __fread_alias (__ptr, __size, __n, __stream);
}
# 935 "/usr/include/stdio.h" 2 3 4






# 24 "./libstruct/misc.h" 2
# 237 "./libstruct/misc.h"
int write_buffer(char *file_name, void *buf, int size);
void *read_buffer(char *file_name, int *psize);
void free_buffer(void *buf);
# 251 "./libstruct/misc.h"
void m2s_host_guest_match_error(char *expr, int host_value, int guest_value);






int hex_str_to_byte_array(char *dest, char *str, int num);
void dump_bin(int x, int digits, FILE *f);
void dump_ptr(void *ptr, int size, FILE *stream);
int log_base2(long long x);
int gcd(int a, int b);
# 23 "policy/srripsage.c" 2
# 1 "./libstruct/string.h" 1
# 29 "./libstruct/string.h"
struct str_map_t
{
        int count;


        struct
        {
                char *string;
                int value;
        } map[];
};

int str_map_string(struct str_map_t *map, char *s);
int str_map_string_err(struct str_map_t *map, char *s, int *err_ptr);
int str_map_string_err_msg(struct str_map_t *map, char *s, char *err_msg);

int str_map_string_case(struct str_map_t *map, char *s);
int str_map_string_case_err(struct str_map_t *map, char *s, int *err_ptr);
int str_map_string_case_err_msg(struct str_map_t *map, char *s, char *err_msg);

char *str_map_value(struct str_map_t *map, int value);
void str_map_value_buf(struct str_map_t *map, int value, char *buf, int size);

void str_map_flags(struct str_map_t *map, int flags, char *out, int length);
# 61 "./libstruct/string.h"
struct list_t *str_token_list_create(char *str, char *delim);
void str_token_list_free(struct list_t *token_list);

char *str_token_list_shift(struct list_t *token_list);
char *str_token_list_first(struct list_t *token_list);



int str_token_list_find(struct list_t *token_list, char *token);



int str_token_list_find_case(struct list_t *token_list, char *token);

void str_token_list_dump(struct list_t *token_list, FILE *f);
# 88 "./libstruct/string.h"
void str_single_spaces(char *dest, int size, char *src);

int str_suffix(char *str, char *suffix);
int str_prefix(char *str, char *prefix);

void str_substr(char *dest, int dest_size, char *src, int src_pos, int src_count);

void str_token(char *dest, int dest_size, char *src, int index, char *delim);




void str_trim(char *dest, int size, char *src);



char *str_error(int err);
# 121 "./libstruct/string.h"
int str_to_int(char *str, int *err);
long long str_to_llint(char *str, int *err);
# 133 "./libstruct/string.h"
void str_int_to_alnum(char *str, int size, unsigned int value);
unsigned int str_alnum_to_int(char *str);






void str_printf(char **pbuf, int *psize, char *fmt, ...) __attribute__((format(printf, 3, 4)));

void str_read_from_file(FILE *f, char *buf, int buf_size);
void str_write_to_file(FILE *f, char *buf);





char *str_set(char *old_str, char *new_str);
char *str_free(char *str);
# 24 "policy/srripsage.c" 2
# 1 "./libmhandle/mhandle.h" 1
# 24 "./libmhandle/mhandle.h"
# 1 "/usr/include/string.h" 1 3 4
# 29 "/usr/include/string.h" 3 4





# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/include/stddef.h" 1 3 4
# 35 "/usr/include/string.h" 2 3 4









extern void *memcpy (void *__restrict __dest,
       __const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void *memmove (void *__dest, __const void *__src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

# 63 "/usr/include/string.h" 3 4


extern void *memset (void *__s, int __c, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int memcmp (__const void *__s1, __const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 95 "/usr/include/string.h" 3 4
extern void *memchr (__const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));


# 126 "/usr/include/string.h" 3 4


extern char *strcpy (char *__restrict __dest, __const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncpy (char *__restrict __dest,
        __const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *strcat (char *__restrict __dest, __const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncat (char *__restrict __dest, __const char *__restrict __src,
        size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcmp (__const char *__s1, __const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern int strncmp (__const char *__s1, __const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcoll (__const char *__s1, __const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern size_t strxfrm (char *__restrict __dest,
         __const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

# 210 "/usr/include/string.h" 3 4

# 235 "/usr/include/string.h" 3 4
extern char *strchr (__const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 262 "/usr/include/string.h" 3 4
extern char *strrchr (__const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));


# 281 "/usr/include/string.h" 3 4



extern size_t strcspn (__const char *__s, __const char *__reject)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern size_t strspn (__const char *__s, __const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 314 "/usr/include/string.h" 3 4
extern char *strpbrk (__const char *__s, __const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 342 "/usr/include/string.h" 3 4
extern char *strstr (__const char *__haystack, __const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strtok (char *__restrict __s, __const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));




extern char *__strtok_r (char *__restrict __s,
    __const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
# 397 "/usr/include/string.h" 3 4


extern size_t strlen (__const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));

# 411 "/usr/include/string.h" 3 4


extern char *strerror (int __errnum) __attribute__ ((__nothrow__ , __leaf__));

# 451 "/usr/include/string.h" 3 4
extern void __bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 634 "/usr/include/string.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/string.h" 1 3 4
# 635 "/usr/include/string.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/string2.h" 1 3 4
# 52 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 37 "/usr/include/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/endian.h" 1 3 4
# 38 "/usr/include/endian.h" 2 3 4
# 53 "/usr/include/x86_64-linux-gnu/bits/string2.h" 2 3 4
# 394 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
extern void *__rawmemchr (const void *__s, int __c);
# 969 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
extern __inline __attribute__ ((__gnu_inline__)) size_t __strcspn_c1 (__const char *__s, int __reject);
extern __inline __attribute__ ((__gnu_inline__)) size_t
__strcspn_c1 (__const char *__s, int __reject)
{
  register size_t __result = 0;
  while (__s[__result] != '\0' && __s[__result] != __reject)
    ++__result;
  return __result;
}

extern __inline __attribute__ ((__gnu_inline__)) size_t __strcspn_c2 (__const char *__s, int __reject1,
         int __reject2);
extern __inline __attribute__ ((__gnu_inline__)) size_t
__strcspn_c2 (__const char *__s, int __reject1, int __reject2)
{
  register size_t __result = 0;
  while (__s[__result] != '\0' && __s[__result] != __reject1
  && __s[__result] != __reject2)
    ++__result;
  return __result;
}

extern __inline __attribute__ ((__gnu_inline__)) size_t __strcspn_c3 (__const char *__s, int __reject1,
         int __reject2, int __reject3);
extern __inline __attribute__ ((__gnu_inline__)) size_t
__strcspn_c3 (__const char *__s, int __reject1, int __reject2,
       int __reject3)
{
  register size_t __result = 0;
  while (__s[__result] != '\0' && __s[__result] != __reject1
  && __s[__result] != __reject2 && __s[__result] != __reject3)
    ++__result;
  return __result;
}
# 1045 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
extern __inline __attribute__ ((__gnu_inline__)) size_t __strspn_c1 (__const char *__s, int __accept);
extern __inline __attribute__ ((__gnu_inline__)) size_t
__strspn_c1 (__const char *__s, int __accept)
{
  register size_t __result = 0;

  while (__s[__result] == __accept)
    ++__result;
  return __result;
}

extern __inline __attribute__ ((__gnu_inline__)) size_t __strspn_c2 (__const char *__s, int __accept1,
        int __accept2);
extern __inline __attribute__ ((__gnu_inline__)) size_t
__strspn_c2 (__const char *__s, int __accept1, int __accept2)
{
  register size_t __result = 0;

  while (__s[__result] == __accept1 || __s[__result] == __accept2)
    ++__result;
  return __result;
}

extern __inline __attribute__ ((__gnu_inline__)) size_t __strspn_c3 (__const char *__s, int __accept1,
        int __accept2, int __accept3);
extern __inline __attribute__ ((__gnu_inline__)) size_t
__strspn_c3 (__const char *__s, int __accept1, int __accept2, int __accept3)
{
  register size_t __result = 0;

  while (__s[__result] == __accept1 || __s[__result] == __accept2
  || __s[__result] == __accept3)
    ++__result;
  return __result;
}
# 1121 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
extern __inline __attribute__ ((__gnu_inline__)) char *__strpbrk_c2 (__const char *__s, int __accept1,
         int __accept2);
extern __inline __attribute__ ((__gnu_inline__)) char *
__strpbrk_c2 (__const char *__s, int __accept1, int __accept2)
{

  while (*__s != '\0' && *__s != __accept1 && *__s != __accept2)
    ++__s;
  return *__s == '\0' ? ((void *)0) : (char *) (size_t) __s;
}

extern __inline __attribute__ ((__gnu_inline__)) char *__strpbrk_c3 (__const char *__s, int __accept1,
         int __accept2, int __accept3);
extern __inline __attribute__ ((__gnu_inline__)) char *
__strpbrk_c3 (__const char *__s, int __accept1, int __accept2,
       int __accept3)
{

  while (*__s != '\0' && *__s != __accept1 && *__s != __accept2
  && *__s != __accept3)
    ++__s;
  return *__s == '\0' ? ((void *)0) : (char *) (size_t) __s;
}
# 1172 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
extern __inline __attribute__ ((__gnu_inline__)) char *__strtok_r_1c (char *__s, char __sep, char **__nextp);
extern __inline __attribute__ ((__gnu_inline__)) char *
__strtok_r_1c (char *__s, char __sep, char **__nextp)
{
  char *__result;
  if (__s == ((void *)0))
    __s = *__nextp;
  while (*__s == __sep)
    ++__s;
  __result = ((void *)0);
  if (*__s != '\0')
    {
      __result = __s++;
      while (*__s != '\0')
 if (*__s++ == __sep)
   {
     __s[-1] = '\0';
     break;
   }
    }
  *__nextp = __s;
  return __result;
}
# 1204 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
extern char *__strsep_g (char **__stringp, __const char *__delim);
# 1222 "/usr/include/x86_64-linux-gnu/bits/string2.h" 3 4
extern __inline __attribute__ ((__gnu_inline__)) char *__strsep_1c (char **__s, char __reject);
extern __inline __attribute__ ((__gnu_inline__)) char *
__strsep_1c (char **__s, char __reject)
{
  register char *__retval = *__s;
  if (__retval != ((void *)0) && (*__s = (__extension__ (__builtin_constant_p (__reject) && !__builtin_constant_p (__retval) && (__reject) == '\0' ? (char *) __rawmemchr (__retval, __reject) : __builtin_strchr (__retval, __reject)))) != ((void *)0))
    *(*__s)++ = '\0';
  return __retval;
}

extern __inline __attribute__ ((__gnu_inline__)) char *__strsep_2c (char **__s, char __reject1, char __reject2);
extern __inline __attribute__ ((__gnu_inline__)) char *
__strsep_2c (char **__s, char __reject1, char __reject2)
{
  register char *__retval = *__s;
  if (__retval != ((void *)0))
    {
      register char *__cp = __retval;
      while (1)
 {
   if (*__cp == '\0')
     {
       __cp = ((void *)0);
   break;
     }
   if (*__cp == __reject1 || *__cp == __reject2)
     {
       *__cp++ = '\0';
       break;
     }
   ++__cp;
 }
      *__s = __cp;
    }
  return __retval;
}

extern __inline __attribute__ ((__gnu_inline__)) char *__strsep_3c (char **__s, char __reject1, char __reject2,
       char __reject3);
extern __inline __attribute__ ((__gnu_inline__)) char *
__strsep_3c (char **__s, char __reject1, char __reject2, char __reject3)
{
  register char *__retval = *__s;
  if (__retval != ((void *)0))
    {
      register char *__cp = __retval;
      while (1)
 {
   if (*__cp == '\0')
     {
       __cp = ((void *)0);
   break;
     }
   if (*__cp == __reject1 || *__cp == __reject2 || *__cp == __reject3)
     {
       *__cp++ = '\0';
       break;
     }
   ++__cp;
 }
      *__s = __cp;
    }
  return __retval;
}
# 638 "/usr/include/string.h" 2 3 4




# 1 "/usr/include/x86_64-linux-gnu/bits/string3.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/string3.h" 3 4
extern void __warn_memset_zero_len (void) __attribute__((__warning__ ("memset used with constant zero length parameter; this could be due to transposed parameters")))
                                                                                                   ;
# 48 "/usr/include/x86_64-linux-gnu/bits/string3.h" 3 4
extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) void *
__attribute__ ((__nothrow__ , __leaf__)) memcpy (void *__restrict __dest, __const void *__restrict __src, size_t __len)

{
  return __builtin___memcpy_chk (__dest, __src, __len, __builtin_object_size (__dest, 0));
}

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) void *
__attribute__ ((__nothrow__ , __leaf__)) memmove (void *__dest, __const void *__src, size_t __len)
{
  return __builtin___memmove_chk (__dest, __src, __len, __builtin_object_size (__dest, 0));
}
# 76 "/usr/include/x86_64-linux-gnu/bits/string3.h" 3 4
extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) void *
__attribute__ ((__nothrow__ , __leaf__)) memset (void *__dest, int __ch, size_t __len)
{
  if (__builtin_constant_p (__len) && __len == 0
      && (!__builtin_constant_p (__ch) || __ch != 0))
    {
      __warn_memset_zero_len ();
      return __dest;
    }
  return __builtin___memset_chk (__dest, __ch, __len, __builtin_object_size (__dest, 0));
}
# 102 "/usr/include/x86_64-linux-gnu/bits/string3.h" 3 4
extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) char *
__attribute__ ((__nothrow__ , __leaf__)) strcpy (char *__restrict __dest, __const char *__restrict __src)
{
  return __builtin___strcpy_chk (__dest, __src, __builtin_object_size (__dest, 2 > 1));
}
# 117 "/usr/include/x86_64-linux-gnu/bits/string3.h" 3 4
extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) char *
__attribute__ ((__nothrow__ , __leaf__)) strncpy (char *__restrict __dest, __const char *__restrict __src, size_t __len)

{
  return __builtin___strncpy_chk (__dest, __src, __len, __builtin_object_size (__dest, 2 > 1));
}


extern char *__stpncpy_chk (char *__dest, __const char *__src, size_t __n,
       size_t __destlen) __attribute__ ((__nothrow__ , __leaf__));
extern char *__stpncpy_alias (char *__dest, __const char *__src, size_t __n) __asm__ ("" "stpncpy") __attribute__ ((__nothrow__ , __leaf__))

                                 ;

extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) char *
__attribute__ ((__nothrow__ , __leaf__)) stpncpy (char *__dest, __const char *__src, size_t __n)
{
  if (__builtin_object_size (__dest, 2 > 1) != (size_t) -1
      && (!__builtin_constant_p (__n) || __n <= __builtin_object_size (__dest, 2 > 1)))
    return __stpncpy_chk (__dest, __src, __n, __builtin_object_size (__dest, 2 > 1));
  return __stpncpy_alias (__dest, __src, __n);
}


extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) char *
__attribute__ ((__nothrow__ , __leaf__)) strcat (char *__restrict __dest, __const char *__restrict __src)
{
  return __builtin___strcat_chk (__dest, __src, __builtin_object_size (__dest, 2 > 1));
}


extern __inline __attribute__ ((__always_inline__)) __attribute__ ((__gnu_inline__, __artificial__)) char *
__attribute__ ((__nothrow__ , __leaf__)) strncat (char *__restrict __dest, __const char *__restrict __src, size_t __len)

{
  return __builtin___strncat_chk (__dest, __src, __len, __builtin_object_size (__dest, 2 > 1));
}
# 643 "/usr/include/string.h" 2 3 4




# 25 "./libmhandle/mhandle.h" 2
# 67 "./libmhandle/mhandle.h"
void *mhandle_malloc(size_t size, char *at);
void *mhandle_calloc(size_t nmemb, size_t size, char *at);
void *mhandle_realloc(void *ptr, size_t size, char *at);
char *mhandle_strdup(const char *s, char *at);
void mhandle_free(void *ptr, char *at);

void *__xmalloc(size_t size, char *at);
void *__xcalloc(size_t nmemb, size_t size, char *at);
void *__xrealloc(void *ptr, size_t size, char *at);
void *__xstrdup(const char *s, char *at);

void __mhandle_check(char *at);
void __mhandle_done();
unsigned long __mhandle_used_memory();





void __mhandle_register_ptr(void *ptr, unsigned long size, char *at);
# 25 "policy/srripsage.c" 2
# 1 "policy/srripsage.h" 1
# 23 "policy/srripsage.h"
# 1 "policy/../common/intermod-common.h" 1
# 25 "policy/../common/intermod-common.h"
typedef signed long sb8;
typedef unsigned long ub8;
typedef signed int sb4;
typedef unsigned int ub4;
typedef float uf4;
typedef signed short sb2;
typedef unsigned short ub2;
typedef char sb1;
typedef unsigned char ub1;
typedef double uf8;

# 1 "policy/../common/./sat-counter.h" 1
# 21 "policy/../common/./sat-counter.h"
typedef struct saturating_counter_t
{
  ub1 width;
  sb4 min;
  sb4 max;
  sb4 val;
}sctr;
# 37 "policy/../common/intermod-common.h" 2
# 163 "policy/../common/intermod-common.h"
typedef enum gpu_unit_name
{
  gpu_unit_unknown,
  gpu_unit_shader,
  gpu_unit_depth,
  gpu_unit_color
}gpu_unit_name;




typedef enum shader_thread_state
{
  fetch_busy,
  fetch_empty,
  decode_empty,
  exec_busy,
  exec_empty,
  control_dependence,
  data_dependence,
  out_of_resource,
  texture_fetch,
  thread_busy,
}shader_thread_state;


typedef enum frag_memory_access_state
{
  memory_access_unknown,
  memory_access_arrived,
  memory_access_pending,
  private_cache_access,
  shared_cache_access,
  llc_access,
  memory_access_finish
}frag_memory_access_state;


typedef enum memory_access_status
{
  memory_access_status_unknown,
  memory_access_hit,
  memory_access_miss
}memory_access_status;
# 216 "policy/../common/intermod-common.h"
typedef struct memory_access_info
{
  gpu_unit_name unit;
  frag_memory_access_state state;
  memory_access_status status;
  ub1 assigned;
  ub4 mapped_rop;
  ub1 sdp_bs_sample;
  ub1 sdp_ps_sample;
  ub1 sdp_cs_sample;
}memory_access_info;

typedef memory_access_info mai;
# 242 "policy/../common/intermod-common.h"
struct attila_data
{
  ub1 seqno;
  ub8 preqs;
  ub8 vaddr;
  ub4 size;
  ub1 unit;
  ub1 access_kind;
  ub8 reqs;
  ub8 scycle;
  ub8 ecycle;
};
# 272 "policy/../common/intermod-common.h"
struct m2s_data
{
  ub4 pid;
  ub8 seqno;
  ub8 vaddr;
  ub4 size;
  ub1 access_kind;
  ub1 stream;
  ub8 instance;
  ub1 dbuf;
  ub1 masked;
  ub1 fpRead;
  ub1 readBypass;
  ub1 writeBypass;
  sb4 mid;
  struct mod_t *mod;
};
# 314 "policy/../common/intermod-common.h"
struct intermod_data
{
  union
  {
    struct attila_data attila_data;
    struct m2s_data m2s_data;
  };

  void (*cb)(struct intermod_data *);
  void *key;
  ub1 status;
  ub1 bypass;
  ub1 pstrm;
  mai *ainfo;
};
# 338 "policy/../common/intermod-common.h"
struct m2s_config
{
  void *mod;
  sb4 agent_mid;
  ub1 cache_blck_sz;
  void (*ini)(int argc, char** argv);
  void (*req)(struct intermod_data *);
  char (*clk)(long int clock);
};
# 449 "policy/../common/intermod-common.h"
struct attila_config
{
  ub4 uc_read_bypass[(16)];
  ub4 uc_write_bypass[(16)];
  ub4 sc_read_bypass[(16)];
  ub4 sc_write_bypass[(16)];
  ub4 lc_read_bypass[(16)];
  ub4 lc_write_bypass[(16)];
  ub4 lc_bypass_all_buf[(16)];
  ub4 lc_fp_read[(16)];
  ub4 cache_blck_sz[(16)];
  ub1 llc_enable;
  ub1 llcReadAlloc;
  void (*ini)(int argc, char **argv);
  void (*cb)(struct intermod_data *);
  char (*clk)(long int clock);
};


struct macsim_config
{
  ub4 cache_blck_sz[4];
  void (*ini)(int argc, char **argv);
  void (*cb)(struct intermod_data *);
  char (*clk)(long int clock);
};


typedef enum shader_instruction_type
{
  texture_operation,
  alu_operation,
  branch_operation,
}shader_instruction_type;


typedef struct shader_pipeline_state
{
  shader_thread_state thread_state;
}shader_pipeline_state;


typedef struct fc_llc_reqs
{
  ub1 req_state;
  ub1 unit;
  ub1 strm;
  ub4 pid;
  ub8 inAddress;
  ub8 outAddress;
  ub8 offset;
  ub8 line;
  ub8 way;
  ub8 size;
  ub1 alloc;
  ub1 spill;
  ub1 fill;
  ub1 invalidate;
  ub1 masked;
  ub1 flush;
  ub1 dbuf;
  ub8 info;
  ub1 bypass;
  ub1 fpath;
  ub1 cmiss;
  ub1 *reqs;
  ub8 id;
  ub4 reserves;
  ub1 rport_avl;
  ub1 wport_avl;
  ub1 mshr_avl;
  ub8 scycle;
  ub8 ecycle;
  ub8 cache;
  ub8 scache;
  ub1 status;
  sb4 miplvl0;
  sb4 miplvl1;
  ub8 texture_base;
  mai *ainfo;
}fc_llc_reqs;

typedef struct pc_stall_data
{
  ub8 pc;
  ub8 access_count;
  ub8 life_cycles;
  ub8 stall_at_head;
  ub8 stall_count;
  ub8 stall_cycles;
  ub8 head_insert_cycle;
  ub8 local_stall_cycles;
  ub8 miss_l1;
  ub8 miss_l2;
  ub8 miss_llc;
  ub8 access_llc;
  ub8 x_set_count;
  ub8 y_set_count;
  ub8 p_set_count;
  ub8 q_set_count;
  ub8 r_set_count;
  ub8 rob_pos;
  ub8 rob_loads;
  ub1 in_r_set;
  ub8 stat_access_count;
  ub8 stat_life_cycles;
  ub8 stat_stall_at_head;
  ub8 stat_stall_count;
  ub8 stat_stall_cycles;
  ub8 stat_head_insert_cycle;
  ub8 stat_local_stall_cycles;
  ub8 stat_miss_l1;
  ub8 stat_miss_l2;
  ub8 stat_miss_llc;
  ub8 stat_access_llc;
  ub8 stat_x_set_count;
  ub8 stat_y_set_count;
  ub8 stat_p_set_count;
  ub8 stat_q_set_count;
  ub8 stat_r_set_count;
  ub8 stat_set_recat;
  ub8 stat_rob_pos;
  ub8 stat_rob_loads;
  ub1 stat_in_r_set;
}pc_stall_data;


typedef struct ffifo_info
{
  ub1 fgi_enable;
  ub4 fgi_count;
  ub8 fgi_access;
  ub1 *fgi_speedup;
  sctr *fgi_utility;
  sctr *fgi_cutility;
}ffifo_info;


typedef struct shader_info
{
  ub4 sgi_count;
  ub8 sgi_access;
  ub4 *sgi_rthreads;
  sctr *sgi_utility;
  sctr *sgi_cutility;
}shader_info;


typedef struct rop_info
{
  ub4 rgi_count;
  ub8 rgi_access;
  sctr *rgi_fills;
  sctr *rgi_utility;
  sctr *rgi_cutility;
}rop_info;

typedef struct frontend_info
{
  ub4 fegi_count;
  ub8 fegi_access;
  sctr fegi_occupancy;
  sctr fegi_utility;
  sctr fegi_cutility;
}frontend_info;


typedef struct memory_trace_in_file
{
  ub8 address;
  ub1 stream;
  ub1 fill;
  ub1 spill;
}memory_trace_in_file;


typedef struct memory_trace
{
  ub8 address;
  ub8 vtl_addr;
  ub8 pc;
  ub1 stream;
  ub4 core;
  ub4 pid;
  ub4 mapped_rop;
  ub1 dbuf;
  ub1 fill;
  ub1 spill;
  ub1 prefetch;
  ub1 dirty;
  ub1 sdp_sample;
  void *policy_data;
}memory_trace;
# 650 "policy/../common/intermod-common.h"
typedef struct cache_access_status
{
  ub8 tag;
  ub8 vtl_addr;
  ub1 fate;
  sb8 way;
  ub1 stream;
  ub8 pc;
  ub8 fillpc;
  ub1 dirty;
  ub1 epoch;
  ub8 access;
  sb8 rdis;
  sb8 prdis;
  ub1 is_bt_block;
  ub1 is_ct_block;
  ub1 op;
}cache_access_status;


typedef struct cache_access_info
{
  ub8 access_count;
  ub1 stream;
  ub8 first_seq;
  ub8 last_seq;
  ub8 list_head;
  ub1 is_bt_block;
  ub1 btuse;
  ub1 is_ct_block;
  ub1 ctuse;
  ub1 rrpv_trans;
}cache_access_info;

int gcd_ab(int a, int b);
# 24 "policy/srripsage.h" 2
# 1 "policy/../common/sat-counter.h" 1
# 25 "policy/srripsage.h" 2
# 1 "policy/policy.h" 1
# 152 "policy/policy.h"
typedef enum cache_policy_t
{
        cache_policy_invalid,


        cache_policy_bypass,


        cache_policy_opt,


        cache_policy_nru,


        cache_policy_lru,


        cache_policy_stridelru,


        cache_policy_fifo,


        cache_policy_lip,


        cache_policy_bip,


        cache_policy_dip,


        cache_policy_srrip,


        cache_policy_srripm,


        cache_policy_srript,


        cache_policy_srripdbp,


        cache_policy_srripsage,


        cache_policy_brrip,


        cache_policy_drrip,


        cache_policy_gspc,



        cache_policy_gspcm,



        cache_policy_gspct,


        cache_policy_gshp,


        cache_policy_ucp,


        cache_policy_tapucp,


        cache_policy_tapdrrip,


        cache_policy_salru,


        cache_policy_cpulast,


        cache_policy_helmdrrip,


        cache_policy_sap,


        cache_policy_sdp,


        cache_policy_pasrrip,


        cache_policy_xsp,


        cache_policy_xsppin,


        cache_policy_xspdbp
}cache_policy_t;

typedef struct list_head_t
{
  int id;
  struct cache_block_t *head;
}list_head_t;
# 26 "policy/srripsage.h" 2
# 1 "policy/rrip.h" 1
# 24 "policy/rrip.h"
typedef struct cache_list_head_rrip_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}rrip_list;
# 27 "policy/srripsage.h" 2
# 1 "policy/srrip.h" 1
# 27 "policy/srrip.h"
# 1 "cache/cache-param.h" 1
# 11 "cache/cache-param.h"
struct cache_params
{
  enum cache_policy_t policy;
  int num_sets;
  int ways;
  int max_rrpv;
  int spill_rrpv;
  double threshold;
  int stream;
  int streams;
  int core_policy1;
  int core_policy2;
  int txs;
  int talpha;
  long highthr;
  long lowthr;
  long tlpthr;
  long pthr;
  long mthr;
  ub4 sdp_samples;
  ub4 sdp_gpu_cores;
  ub4 sdp_cpu_cores;
  ub4 sdp_streams;
  ub1 sdp_cpu_tpset;
  ub1 sdp_cpu_tqset;
  ub8 sdp_tactivate;
  ub1 sdp_psetinstp;
  ub1 sdp_greplace;
  ub1 sdp_psetbmi;
  ub1 sdp_psetrecat;
  ub1 sdp_psetbse;
  ub1 sdp_psetmrt;
  ub1 sdp_psethrt;
  ub1 stl_stream;
  ub1 rpl_on_miss;
  ub1 use_long_bv;
};
# 28 "policy/srrip.h" 2
# 1 "cache/cache-block.h" 1



enum cache_block_state_t
{
  cache_block_invalid = 0,
  cache_block_modified,
  cache_block_exclusive,
  cache_block_shared
};

struct cache_block_t
{
  struct cache_block_t *next;
  struct cache_block_t *prev;
  void *data;


  unsigned int block_state;


  unsigned int pid;


  unsigned int stream;


  unsigned int sap_stream;


  unsigned char is_bt_block;


  unsigned char is_ct_block;


  unsigned char is_zt_block;


  unsigned int access;


  enum cache_block_state_t state;

  long long tag;
  long long vtl_addr;
  long long tag_end;
  int way;
  long long replacements;
  long long hits;
  long long false_hits;
  long long unknown_hits;
  long long max_stride;
  long long max_use;
  long long use;
  unsigned char *long_use;
  unsigned char *long_hit;
  long long hit;
  long long expand;
  unsigned int use_bitmap;
  unsigned int max_use_bitmap;
  unsigned int hit_bitmap;
  unsigned int max_hit_bitmap;
  unsigned int captured_use;
  unsigned int color_transition;
  long long x_stream_use;
  long long s_stream_use;







  unsigned int busy : 1;






  unsigned int cached : 1;



  unsigned int prefetch : 1;


  unsigned int nru : 1;


  unsigned int dirty : 1;

  unsigned int epoch;


  unsigned long long recency;

  unsigned long long pc;
};
# 29 "policy/srrip.h" 2


typedef struct cache_list_head_srrip_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}srrip_list;
# 54 "policy/srrip.h"
typedef struct cache_policy_srrip_data_t
{
  cache_policy_t current_fill_policy;
  cache_policy_t default_fill_policy;
  cache_policy_t current_access_policy;
  cache_policy_t default_access_policy;
  cache_policy_t current_replacement_policy;
  cache_policy_t default_replacement_policy;
  ub4 max_rrpv;
  ub4 spill_rrpv;
  rrip_list *valid_head;
  rrip_list *valid_tail;
  list_head_t *free_head;
  list_head_t *free_tail;

  struct cache_block_t *blocks;
}srrip_data;
# 94 "policy/srrip.h"
void cache_init_srrip(struct cache_params *params, srrip_data *policy_data);
# 115 "policy/srrip.h"
void cache_free_srrip(srrip_data *policy_data);
# 137 "policy/srrip.h"
struct cache_block_t * cache_find_block_srrip(srrip_data *policy_data, long long tag);
# 163 "policy/srrip.h"
void cache_fill_block_srrip(srrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info);
# 186 "policy/srrip.h"
int cache_replace_block_srrip(srrip_data *policy_data);
# 211 "policy/srrip.h"
void cache_access_block_srrip(srrip_data *policy_data, int way, int strm,
  memory_trace *info);
# 236 "policy/srrip.h"
struct cache_block_t cache_get_block_srrip(srrip_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr);
# 263 "policy/srrip.h"
void cache_set_block_srrip(srrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);
# 286 "policy/srrip.h"
int cache_get_fill_rrpv_srrip(srrip_data *policy_data, memory_trace *info);
# 307 "policy/srrip.h"
int cache_get_replacement_rrpv_srrip(srrip_data *policy_data);
# 328 "policy/srrip.h"
int cache_get_new_rrpv_srrip(int old_rrpv);
# 350 "policy/srrip.h"
int cache_count_block_srrip(srrip_data *policy_data, ub1 strm);
# 372 "policy/srrip.h"
void cache_set_current_fill_policy_srrip(srrip_data *policy_data, cache_policy_t policy);
# 394 "policy/srrip.h"
void cache_set_current_access_policy_srrip(srrip_data *policy_data, cache_policy_t policy);
# 416 "policy/srrip.h"
void cache_set_current_replacement_policy_srrip(srrip_data *policy_data, cache_policy_t policy);
# 28 "policy/srripsage.h" 2




typedef struct cache_list_head_srripsage_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}srripsage_list;
# 55 "policy/srripsage.h"
typedef struct cache_policy_srripsage_data_t
{
  cache_policy_t following;
  ub4 max_rrpv;
  ub4 threshold;
  ub4 spill_rrpv;
  rrip_list *valid_head;
  rrip_list *valid_tail;
  list_head_t *free_head;
  list_head_t *free_tail;
  srrip_data srrip_policy_data;

  struct cache_block_t *blocks;
}srripsage_data;

typedef struct cache_policy_srripsage_gdata_t
{

  struct saturating_counter_t tex_e0_fill_ctr;
  struct saturating_counter_t tex_e0_hit_ctr;
  struct saturating_counter_t tex_e1_fill_ctr;
  struct saturating_counter_t tex_e1_hit_ctr;
  struct saturating_counter_t acc_all_ctr;
}srripsage_gdata;
# 103 "policy/srripsage.h"
void cache_init_srripsage(int set_indx, struct cache_params *params,
    srripsage_data *policy_data, srripsage_gdata *global_data);
# 125 "policy/srripsage.h"
void cache_free_srripsage(srripsage_data *policy_data);
# 147 "policy/srripsage.h"
struct cache_block_t * cache_find_block_srripsage(srripsage_data *policy_data, long long tag);
# 173 "policy/srripsage.h"
void cache_fill_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, int way, long long tag,
    enum cache_block_state_t state, int strm, memory_trace *info);
# 197 "policy/srripsage.h"
int cache_replace_block_srripsage(srripsage_data *policy_data);
# 222 "policy/srripsage.h"
void cache_access_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, int way, int strm, memory_trace *info);
# 247 "policy/srripsage.h"
struct cache_block_t cache_get_block_srripsage(srripsage_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr);
# 274 "policy/srripsage.h"
void cache_set_block_srripsage(srripsage_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);
# 297 "policy/srripsage.h"
int cache_get_fill_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info);
# 319 "policy/srripsage.h"
int cache_get_replacement_rrpv_srripsage(srripsage_data *policy_data);
# 340 "policy/srripsage.h"
int cache_get_new_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, int way, int old_rrpv);
# 363 "policy/srripsage.h"
int cache_count_block_srripsage(srripsage_data *policy_data, ub1 strm);
# 26 "policy/srripsage.c" 2
# 1 "policy/sap.h" 1
# 36 "policy/sap.h"
# 1 "policy/brrip.h" 1
# 31 "policy/brrip.h"
typedef struct cache_list_head_brrip_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}brrip_list;
# 55 "policy/brrip.h"
typedef struct cache_policy_brrip_data_t
{
  ub4 threshold;
  cache_policy_t current_fill_policy;
  cache_policy_t default_fill_policy;
  cache_policy_t current_access_policy;
  cache_policy_t default_access_policy;
  cache_policy_t current_replacement_policy;
  cache_policy_t default_replacement_policy;
  ub4 max_rrpv;
  ub4 spill_rrpv;
  rrip_list *valid_head;
  rrip_list *valid_tail;
  list_head_t *free_head;
  list_head_t *free_tail;

  struct cache_block_t *blocks;
}brrip_data;


typedef struct cache_policy_brrip_gdata_t
{
  ub4 threshold;
  sctr access_ctr;
}brrip_gdata;
# 104 "policy/brrip.h"
void cache_init_brrip(struct cache_params *params, brrip_data *policy_data,
  brrip_gdata *global_data);
# 126 "policy/brrip.h"
void cache_free_brrip(brrip_data *policy_data);
# 147 "policy/brrip.h"
struct cache_block_t * cache_find_block_brrip(brrip_data *policy_data, long long tag);
# 174 "policy/brrip.h"
void cache_fill_block_brrip(brrip_data *policy_data, brrip_gdata *global_data,
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace *info);
# 198 "policy/brrip.h"
int cache_replace_block_brrip(brrip_data *policy_data);
# 223 "policy/brrip.h"
void cache_access_block_brrip(brrip_data *policy_data, int way, int strm,
  memory_trace *info);
# 248 "policy/brrip.h"
struct cache_block_t cache_get_block_brrip(brrip_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream);
# 274 "policy/brrip.h"
void cache_set_block_brrip(brrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);
# 297 "policy/brrip.h"
int cache_get_fill_rrpv_brrip(brrip_data *policy_data, brrip_gdata *global_data);
# 318 "policy/brrip.h"
int cache_get_replacement_rrpv_brrip(brrip_data *policy_data);
# 339 "policy/brrip.h"
int cache_get_new_rrpv_brrip(int old_rrpv);
# 361 "policy/brrip.h"
void cache_set_current_fill_policy_brrip(brrip_data *policy_data, cache_policy_t policy);
# 383 "policy/brrip.h"
void cache_set_current_access_policy_brrip(brrip_data *policy_data, cache_policy_t policy);
# 405 "policy/brrip.h"
void cache_set_current_replacement_policy_brrip(brrip_data *policy_data, cache_policy_t policy);
# 37 "policy/sap.h" 2







typedef enum sap_stream
{
  sap_stream_x = 1,
  sap_stream_y,
  sap_stream_p,
  sap_stream_q,
  sap_stream_r
}sap_stream;


typedef struct cache_policy_sap_stats_t
{
  FILE *stat_file;
  ub8 sap_x_fill;
  ub8 sap_y_fill;
  ub8 sap_p_fill;
  ub8 sap_q_fill;
  ub8 sap_r_fill;
  ub8 sap_x_evct;
  ub8 sap_y_evct;
  ub8 sap_p_evct;
  ub8 sap_q_evct;
  ub8 sap_r_evct;
  ub8 sap_x_access;
  ub8 sap_y_access;
  ub8 sap_p_access;
  ub8 sap_q_access;
  ub8 sap_r_access;
  ub8 sap_psetrecat;
  ub8 next_schedule;
  ub8 schedule_period;
  ub1 hdr_printed;
}sap_stats;


typedef struct sap_pdata
{
  sap_stream stream;
}sap_pdata;
# 95 "policy/sap.h"
typedef struct cache_policy_sap_data_t
{

  ub4 max_rrpv;
  rrip_list *valid_head;
  rrip_list *valid_tail;

  list_head_t *free_head;
  list_head_t *free_tail;

  struct cache_block_t *blocks;

  srrip_data srrip;
  cache_policy_t *per_stream_policy;
}sap_data;





typedef struct cache_policy_sap_gdata_t
{
  ub4 sap_cpu_cores;
  ub4 sap_gpu_cores;
  ub4 sap_streams;
  ub4 policy_set;
  ub1 sap_cpu_tpset;
  ub1 sap_cpu_tqset;
  ub8 sap_y_access;
  ub8 sap_y_miss;
  ub8 activate_access;
  sap_stats stats;
  ub8 threshold;
  ub1 sdp_psetinstp;
  ub1 sdp_greplace;
  ub1 sdp_psetbmi;
  ub1 sdp_psetrecat;
  ub1 sdp_psetbse;
  ub1 sdp_psetmrt;
  ub1 sdp_psethrt;
  sctr access_ctr;
}sap_gdata;
# 162 "policy/sap.h"
void cache_init_sap(long long int set_indx, struct cache_params *params,
  sap_data *policy_data, sap_gdata *global_data);
# 186 "policy/sap.h"
void cache_free_sap(long long int set_indx, sap_data *policy_data, sap_gdata *global_data);
# 208 "policy/sap.h"
struct cache_block_t * cache_find_block_sap(sap_data *policy_data, long long tag);
# 235 "policy/sap.h"
void cache_fill_block_sap(sap_data *policy_data, sap_gdata *global_data, int way, long long tag,
    enum cache_block_state_t state, int strm, memory_trace *info);
# 260 "policy/sap.h"
int cache_replace_block_sap(sap_data *policy_data, sap_gdata *global_data,
  memory_trace *info);
# 287 "policy/sap.h"
void cache_access_block_sap(sap_data *policy_data, sap_gdata *global_data,
  int way, int stream, memory_trace *info);
# 314 "policy/sap.h"
struct cache_block_t cache_get_block_sap(sap_data *policy_data, sap_gdata *global_data,
  int way, long long *tag_ptr, enum cache_block_state_t *state_ptr,
  int *stream_ptr);
# 342 "policy/sap.h"
void cache_set_block_sap(sap_data *policy_data, sap_gdata *global_data,
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info);
# 370 "policy/sap.h"
void set_per_stream_policy_sap(sap_data *policy_data, sap_gdata *global_data,
  ub4 core, ub1 stream, cache_policy_t policy);
# 396 "policy/sap.h"
sap_stream get_sap_stream(sap_gdata *global_data, ub1 stream_in, ub1 pid_in,
  memory_trace *info);
# 419 "policy/sap.h"
void cache_init_sap_stats(sap_stats *stats, char *file_name);
# 440 "policy/sap.h"
void cache_fini_sap_stats(sap_stats *stats);
# 462 "policy/sap.h"
void cache_update_sap_access_stats(sap_gdata *global_data, memory_trace *info, ub1 psetrecat);
# 488 "policy/sap.h"
void cache_update_sap_fill_stats(sap_gdata *global_data, sap_stream sstream);
# 515 "policy/sap.h"
void cache_update_sap_replacement_stats(sap_gdata *global_data, sap_stream sstream);
# 540 "policy/sap.h"
void cache_update_sap_fill_stall_stats(sap_gdata *global_data, memory_trace *info);
# 562 "policy/sap.h"
void cache_dump_sap_stats(sap_stats *stats, ub8 cycle);
# 27 "policy/srripsage.c" 2
# 187 "policy/srripsage.c"
int srripsage_blocks = 0;

static int get_set_type_srripsage(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return (0);
  }

  return (1);
}


static void cache_update_fill_counter_srripsage(srrip_data *policy_data,
    srripsage_gdata *global_data, int way, int strm)
{
# 221 "policy/srripsage.c"
  if (strm == (4))
  {
    ((global_data->tex_e0_fill_ctr).val = ((global_data->tex_e0_fill_ctr).val < (global_data->tex_e0_fill_ctr).max) ? (global_data->tex_e0_fill_ctr).val + 1 : (global_data->tex_e0_fill_ctr).val);
    ((policy_data)->blocks)[way].block_state = 0;
  }

  ((global_data->acc_all_ctr).val = ((global_data->acc_all_ctr).val < (global_data->acc_all_ctr).max) ? (global_data->acc_all_ctr).val + 1 : (global_data->acc_all_ctr).val);


  if (((global_data->acc_all_ctr).val == (global_data->acc_all_ctr).max))
  {
    ((global_data->tex_e0_fill_ctr).val = ((global_data->tex_e0_fill_ctr).val > 1) ? ((global_data->tex_e0_fill_ctr).val / 2) : (global_data->tex_e0_fill_ctr).val);
    ((global_data->tex_e0_hit_ctr).val = ((global_data->tex_e0_hit_ctr).val > 1) ? ((global_data->tex_e0_hit_ctr).val / 2) : (global_data->tex_e0_hit_ctr).val);
    ((global_data->tex_e1_fill_ctr).val = ((global_data->tex_e1_fill_ctr).val > 1) ? ((global_data->tex_e1_fill_ctr).val / 2) : (global_data->tex_e1_fill_ctr).val);
    ((global_data->tex_e1_hit_ctr).val = ((global_data->tex_e1_hit_ctr).val > 1) ? ((global_data->tex_e1_hit_ctr).val / 2) : (global_data->tex_e1_hit_ctr).val);

    ((global_data->acc_all_ctr).val = (0));
  }
}


static void cache_update_hit_counter_srripsage(srrip_data *policy_data,
    srripsage_gdata *global_data, int way, int strm)
{
# 258 "policy/srripsage.c"
  struct cache_block_t *block;

  block = &(((policy_data)->blocks)[way]);
  switch (strm)
  {
    case (4):

      if (block->block_state == 0)
      {
        ((global_data->tex_e0_hit_ctr).val = ((global_data->tex_e0_hit_ctr).val < (global_data->tex_e0_hit_ctr).max) ? (global_data->tex_e0_hit_ctr).val + 1 : (global_data->tex_e0_hit_ctr).val);
        ((global_data->tex_e1_fill_ctr).val = ((global_data->tex_e1_fill_ctr).val < (global_data->tex_e1_fill_ctr).max) ? (global_data->tex_e1_fill_ctr).val + 1 : (global_data->tex_e1_fill_ctr).val);
        block->block_state = 1;
      }
      else
      {
        if (block->block_state == 1)
        {
          ((global_data->tex_e1_hit_ctr).val = ((global_data->tex_e1_hit_ctr).val < (global_data->tex_e1_hit_ctr).max) ? (global_data->tex_e1_hit_ctr).val + 1 : (global_data->tex_e1_hit_ctr).val);
          block->block_state = 2;
        }
        else
        {
          if (block->block_state == 3)
          {
            ((global_data->tex_e0_fill_ctr).val = ((global_data->tex_e0_fill_ctr).val < (global_data->tex_e0_fill_ctr).max) ? (global_data->tex_e0_fill_ctr).val + 1 : (global_data->tex_e0_fill_ctr).val);
            block->block_state = 0;
          }
        }
      }

      break;
  }

  ((global_data->acc_all_ctr).val = ((global_data->acc_all_ctr).val < (global_data->acc_all_ctr).max) ? (global_data->acc_all_ctr).val + 1 : (global_data->acc_all_ctr).val);


  if (((global_data->acc_all_ctr).val == (global_data->acc_all_ctr).max))
  {
    ((global_data->tex_e0_fill_ctr).val = ((global_data->tex_e0_fill_ctr).val > 1) ? ((global_data->tex_e0_fill_ctr).val / 2) : (global_data->tex_e0_fill_ctr).val);
    ((global_data->tex_e0_hit_ctr).val = ((global_data->tex_e0_hit_ctr).val > 1) ? ((global_data->tex_e0_hit_ctr).val / 2) : (global_data->tex_e0_hit_ctr).val);
    ((global_data->tex_e1_fill_ctr).val = ((global_data->tex_e1_fill_ctr).val > 1) ? ((global_data->tex_e1_fill_ctr).val / 2) : (global_data->tex_e1_fill_ctr).val);
    ((global_data->tex_e1_hit_ctr).val = ((global_data->tex_e1_hit_ctr).val > 1) ? ((global_data->tex_e1_hit_ctr).val / 2) : (global_data->tex_e1_hit_ctr).val);

    ((global_data->acc_all_ctr).val = (0));
  }
}

void cache_init_srripsage(int set_indx, struct cache_params *params,
    srripsage_data *policy_data, srripsage_gdata *global_data)
{

  ((params) ? (void) (0) : __assert_fail ("params", "policy/srripsage.c", 309, __PRETTY_FUNCTION__));
  ((policy_data) ? (void) (0) : __assert_fail ("policy_data", "policy/srripsage.c", 310, __PRETTY_FUNCTION__));






  ((policy_data)->valid_head) = ((rrip_list *)(__xcalloc(((params->max_rrpv) + 1), (sizeof(rrip_list)), "policy/srripsage.c" ":" "317")));
  ((policy_data)->valid_tail) = ((rrip_list *)(__xcalloc(((params->max_rrpv) + 1), (sizeof(rrip_list)), "policy/srripsage.c" ":" "318")));



  ((((policy_data)->valid_head)) ? (void) (0) : __assert_fail ("((policy_data)->valid_head)", "policy/srripsage.c", 322, __PRETTY_FUNCTION__));
  ((((policy_data)->valid_tail)) ? (void) (0) : __assert_fail ("((policy_data)->valid_tail)", "policy/srripsage.c", 323, __PRETTY_FUNCTION__));


  ((policy_data)->max_rrpv) = (params->max_rrpv);
  ((policy_data)->spill_rrpv) = params->spill_rrpv;

  ((params->spill_rrpv <= (params->max_rrpv)) ? (void) (0) : __assert_fail ("params->spill_rrpv <= (params->max_rrpv)", "policy/srripsage.c", 329, __PRETTY_FUNCTION__));


  for (int i = 0; i <= (params->max_rrpv); i++)
  {
    ((policy_data)->valid_head)[i].rrpv = i;
    ((policy_data)->valid_head)[i].head = ((void *)0);
    ((policy_data)->valid_tail)[i].head = ((void *)0);
  }


  ((policy_data)->blocks) =
    (struct cache_block_t *)(__xcalloc((params->ways), (sizeof (struct cache_block_t)), "policy/srripsage.c" ":" "341"));



  ((policy_data)->free_head) = ((list_head_t *)(__xcalloc((1), (sizeof(list_head_t)), "policy/srripsage.c" ":" "345")));
  ((((policy_data)->free_head)) ? (void) (0) : __assert_fail ("((policy_data)->free_head)", "policy/srripsage.c", 346, __PRETTY_FUNCTION__));

  ((policy_data)->free_tail) = ((list_head_t *)(__xcalloc((1), (sizeof(list_head_t)), "policy/srripsage.c" ":" "348")));
  ((((policy_data)->free_tail)) ? (void) (0) : __assert_fail ("((policy_data)->free_tail)", "policy/srripsage.c", 349, __PRETTY_FUNCTION__));




  ((policy_data)->free_head->head) = &(((policy_data)->blocks)[0]);
  ((policy_data)->free_tail->head) = &(((policy_data)->blocks)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&((policy_data)->blocks)[way])->way = way;
    (&((policy_data)->blocks)[way])->state = cache_block_invalid;
    (&((policy_data)->blocks)[way])->next = way ?
      (&((policy_data)->blocks)[way - 1]) : ((void *)0);
    (&((policy_data)->blocks)[way])->prev = way < params->ways - 1 ?
      (&((policy_data)->blocks)[way + 1]) : ((void *)0);
  }

  if (set_indx == 0)
  {

    do { (global_data->tex_e0_fill_ctr).val = 0; (global_data->tex_e0_fill_ctr).width = 8; (global_data->tex_e0_fill_ctr).min = 0; (global_data->tex_e0_fill_ctr).max = 255; }while(0);
    do { (global_data->tex_e0_hit_ctr).val = 0; (global_data->tex_e0_hit_ctr).width = 8; (global_data->tex_e0_hit_ctr).min = 0; (global_data->tex_e0_hit_ctr).max = 255; }while(0);

    do { (global_data->tex_e1_fill_ctr).val = 0; (global_data->tex_e1_fill_ctr).width = 8; (global_data->tex_e1_fill_ctr).min = 0; (global_data->tex_e1_fill_ctr).max = 255; }while(0);
    do { (global_data->tex_e1_hit_ctr).val = 0; (global_data->tex_e1_hit_ctr).width = 8; (global_data->tex_e1_hit_ctr).min = 0; (global_data->tex_e1_hit_ctr).max = 255; }while(0);
  }
# 387 "policy/srripsage.c"
  {
    policy_data->following = cache_policy_srripsage;
  }

  ((((policy_data)->max_rrpv) != 0) ? (void) (0) : __assert_fail ("((policy_data)->max_rrpv) != 0", "policy/srripsage.c", 391, __PRETTY_FUNCTION__));


}


void cache_free_srripsage(srripsage_data *policy_data)
{

  free(((policy_data)->blocks));


  if (((policy_data)->valid_head))
  {
    free(((policy_data)->valid_head));
  }


  if (((policy_data)->valid_tail))
  {
    free(((policy_data)->valid_tail));
  }


  if (policy_data->following == cache_policy_srrip)
  {
    cache_free_srrip(&(policy_data->srrip_policy_data));
  }
}

struct cache_block_t* cache_find_block_srripsage(srripsage_data *policy_data, long long tag)
{
  int max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  if (policy_data->following == cache_policy_srrip)
  {
    node = cache_find_block_srrip(&(policy_data->srrip_policy_data), tag);
  }
  else
  {
    max_rrpv = policy_data->max_rrpv;
    node = ((void *)0);

    for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
    {
      head = ((policy_data)->valid_head)[rrpv].head;

      for (node = head; node; node = node->prev)
      {
        ((node->state != cache_block_invalid) ? (void) (0) : __assert_fail ("node->state != cache_block_invalid", "policy/srripsage.c", 442, __PRETTY_FUNCTION__));

        if (node->tag == tag)
          goto end;
      }
    }
  }

end:
  return node;
}

void cache_fill_block_srripsage(srripsage_data *policy_data,
  srripsage_gdata *global_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;
  int rrpv;


  ((tag >= 0) ? (void) (0) : __assert_fail ("tag >= 0", "policy/srripsage.c", 462, __PRETTY_FUNCTION__));
  ((state != cache_block_invalid) ? (void) (0) : __assert_fail ("state != cache_block_invalid", "policy/srripsage.c", 463, __PRETTY_FUNCTION__));


  block = &(((policy_data)->blocks)[way]);

  ((block->stream == 0) ? (void) (0) : __assert_fail ("block->stream == 0", "policy/srripsage.c", 468, __PRETTY_FUNCTION__));

  if (policy_data->following == cache_policy_srrip)
  {

    cache_fill_block_srrip(&(policy_data->srrip_policy_data), way, tag,
        state, strm, info);


    cache_update_fill_counter_srripsage(&(policy_data->srrip_policy_data),
        global_data, way, strm);
  }
  else
  {
    ((policy_data->following == cache_policy_srripsage) ? (void) (0) : __assert_fail ("policy_data->following == cache_policy_srripsage", "policy/srripsage.c", 482, __PRETTY_FUNCTION__));


    rrpv = cache_get_fill_rrpv_srripsage(policy_data, global_data, info);

    if (info && info->spill == (1))
    {
      rrpv = ((policy_data)->spill_rrpv);
    }


    if (rrpv != (-1))
    {

      ((rrpv >= 0 && rrpv <= policy_data->max_rrpv) ? (void) (0) : __assert_fail ("rrpv >= 0 && rrpv <= policy_data->max_rrpv", "policy/srripsage.c", 496, __PRETTY_FUNCTION__));


      do { ((((policy_data)->free_head->head) && ((policy_data)->free_head->head)) ? (void) (0) : __assert_fail ("((policy_data)->free_head->head) && ((policy_data)->free_head->head)", "policy/srripsage.c", 499, __PRETTY_FUNCTION__)); (((block)->state == cache_block_invalid) ? (void) (0) : __assert_fail ("(block)->state == cache_block_invalid", "policy/srripsage.c", 499, __PRETTY_FUNCTION__)); do { if ((block)->next == ((void *)0)) { (((policy_data)->free_head->head)) = (block)->prev; if ((block)->prev) { (block)->prev->next = ((void *)0); } } if ((block)->prev == ((void *)0)) { (((policy_data)->free_tail->head)) = (block)->next; if ((block)->next) { (block)->next->prev = ((void *)0); } } if ((block)->next != ((void *)0) && (block)->prev != ((void *)0)) { ((block)->next)->prev = (block)->prev; ((block)->prev)->next = (block)->next; } (block)->next = ((void *)0); (block)->prev = ((void *)0); }while(0); (block)->next = ((void *)0); (block)->prev = ((void *)0); (block)->busy = 0; (block)->cached = 0; (block)->prefetch = 0; }while(0);;


      do { (block)->tag = (tag); (block)->vtl_addr = (info->vtl_addr); (block)->state = (state); }while(0);
      do { if ((block)->stream == (2) && strm != (2)) { srripsage_blocks--; } if (strm == (2) && (block)->stream != (2)) { srripsage_blocks++; } (block)->stream = strm; }while(0);
      block->dirty = (info && info->spill) ? 1 : 0;

      switch (strm)
      {
        case (2):
          block->epoch = 3;
          break;

        case (4):
          block->epoch = 0;
          break;
      }


      do { if ((((policy_data)->valid_head)[rrpv]).head == ((void *)0) && (((policy_data)->valid_tail)[rrpv]).head == ((void *)0)) { (((policy_data)->valid_head)[rrpv]).head = (block); (((policy_data)->valid_tail)[rrpv]).head = (block); (block)->data = (&((policy_data)->valid_head)[rrpv]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 518 "policy/srripsage.c"
      ,

 520
# 518 "policy/srripsage.c"
      , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[rrpv]).head)->prev = (block); (block)->next = (((policy_data)->valid_tail)[rrpv]).head; (((policy_data)->valid_tail)[rrpv]).head = (block); (block)->data = (&((policy_data)->valid_head)[rrpv]); }while(0)

                                                       ;
    }
  }
}

int cache_replace_block_srripsage(srripsage_data *policy_data)
{
  struct cache_block_t *block;
  int rrpv;


  unsigned int min_wayid = ~(0);

  if (policy_data->following == cache_policy_srrip)
  {
    min_wayid = cache_replace_block_srrip(&(policy_data->srrip_policy_data));
  }
  else
  {
    ((policy_data->following == cache_policy_srripsage) ? (void) (0) : __assert_fail ("policy_data->following == cache_policy_srripsage", "policy/srripsage.c", 539, __PRETTY_FUNCTION__));


    for (block = ((policy_data)->free_head->head); block; block = block->prev)
    {
      if (block->way < min_wayid)
        min_wayid = block->way;
    }


    rrpv = cache_get_replacement_rrpv_srripsage(policy_data);


    ((rrpv >= 0 && rrpv <= ((policy_data)->max_rrpv)) ? (void) (0) : __assert_fail ("rrpv >= 0 && rrpv <= ((policy_data)->max_rrpv)", "policy/srripsage.c", 552, __PRETTY_FUNCTION__));

    if (min_wayid == ~(0))
    {


      if (((policy_data)->valid_head)[rrpv].head == ((void *)0))
      {
        ((((policy_data)->valid_tail)[rrpv].head == ((void *)0)) ? (void) (0) : __assert_fail ("((policy_data)->valid_tail)[rrpv].head == ((void *)0)", "policy/srripsage.c", 560, __PRETTY_FUNCTION__));

        do { int dif = 0; for (int i = rrpv - 1; i >= 0; i--) { ((i <= rrpv) ? (void) (0) : __assert_fail ("i <= rrpv",
 "policy/srripsage.c"
# 562 "policy/srripsage.c"
        ,
 563
# 562 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); if ((((policy_data)->valid_head))[i].head) { if (!dif) { dif = rrpv - i; } if (i == 0) { struct cache_block_t *node = (((policy_data)->valid_head))[i].head; for (; node; node = node->prev) { do { if ((node)->next == ((void *)0)) { (((policy_data)->valid_head)[i]).head = (node)->prev; if ((node)->prev) { (node)->prev->next = ((void *)0); } } if ((node)->prev == ((void *)0)) { (((policy_data)->valid_tail)[i]).head = (node)->next; if ((node)->next) { (node)->next->prev = ((void *)0); } } if ((node)->next != ((void *)0) && (node)->prev != ((void *)0)) { ((node)->next)->prev = (node)->prev; ((node)->prev)->next = (node)->next; } (node)->data = ((void *)0); (node)->next = ((void *)0); (node)->prev = ((void *)0); }while(0); node->data = &(((policy_data)->valid_head)[i + dif]); do { if ((((policy_data)->valid_head)[i + dif]).head == ((void *)0) && (((policy_data)->valid_tail)[i + dif]).head == ((void *)0)) { (((policy_data)->valid_head)[i + dif]).head = (node); (((policy_data)->valid_tail)[i + dif]).head = (node); (node)->data = (&((policy_data)->valid_head)[i + dif]); break; } (((node->next == ((void *)0)) && (node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(node->next == ((void *)0)) && (node)->prev == ((void *)0)",
 "policy/srripsage.c"
# 562 "policy/srripsage.c"
        ,
 563
# 562 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[i + dif]).head)->prev = (node); (node)->next = (((policy_data)->valid_tail)[i + dif]).head; (((policy_data)->valid_tail)[i + dif]).head = (node); (node)->data = (&((policy_data)->valid_head)[i + dif]); }while(0); } } else { struct cache_block_t *node = (((policy_data)->valid_tail))[i].head; if((((policy_data)->valid_head))[i + dif].head != ((void *)0) || (((policy_data)->valid_tail))[i + dif].head != ((void *)0)); { printf("%d %d %d %lx %lx\n", i, dif, rrpv, (((policy_data)->valid_head))[i + dif].head, (((policy_data)->valid_tail))[i + dif].head); exit(-1); } for (; node; node = node->next) { do { if ((node)->next == ((void *)0)) { ((((policy_data)->valid_head))[i]).head = (node)->prev; if ((node)->prev) { (node)->prev->next = ((void *)0); } } if ((node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[i]).head = (node)->next; if ((node)->next) { (node)->next->prev = ((void *)0); } } if ((node)->next != ((void *)0) && (node)->prev != ((void *)0)) { ((node)->next)->prev = (node)->prev; ((node)->prev)->next = (node)->next; } (node)->data = ((void *)0); (node)->next = ((void *)0); (node)->prev = ((void *)0); }while(0); node->data = &(((policy_data)->valid_head)[i + dif]); do { if (((((policy_data)->valid_head))[i + dif]).head == ((void *)0) && ((((policy_data)->valid_tail))[i + dif]).head == ((void *)0)) { ((((policy_data)->valid_head))[i + dif]).head = (node); ((((policy_data)->valid_tail))[i + dif]).head = (node); (node)->data = (&(((policy_data)->valid_head))[i + dif]); break; } (((node->next == ((void *)0)) && (node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(node->next == ((void *)0)) && (node)->prev == ((void *)0)",
 "policy/srripsage.c"
# 562 "policy/srripsage.c"
        ,
 563
# 562 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); (((((policy_data)->valid_head))[i + dif]).head)->next = (node); (node)->prev = ((((policy_data)->valid_head))[i + dif]).head; ((((policy_data)->valid_head))[i + dif]).head = (node); (node)->data = (&(((policy_data)->valid_head))[i + dif]); }while(0); } } } else { ((!((((policy_data)->valid_tail))[i].head)) ? (void) (0) : __assert_fail ("!((((policy_data)->valid_tail))[i].head)",
 "policy/srripsage.c"
# 562 "policy/srripsage.c"
        ,
 563
# 562 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); } } }while(0)
                                                         ;




      }

      for (block = ((policy_data)->valid_tail)[rrpv].head; block; block = block->next)
      {
        if (!block->busy && block->way < min_wayid)
          min_wayid = block->way;
      }
    }
  }


  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_srripsage(srripsage_data *policy_data,
  srripsage_gdata *global_data, int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk = ((void *)0);
  struct cache_block_t *next = ((void *)0);
  struct cache_block_t *prev = ((void *)0);

  int old_rrpv;
  int new_rrpv;

  if (policy_data->following == cache_policy_srrip)
  {

    cache_access_block_srrip(&(policy_data->srrip_policy_data), way, strm,
        info);


    if (info && info->fill == (1))
    {
      cache_update_hit_counter_srripsage(&(policy_data->srrip_policy_data),
          global_data, way, strm);
    }
  }
  else
  {
    ((policy_data->following == cache_policy_srripsage) ? (void) (0) : __assert_fail ("policy_data->following == cache_policy_srripsage", "policy/srripsage.c", 607, __PRETTY_FUNCTION__));

    blk = &(((policy_data)->blocks)[way]);
    prev = blk->prev;
    next = blk->next;


    ((blk->tag >= 0) ? (void) (0) : __assert_fail ("blk->tag >= 0", "policy/srripsage.c", 614, __PRETTY_FUNCTION__));
    ((blk->state != cache_block_invalid) ? (void) (0) : __assert_fail ("blk->state != cache_block_invalid", "policy/srripsage.c", 615, __PRETTY_FUNCTION__));


    old_rrpv = (((rrip_list *)(blk->data))->rrpv);
    new_rrpv = old_rrpv;


    if (info && info->fill == (1))
    {
      new_rrpv = cache_get_new_rrpv_srripsage(policy_data, global_data, way, old_rrpv);
    }
    else
    {
      new_rrpv = ((policy_data)->spill_rrpv);
    }


    if (new_rrpv != old_rrpv)
    {
      do { if ((blk)->next == ((void *)0)) { (((policy_data)->valid_head)[old_rrpv]).head = (blk)->prev; if ((blk)->prev) { (blk)->prev->next = ((void *)0); } } if ((blk)->prev == ((void *)0)) { (((policy_data)->valid_tail)[old_rrpv]).head = (blk)->next; if ((blk)->next) { (blk)->next->prev = ((void *)0); } } if ((blk)->next != ((void *)0) && (blk)->prev != ((void *)0)) { ((blk)->next)->prev = (blk)->prev; ((blk)->prev)->next = (blk)->next; } (blk)->data = ((void *)0); (blk)->next = ((void *)0); (blk)->prev = ((void *)0); }while(0)
                                                           ;
      do { if ((((policy_data)->valid_head)[new_rrpv]).head == ((void *)0) && (((policy_data)->valid_tail)[new_rrpv]).head == ((void *)0)) { (((policy_data)->valid_head)[new_rrpv]).head = (blk); (((policy_data)->valid_tail)[new_rrpv]).head = (blk); (blk)->data = (&((policy_data)->valid_head)[new_rrpv]); break; } (((blk->next == ((void *)0)) && (blk)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(blk->next == ((void *)0)) && (blk)->prev == ((void *)0)",
 "policy/srripsage.c"
# 636 "policy/srripsage.c"
      ,
 637
# 636 "policy/srripsage.c"
      , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[new_rrpv]).head)->prev = (blk); (blk)->next = (((policy_data)->valid_tail)[new_rrpv]).head; (((policy_data)->valid_tail)[new_rrpv]).head = (blk); (blk)->data = (&((policy_data)->valid_head)[new_rrpv]); }while(0)
                                                           ;
    }
    else
    {

      if (new_rrpv == !((policy_data)->max_rrpv))
      {
        do { if ((blk)->next == ((void *)0)) { (((policy_data)->valid_head)[old_rrpv]).head = (blk)->prev; if ((blk)->prev) { (blk)->prev->next = ((void *)0); } } if ((blk)->prev == ((void *)0)) { (((policy_data)->valid_tail)[old_rrpv]).head = (blk)->next; if ((blk)->next) { (blk)->next->prev = ((void *)0); } } if ((blk)->next != ((void *)0) && (blk)->prev != ((void *)0)) { ((blk)->next)->prev = (blk)->prev; ((blk)->prev)->next = (blk)->next; } (blk)->data = ((void *)0); (blk)->next = ((void *)0); (blk)->prev = ((void *)0); }while(0)
                                                             ;
        do { if ((((policy_data)->valid_head)[new_rrpv]).head == ((void *)0) && (((policy_data)->valid_tail)[new_rrpv]).head == ((void *)0)) { (((policy_data)->valid_head)[new_rrpv]).head = (blk); (((policy_data)->valid_tail)[new_rrpv]).head = (blk); (blk)->data = (&((policy_data)->valid_head)[new_rrpv]); break; } (((blk->next == ((void *)0)) && (blk)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(blk->next == ((void *)0)) && (blk)->prev == ((void *)0)",
 "policy/srripsage.c"
# 646 "policy/srripsage.c"
        ,
 647
# 646 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[new_rrpv]).head)->prev = (blk); (blk)->next = (((policy_data)->valid_tail)[new_rrpv]).head; (((policy_data)->valid_tail)[new_rrpv]).head = (blk); (blk)->data = (&((policy_data)->valid_head)[new_rrpv]); }while(0)
                                                             ;
      }
    }

    if (strm == (4))
    {
      if (info && blk->stream == info->stream)
      {
        blk->epoch = (blk->epoch < 2) ? blk->epoch + 1 : 2;
      }
      else
      {
        blk->epoch = 0;
      }
    }

    do { if ((blk)->stream == (2) && strm != (2)) { srripsage_blocks--; } if (strm == (2) && (blk)->stream != (2)) { srripsage_blocks++; } (blk)->stream = strm; }while(0);
    blk->dirty = (info && info->dirty) ? 1 : 0;
  }
}

int cache_get_fill_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info)
{
  int ret_rrpv;
# 701 "policy/srripsage.c"
  ret_rrpv = ((policy_data)->max_rrpv) - 1;

  return ret_rrpv;
}

int cache_get_replacement_rrpv_srripsage(srripsage_data *policy_data)
{
  return ((policy_data)->max_rrpv);
}

int cache_get_new_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, int way, int old_rrpv)
{

  unsigned int state;
# 766 "policy/srripsage.c"
  return 0;
}


void cache_set_block_srripsage(srripsage_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  if (policy_data->following == cache_policy_srrip)
  {
    cache_set_block_srrip(&(policy_data->srrip_policy_data), way, tag, state,
      stream, info);
  }
  else
  {
    block = &(((policy_data)->blocks))[way];


    ((block->tag == tag) ? (void) (0) : __assert_fail ("block->tag == tag", "policy/srripsage.c", 785, __PRETTY_FUNCTION__));



    ((block->state != cache_block_invalid) ? (void) (0) : __assert_fail ("block->state != cache_block_invalid", "policy/srripsage.c", 789, __PRETTY_FUNCTION__));

    if (state != cache_block_invalid)
    {

      do { (block)->tag = (tag); (block)->vtl_addr = (info->vtl_addr); (block)->state = (state); }while(0);
      do { if ((block)->stream == (2) && stream != (2)) { srripsage_blocks--; } if (stream == (2) && (block)->stream != (2)) { srripsage_blocks++; } (block)->stream = stream; }while(0);
      return;
    }


    do { (block)->tag = (tag); (block)->vtl_addr = (info->vtl_addr); (block)->state = (cache_block_invalid); }while(0);
    do { if ((block)->stream == (2) && (0) != (2)) { srripsage_blocks--; } if ((0) == (2) && (block)->stream != (2)) { srripsage_blocks++; } (block)->stream = (0); }while(0);
    block->epoch = 0;


    int old_rrpv = (((rrip_list *)(block->data))->rrpv);


    do { if ((block)->next == ((void *)0)) { (((policy_data)->valid_head)[old_rrpv]).head = (block)->prev; if ((block)->prev) { (block)->prev->next = ((void *)0); } } if ((block)->prev == ((void *)0)) { (((policy_data)->valid_tail)[old_rrpv]).head = (block)->next; if ((block)->next) { (block)->next->prev = ((void *)0); } } if ((block)->next != ((void *)0) && (block)->prev != ((void *)0)) { ((block)->next)->prev = (block)->prev; ((block)->prev)->next = (block)->next; } (block)->data = ((void *)0); (block)->next = ((void *)0); (block)->prev = ((void *)0); }while(0)
                                                         ;
    do { if ((((policy_data)->free_head->head)) == ((void *)0) && (((policy_data)->free_tail->head)) == ((void *)0)) { (((policy_data)->free_head->head)) = (block); (((policy_data)->free_tail->head)) = (block); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",
 "policy/srripsage.c"
# 810 "policy/srripsage.c"
    ,
 811
# 810 "policy/srripsage.c"
    , __PRETTY_FUNCTION__)); ((((policy_data)->free_tail->head)))->prev = (block); (block)->next = (((policy_data)->free_tail->head)); (((policy_data)->free_tail->head)) = (block); }while(0)
                                              ;
  }
}



struct cache_block_t cache_get_block_srripsage(srripsage_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  ((policy_data) ? (void) (0) : __assert_fail ("policy_data", "policy/srripsage.c", 820, __PRETTY_FUNCTION__));
  ((tag_ptr) ? (void) (0) : __assert_fail ("tag_ptr", "policy/srripsage.c", 821, __PRETTY_FUNCTION__));
  ((state_ptr) ? (void) (0) : __assert_fail ("state_ptr", "policy/srripsage.c", 822, __PRETTY_FUNCTION__));
  ((stream_ptr) ? (void) (0) : __assert_fail ("stream_ptr", "policy/srripsage.c", 823, __PRETTY_FUNCTION__));

  if (policy_data->following == cache_policy_srrip)
  {
    return cache_get_block_srrip(&(policy_data->srrip_policy_data), way, tag_ptr,
      state_ptr, stream_ptr);
  }
  else
  {
    if (tag_ptr) *(tag_ptr) = ((((policy_data)->blocks)[way]).tag);
    if (state_ptr) *(state_ptr) = ((((policy_data)->blocks)[way]).state);
    if (stream_ptr) *(stream_ptr) = ((((policy_data)->blocks)[way]).stream);

    return ((policy_data)->blocks)[way];
  }
}


int cache_count_block_srripsage(srripsage_data *policy_data, ub1 strm)
{
  int max_rrpv;
  int count;
  struct cache_block_t *head;
  struct cache_block_t *node;

  ((policy_data) ? (void) (0) : __assert_fail ("policy_data", "policy/srripsage.c", 848, __PRETTY_FUNCTION__));

  max_rrpv = policy_data->max_rrpv;
  node = ((void *)0);
  count = 0;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = ((policy_data)->valid_head)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      ((node->state != cache_block_invalid) ? (void) (0) : __assert_fail ("node->state != cache_block_invalid", "policy/srripsage.c", 860, __PRETTY_FUNCTION__));
      if (node->stream == strm)
        count++;
    }
  }

  return count;
}

# 1 "policy/srripsage.c"
# 1 "/home/sidrail/work/offline-cache//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "policy/srripsage.c"
# 20 "policy/srripsage.c"
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
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ , __leaf__)) ;




extern double atof (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





__extension__ extern long long int atoll (__const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





extern double strtod (__const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;





extern float strtof (__const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;

extern long double strtold (__const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;





extern long int strtol (__const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;

extern unsigned long int strtoul (__const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;

# 207 "/usr/include/stdlib.h" 3 4


__extension__
extern long long int strtoll (__const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;

__extension__
extern unsigned long long int strtoull (__const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;

# 378 "/usr/include/stdlib.h" 3 4


extern int rand (void) __attribute__ ((__nothrow__ , __leaf__));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));

# 469 "/usr/include/stdlib.h" 3 4


extern void *malloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) ;

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) ;










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






extern char *getenv (__const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern char *__secure_getenv (__const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 712 "/usr/include/stdlib.h" 3 4





extern int system (__const char *__command) ;

# 742 "/usr/include/stdlib.h" 3 4
typedef int (*__compar_fn_t) (__const void *, __const void *);
# 752 "/usr/include/stdlib.h" 3 4



extern void *bsearch (__const void *__key, __const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;



extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));
# 771 "/usr/include/stdlib.h" 3 4
extern int abs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;



__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;







extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;




__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;

# 857 "/usr/include/stdlib.h" 3 4



extern int mblen (__const char *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) ;


extern int mbtowc (wchar_t *__restrict __pwc,
     __const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) ;


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ , __leaf__)) ;



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   __const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));

extern size_t wcstombs (char *__restrict __s,
   __const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__));

# 964 "/usr/include/stdlib.h" 3 4

# 21 "policy/srripsage.c" 2
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



# 22 "policy/srripsage.c" 2

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














extern FILE *tmpfile (void) ;
# 210 "/usr/include/stdio.h" 3 4
extern char *tmpnam (char *__s) __attribute__ ((__nothrow__ , __leaf__)) ;

# 233 "/usr/include/stdio.h" 3 4





extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);

# 267 "/usr/include/stdio.h" 3 4






extern FILE *fopen (__const char *__restrict __filename,
      __const char *__restrict __modes) ;




extern FILE *freopen (__const char *__restrict __filename,
        __const char *__restrict __modes,
        FILE *__restrict __stream) ;
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
     __const char *__restrict __format, ...) ;




extern int scanf (__const char *__restrict __format, ...) ;

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));
# 449 "/usr/include/stdio.h" 3 4
extern int fscanf (FILE *__restrict __stream, __const char *__restrict __format, ...) __asm__ ("" "__isoc99_fscanf")

                               ;
extern int scanf (__const char *__restrict __format, ...) __asm__ ("" "__isoc99_scanf")
                              ;
extern int sscanf (__const char *__restrict __s, __const char *__restrict __format, ...) __asm__ ("" "__isoc99_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
# 469 "/usr/include/stdio.h" 3 4








extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) ;





extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (__const char *__restrict __s,
      __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format__ (__scanf__, 2, 0)));
# 500 "/usr/include/stdio.h" 3 4
extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc99_vfscanf")



     __attribute__ ((__format__ (__scanf__, 2, 0))) ;
extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc99_vscanf")

     __attribute__ ((__format__ (__scanf__, 1, 0))) ;
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
     ;






extern char *gets (char *__s) ;

# 681 "/usr/include/stdio.h" 3 4





extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream);






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream) ;




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s);

# 741 "/usr/include/stdio.h" 3 4





extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream) ;




extern void rewind (FILE *__stream);

# 789 "/usr/include/stdio.h" 3 4






extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos);




extern int fsetpos (FILE *__stream, __const fpos_t *__pos);
# 812 "/usr/include/stdio.h" 3 4

# 821 "/usr/include/stdio.h" 3 4


extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) ;

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) ;

# 838 "/usr/include/stdio.h" 3 4





extern void perror (__const char *__s);






# 1 "/usr/include/x86_64-linux-gnu/bits/sys_errlist.h" 1 3 4
# 851 "/usr/include/stdio.h" 2 3 4
# 940 "/usr/include/stdio.h" 3 4

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
# 24 "policy/srripsage.c" 2
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
# 25 "policy/srripsage.c" 2
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
# 646 "/usr/include/string.h" 3 4

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
# 26 "policy/srripsage.c" 2
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
  ub4 last_rrpv;
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


        cache_policy_gsdrrip,


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
  ub4 gsdrrip_streams;
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


  unsigned char demote;


  unsigned char fill_at_head;


  unsigned char fill_at_tail;


  unsigned char demote_at_head;


  unsigned char demote_at_tail;


  unsigned int head : 1;


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
  int last_rrpv;







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
# 56 "policy/srripsage.h"
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
  ub8 evictions;
  ub8 last_eviction;
  ub1 *rrpv_blocks;
  ub8 per_stream_fill[(10) + 1];
  ub1 hit_post_fill[(10) + 1];
  ub1 set_type;
  ub1 fill_at_lru[(10) + 1];
  ub1 dem_to_lru[(10) + 1];
  struct cache_block_t *blocks;
}srripsage_data;



typedef struct cache_policy_srripsage_gdata_t
{
  ub8 bm_ctr;
  ub8 bm_thr;
  ub4 active_stream;
  ub4 lru_sample_set;
  ub8 lru_sample_access[(10) + 1];
  ub8 lru_sample_hit[(10) + 1];
  ub8 lru_sample_dem[(10) + 1];
  ub4 mru_sample_set;
  ub8 mru_sample_access[(10) + 1];
  ub8 mru_sample_hit[(10) + 1];
  ub8 mru_sample_dem[(10) + 1];
  ub4 thr_sample_set;
  ub8 thr_sample_access[(10) + 1];
  ub8 thr_sample_hit[(10) + 1];
  ub8 thr_sample_dem[(10) + 1];
  ub8 per_stream_fill[(10) + 1];
  ub8 per_stream_hit[(10) + 1];
  ub8 per_stream_xevct[(10) + 1];
  ub8 per_stream_sevct[(10) + 1];
  ub8 per_stream_demote[4][(10) + 1];
  ub8 per_stream_rrpv_hit[4][(10) + 1];
  ub8 access_count;

  ub8 rcy_thr[(4)][(10) + 1];
  ub8 per_stream_reuse[(10) + 1];
  ub8 per_stream_reuse_blocks[(10) + 1];
  ub8 per_stream_max_reuse[(10) + 1];
  ub8 per_stream_cur_thr[(10) + 1];
  ub8 occ_thr[(4)][(10) + 1];
  ub8 per_stream_oreuse[(10) + 1];
  ub8 per_stream_oreuse_blocks[(10) + 1];
  ub8 per_stream_max_oreuse[(10) + 1];
  ub8 per_stream_occ_thr[(10) + 1];
  ub8 dem_thr[(4)][(10) + 1];
  ub8 per_stream_dreuse[(10) + 1];
  ub8 per_stream_dreuse_blocks[(10) + 1];
  ub8 per_stream_max_dreuse[(10) + 1];
  ub8 per_stream_dem_thr[(10) + 1];
  ub1 fill_at_head[(10) + 1];
  ub1 demote_at_head[(10) + 1];
  ub1 demote_on_hit[(10) + 1];
  ub8 *rrpv_blocks;
  ub8 fills_at_head[(10) + 1];
  ub8 fills_at_tail[(10) + 1];
  ub8 dems_at_head[(10) + 1];
  ub8 fail_demotion[(10) + 1];
  ub8 valid_demotion[(10) + 1];


  struct saturating_counter_t tex_e0_fill_ctr;
  struct saturating_counter_t tex_e0_hit_ctr;
  struct saturating_counter_t tex_e1_fill_ctr;
  struct saturating_counter_t tex_e1_hit_ctr;
  struct saturating_counter_t acc_all_ctr;

  struct saturating_counter_t fath_ctr[(10) + 1];
  struct saturating_counter_t fathm_ctr[(10) + 1];
  struct saturating_counter_t dem_ctr[(10) + 1];
  struct saturating_counter_t demm_ctr[(10) + 1];
  struct saturating_counter_t cb_ctr;
  struct saturating_counter_t bc_ctr;
  struct saturating_counter_t ct_ctr;
  struct saturating_counter_t bt_ctr;
  struct saturating_counter_t tb_ctr;
  struct saturating_counter_t zt_ctr;

  struct saturating_counter_t gfath_ctr;
  struct saturating_counter_t gfathm_ctr;
  struct saturating_counter_t gdem_ctr;

  struct saturating_counter_t fill_list_fctr[(10) + 1];
  struct saturating_counter_t fill_list_hctr[(10) + 1];
}srripsage_gdata;
# 180 "policy/srripsage.h"
void cache_init_srripsage(int set_indx, struct cache_params *params,
    srripsage_data *policy_data, srripsage_gdata *global_data);
# 202 "policy/srripsage.h"
void cache_free_srripsage(srripsage_data *policy_data);
# 227 "policy/srripsage.h"
struct cache_block_t * cache_find_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, long long tag, int strm, memory_trace *info);
# 254 "policy/srripsage.h"
void cache_fill_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, int way, long long tag,
    enum cache_block_state_t state, int strm, memory_trace *info);
# 279 "policy/srripsage.h"
int cache_replace_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info);
# 305 "policy/srripsage.h"
void cache_access_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, int way, int strm, memory_trace *info);
# 330 "policy/srripsage.h"
struct cache_block_t cache_get_block_srripsage(srripsage_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr);
# 357 "policy/srripsage.h"
void cache_set_block_srripsage(srripsage_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);
# 380 "policy/srripsage.h"
int cache_get_fill_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info);
# 402 "policy/srripsage.h"
int cache_get_replacement_rrpv_srripsage(srripsage_data *policy_data);
# 423 "policy/srripsage.h"
int cache_get_new_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info, int way,
    int old_rrpv, unsigned long long old_recency);
# 447 "policy/srripsage.h"
int cache_count_block_srripsage(srripsage_data *policy_data, ub1 strm);

ub1 cache_override_fill_at_head(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info);
# 27 "policy/srripsage.c" 2
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
# 28 "policy/srripsage.c" 2
# 91 "policy/srripsage.c"
void cache_fill_nru_block(rrip_list *head, struct cache_block_t *block)
{
  struct cache_block_t *current;
  ub1 all_nru;

  current = head->head;
  all_nru = (1);




  while (current)
  {
    if (current->nru)
    {
      all_nru = (0);
      break;
    }

    current = current->prev;
  }

  current = head->head;
  if (all_nru)
  {
    while (current)
    {
      current->nru = (1);
      current = current->prev;
    }
  }

  block->nru = (0);
}

int get_min_wayid_from_head(struct cache_block_t *head)
{
  struct cache_block_t *node;
  int min_wayid = 0xff;

  ((head) ? (void) (0) : __assert_fail ("head", "policy/srripsage.c", 131, __PRETTY_FUNCTION__));

  node = head;

  while (node)
  {
    if (node->way < min_wayid)
    {
      min_wayid = node->way;
    }

    node = node->prev;
  }

  return min_wayid;
}

struct cache_block_t* get_nru_from_head(struct cache_block_t *head)
{
  struct cache_block_t *node;
  struct cache_block_t *ret_node;
  int min_wayid;

  ((head) ? (void) (0) : __assert_fail ("head", "policy/srripsage.c", 154, __PRETTY_FUNCTION__));

  node = head;
  ret_node = ((void *)0);
  min_wayid = 0xff;

  while (node)
  {
    if (node->nru && node->way < min_wayid)
    {
      ret_node = node;
      min_wayid = node->way;
    }

    node = node->prev;
  }

  return ret_node;
}

int get_min_wayid_from_tail(struct cache_block_t *tail)
{
  struct cache_block_t *node;
  int min_wayid = 0xff;

  ((tail) ? (void) (0) : __assert_fail ("tail", "policy/srripsage.c", 179, __PRETTY_FUNCTION__));

  node = tail;

  while (node)
  {
    if (node->way < min_wayid)
    {
      min_wayid = node->way;
    }

    node = node->next;
  }

  return min_wayid;
}

static struct cache_block_t* cache_get_fill_list_victim(srripsage_data *policy_data,
    srripsage_gdata *global_data)
{
  struct cache_block_t *block;
  struct cache_block_t *lru_block;
  struct cache_block_t *victim;

  block = policy_data->valid_head[(2)].head;
  lru_block = block;
  victim = ((void *)0);


  while (block)
  {
    if (block->recency < lru_block->recency)
    {
      lru_block = block;
    }

    block = block->prev;
  }
# 226 "policy/srripsage.c"
  if (lru_block && ((((((((global_data))->fill_list_fctr[((lru_block))->stream]).val)) - 2 * (((((global_data))->fill_list_hctr[((lru_block))->stream]).val))) > 0) || (lru_block)->demote_at_tail))
  {
    victim = lru_block;
  }
  else
  {
    victim = policy_data->valid_tail[(2)].head;
  }






  return policy_data->valid_tail[(2)].head;
}
# 561 "policy/srripsage.c"
int srripsage_blocks = 0;

static ub1 get_set_type_srripsage(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x07;
  msb_bits = (indx >> 7) & 0x07;
  mid_bits = (indx >> 3) & 0x0f;

  if (lsb_bits == msb_bits && mid_bits == 0x00)
  {
    return (12);
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x00)
  {
    return (13);
  }

  if (lsb_bits == msb_bits && mid_bits == 0x01)
  {
    return (0);
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x01)
  {
    return (4);
  }

  if (lsb_bits == msb_bits && mid_bits == 0x02)
  {
    return (5);
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x02)
  {
    return (6);
  }

  if (lsb_bits == msb_bits && mid_bits == 0x03)
  {
    return (7);
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x03)
  {
    return (8);
  }

  if (lsb_bits == msb_bits && mid_bits == 0x04)
  {
    return (9);
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x04)
  {
    return (10);
  }

  if (lsb_bits == msb_bits && mid_bits == 0x05)
  {
    return (11);
  }

  return (3);
}
# 722 "policy/srripsage.c"
void cache_add_reuse_blocks(srripsage_gdata *global_data, ub1 op,
    ub1 stream)
{
  ((op == (1) || op == (0) || op == (2)) ? (void) (0) : __assert_fail ("op == (1) || op == (0) || op == (2)", "policy/srripsage.c", 725, __PRETTY_FUNCTION__));

  switch (op)
  {
    case (0):
      global_data->per_stream_reuse_blocks[stream]++;
      break;

    case (1):
      global_data->per_stream_oreuse_blocks[stream]++;
      ((global_data->fill_list_fctr[stream]).val = ((global_data->fill_list_fctr[stream]).val < (global_data->fill_list_fctr[stream]).max) ? (global_data->fill_list_fctr[stream]).val + 1 : (global_data->fill_list_fctr[stream]).val);
      break;

    case (2):
      global_data->per_stream_dreuse_blocks[stream]++;
      break;

    default:
      printf("Unknown op %s : %d\n", __FUNCTION__, 743);
      ((0) ? (void) (0) : __assert_fail ("0", "policy/srripsage.c", 744, __PRETTY_FUNCTION__));
  }
}


void cache_remove_reuse_blocks(srripsage_gdata *global_data, ub1 op,
    ub1 stream)
{
  ((op == (1) || op == (0) || op == (2)) ? (void) (0) : __assert_fail ("op == (1) || op == (0) || op == (2)", "policy/srripsage.c", 752, __PRETTY_FUNCTION__));

  switch (op)
  {
    case (0):
      if(global_data->per_stream_reuse_blocks[stream])
      {
        global_data->per_stream_reuse_blocks[stream]--;
      }
      break;

    case (1):
      if(global_data->per_stream_oreuse_blocks[stream])
      {
        global_data->per_stream_oreuse_blocks[stream]--;
      }
      break;

    case (2):
      if(global_data->per_stream_dreuse_blocks[stream])
      {
        global_data->per_stream_dreuse_blocks[stream]--;
      }
      break;

    default:
      printf("Unknown op %s : %d\n", __FUNCTION__, 778);
      ((0) ? (void) (0) : __assert_fail ("0", "policy/srripsage.c", 779, __PRETTY_FUNCTION__));
  }
}


static void cache_update_reuse(srripsage_gdata *global_data, ub1 op,
    ub1 stream)
{
  ((op == (1) || op == (0) || op == (2)) ? (void) (0) : __assert_fail ("op == (1) || op == (0) || op == (2)", "policy/srripsage.c", 787, __PRETTY_FUNCTION__));

  switch (op)
  {
    case (0):
      global_data->per_stream_reuse[stream]++;
      break;

    case (1):
      global_data->per_stream_oreuse[stream]++;

      ((global_data->fill_list_hctr[stream]).val = ((global_data->fill_list_hctr[stream]).val < (global_data->fill_list_hctr[stream]).max) ? (global_data->fill_list_hctr[stream]).val + 1 : (global_data->fill_list_hctr[stream]).val);
      break;

    case (2):
      global_data->per_stream_dreuse[stream]++;
      break;

    default:
      printf("Unknown op %s : %d\n", __FUNCTION__, 806);
      ((0) ? (void) (0) : __assert_fail ("0", "policy/srripsage.c", 807, __PRETTY_FUNCTION__));
  }
}

void cache_init_srripsage(int set_indx, struct cache_params *params,
    srripsage_data *policy_data, srripsage_gdata *global_data)
{

  ((params) ? (void) (0) : __assert_fail ("params", "policy/srripsage.c", 815, __PRETTY_FUNCTION__));
  ((policy_data) ? (void) (0) : __assert_fail ("policy_data", "policy/srripsage.c", 816, __PRETTY_FUNCTION__));






  ((policy_data)->valid_head) = ((rrip_list *)(__xcalloc(((params->max_rrpv) + 1), (sizeof(rrip_list)), "policy/srripsage.c" ":" "823")));
  ((policy_data)->valid_tail) = ((rrip_list *)(__xcalloc(((params->max_rrpv) + 1), (sizeof(rrip_list)), "policy/srripsage.c" ":" "824")));



  ((((policy_data)->valid_head)) ? (void) (0) : __assert_fail ("((policy_data)->valid_head)", "policy/srripsage.c", 828, __PRETTY_FUNCTION__));
  ((((policy_data)->valid_tail)) ? (void) (0) : __assert_fail ("((policy_data)->valid_tail)", "policy/srripsage.c", 829, __PRETTY_FUNCTION__));


  ((policy_data)->max_rrpv) = (params->max_rrpv);
  ((policy_data)->spill_rrpv) = params->spill_rrpv;

  ((params->spill_rrpv <= (params->max_rrpv)) ? (void) (0) : __assert_fail ("params->spill_rrpv <= (params->max_rrpv)", "policy/srripsage.c", 835, __PRETTY_FUNCTION__));


  for (int i = 0; i <= (params->max_rrpv); i++)
  {
    ((policy_data)->valid_head)[i].rrpv = i;
    ((policy_data)->valid_head)[i].head = ((void *)0);
    ((policy_data)->valid_tail)[i].head = ((void *)0);
  }


  ((policy_data)->blocks) =
    (struct cache_block_t *)(__xcalloc((params->ways), (sizeof (struct cache_block_t)), "policy/srripsage.c" ":" "847"));



  ((policy_data)->free_head) = ((list_head_t *)(__xcalloc((1), (sizeof(list_head_t)), "policy/srripsage.c" ":" "851")));
  ((((policy_data)->free_head)) ? (void) (0) : __assert_fail ("((policy_data)->free_head)", "policy/srripsage.c", 852, __PRETTY_FUNCTION__));

  ((policy_data)->free_tail) = ((list_head_t *)(__xcalloc((1), (sizeof(list_head_t)), "policy/srripsage.c" ":" "854")));
  ((((policy_data)->free_tail)) ? (void) (0) : __assert_fail ("((policy_data)->free_tail)", "policy/srripsage.c", 855, __PRETTY_FUNCTION__));




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

    global_data->bm_ctr = 0;
    global_data->bm_thr = params->threshold;
    global_data->access_count = 0;
    global_data->active_stream = (0);


    for (int strm = 0; strm <= (10); strm++)
    {
      global_data->rcy_thr[(0)][strm] = 1;
      global_data->rcy_thr[(3)][strm] = 1;
      global_data->occ_thr[(0)][strm] = 1;
      global_data->occ_thr[(3)][strm] = 1;
      global_data->per_stream_cur_thr[strm] = 1;
      global_data->per_stream_occ_thr[strm] = 1;
      global_data->fill_at_head[strm] = (0);
      global_data->demote_on_hit[strm] = (0);
    }


    global_data->rcy_thr[(0)][(2)] = global_data->per_stream_cur_thr[(2)];
    global_data->rcy_thr[(0)][(3)] = global_data->per_stream_cur_thr[(3)];
    global_data->rcy_thr[(0)][(4)] = global_data->per_stream_cur_thr[(4)];
    global_data->rcy_thr[(0)][(7)] = global_data->per_stream_cur_thr[(7)];
    global_data->rcy_thr[(0)][(10)] = global_data->per_stream_cur_thr[(10)];


    global_data->rcy_thr[(3)][(2)] = 1;
    global_data->rcy_thr[(3)][(3)] = 1;
    global_data->rcy_thr[(3)][(4)] = 1;
    global_data->rcy_thr[(3)][(7)] = 1;
    global_data->rcy_thr[(3)][(10)] = 1;


    global_data->occ_thr[(0)][(2)] = global_data->per_stream_occ_thr[(2)];
    global_data->occ_thr[(0)][(3)] = global_data->per_stream_occ_thr[(3)];
    global_data->occ_thr[(0)][(4)] = global_data->per_stream_occ_thr[(4)];
    global_data->occ_thr[(0)][(7)] = global_data->per_stream_occ_thr[(7)];
    global_data->occ_thr[(0)][(10)] = global_data->per_stream_occ_thr[(10)];


    global_data->occ_thr[(3)][(2)] = 1;
    global_data->occ_thr[(3)][(3)] = 1;
    global_data->occ_thr[(3)][(4)] = 1;
    global_data->occ_thr[(3)][(7)] = 1;
    global_data->occ_thr[(3)][(10)] = 1;

    memset(global_data->per_stream_fill, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_hit, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_sevct, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_xevct, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_rrpv_hit, 0, sizeof(ub8) * ((10) + 1) * 4);
    memset(global_data->per_stream_demote, 0, sizeof(ub8) * ((10) + 1) * 4);
    memset(global_data->fills_at_head, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->fills_at_tail, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->dems_at_head, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->fail_demotion, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->valid_demotion, 0, sizeof(ub8) * ((10) + 1));


    memset(global_data->per_stream_reuse_blocks, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_reuse, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_max_reuse, 0, sizeof(ub8) * ((10) + 1));


    memset(global_data->per_stream_oreuse_blocks, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_oreuse, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_max_oreuse, 0, sizeof(ub8) * ((10) + 1));


    memset(global_data->per_stream_dreuse_blocks, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_dreuse, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_max_dreuse, 0, sizeof(ub8) * ((10) + 1));


    global_data->rrpv_blocks = (__xcalloc((((policy_data)->max_rrpv) + 1), (sizeof(ub8)), "policy/srripsage.c" ":" "956"));
    ((global_data->rrpv_blocks) ? (void) (0) : __assert_fail ("global_data->rrpv_blocks", "policy/srripsage.c", 957, __PRETTY_FUNCTION__));

    memset(global_data->rrpv_blocks, 0, sizeof(ub8) * ((policy_data)->max_rrpv) + 1);


    for (ub1 i = 0; i <= (10); i++)
    {
      do { (global_data->fath_ctr[i]).val = 0; (global_data->fath_ctr[i]).width = (6); (global_data->fath_ctr[i]).min = (0x00); (global_data->fath_ctr[i]).max = (0x3f); }while(0);
      ((global_data->fath_ctr[i]).val = ((32)));

      do { (global_data->fathm_ctr[i]).val = 0; (global_data->fathm_ctr[i]).width = (6); (global_data->fathm_ctr[i]).min = (0x00); (global_data->fathm_ctr[i]).max = (0x3f); }while(0);
      ((global_data->fathm_ctr[i]).val = ((32)));

      do { (global_data->dem_ctr[i]).val = 0; (global_data->dem_ctr[i]).width = (6); (global_data->dem_ctr[i]).min = (0x00); (global_data->dem_ctr[i]).max = (0x3f); }while(0);
      ((global_data->dem_ctr[i]).val = ((32)));

      memset(&(global_data->lru_sample_access[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->lru_sample_hit[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->lru_sample_dem[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->mru_sample_access[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->mru_sample_hit[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->mru_sample_dem[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->thr_sample_access[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->thr_sample_hit[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->thr_sample_dem[i]), 0, sizeof(ub8) * ((10) + 1));
    }

    do { (global_data->gfath_ctr).val = 0; (global_data->gfath_ctr).width = (6); (global_data->gfath_ctr).min = (0x00); (global_data->gfath_ctr).max = (0x3f); }while(0);
    ((global_data->gfath_ctr).val = ((32)));

    do { (global_data->gdem_ctr).val = 0; (global_data->gdem_ctr).width = (6); (global_data->gdem_ctr).min = (0x00); (global_data->gdem_ctr).max = (0x3f); }while(0);
    ((global_data->gdem_ctr).val = ((32)));


    for (ub1 i = 0; i <= (10); i++)
    {
      do { (global_data->fill_list_fctr[i]).val = 0; (global_data->fill_list_fctr[i]).width = (6); (global_data->fill_list_fctr[i]).min = (0x00); (global_data->fill_list_fctr[i]).max = (0x3f); }while(0);
      do { (global_data->fill_list_hctr[i]).val = 0; (global_data->fill_list_hctr[i]).width = (6); (global_data->fill_list_hctr[i]).min = (0x00); (global_data->fill_list_hctr[i]).max = (0x3f); }while(0);
    }

    do { (global_data->gfathm_ctr).val = 0; (global_data->gfathm_ctr).width = (6); (global_data->gfathm_ctr).min = (0x00); (global_data->gfathm_ctr).max = (0x3f); }while(0);
    ((global_data->gfathm_ctr).val = ((32)));

    do { (global_data->cb_ctr).val = 0; (global_data->cb_ctr).width = (6); (global_data->cb_ctr).min = (0x00); (global_data->cb_ctr).max = (0x3f); }while(0);
    ((global_data->cb_ctr).val = 0);

    do { (global_data->bc_ctr).val = 0; (global_data->bc_ctr).width = (6); (global_data->bc_ctr).min = (0x00); (global_data->bc_ctr).max = (0x3f); }while(0);
    ((global_data->bc_ctr).val = 0);

    do { (global_data->ct_ctr).val = 0; (global_data->ct_ctr).width = (6); (global_data->ct_ctr).min = (0x00); (global_data->ct_ctr).max = (0x3f); }while(0);
    ((global_data->ct_ctr).val = 0);

    do { (global_data->bt_ctr).val = 0; (global_data->bt_ctr).width = (6); (global_data->bt_ctr).min = (0x00); (global_data->bt_ctr).max = (0x3f); }while(0);
    ((global_data->bt_ctr).val = 0);

    do { (global_data->tb_ctr).val = 0; (global_data->tb_ctr).width = (6); (global_data->tb_ctr).min = (0x00); (global_data->tb_ctr).max = (0x3f); }while(0);
    ((global_data->tb_ctr).val = 0);

    do { (global_data->zt_ctr).val = 0; (global_data->zt_ctr).width = (6); (global_data->zt_ctr).min = (0x00); (global_data->zt_ctr).max = (0x3f); }while(0);
    ((global_data->zt_ctr).val = 0);

    global_data->lru_sample_set = 0;
    global_data->mru_sample_set = 0;
  }


  ((policy_data)->rrpv_blocks) = (__xcalloc((((policy_data)->max_rrpv) + 1), (sizeof(ub1)), "policy/srripsage.c" ":" "1023"));
  ((((policy_data)->rrpv_blocks)) ? (void) (0) : __assert_fail ("((policy_data)->rrpv_blocks)", "policy/srripsage.c", 1024, __PRETTY_FUNCTION__));

  memset(((policy_data)->rrpv_blocks), 0, sizeof(ub1) * ((policy_data)->max_rrpv) + 1);

  ub1 set_type;


  set_type = get_set_type_srripsage(set_indx);

  if (set_type == (4) || set_type == (8) ||
      set_type == (6) || set_type == (10) ||
      set_type == (12) )
  {

    cache_init_srrip(params, &(policy_data->srrip_policy_data));

    policy_data->following = cache_policy_srrip;

    global_data->lru_sample_set +=1;
  }
  else
  {
    policy_data->following = cache_policy_srripsage;

    if (set_type == (5) || set_type == (9) ||
        set_type == (7) || set_type == (11) ||
        set_type == (13))
    {
      global_data->mru_sample_set +=1;
    }
    else
    {
      if (set_type == (0))
      {
        global_data->thr_sample_set += 1;
      }
    }
  }





  memset(policy_data->fill_at_lru, 0, sizeof(ub1) * ((10) + 1));
  memset(policy_data->dem_to_lru, 0, sizeof(ub1) * ((10) + 1));

  switch (set_type)
  {
    case (4):
      policy_data->set_type = (1);
      policy_data->fill_at_lru[(2)] = (1);
      break;

    case (5):
      policy_data->set_type = (2);
      policy_data->dem_to_lru[(2)] = (1);
      break;

    case (6):
      policy_data->set_type = (1);
      policy_data->fill_at_lru[(3)] = (1);
      break;

    case (7):
      policy_data->set_type = (2);
      policy_data->dem_to_lru[(3)] = (1);
      break;

    case (8):
      policy_data->set_type = (1);
      policy_data->fill_at_lru[(4)] = (1);
      break;

    case (9):
      policy_data->set_type = (2);
      policy_data->dem_to_lru[(4)] = (1);
      break;

    case (10):
      policy_data->set_type = (1);
      policy_data->fill_at_lru[(7)] = (1);
      break;

    case (11):
      policy_data->set_type = (2);
      policy_data->dem_to_lru[(7)] = (1);
      break;

    case (12):
      policy_data->set_type = (1);
      policy_data->fill_at_lru[(10)] = (1);
      break;

    case (13):
      policy_data->set_type = (2);
      policy_data->dem_to_lru[(10)] = (1);
      break;

    case (3):
      policy_data->set_type = (3);
      break;

    case (0):
      policy_data->set_type = (0);
      break;

    default:
      printf("Invalid set type %d - %s : %d\n", set_type, __FUNCTION__, 1131);
  }


  memset(policy_data->per_stream_fill, 0, sizeof(ub8) * ((10) + 1));


  memset(policy_data->hit_post_fill, 0, sizeof(ub1) * ((10) + 1));

  ((((policy_data)->max_rrpv) != 0) ? (void) (0) : __assert_fail ("((policy_data)->max_rrpv) != 0", "policy/srripsage.c", 1140, __PRETTY_FUNCTION__));


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

struct cache_block_t* cache_find_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, long long tag, int strm, memory_trace *info)
{
  int max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;
  enum cache_block_state_t state_ptr;

  int stream_ptr;
  int min_wayid;
  int i;

  long long tag_ptr;

  ub8 reuse;
  ub8 reuse_blocks;


  global_data->per_stream_fill[strm]++;
  policy_data->per_stream_fill[strm]++;

  if (policy_data->set_type == (1))
  {
    global_data->lru_sample_access[strm]++;
  }
  else
  {
    if (policy_data->set_type == (2))
    {
      global_data->mru_sample_access[strm]++;
    }
    else
    {
      if (policy_data->set_type == (0))
      {
        global_data->thr_sample_access[strm]++;
      }
    }
  }

  if (policy_data->following == cache_policy_srrip)
  {
    node = cache_find_block_srrip(&(policy_data->srrip_policy_data), tag);
    if (node)
    {
      cache_get_block_srrip(&(policy_data->srrip_policy_data), node->way,
          &tag_ptr, &state_ptr, &stream_ptr);


      cache_access_block_srrip(&(policy_data->srrip_policy_data), node->way, strm,
          info);
    }
    else
    {
      min_wayid = cache_replace_block_srrip(&(policy_data->srrip_policy_data));

      ((min_wayid != -1) ? (void) (0) : __assert_fail ("min_wayid != -1", "policy/srripsage.c", 1226, __PRETTY_FUNCTION__));

      cache_get_block_srrip(&(policy_data->srrip_policy_data), min_wayid,
          &tag_ptr, &state_ptr, &stream_ptr);

      if (state_ptr != cache_block_invalid)
      {
        cache_set_block_srrip(&(policy_data->srrip_policy_data), min_wayid,
            tag_ptr, cache_block_invalid, stream_ptr, info);
      }

      if (policy_data->blocks[min_wayid].demote)
      {
        cache_remove_reuse_blocks(global_data, (2), policy_data->blocks[min_wayid].stream);
      }


      cache_fill_block_srrip(&(policy_data->srrip_policy_data), min_wayid, tag,
          cache_block_exclusive, strm, info);
    }
  }

  {
    max_rrpv = policy_data->max_rrpv;
    node = ((void *)0);

    for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
    {
      head = ((policy_data)->valid_head)[rrpv].head;

      for (node = head; node; node = node->prev)
      {
        ((node->state != cache_block_invalid) ? (void) (0) : __assert_fail ("node->state != cache_block_invalid", "policy/srripsage.c", 1258, __PRETTY_FUNCTION__));

        if (node->tag == tag)
          goto end;
      }
    }
  }

end:

  if (++(global_data->access_count) == (5000))
  {

    printf("CB:%d BC:%d CT:%d BT: %d TB:%d ZT: %d\n", ((global_data->cb_ctr).val),
        ((global_data->cb_ctr).val), ((global_data->ct_ctr).val),
        ((global_data->bt_ctr).val), ((global_data->tb_ctr).val),
        ((global_data->zt_ctr).val));


    printf("LRUSS:%d MRUSS: %d THRSS: %d\n", global_data->lru_sample_set,
        global_data->mru_sample_set, global_data->thr_sample_set);

    printf("LACC-C:%d LACC-Z: %d LACC-T: %d LACC-B: %d LACC-P: %d\n", global_data->lru_sample_access[(2)],
        global_data->lru_sample_access[(3)], global_data->lru_sample_access[(4)],
        global_data->lru_sample_access[(7)], global_data->lru_sample_access[(10)]);

    printf("MACC-C:%ld MACC-Z: %ld MACC-T: %ld MACC-B: %ld MACC-P: %ld\n", global_data->mru_sample_access[(2)],
        global_data->mru_sample_access[(3)], global_data->mru_sample_access[(4)],
        global_data->mru_sample_access[(7)], global_data->mru_sample_access[(10)]);

    printf("LHIT-C:%d LHIT-Z: %d LHIT-T: %d LHIT-B: %d LHIT-P: %d\n", global_data->lru_sample_hit[(2)],
        global_data->lru_sample_hit[(3)], global_data->lru_sample_hit[(4)],
        global_data->lru_sample_hit[(7)], global_data->lru_sample_hit[(10)]);

    printf("MHIT-C:%d MHIT-Z: %d MHIT-T: %d MHIT-B: %d MHIT-P: %d\n", global_data->mru_sample_hit[(2)],
        global_data->mru_sample_hit[(3)], global_data->mru_sample_hit[(4)],
        global_data->mru_sample_hit[(7)], global_data->mru_sample_hit[(10)]);

    ub4 lru_hr[(10) + 1];
    ub4 mru_hr[(10) + 1];

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        reuse_blocks = global_data->lru_sample_access[i];
        reuse = global_data->lru_sample_hit[i];
        lru_hr[i] = 0;

        if (reuse_blocks)
        {
          lru_hr[i] = (reuse * 100) / reuse_blocks;
        }

        printf("LHR:%d:%ld ", i, lru_hr[i]);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        reuse_blocks = global_data->mru_sample_access[i];
        reuse = global_data->mru_sample_hit[i];
        mru_hr[i] = 0;

        if (reuse_blocks)
        {
          mru_hr[i] = (reuse * 100) / reuse_blocks;
        }

        printf("MHR:%d:%ld ", i, mru_hr[i]);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {

        printf("FATH:%d:%ld ", i, global_data->fill_at_head[i]);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {

        printf("DONH:%d:%ld ", i, global_data->demote_on_hit[i]);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {

        printf("DATH:%d:%ld ", i, global_data->demote_at_head[i]);
      }
    }

    printf("\n");

    ub8 demoted;

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        demoted = global_data->per_stream_dreuse_blocks[i];

        printf("DBLKS:%d:%ld ", i, demoted);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        reuse = global_data->per_stream_dreuse[i];

        printf("DUSE:%d:%ld ", i, reuse);
      }
    }

    printf("\n");

    printf("LDEM-C:%d LDEM-Z: %d LDEM-T: %d LDEM-B: %d LDEM-P: %d\n", global_data->lru_sample_dem[(2)],
        global_data->lru_sample_dem[(3)], global_data->lru_sample_dem[(4)],
        global_data->lru_sample_dem[(7)], global_data->lru_sample_dem[(10)]);

    printf("MDEM-C:%d MDEM-Z: %d MDEM-T: %d MDEM-B: %d MDEM-P: %d\n", global_data->mru_sample_dem[(2)],
        global_data->mru_sample_dem[(3)], global_data->mru_sample_dem[(4)],
        global_data->mru_sample_dem[(7)], global_data->mru_sample_dem[(10)]);

    printf("CF:%9ld ZF:%9ld TF:%9ld BF:%9ld PF:%9ld\n", global_data->per_stream_fill[(2)],
        global_data->per_stream_fill[(3)], global_data->per_stream_fill[(4)],
        global_data->per_stream_fill[(7)], global_data->per_stream_fill[(10)]);

    printf("CH:%8ld ZH:%8ld TH:%8ld BH:%8ld\n", global_data->per_stream_hit[(2)],
        global_data->per_stream_hit[(3)], global_data->per_stream_hit[(4)],
        global_data->per_stream_hit[(7)]);

    printf("CSE:%8ld ZSE:%8ld TSE:%8ld BSE:%8ld\n", global_data->per_stream_sevct[(2)],
        global_data->per_stream_sevct[(3)], global_data->per_stream_sevct[(4)],
        global_data->per_stream_sevct[(7)]);

    printf("CXE:%8ld ZXE:%8ld TXE:%8ld BXE:%8ld\n", global_data->per_stream_xevct[(2)],
        global_data->per_stream_xevct[(3)], global_data->per_stream_xevct[(4)],
        global_data->per_stream_xevct[(7)]);

    printf("CD0:%8ld ZD0:%8ld TD0:%8ld BD0:%8ld\n", global_data->per_stream_demote[0][(2)],
        global_data->per_stream_demote[0][(3)], global_data->per_stream_demote[0][(4)],
        global_data->per_stream_demote[0][(7)]);

    printf("CD1:%8ld ZD1:%8ld TD1:%8ld BD1:%8ld\n", global_data->per_stream_demote[1][(2)],
        global_data->per_stream_demote[1][(3)], global_data->per_stream_demote[1][(4)],
        global_data->per_stream_demote[1][(7)]);

    printf("CD2:%8ld ZD2:%8ld TD2:%8ld BD2:%8ld\n", global_data->per_stream_demote[2][(2)],
        global_data->per_stream_demote[2][(3)], global_data->per_stream_demote[2][(4)],
        global_data->per_stream_demote[2][(7)]);

    printf("CH0:%8ld ZH0:%8ld TH0:%8ld BH0:%8ld\n", global_data->per_stream_rrpv_hit[0][(2)],
        global_data->per_stream_rrpv_hit[0][(3)], global_data->per_stream_rrpv_hit[0][(4)],
        global_data->per_stream_rrpv_hit[0][(7)]);

    printf("CH1:%8ld ZH1:%8ld TH1:%8ld BH1:%8ld\n", global_data->per_stream_rrpv_hit[1][(2)],
        global_data->per_stream_rrpv_hit[1][(3)], global_data->per_stream_rrpv_hit[1][(4)],
        global_data->per_stream_rrpv_hit[1][(7)]);

    printf("CH2:%8ld ZH2:%8ld TH2:%8ld BH2:%8ld\n", global_data->per_stream_rrpv_hit[2][(2)],
        global_data->per_stream_rrpv_hit[2][(3)], global_data->per_stream_rrpv_hit[2][(4)],
        global_data->per_stream_rrpv_hit[2][(7)]);

    printf("CH3:%8ld ZH3:%8ld TH3:%8ld BH3:%8ld\n", global_data->per_stream_rrpv_hit[3][(2)],
        global_data->per_stream_rrpv_hit[3][(3)], global_data->per_stream_rrpv_hit[3][(4)],
        global_data->per_stream_rrpv_hit[3][(7)]);

    printf("R0:%9ld R1:%9ld R2:%8ld R3:%9ld\n", global_data->rrpv_blocks[0],
        global_data->rrpv_blocks[1], global_data->rrpv_blocks[2],
        global_data->rrpv_blocks[3]);

    printf("FHC:%8ld FHZ:%8ld FHT:%8ld FHB:%8ld FHP:%8ld\n", global_data->fills_at_head[(2)],
        global_data->fills_at_head[(3)], global_data->fills_at_head[(4)],
        global_data->fills_at_head[(7)], global_data->fills_at_head[(10)]);

    printf("FTC:%8ld FTZ:%8ld FTT:%8ld FTB:%8ld FTP:%8ld\n", global_data->fills_at_tail[(2)],
        global_data->fills_at_tail[(3)], global_data->fills_at_tail[(4)],
        global_data->fills_at_tail[(7)], global_data->fills_at_tail[(10)]);

    printf("DMC:%8ld DMZ:%8ld DMT:%8ld DMB:%8ld DMP:%8ld\n", global_data->dems_at_head[(2)],
        global_data->dems_at_head[(3)], global_data->dems_at_head[(4)],
        global_data->dems_at_head[(7)], global_data->dems_at_head[(10)]);

    printf("FDMC:%8ld FDMZ:%8ld FDMT:%8ld FDMB:%8ld FDMP:%8ld\n", global_data->fail_demotion[(2)],
        global_data->fail_demotion[(3)], global_data->fail_demotion[(4)],
        global_data->fail_demotion[(7)], global_data->fail_demotion[(10)]);

    printf("VDMC:%8ld VDMZ:%8ld VDMT:%8ld VDMB:%8ld VDMP:%8ld\n", global_data->valid_demotion[(2)],
        global_data->valid_demotion[(3)], global_data->valid_demotion[(4)],
        global_data->valid_demotion[(7)], global_data->valid_demotion[(10)]);

    printf("DCTRC:%6d DCTRZ:%6d DCTRT:%6d DCTRB:%6d DCTRP:%6d\n",
        ((global_data->dem_ctr[(2)]).val),
        ((global_data->dem_ctr[(3)]).val),
        ((global_data->dem_ctr[(4)]).val),
        ((global_data->dem_ctr[(7)]).val),
        ((global_data->dem_ctr[(10)]).val));

    printf("FCTRC:%6d FCTRZ:%6d FCTRT:%6d FCTRB:%6d FCTRP:%6d\n",
        ((global_data->fath_ctr[(2)]).val),
        ((global_data->fath_ctr[(3)]).val),
        ((global_data->fath_ctr[(4)]).val),
        ((global_data->fath_ctr[(7)]).val),
        ((global_data->fath_ctr[(10)]).val));

    printf("FMCTRC:%5d FMCTRZ:%5d FMCTRT:%5d FMCTRB:%5d FMCTRP:%5d\n",
        ((global_data->fathm_ctr[(2)]).val),
        ((global_data->fathm_ctr[(3)]).val),
        ((global_data->fathm_ctr[(4)]).val),
        ((global_data->fathm_ctr[(7)]).val),
        ((global_data->fathm_ctr[(10)]).val));

    printf("GFCTR:%6d GDCTR:%6d GFMCTR:%5d\n", ((global_data->gfath_ctr).val),
        ((global_data->gdem_ctr).val), ((global_data->gfathm_ctr).val));

    ub8 cur_thr;

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        cur_thr = global_data->per_stream_cur_thr[i];

        printf("DTH%d:%7ld ", i, cur_thr);
      }
    }

    printf("DST%d ", global_data->active_stream);

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        cur_thr = global_data->rcy_thr[(3)][i];

        printf("TH%d:%8ld ", i, cur_thr);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        cur_thr = global_data->occ_thr[(3)][i];

        printf("OTH%d:%7ld ", i, cur_thr);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        reuse_blocks = global_data->per_stream_reuse_blocks[i];

        printf("SB%d:%8ld ", i, reuse_blocks);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        reuse = global_data->per_stream_reuse[i];

        printf("SU%d:%8ld ", i, reuse);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {
        reuse = global_data->per_stream_reuse[i];
        reuse_blocks = global_data->per_stream_reuse_blocks[i];



          if (reuse_blocks && global_data->active_stream == i)
          {
            printf("S%d:%9ld ", i, reuse);



              if (global_data->per_stream_max_reuse[i] < reuse)
              {



                global_data->per_stream_max_reuse[i] = reuse;



                {
                  global_data->rcy_thr[(3)][i] = global_data->per_stream_cur_thr[i];
                }
              }
          }
          else
          {
            printf("S%d:%9ld ", i, 0);
          }
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {

        reuse = global_data->per_stream_oreuse[i];
        reuse_blocks = global_data->per_stream_oreuse_blocks[i];

        if (reuse_blocks)
        {
          printf("O%d:%9ld ", i, reuse);



            if (global_data->per_stream_max_oreuse[i] < reuse)
            {



              global_data->per_stream_max_oreuse[i] = reuse;
              global_data->occ_thr[(3)][i] = global_data->per_stream_cur_thr[i];
            }
        }
        else
        {
          printf("O%d:%9ld ", i, 0);
        }
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= (10); i++)
    {
      if (i == (2) || i == (4) || i == (3) || i == (7) || i == (10))
      {

        reuse = global_data->per_stream_dreuse[i];
        reuse_blocks = global_data->per_stream_dreuse_blocks[i];

        if (reuse_blocks)
        {
          printf("D%d:%9ld ", i, reuse / reuse_blocks);



            if (global_data->per_stream_max_dreuse[i] < reuse)
            {



              global_data->per_stream_max_dreuse[i] = reuse;
              global_data->dem_thr[(3)][i] = global_data->per_stream_cur_thr[i];
            }
        }
        else
        {
          printf("D%d:%9ld:0", i, reuse);
        }
      }
    }

    printf("\n");
# 1689 "policy/srripsage.c"
    global_data->access_count = 0;

    memset(global_data->per_stream_reuse_blocks, 0, sizeof(ub8) * ((10) + 1));
    memset(global_data->per_stream_reuse, 0, sizeof(ub8) * ((10) + 1));





    for (ub1 i = 0; i <= (10); i++)
    {

      if (((global_data->fill_list_hctr[i]).val == (global_data->fill_list_hctr[i]).max))
      {
        ((global_data->fill_list_hctr[i]).val = ((global_data->fill_list_hctr[i]).val > 1) ? ((global_data->fill_list_hctr[i]).val / 2) : (global_data->fill_list_hctr[i]).val);
      }

      if (((global_data->fill_list_fctr[i]).val == (global_data->fill_list_fctr[i]).max))
      {
        ((global_data->fill_list_fctr[i]).val = ((global_data->fill_list_fctr[i]).val > 1) ? ((global_data->fill_list_fctr[i]).val / 2) : (global_data->fill_list_fctr[i]).val);
      }
    }
# 1726 "policy/srripsage.c"
    for (i = 0; i <= (10); i++)
    {
      global_data->per_stream_dreuse_blocks[i] >>= 1;
      global_data->per_stream_dreuse[i] >>= 1;
      global_data->per_stream_max_dreuse[i] >>= 1;
    }

    if (global_data->per_stream_cur_thr[(2)] == (1) &&
        global_data->per_stream_cur_thr[(3)] == (1) &&
        global_data->per_stream_cur_thr[(4)] == (1) &&
        global_data->per_stream_cur_thr[(7)] == (1) &&
        global_data->per_stream_cur_thr[(10)] == (1))
    {
      global_data->per_stream_cur_thr[(2)] <<= 1;
      global_data->active_stream = (2);
    }
    else
    {
      if (global_data->per_stream_cur_thr[(2)] != 1 &&
          global_data->per_stream_cur_thr[(2)] != (16))
      {

        ((global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(7)] == 1 && global_data->per_stream_cur_thr[(10)] == 1) ? (void) (0) : __assert_fail ("global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(7)] == 1 && global_data->per_stream_cur_thr[(10)] == 1",


 "policy/srripsage.c"
# 1748 "policy/srripsage.c"
        ,


 1751
# 1748 "policy/srripsage.c"
        , __PRETTY_FUNCTION__))


                                                     ;

        global_data->per_stream_cur_thr[(2)] <<= 1;
        global_data->active_stream = (2);
      }
      else
      {
        if (global_data->per_stream_cur_thr[(3)] != 1 &&
            global_data->per_stream_cur_thr[(3)] != (16))
        {

          ((global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(7)] == 1 && global_data->per_stream_cur_thr[(10)] == 1) ? (void) (0) : __assert_fail ("global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(7)] == 1 && global_data->per_stream_cur_thr[(10)] == 1",


 "policy/srripsage.c"
# 1762 "policy/srripsage.c"
          ,


 1765
# 1762 "policy/srripsage.c"
          , __PRETTY_FUNCTION__))


                                                       ;

          global_data->per_stream_cur_thr[(3)] <<= 1;
          global_data->active_stream = (3);
        }
        else
        {
          if (global_data->per_stream_cur_thr[(4)] != 1 &&
              global_data->per_stream_cur_thr[(4)] != (16))
          {

            ((global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(7)] == 1 && global_data->per_stream_cur_thr[(10)] == 1) ? (void) (0) : __assert_fail ("global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(7)] == 1 && global_data->per_stream_cur_thr[(10)] == 1",


 "policy/srripsage.c"
# 1776 "policy/srripsage.c"
            ,


 1779
# 1776 "policy/srripsage.c"
            , __PRETTY_FUNCTION__))


                                                         ;

            global_data->per_stream_cur_thr[(4)] <<= 1;
            global_data->active_stream = (4);
          }
          else
          {
            if (global_data->per_stream_cur_thr[(7)] != 1 &&
                global_data->per_stream_cur_thr[(7)] != (16))
            {

              ((global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(10)] == 1) ? (void) (0) : __assert_fail ("global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(10)] == 1",


 "policy/srripsage.c"
# 1790 "policy/srripsage.c"
              ,


 1793
# 1790 "policy/srripsage.c"
              , __PRETTY_FUNCTION__))


                                                           ;

              global_data->per_stream_cur_thr[(7)] <<= 1;
              global_data->active_stream = (7);
            }
            else
            {
              if (global_data->per_stream_cur_thr[(10)] != 1 &&
                  global_data->per_stream_cur_thr[(10)] != (16))
              {

                ((global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(7)] == 1) ? (void) (0) : __assert_fail ("global_data->per_stream_cur_thr[(2)] == 1 && global_data->per_stream_cur_thr[(3)] == 1 && global_data->per_stream_cur_thr[(4)] == 1 && global_data->per_stream_cur_thr[(7)] == 1",


 "policy/srripsage.c"
# 1804 "policy/srripsage.c"
                ,


 1807
# 1804 "policy/srripsage.c"
                , __PRETTY_FUNCTION__))


                                                             ;

                global_data->per_stream_cur_thr[(10)] <<= 1;
                global_data->active_stream = (10);
              }
              else
              {

                if (global_data->per_stream_cur_thr[(2)] == (16))
                {
                  global_data->per_stream_cur_thr[(2)] = 1;
                  global_data->per_stream_cur_thr[(3)] <<= 1;

                  global_data->active_stream = (3);
                  global_data->per_stream_max_reuse[(3)] = 0;
                }

                if (global_data->per_stream_cur_thr[(3)] == (16))
                {
                  global_data->per_stream_cur_thr[(3)] = 1;
                  global_data->per_stream_cur_thr[(4)] <<= 1;

                  global_data->active_stream = (4);
                  global_data->per_stream_max_reuse[(4)] = 0;
                }

                if (global_data->per_stream_cur_thr[(4)] == (16))
                {
                  global_data->per_stream_cur_thr[(4)] = 1;
                  global_data->per_stream_cur_thr[(7)] <<= 1;

                  global_data->active_stream = (7);
                  global_data->per_stream_max_reuse[(7)] = 0;
                }

                if (global_data->per_stream_cur_thr[(7)] == (16))
                {
                  global_data->per_stream_cur_thr[(7)] = 1;
                  global_data->per_stream_cur_thr[(10)] <<= 1;

                  global_data->active_stream = (10);
                  global_data->per_stream_max_reuse[(10)] = 0;
                }

                if (global_data->per_stream_cur_thr[(10)] == (16))
                {
                  global_data->per_stream_cur_thr[(10)] = 1;
                }
              }
            }
          }
        }
      }
    }


    global_data->rcy_thr[(0)][(2)] = global_data->per_stream_cur_thr[(2)];
    global_data->rcy_thr[(0)][(3)] = global_data->per_stream_cur_thr[(3)];
    global_data->rcy_thr[(0)][(4)] = global_data->per_stream_cur_thr[(4)];
    global_data->rcy_thr[(0)][(7)] = global_data->per_stream_cur_thr[(7)];
    global_data->rcy_thr[(0)][(10)] = global_data->per_stream_cur_thr[(10)];

    if (global_data->active_stream == (2))
    {
      global_data->rcy_thr[(0)][(2)] = global_data->per_stream_cur_thr[(2)];
    }
    else
    {
      global_data->rcy_thr[(0)][(2)] = global_data->rcy_thr[(3)][(2)];
    }


    if (global_data->active_stream == (3))
    {
      global_data->rcy_thr[(0)][(3)] = global_data->per_stream_cur_thr[(3)];
    }
    else
    {
      global_data->rcy_thr[(0)][(3)] = global_data->rcy_thr[(3)][(3)];
    }

    if (global_data->active_stream == (4))
    {
      global_data->rcy_thr[(0)][(4)] = global_data->per_stream_cur_thr[(4)];
    }
    else
    {
      global_data->rcy_thr[(0)][(4)] = global_data->rcy_thr[(3)][(4)];
    }

    if (global_data->active_stream == (7))
    {
      global_data->rcy_thr[(0)][(7)] = global_data->per_stream_cur_thr[(7)];
    }
    else
    {
      global_data->rcy_thr[(0)][(7)] = global_data->rcy_thr[(3)][(7)];
    }

    if (global_data->active_stream == (10))
    {
      global_data->rcy_thr[(0)][(10)] = global_data->per_stream_cur_thr[(10)];
    }
    else
    {
      global_data->rcy_thr[(0)][(10)] = global_data->rcy_thr[(3)][(10)];
    }
# 1924 "policy/srripsage.c"
    global_data->rcy_thr[(3)][(4)] = 1;
# 1938 "policy/srripsage.c"
    ub1 pstreams[(10) + 1];

    memset(pstreams, 0, sizeof(ub1) * ((10) + 1));
# 1961 "policy/srripsage.c"
    for (ub1 i = (0); i <= (10); i++)
    {
      if (global_data->per_stream_fill[i])
      {
        global_data->fill_at_head[i] = (0);



        {
          if (((global_data->fathm_ctr[i]).val) < (32))
          {
              global_data->fill_at_head[i] = (1);
          }
        }
# 1984 "policy/srripsage.c"
      }

      if (((global_data->dem_ctr[i]).val) < (32))
      {
        global_data->demote_at_head[i] = (1);
      }
      else
      {
        global_data->demote_at_head[i] = (0);
      }



      if (((global_data->fathm_ctr[i]).val) > ((32) + (8))&&
          ((global_data->fath_ctr[i]).val) > ((32) + (8)))
      {
        switch (i)
        {
          case (2):
            if (((global_data->ct_ctr).val) ||
                ((global_data->cb_ctr).val))
            {
              global_data->demote_on_hit[i] = (1);
            }
            break;

          case (7):
            if (((global_data->bt_ctr).val) ||
                ((global_data->bc_ctr).val))
            {
              global_data->demote_on_hit[i] = (1);
            }

            break;

          case (3):
            if (((global_data->zt_ctr).val))
            {
              global_data->demote_on_hit[i] = (1);
            }
            break;

          case (4):
            if (((global_data->tb_ctr).val))
            {
              global_data->demote_on_hit[i] = (1);
            }

          default:
            global_data->demote_on_hit[i] = (1);
        }
      }
      else
      {
        global_data->demote_on_hit[i] = (0);
      }

      if (((global_data->dem_ctr[i]).val) > (32))
      {
        global_data->demote_on_hit[i] = (1);
      }
      else
      {
        global_data->demote_on_hit[i] = (0);
      }
# 2099 "policy/srripsage.c"
      }
# 2160 "policy/srripsage.c"
    global_data->fill_at_head[(10)] = (0);
# 2243 "policy/srripsage.c"
    for (ub1 i = 0; i <= (10); i++)
    {
      memset(&(global_data->lru_sample_access[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->lru_sample_hit[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->lru_sample_dem[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->mru_sample_access[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->mru_sample_hit[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->mru_sample_dem[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->thr_sample_access[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->thr_sample_hit[i]), 0, sizeof(ub8) * ((10) + 1));
      memset(&(global_data->thr_sample_dem[i]), 0, sizeof(ub8) * ((10) + 1));
    }

    ((global_data->cb_ctr).val = 0);
    ((global_data->bc_ctr).val = 0);
    ((global_data->ct_ctr).val = 0);
    ((global_data->bt_ctr).val = 0);
    ((global_data->tb_ctr).val = 0);
    ((global_data->zt_ctr).val = 0);

    for (ub1 i = 0; i <= (10); i++)
    {
      ((global_data->fath_ctr[i]).val = ((32)));



    }
  }

  if (!node)
  {
    if (policy_data->set_type == (1))
    {

      if (policy_data->fill_at_lru[info->stream])
      {
        ((global_data->fathm_ctr[info->stream]).val = ((global_data->fathm_ctr[info->stream]).val < (global_data->fathm_ctr[info->stream]).max) ? (global_data->fathm_ctr[info->stream]).val + 1 : (global_data->fathm_ctr[info->stream]).val);
      }

      ((global_data->gfathm_ctr).val = ((global_data->gfathm_ctr).val < (global_data->gfathm_ctr).max) ? (global_data->gfathm_ctr).val + 1 : (global_data->gfathm_ctr).val);
    }
    else
    {
      if (policy_data->set_type == (2))
      {

        if (policy_data->dem_to_lru[info->stream])
        {
          ((global_data->fathm_ctr[info->stream]).val = ((global_data->fathm_ctr[info->stream]).val > (global_data->fathm_ctr[info->stream]).min) ? (global_data->fathm_ctr[info->stream]).val - 1 : (global_data->fathm_ctr[info->stream]).val);
        }

        ((global_data->gfathm_ctr).val = ((global_data->gfathm_ctr).val > (global_data->gfathm_ctr).min) ? (global_data->gfathm_ctr).val - 1 : (global_data->gfathm_ctr).val);
      }
    }
  }

  return node;
}

static ub1 is_unique_fath_stream(srripsage_gdata *global_data, ub1 stream)
{
  ub1 uniq = (1);

  if (stream == (4) || stream == (10))
  {
    for (ub1 i = (0); i <= (10); i++)
    {
      if ((i == (2) || i == (3) || i == (4) || i == (7) || i == (10)) && i != stream)
      {
        if (global_data->fill_at_head[i])
        {
          uniq = (0);
        }
      }
    }
  }

  return uniq;
}

static void update_bm_ctr(srripsage_gdata *global_data)
{
  if (++(global_data->bm_ctr) >= global_data->bm_thr)
  {
    global_data->bm_ctr = 0;
  }
}

static void fill_in_followers(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info, int rrpv,
    struct cache_block_t *block)
{
# 2349 "policy/srripsage.c"
  if (global_data->fill_at_head[info->stream] == (1))
  {
    do { if ((((policy_data)->valid_head)[(1)]).head == ((void *)0) && (((policy_data)->valid_tail)[(1)]).head == ((void *)0)) { (((policy_data)->valid_head)[(1)]).head = (block); (((policy_data)->valid_tail)[(1)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(1)]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2351 "policy/srripsage.c"
    ,

 2353
# 2351 "policy/srripsage.c"
    , __PRETTY_FUNCTION__)); ((((policy_data)->valid_head)[(1)]).head)->next = (block); (block)->prev = (((policy_data)->valid_head)[(1)]).head; (((policy_data)->valid_head)[(1)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(1)]); }while(0)

                                                                   ;

    global_data->fills_at_head[info->stream] += 1;
    block->fill_at_head = (1);
  }
  else
  {
    if (global_data->bm_ctr == 0)
    {
      do { if ((((policy_data)->valid_head)[(1)]).head == ((void *)0) && (((policy_data)->valid_tail)[(1)]).head == ((void *)0)) { (((policy_data)->valid_head)[(1)]).head = (block); (((policy_data)->valid_tail)[(1)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(1)]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2362 "policy/srripsage.c"
      ,

 2364
# 2362 "policy/srripsage.c"
      , __PRETTY_FUNCTION__)); ((((policy_data)->valid_head)[(1)]).head)->next = (block); (block)->prev = (((policy_data)->valid_head)[(1)]).head; (((policy_data)->valid_head)[(1)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(1)]); }while(0)

                                                                     ;

      global_data->fills_at_head[info->stream] += 1;
      block->fill_at_head = (1);
    }
    else
    {
      do { if ((((policy_data)->valid_head)[(2)]).head == ((void *)0) && (((policy_data)->valid_tail)[(2)]).head == ((void *)0)) { (((policy_data)->valid_head)[(2)]).head = (block); (((policy_data)->valid_tail)[(2)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(2)]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2371 "policy/srripsage.c"
      ,

 2373
# 2371 "policy/srripsage.c"
      , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[(2)]).head)->prev = (block); (block)->next = (((policy_data)->valid_tail)[(2)]).head; (((policy_data)->valid_tail)[(2)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(2)]); }while(0)

                                                                     ;

      global_data->fills_at_tail[info->stream] += 1;
      block->fill_at_tail = (1);
    }
  }


}

void cache_fill_block_srripsage(srripsage_data *policy_data,
  srripsage_gdata *global_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;
  struct cache_block_t *rrpv_block;
  int rrpv;
  int min_wayid;
  long long tag_ptr;
  int stream_ptr;
  enum cache_block_state_t state_ptr;


  ((tag >= 0) ? (void) (0) : __assert_fail ("tag >= 0", "policy/srripsage.c", 2396, __PRETTY_FUNCTION__));
  ((state != cache_block_invalid) ? (void) (0) : __assert_fail ("state != cache_block_invalid", "policy/srripsage.c", 2397, __PRETTY_FUNCTION__));


  block = &(((policy_data)->blocks)[way]);

  ((block->stream == 0) ? (void) (0) : __assert_fail ("block->stream == 0", "policy/srripsage.c", 2402, __PRETTY_FUNCTION__));

  ((policy_data->following == cache_policy_srripsage || policy_data->following == cache_policy_srrip) ? (void) (0) : __assert_fail ("policy_data->following == cache_policy_srripsage || policy_data->following == cache_policy_srrip",
 "policy/srripsage.c"
# 2404 "policy/srripsage.c"
  ,
 2405
# 2404 "policy/srripsage.c"
  , __PRETTY_FUNCTION__))
                                                 ;


  rrpv = cache_get_fill_rrpv_srripsage(policy_data, global_data, info);

  ((rrpv <= ((policy_data)->max_rrpv)) ? (void) (0) : __assert_fail ("rrpv <= ((policy_data)->max_rrpv)", "policy/srripsage.c", 2410, __PRETTY_FUNCTION__));


  if (rrpv != (-1))
  {

    ((rrpv >= 0 && rrpv <= policy_data->max_rrpv) ? (void) (0) : __assert_fail ("rrpv >= 0 && rrpv <= policy_data->max_rrpv", "policy/srripsage.c", 2416, __PRETTY_FUNCTION__));


    do { ((((policy_data)->free_head->head) && ((policy_data)->free_head->head)) ? (void) (0) : __assert_fail ("((policy_data)->free_head->head) && ((policy_data)->free_head->head)", "policy/srripsage.c", 2419, __PRETTY_FUNCTION__)); (((block)->state == cache_block_invalid) ? (void) (0) : __assert_fail ("(block)->state == cache_block_invalid", "policy/srripsage.c", 2419, __PRETTY_FUNCTION__)); do { if ((block)->next == ((void *)0)) { (((policy_data)->free_head->head)) = (block)->prev; if ((block)->prev) { (block)->prev->next = ((void *)0); } } if ((block)->prev == ((void *)0)) { (((policy_data)->free_tail->head)) = (block)->next; if ((block)->next) { (block)->next->prev = ((void *)0); } } if ((block)->next != ((void *)0) && (block)->prev != ((void *)0)) { ((block)->next)->prev = (block)->prev; ((block)->prev)->next = (block)->next; } (block)->next = ((void *)0); (block)->prev = ((void *)0); }while(0); (block)->next = ((void *)0); (block)->prev = ((void *)0); (block)->busy = 0; (block)->cached = 0; (block)->prefetch = 0; }while(0);;


    do { (block)->tag = (tag); (block)->vtl_addr = (info->vtl_addr); (block)->state = (state); }while(0);
    do { if ((block)->stream == (2) && strm != (2)) { srripsage_blocks--; } if (strm == (2) && (block)->stream != (2)) { srripsage_blocks++; } (block)->stream = strm; }while(0);

    block->dirty = (info && info->spill) ? 1 : 0;
    block->recency = policy_data->evictions;
    block->last_rrpv = rrpv;
    block->access = 0;
    block->demote = (0);
    block->demote_at_head = (0);
    block->demote_at_tail = (0);
    block->fill_at_head = (0);
    block->fill_at_tail = (0);
    block->nru = (0);

    switch (strm)
    {
      case (4):
        block->epoch = 0;
        break;

      default:
        block->epoch = 3;
        break;
    }


    if (policy_data->set_type == (0))
    {







      {
        if (rrpv == !((policy_data)->max_rrpv))
        {
          cache_add_reuse_blocks(global_data, (0), info->stream);
        }
      }
    }


    if (policy_data->set_type == (1) &&
        rrpv == ((policy_data)->max_rrpv) - 1)
    {

      if (policy_data->fill_at_lru[info->stream])
      {
        CACHE_PREPEND_TO_QUEUE;



        block->fill_at_head = (1);
      }
      else
      {
        do { if ((((policy_data)->valid_head)[(2)]).head == ((void *)0) && (((policy_data)->valid_tail)[(2)]).head == ((void *)0)) { (((policy_data)->valid_head)[(2)]).head = (block); (((policy_data)->valid_tail)[(2)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(2)]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2480 "policy/srripsage.c"
        ,

 2482
# 2480 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[(2)]).head)->prev = (block); (block)->next = (((policy_data)->valid_tail)[(2)]).head; (((policy_data)->valid_tail)[(2)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(2)]); }while(0)

                                                                       ;

        block->fill_at_tail = (1);
      }
    }
    else
    {
      if (policy_data->set_type == (2) &&
          rrpv == ((policy_data)->max_rrpv) - 1)
      {
        if (global_data->bm_ctr == 0)
        {

          do { if ((((policy_data)->valid_head)[(1)]).head == ((void *)0) && (((policy_data)->valid_tail)[(1)]).head == ((void *)0)) { (((policy_data)->valid_head)[(1)]).head = (block); (((policy_data)->valid_tail)[(1)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(1)]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2495 "policy/srripsage.c"
          ,

 2497
# 2495 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); ((((policy_data)->valid_head)[(1)]).head)->next = (block); (block)->prev = (((policy_data)->valid_head)[(1)]).head; (((policy_data)->valid_head)[(1)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(1)]); }while(0)

                                                                         ;

          block->fill_at_head = (1);
        }
        else
        {

          do { if ((((policy_data)->valid_head)[(2)]).head == ((void *)0) && (((policy_data)->valid_tail)[(2)]).head == ((void *)0)) { (((policy_data)->valid_head)[(2)]).head = (block); (((policy_data)->valid_tail)[(2)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(2)]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2504 "policy/srripsage.c"
          ,

 2506
# 2504 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[(2)]).head)->prev = (block); (block)->next = (((policy_data)->valid_tail)[(2)]).head; (((policy_data)->valid_tail)[(2)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(2)]); }while(0)

                                                                         ;

          block->fill_at_tail = (1);
        }
      }
      else
      {
        if (rrpv == ((policy_data)->max_rrpv) - 1)
        {
          ((policy_data->set_type == (3) || policy_data->set_type == (0)) ? (void) (0) : __assert_fail ("policy_data->set_type == (3) || policy_data->set_type == (0)",
 "policy/srripsage.c"
# 2515 "policy/srripsage.c"
          ,
 2516
# 2515 "policy/srripsage.c"
          , __PRETTY_FUNCTION__))
                                                       ;

          fill_in_followers(policy_data, global_data, info, rrpv, block);

          if (block->fill_at_head && policy_data->set_type == (0))
          {
            if (rrpv == ((policy_data)->max_rrpv) - 1)
            {
              cache_add_reuse_blocks(global_data, (1), info->stream);
            }
          }
        }
        else
        {
          do { if ((((policy_data)->valid_head)[(3)]).head == ((void *)0) && (((policy_data)->valid_tail)[(3)]).head == ((void *)0)) { (((policy_data)->valid_head)[(3)]).head = (block); (((policy_data)->valid_tail)[(3)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(3)]); break; } (((block->next == ((void *)0)) && (block)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(block->next == ((void *)0)) && (block)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2530 "policy/srripsage.c"
          ,

 2532
# 2530 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[(3)]).head)->prev = (block); (block)->next = (((policy_data)->valid_tail)[(3)]).head; (((policy_data)->valid_tail)[(3)]).head = (block); (block)->data = (&((policy_data)->valid_head)[(3)]); }while(0)

                                                                     ;

          block->fill_at_tail = (1);
        }
      }
    }


    ((((policy_data)->rrpv_blocks)[rrpv] < 16) ? (void) (0) : __assert_fail ("((policy_data)->rrpv_blocks)[rrpv] < 16", "policy/srripsage.c", 2540, __PRETTY_FUNCTION__));
    ((policy_data)->rrpv_blocks)[rrpv]++;
    global_data->rrpv_blocks[rrpv]++;
  }

  update_bm_ctr(global_data);


  policy_data->hit_post_fill[info->stream] = (0);
}

int cache_replace_block_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info)
{
  struct cache_block_t *block;
  struct cache_block_t *lrublock;
  int rrpv;
  int old_rrpv;


  unsigned int min_wayid = ~(0);
  lrublock = ((void *)0);

  ((policy_data->following == cache_policy_srripsage || policy_data->following == cache_policy_srrip) ? (void) (0) : __assert_fail ("policy_data->following == cache_policy_srripsage || policy_data->following == cache_policy_srrip",
 "policy/srripsage.c"
# 2563 "policy/srripsage.c"
  ,
 2564
# 2563 "policy/srripsage.c"
  , __PRETTY_FUNCTION__))
                                                   ;


  for (block = ((policy_data)->free_head->head); block; block = block->prev)
  {
    return block->way;
  }


  rrpv = cache_get_replacement_rrpv_srripsage(policy_data);


  ((rrpv >= 0 && rrpv <= ((policy_data)->max_rrpv)) ? (void) (0) : __assert_fail ("rrpv >= 0 && rrpv <= ((policy_data)->max_rrpv)", "policy/srripsage.c", 2576, __PRETTY_FUNCTION__));

  if (min_wayid == ~(0))
  {


    if (((policy_data)->valid_head)[rrpv].head == ((void *)0))
    {
      ((((policy_data)->valid_tail)[rrpv].head == ((void *)0)) ? (void) (0) : __assert_fail ("((policy_data)->valid_tail)[rrpv].head == ((void *)0)", "policy/srripsage.c", 2584, __PRETTY_FUNCTION__));


      if (policy_data->set_type == (1))
      {

        do { int dif = 0; int min_wayid = 0xff; int old_rrpv; struct cache_block_t *rpl_node = ((void *)0); old_rrpv = rrpv; { if ((((policy_data)->valid_head))[(0)].head && (((policy_data)->valid_head))[(2)].head == ((void *)0)) { { struct cache_block_t *node = (((policy_data)->valid_head))[(0)].head; struct cache_block_t *nru_node; nru_node = get_nru_from_head(((policy_data)->valid_head)[(0)].head); if (node) { min_wayid = get_min_wayid_from_head(node); rpl_node = nru_node; if ((nru_node == ((void *)0)) || (nru_node && ((policy_data)->evictions - nru_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(nru_node->stream)]) : (((global_data))->rcy_thr[(3)][(nru_node->stream)])))) { rpl_node = &((policy_data)->blocks[min_wayid]); } ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",

 "policy/srripsage.c"
# 2590 "policy/srripsage.c"
        ,

 2592
# 2590 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); do { if ((rpl_node)->next == ((void *)0)) { (((policy_data)->valid_head)[(0)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { (((policy_data)->valid_tail)[(0)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); rpl_node->data = &(((policy_data)->valid_head)[(2)]); rpl_node->demote = (1); if (((policy_data)->evictions - rpl_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(rpl_node->stream)]) : (((global_data))->rcy_thr[(3)][(rpl_node->stream)]))) { (global_data)->fail_demotion[rpl_node->stream]++; } else { (global_data)->valid_demotion[rpl_node->stream]++; } cache_add_reuse_blocks((global_data), (2), rpl_node->stream); do { if ((((policy_data)->valid_head)[(2)]).head == ((void *)0) && (((policy_data)->valid_tail)[(2)]).head == ((void *)0)) { (((policy_data)->valid_head)[(2)]).head = (rpl_node); (((policy_data)->valid_tail)[(2)]).head = (rpl_node); (rpl_node)->data = (&((policy_data)->valid_head)[(2)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2590 "policy/srripsage.c"
        ,

 2592
# 2590 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[(2)]).head)->prev = (rpl_node); (rpl_node)->next = (((policy_data)->valid_tail)[(2)]).head; (((policy_data)->valid_tail)[(2)]).head = (rpl_node); (rpl_node)->data = (&((policy_data)->valid_head)[(2)]); }while(0); } } } { struct cache_block_t *node; if ((((policy_data)->valid_head))[(2)].head || (((policy_data)->valid_head))[(1)].head) { if ((((policy_data)->valid_head))[(2)].head) { rpl_node = (((policy_data)->valid_tail))[(2)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(2)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(2)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } else { rpl_node = (((policy_data)->valid_tail))[(1)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(1)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(1)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",

 "policy/srripsage.c"
# 2590 "policy/srripsage.c"
        ,

 2592
# 2590 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); rpl_node->data = &(((policy_data)->valid_head)[(3)]); rpl_node->demote = (1); rpl_node->demote_at_tail = (1); cache_add_reuse_blocks((global_data), (2), rpl_node->stream); do { if (((((policy_data)->valid_head))[(3)]).head == ((void *)0) && ((((policy_data)->valid_tail))[(3)]).head == ((void *)0)) { ((((policy_data)->valid_head))[(3)]).head = (rpl_node); ((((policy_data)->valid_tail))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2590 "policy/srripsage.c"
        ,

 2592
# 2590 "policy/srripsage.c"
        , __PRETTY_FUNCTION__)); (((((policy_data)->valid_tail))[(3)]).head)->prev = (rpl_node); (rpl_node)->next = ((((policy_data)->valid_tail))[(3)]).head; ((((policy_data)->valid_tail))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); }while(0); } } } }while(0)

                        ;
      }
      else
      {
        if (policy_data->set_type == (2))
        {

          if (policy_data->dem_to_lru[info->stream])
          {
            do { int dif = 0; int old_rrpv; int min_wayid = 0xff; struct cache_block_t *rpl_node = ((void *)0); { if ((((policy_data)->valid_head))[(0)].head && (((policy_data)->valid_head))[(2)].head == ((void *)0)) { { struct cache_block_t *node = (((policy_data)->valid_head))[(0)].head; struct cache_block_t *nru_node; nru_node = get_nru_from_head(((policy_data)->valid_head)[(0)].head); if (node) { min_wayid = get_min_wayid_from_head(node); rpl_node = nru_node; if ((nru_node == ((void *)0)) || (nru_node && ((policy_data)->evictions - nru_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(nru_node->stream)]) : (((global_data))->rcy_thr[(3)][(nru_node->stream)])))) { rpl_node = &((policy_data)->blocks[min_wayid]); } ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",

 "policy/srripsage.c"
# 2601 "policy/srripsage.c"
            ,

 2603
# 2601 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); do { if ((rpl_node)->next == ((void *)0)) { (((policy_data)->valid_head)[(0)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { (((policy_data)->valid_tail)[(0)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); rpl_node->data = &(((policy_data)->valid_head)[(1)]); rpl_node->demote = (1); if (((policy_data)->evictions - rpl_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(rpl_node->stream)]) : (((global_data))->rcy_thr[(3)][(rpl_node->stream)]))) { (global_data)->fail_demotion[rpl_node->stream]++; } else { (global_data)->valid_demotion[rpl_node->stream]++; } cache_add_reuse_blocks((global_data), (2), rpl_node->stream); do { if ((((policy_data)->valid_head)[(1)]).head == ((void *)0) && (((policy_data)->valid_tail)[(1)]).head == ((void *)0)) { (((policy_data)->valid_head)[(1)]).head = (rpl_node); (((policy_data)->valid_tail)[(1)]).head = (rpl_node); (rpl_node)->data = (&((policy_data)->valid_head)[(1)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2601 "policy/srripsage.c"
            ,

 2603
# 2601 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); ((((policy_data)->valid_head)[(1)]).head)->next = (rpl_node); (rpl_node)->prev = (((policy_data)->valid_head)[(1)]).head; (((policy_data)->valid_head)[(1)]).head = (rpl_node); (rpl_node)->data = (&((policy_data)->valid_head)[(1)]); }while(0); } } } { struct cache_block_t *node; if ((((policy_data)->valid_head))[(2)].head || (((policy_data)->valid_head))[(1)].head) { ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",

 "policy/srripsage.c"
# 2601 "policy/srripsage.c"
            ,

 2603
# 2601 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); if ((((policy_data)->valid_head))[(2)].head) { rpl_node = (((policy_data)->valid_tail))[(2)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(2)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(2)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } else { rpl_node = (((policy_data)->valid_tail))[(1)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(1)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(1)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } rpl_node->data = &(((policy_data)->valid_head)[(3)]); rpl_node->demote = (1); rpl_node->demote_at_head = (1); cache_add_reuse_blocks((global_data), (2), rpl_node->stream); do { if (((((policy_data)->valid_head))[(3)]).head == ((void *)0) && ((((policy_data)->valid_tail))[(3)]).head == ((void *)0)) { ((((policy_data)->valid_head))[(3)]).head = (rpl_node); ((((policy_data)->valid_tail))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2601 "policy/srripsage.c"
            ,

 2603
# 2601 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); (((((policy_data)->valid_head))[(3)]).head)->next = (rpl_node); (rpl_node)->prev = ((((policy_data)->valid_head))[(3)]).head; ((((policy_data)->valid_head))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); }while(0); } } } }while(0)

                            ;
          }
          else
          {
            do { int dif = 0; int min_wayid = 0xff; int old_rrpv; struct cache_block_t *rpl_node = ((void *)0); old_rrpv = rrpv; { if ((((policy_data)->valid_head))[(0)].head && (((policy_data)->valid_head))[(2)].head == ((void *)0)) { { struct cache_block_t *node = (((policy_data)->valid_head))[(0)].head; struct cache_block_t *nru_node; nru_node = get_nru_from_head(((policy_data)->valid_head)[(0)].head); if (node) { min_wayid = get_min_wayid_from_head(node); rpl_node = nru_node; if ((nru_node == ((void *)0)) || (nru_node && ((policy_data)->evictions - nru_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(nru_node->stream)]) : (((global_data))->rcy_thr[(3)][(nru_node->stream)])))) { rpl_node = &((policy_data)->blocks[min_wayid]); } ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",

 "policy/srripsage.c"
# 2607 "policy/srripsage.c"
            ,

 2609
# 2607 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); do { if ((rpl_node)->next == ((void *)0)) { (((policy_data)->valid_head)[(0)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { (((policy_data)->valid_tail)[(0)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); rpl_node->data = &(((policy_data)->valid_head)[(2)]); rpl_node->demote = (1); if (((policy_data)->evictions - rpl_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(rpl_node->stream)]) : (((global_data))->rcy_thr[(3)][(rpl_node->stream)]))) { (global_data)->fail_demotion[rpl_node->stream]++; } else { (global_data)->valid_demotion[rpl_node->stream]++; } cache_add_reuse_blocks((global_data), (2), rpl_node->stream); do { if ((((policy_data)->valid_head)[(2)]).head == ((void *)0) && (((policy_data)->valid_tail)[(2)]).head == ((void *)0)) { (((policy_data)->valid_head)[(2)]).head = (rpl_node); (((policy_data)->valid_tail)[(2)]).head = (rpl_node); (rpl_node)->data = (&((policy_data)->valid_head)[(2)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2607 "policy/srripsage.c"
            ,

 2609
# 2607 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[(2)]).head)->prev = (rpl_node); (rpl_node)->next = (((policy_data)->valid_tail)[(2)]).head; (((policy_data)->valid_tail)[(2)]).head = (rpl_node); (rpl_node)->data = (&((policy_data)->valid_head)[(2)]); }while(0); } } } { struct cache_block_t *node; if ((((policy_data)->valid_head))[(2)].head || (((policy_data)->valid_head))[(1)].head) { if ((((policy_data)->valid_head))[(2)].head) { rpl_node = (((policy_data)->valid_tail))[(2)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(2)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(2)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } else { rpl_node = (((policy_data)->valid_tail))[(1)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(1)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(1)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",

 "policy/srripsage.c"
# 2607 "policy/srripsage.c"
            ,

 2609
# 2607 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); rpl_node->data = &(((policy_data)->valid_head)[(3)]); rpl_node->demote = (1); rpl_node->demote_at_tail = (1); cache_add_reuse_blocks((global_data), (2), rpl_node->stream); do { if (((((policy_data)->valid_head))[(3)]).head == ((void *)0) && ((((policy_data)->valid_tail))[(3)]).head == ((void *)0)) { ((((policy_data)->valid_head))[(3)]).head = (rpl_node); ((((policy_data)->valid_tail))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",

 "policy/srripsage.c"
# 2607 "policy/srripsage.c"
            ,

 2609
# 2607 "policy/srripsage.c"
            , __PRETTY_FUNCTION__)); (((((policy_data)->valid_tail))[(3)]).head)->prev = (rpl_node); (rpl_node)->next = ((((policy_data)->valid_tail))[(3)]).head; ((((policy_data)->valid_tail))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); }while(0); } } } }while(0)

                            ;
          }
        }
        else
        {
          ((policy_data->set_type == (3) || policy_data->set_type == (0)) ? (void) (0) : __assert_fail ("policy_data->set_type == (3) || policy_data->set_type == (0)",
 "policy/srripsage.c"
# 2614 "policy/srripsage.c"
          ,
 2615
# 2614 "policy/srripsage.c"
          , __PRETTY_FUNCTION__))
                                                       ;


          do { int dif = 0; int min_wayid = 0xff; int old_rrpv; struct cache_block_t *rpl_node = ((void *)0); { if ((((policy_data)->valid_head))[(0)].head && (((policy_data)->valid_head))[(2)].head == ((void *)0)) { { struct cache_block_t *node = (((policy_data)->valid_head))[(0)].head; struct cache_block_t *nru_node; nru_node = get_nru_from_head(((policy_data)->valid_head)[(0)].head); { min_wayid = get_min_wayid_from_head(node); rpl_node = nru_node; if ((nru_node == ((void *)0)) || (nru_node && ((policy_data)->evictions - nru_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(nru_node->stream)]) : (((global_data))->rcy_thr[(3)][(nru_node->stream)])))) { rpl_node = &((policy_data)->blocks[min_wayid]); } ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",
 "policy/srripsage.c"
# 2618 "policy/srripsage.c"
          ,
 2619
# 2618 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); do { if ((rpl_node)->next == ((void *)0)) { (((policy_data)->valid_head)[(0)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { (((policy_data)->valid_tail)[(0)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); if (((policy_data)->evictions - rpl_node->recency) < ((((policy_data))->set_type == (0)) ? (((global_data))->rcy_thr[(0)][(rpl_node->stream)]) : (((global_data))->rcy_thr[(3)][(rpl_node->stream)]))) { (global_data)->fail_demotion[rpl_node->stream]++; } else { (global_data)->valid_demotion[rpl_node->stream]++; } if ((global_data)->demote_at_head[rpl_node->stream] == (0)) { rpl_node->data = &(((policy_data)->valid_head)[(2)]); rpl_node->demote = (1); { do { if (((((policy_data)->valid_head))[(2)]).head == ((void *)0) && ((((policy_data)->valid_tail))[(2)]).head == ((void *)0)) { ((((policy_data)->valid_head))[(2)]).head = (rpl_node); ((((policy_data)->valid_tail))[(2)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(2)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",
 "policy/srripsage.c"
# 2618 "policy/srripsage.c"
          ,
 2619
# 2618 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); (((((policy_data)->valid_tail))[(2)]).head)->prev = (rpl_node); (rpl_node)->next = ((((policy_data)->valid_tail))[(2)]).head; ((((policy_data)->valid_tail))[(2)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(2)]); }while(0); rpl_node->demote_at_tail = (1); } } else { rpl_node->data = &(((policy_data)->valid_head)[(1)]); rpl_node->demote = (1); do { if (((((policy_data)->valid_head))[(1)]).head == ((void *)0) && ((((policy_data)->valid_tail))[(1)]).head == ((void *)0)) { ((((policy_data)->valid_head))[(1)]).head = (rpl_node); ((((policy_data)->valid_tail))[(1)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(1)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",
 "policy/srripsage.c"
# 2618 "policy/srripsage.c"
          ,
 2619
# 2618 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); (((((policy_data)->valid_head))[(1)]).head)->next = (rpl_node); (rpl_node)->prev = ((((policy_data)->valid_head))[(1)]).head; ((((policy_data)->valid_head))[(1)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(1)]); }while(0); rpl_node->demote_at_head = (1); } } } } { struct cache_block_t *node; if ((((policy_data)->valid_head))[(2)].head || (((policy_data)->valid_head))[(1)].head) { if ((((policy_data)->valid_head))[(2)].head) { rpl_node = (((policy_data)->valid_tail))[(2)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(2)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(2)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } else { rpl_node = (((policy_data)->valid_tail))[(1)].head; do { if ((rpl_node)->next == ((void *)0)) { ((((policy_data)->valid_head))[(1)]).head = (rpl_node)->prev; if ((rpl_node)->prev) { (rpl_node)->prev->next = ((void *)0); } } if ((rpl_node)->prev == ((void *)0)) { ((((policy_data)->valid_tail))[(1)]).head = (rpl_node)->next; if ((rpl_node)->next) { (rpl_node)->next->prev = ((void *)0); } } if ((rpl_node)->next != ((void *)0) && (rpl_node)->prev != ((void *)0)) { ((rpl_node)->next)->prev = (rpl_node)->prev; ((rpl_node)->prev)->next = (rpl_node)->next; } (rpl_node)->data = ((void *)0); (rpl_node)->next = ((void *)0); (rpl_node)->prev = ((void *)0); }while(0); } ((rpl_node) ? (void) (0) : __assert_fail ("rpl_node",
 "policy/srripsage.c"
# 2618 "policy/srripsage.c"
          ,
 2619
# 2618 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); rpl_node->data = &(((policy_data)->valid_head)[(3)]); rpl_node->demote = (1); do { if (((((policy_data)->valid_head))[(3)]).head == ((void *)0) && ((((policy_data)->valid_tail))[(3)]).head == ((void *)0)) { ((((policy_data)->valid_head))[(3)]).head = (rpl_node); ((((policy_data)->valid_tail))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); break; } (((rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(rpl_node->next == ((void *)0)) && (rpl_node)->prev == ((void *)0)",
 "policy/srripsage.c"
# 2618 "policy/srripsage.c"
          ,
 2619
# 2618 "policy/srripsage.c"
          , __PRETTY_FUNCTION__)); (((((policy_data)->valid_tail))[(3)]).head)->prev = (rpl_node); (rpl_node)->next = ((((policy_data)->valid_tail))[(3)]).head; ((((policy_data)->valid_tail))[(3)]).head = (rpl_node); (rpl_node)->data = (&(((policy_data)->valid_head))[(3)]); }while(0); } } } }while(0)
                                                                                     ;


          if (global_data->per_stream_dreuse[info->stream] > 4)
          {
            global_data->dems_at_head[info->stream]++;
          }
        }
      }
    }

    for (block = ((policy_data)->valid_tail)[rrpv].head; block; block = block->prev)
    {
      if (!block->busy && block->way < min_wayid)
        min_wayid = block->way;
    }


    old_rrpv = (policy_data->blocks)[min_wayid].last_rrpv;


    policy_data->evictions += 1;


    ((((policy_data)->rrpv_blocks)[old_rrpv] > 0) ? (void) (0) : __assert_fail ("((policy_data)->rrpv_blocks)[old_rrpv] > 0", "policy/srripsage.c", 2643, __PRETTY_FUNCTION__));
    ((policy_data)->rrpv_blocks)[old_rrpv]--;

    global_data->rrpv_blocks[old_rrpv]--;

    if (policy_data->blocks[min_wayid].last_rrpv == 2)
    {
      ub1 src_stream = policy_data->blocks[min_wayid].stream;

      if (src_stream == info->stream)
      {
        global_data->per_stream_sevct[src_stream] += 1;
      }
      else
      {
        global_data->per_stream_xevct[src_stream] += 1;
      }
    }
# 2671 "policy/srripsage.c"
  }


  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_srripsage(srripsage_data *policy_data,
  srripsage_gdata *global_data, int way, int strm, memory_trace *info)
{
  enum cache_block_state_t state_ptr;
  struct cache_block_t *blk = ((void *)0);
  struct cache_block_t *next = ((void *)0);
  struct cache_block_t *prev = ((void *)0);

  int min_wayid;
  int stream_ptr;
  long long tag_ptr;

  int old_rrpv;
  int new_rrpv;


  global_data->per_stream_hit[strm]++;

  ((policy_data->following == cache_policy_srripsage || policy_data->following == cache_policy_srrip) ? (void) (0) : __assert_fail ("policy_data->following == cache_policy_srripsage || policy_data->following == cache_policy_srrip",
 "policy/srripsage.c"
# 2695 "policy/srripsage.c"
  ,
 2696
# 2695 "policy/srripsage.c"
  , __PRETTY_FUNCTION__))
                                                   ;

  blk = &(((policy_data)->blocks)[way]);
  prev = blk->prev;
  next = blk->next;


  ((blk->tag >= 0) ? (void) (0) : __assert_fail ("blk->tag >= 0", "policy/srripsage.c", 2703, __PRETTY_FUNCTION__));
  ((blk->state != cache_block_invalid) ? (void) (0) : __assert_fail ("blk->state != cache_block_invalid", "policy/srripsage.c", 2704, __PRETTY_FUNCTION__));


  if (policy_data->set_type == (1))
  {
    if (info->stream == (4))
    {
      if (blk->stream == (2))
      {
        ((global_data->ct_ctr).val = ((global_data->ct_ctr).val < (global_data->ct_ctr).max) ? (global_data->ct_ctr).val + 1 : (global_data->ct_ctr).val);
      }

      if (blk->stream == (7))
      {
        ((global_data->bt_ctr).val = ((global_data->bt_ctr).val < (global_data->bt_ctr).max) ? (global_data->bt_ctr).val + 1 : (global_data->bt_ctr).val);
      }

      if (blk->stream == (3))
      {
        ((global_data->zt_ctr).val = ((global_data->zt_ctr).val < (global_data->zt_ctr).max) ? (global_data->zt_ctr).val + 1 : (global_data->zt_ctr).val);
      }
    }

    if (info->stream == (7))
    {
      if (blk->stream == (2))
      {
        ((global_data->cb_ctr).val = ((global_data->cb_ctr).val < (global_data->cb_ctr).max) ? (global_data->cb_ctr).val + 1 : (global_data->cb_ctr).val);
      }

      if (blk->stream == (4))
      {
        ((global_data->tb_ctr).val = ((global_data->tb_ctr).val < (global_data->tb_ctr).max) ? (global_data->tb_ctr).val + 1 : (global_data->tb_ctr).val);
      }
    }

    if (info->stream == (2))
    {
      if (blk->stream == (7))
      {
        ((global_data->bc_ctr).val = ((global_data->bc_ctr).val < (global_data->bc_ctr).max) ? (global_data->bc_ctr).val + 1 : (global_data->bc_ctr).val);
      }
    }
  }


  old_rrpv = (((rrip_list *)(blk->data))->rrpv);
  new_rrpv = old_rrpv;


  new_rrpv = cache_get_new_rrpv_srripsage(policy_data, global_data, info, way,
      old_rrpv, blk->recency);


  if (policy_data->set_type == (0))
  {

    if (new_rrpv == !((policy_data)->max_rrpv))
    {
      if (old_rrpv != new_rrpv)
      {

        cache_add_reuse_blocks(global_data, (0), blk->stream);







      }
      else
      {

        cache_update_reuse(global_data, (0), blk->stream);
      }

      if (old_rrpv == ((policy_data)->max_rrpv) - 1)
      {

        if (blk->demote == (0) && blk->fill_at_head)
        {
          cache_update_reuse(global_data, (1), blk->stream);
        }
      }
    }

    if (blk->demote)
    {
      cache_update_reuse(global_data, (2), blk->stream);
    }
  }


  if (new_rrpv == ((policy_data)->max_rrpv))
  {
    global_data->per_stream_demote[old_rrpv][info->stream]++;
  }

  if (old_rrpv == ((policy_data)->max_rrpv) - 1 &&
      blk->demote == (0))
  {
    if (policy_data->set_type == (1))
    {

      ((global_data->fath_ctr[blk->stream]).val = ((global_data->fath_ctr[blk->stream]).val < (global_data->fath_ctr[blk->stream]).max) ? (global_data->fath_ctr[blk->stream]).val + 1 : (global_data->fath_ctr[blk->stream]).val);

      global_data->lru_sample_hit[blk->stream]++;


      ((global_data->gfath_ctr).val = ((global_data->gfath_ctr).val < (global_data->gfath_ctr).max) ? (global_data->gfath_ctr).val + 1 : (global_data->gfath_ctr).val);
    }
    else
    {
      if (policy_data->set_type == (2))
      {

        ((global_data->fath_ctr[blk->stream]).val = ((global_data->fath_ctr[blk->stream]).val > (global_data->fath_ctr[blk->stream]).min) ? (global_data->fath_ctr[blk->stream]).val - 1 : (global_data->fath_ctr[blk->stream]).val);
        global_data->mru_sample_hit[blk->stream]++;


        ((global_data->gfath_ctr).val = ((global_data->gfath_ctr).val > (global_data->gfath_ctr).min) ? (global_data->gfath_ctr).val - 1 : (global_data->gfath_ctr).val);
      }
    }
  }
  else
  {
    if (old_rrpv == ((policy_data)->max_rrpv) - 1 &&
        blk->demote == (1))
    {
      if (policy_data->set_type == (1))
      {
        if (policy_data->fill_at_lru[blk->stream])
        {

          ((global_data->dem_ctr[blk->stream]).val = ((global_data->dem_ctr[blk->stream]).val < (global_data->dem_ctr[blk->stream]).max) ? (global_data->dem_ctr[blk->stream]).val + 1 : (global_data->dem_ctr[blk->stream]).val);

          global_data->lru_sample_dem[blk->stream]++;


          ((global_data->gdem_ctr).val = ((global_data->gdem_ctr).val < (global_data->gdem_ctr).max) ? (global_data->gdem_ctr).val + 1 : (global_data->gdem_ctr).val);
        }
      }
      else
      {
        if (policy_data->set_type == (2))
        {

          if (policy_data->dem_to_lru[blk->stream])
          {
            ((global_data->dem_ctr[blk->stream]).val = ((global_data->dem_ctr[blk->stream]).val > (global_data->dem_ctr[blk->stream]).min) ? (global_data->dem_ctr[blk->stream]).val - 1 : (global_data->dem_ctr[blk->stream]).val);

            global_data->mru_sample_dem[blk->stream]++;


            ((global_data->gdem_ctr).val = ((global_data->gdem_ctr).val > (global_data->gdem_ctr).min) ? (global_data->gdem_ctr).val - 1 : (global_data->gdem_ctr).val);
          }
        }
      }
    }
  }


  if (new_rrpv != old_rrpv)
  {

    do { if ((blk)->next == ((void *)0)) { (((policy_data)->valid_head)[old_rrpv]).head = (blk)->prev; if ((blk)->prev) { (blk)->prev->next = ((void *)0); } } if ((blk)->prev == ((void *)0)) { (((policy_data)->valid_tail)[old_rrpv]).head = (blk)->next; if ((blk)->next) { (blk)->next->prev = ((void *)0); } } if ((blk)->next != ((void *)0) && (blk)->prev != ((void *)0)) { ((blk)->next)->prev = (blk)->prev; ((blk)->prev)->next = (blk)->next; } (blk)->data = ((void *)0); (blk)->next = ((void *)0); (blk)->prev = ((void *)0); }while(0)
                                                         ;

    do { if ((((policy_data)->valid_head)[new_rrpv]).head == ((void *)0) && (((policy_data)->valid_tail)[new_rrpv]).head == ((void *)0)) { (((policy_data)->valid_head)[new_rrpv]).head = (blk); (((policy_data)->valid_tail)[new_rrpv]).head = (blk); (blk)->data = (&((policy_data)->valid_head)[new_rrpv]); break; } (((blk->next == ((void *)0)) && (blk)->prev == ((void *)0)) ? (void) (0) : __assert_fail ("(blk->next == ((void *)0)) && (blk)->prev == ((void *)0)",
 "policy/srripsage.c"
# 2873 "policy/srripsage.c"
    ,
 2874
# 2873 "policy/srripsage.c"
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
# 2884 "policy/srripsage.c"
      ,
 2885
# 2884 "policy/srripsage.c"
      , __PRETTY_FUNCTION__)); ((((policy_data)->valid_tail)[new_rrpv]).head)->prev = (blk); (blk)->next = (((policy_data)->valid_tail)[new_rrpv]).head; (((policy_data)->valid_tail)[new_rrpv]).head = (blk); (blk)->data = (&((policy_data)->valid_head)[new_rrpv]); }while(0)
                                                           ;
    }
  }


  if (new_rrpv == !((policy_data)->max_rrpv))
  {
    cache_fill_nru_block(&((policy_data)->valid_head)[new_rrpv], blk);
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
  else
  {
    if (blk->stream == (4))
    {
      blk->epoch = 3;
    }
  }

  if (blk->last_rrpv != new_rrpv)
  {

    ((((policy_data)->rrpv_blocks)[blk->last_rrpv] > 0) ? (void) (0) : __assert_fail ("((policy_data)->rrpv_blocks)[blk->last_rrpv] > 0", "policy/srripsage.c", 2917, __PRETTY_FUNCTION__));

    ((policy_data)->rrpv_blocks)[blk->last_rrpv]--;
    global_data->rrpv_blocks[blk->last_rrpv]--;

    ((policy_data)->rrpv_blocks)[new_rrpv]++;
    global_data->rrpv_blocks[new_rrpv]++;
  }

  global_data->per_stream_rrpv_hit[old_rrpv][strm]++;

  do { if ((blk)->stream == (2) && strm != (2)) { srripsage_blocks--; } if (strm == (2) && (blk)->stream != (2)) { srripsage_blocks++; } (blk)->stream = strm; }while(0);
  blk->dirty = (info && info->dirty) ? 1 : 0;
  blk->recency = policy_data->evictions;
  blk->last_rrpv = new_rrpv;
  blk->demote = (0);
  blk->demote_at_head = (0);
  blk->demote_at_tail = (0);
  blk->access += 1;

  policy_data->last_eviction = policy_data->evictions;


  policy_data->hit_post_fill[info->stream] = (1);
}

ub1 cache_override_fill_at_head(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info)
{
  ub1 fath;
  struct cache_block_t *block;

  fath = (1);
  block = ((policy_data)->valid_head)[0].head;

  if (info->stream != (2) && info->stream != (3) && info->stream != (4) &&
      info->stream != (7))
  {
    fath = (0);
  }
  else
  {
    if (block && policy_data->evictions)
    {

      ((block->recency <= policy_data->evictions) ? (void) (0) : __assert_fail ("block->recency <= policy_data->evictions", "policy/srripsage.c", 2962, __PRETTY_FUNCTION__));

      if ((policy_data->evictions - block->recency) < (((policy_data)->set_type == (0)) ? ((global_data)->rcy_thr[(0)][(block->stream)]) : ((global_data)->rcy_thr[(3)][(block->stream)])))
      {
        fath = (0);
      }
    }
  }

  return fath;
}

int cache_get_fill_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info)
{
  int ret_rrpv;
  int tex_alloc;
  struct cache_block_t *block;

  tex_alloc = (0);

  block = ((void *)0);

  switch (info->stream)
  {
    case (4):
      ret_rrpv = ((policy_data)->max_rrpv) - 1;
      break;

    case (7):
      ret_rrpv = ((policy_data)->max_rrpv) - 1;
      break;

    case (2):
      ret_rrpv = ((policy_data)->max_rrpv) - 1;
      break;

    case (3):
      ret_rrpv = ((policy_data)->max_rrpv) - 1;
      break;

    case (10):
      ret_rrpv = ((policy_data)->max_rrpv);
      break;

    default:
      ret_rrpv = ((policy_data)->max_rrpv);
      break;
  }

  block = ((policy_data)->valid_head)[0].head;

  if (block && policy_data->evictions)
  {

    ((block->recency <= policy_data->evictions) ? (void) (0) : __assert_fail ("block->recency <= policy_data->evictions", "policy/srripsage.c", 3017, __PRETTY_FUNCTION__));

    if ((policy_data->evictions - block->recency) < (((policy_data)->set_type == (0)) ? ((global_data)->rcy_thr[(0)][(block->stream)]) : ((global_data)->rcy_thr[(3)][(block->stream)])))
    {
      ret_rrpv = ((policy_data)->max_rrpv);
    }
  }

  return ret_rrpv;
}

int cache_get_replacement_rrpv_srripsage(srripsage_data *policy_data)
{
  return ((policy_data)->max_rrpv);
}

int cache_get_new_rrpv_srripsage(srripsage_data *policy_data,
    srripsage_gdata *global_data, memory_trace *info, int way, int old_rrpv,
    unsigned long long old_recency)
{

  unsigned int state;
  int ret_rrpv;
  struct cache_block_t *block;

  ret_rrpv = 0;


  block = ((policy_data)->valid_head)[ret_rrpv].head;
# 3090 "policy/srripsage.c"
  return ret_rrpv;
}


void cache_set_block_srripsage(srripsage_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  {
    block = &(((policy_data)->blocks))[way];


    ((block->tag == tag) ? (void) (0) : __assert_fail ("block->tag == tag", "policy/srripsage.c", 3103, __PRETTY_FUNCTION__));



    ((block->state != cache_block_invalid) ? (void) (0) : __assert_fail ("block->state != cache_block_invalid", "policy/srripsage.c", 3107, __PRETTY_FUNCTION__));

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
# 3128 "policy/srripsage.c"
    ,
 3129
# 3128 "policy/srripsage.c"
    , __PRETTY_FUNCTION__)); ((((policy_data)->free_tail->head)))->prev = (block); (block)->next = (((policy_data)->free_tail->head)); (((policy_data)->free_tail->head)) = (block); }while(0)
                                              ;
  }
}



struct cache_block_t cache_get_block_srripsage(srripsage_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  ((policy_data) ? (void) (0) : __assert_fail ("policy_data", "policy/srripsage.c", 3138, __PRETTY_FUNCTION__));
  ((tag_ptr) ? (void) (0) : __assert_fail ("tag_ptr", "policy/srripsage.c", 3139, __PRETTY_FUNCTION__));
  ((state_ptr) ? (void) (0) : __assert_fail ("state_ptr", "policy/srripsage.c", 3140, __PRETTY_FUNCTION__));
  ((stream_ptr) ? (void) (0) : __assert_fail ("stream_ptr", "policy/srripsage.c", 3141, __PRETTY_FUNCTION__));

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

  ((policy_data) ? (void) (0) : __assert_fail ("policy_data", "policy/srripsage.c", 3160, __PRETTY_FUNCTION__));

  max_rrpv = policy_data->max_rrpv;
  node = ((void *)0);
  count = 0;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = ((policy_data)->valid_head)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      ((node->state != cache_block_invalid) ? (void) (0) : __assert_fail ("node->state != cache_block_invalid", "policy/srripsage.c", 3172, __PRETTY_FUNCTION__));
      if (node->stream == strm)
        count++;
    }
  }

  return count;
}

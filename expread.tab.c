/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 19 "expread.y"

#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "frame.h"
#include "expression.h"
#include "ui.h"

#ifdef TEK_HACK
#include <ctype.h>
#endif /* TEK_HACK */
#include <stdio.h>
//#include <a.out.h>
#include "a.out.gnu.h"

#ifdef GHSFORTRAN
#include "value.h"
static struct block *blk;
#endif
static struct expression *expout;
static int expout_size;
static int expout_ptr;

static int yylex ();
static void yyerror ();
static void write_exp_elt ();
static void write_exp_elt_opcode ();
static void write_exp_elt_sym ();
static void write_exp_elt_longcst ();
static void write_exp_elt_dblcst ();
static void write_exp_elt_type ();
static void write_exp_elt_intern ();
static void write_exp_string ();
static void start_arglist ();
static int end_arglist ();
static void free_funcalls ();
static char *copy_name ();

/* If this is nonzero, this block is used as the lexical context
   for symbol names.  */

/* Little change here to make calling of get_selected_block() in
   c_parse1() lazy so that the cross-debugger doesn't choke on a dead-target
   access whenever any expression is parsed by the CLI -rcb 901126 */

static struct block *expression_context_block_var;

#define expression_context_block (expression_context_block_var ? \
                          expression_context_block_var : get_selected_block())

/* The innermost context required by the stack and register variables
   we've encountered so far. */
struct block *innermost_block;

/* The block in which the most recently discovered symbol was found. */
struct block *block_found;

/* Number of arguments seen so far in innermost function call.  */
static int arglist_len;

/* Data structure for saving values of arglist_len
   for function calls whose arguments contain other function calls.  */

struct funcall
  {
    struct funcall *next;
    int arglist_len;
  };

struct funcall *funcall_chain;

/* This kind of datum is used to represent the name
   of a symbol token.  */

struct stoken
  {
    char *ptr;
    int length;
  };

/* For parsing of complicated types.
   An array should be preceded in the list by the size of the array.  */
enum type_pieces
  {tp_end = -1, tp_pointer, tp_reference, tp_array, tp_function};
static enum type_pieces *type_stack;
static int type_stack_depth, type_stack_size;

static void push_type ();
static enum type_pieces pop_type ();

/* Allow debugging of parsing.  */
#define YYDEBUG 1

#line 165 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INT = 258,                     /* INT  */
    CHAR = 259,                    /* CHAR  */
    UINT = 260,                    /* UINT  */
    FLOAT = 261,                   /* FLOAT  */
    NAME = 262,                    /* NAME  */
    TYPENAME = 263,                /* TYPENAME  */
    BLOCKNAME = 264,               /* BLOCKNAME  */
    STRING = 265,                  /* STRING  */
    STRUCT = 266,                  /* STRUCT  */
    UNION = 267,                   /* UNION  */
    ENUM = 268,                    /* ENUM  */
    SIZEOF = 269,                  /* SIZEOF  */
    UNSIGNED = 270,                /* UNSIGNED  */
    COLONCOLON = 271,              /* COLONCOLON  */
    SIGNED = 272,                  /* SIGNED  */
    LONG = 273,                    /* LONG  */
    SHORT = 274,                   /* SHORT  */
    INT_KEYWORD = 275,             /* INT_KEYWORD  */
    LAST = 276,                    /* LAST  */
    REGNAME = 277,                 /* REGNAME  */
    VARIABLE = 278,                /* VARIABLE  */
    ASSIGN_MODIFY = 279,           /* ASSIGN_MODIFY  */
    THIS = 280,                    /* THIS  */
    ABOVE_COMMA = 281,             /* ABOVE_COMMA  */
    OR = 282,                      /* OR  */
    AND = 283,                     /* AND  */
    EQUAL = 284,                   /* EQUAL  */
    NOTEQUAL = 285,                /* NOTEQUAL  */
    LEQ = 286,                     /* LEQ  */
    GEQ = 287,                     /* GEQ  */
    LSH = 288,                     /* LSH  */
    RSH = 289,                     /* RSH  */
    UNARY = 290,                   /* UNARY  */
    INCREMENT = 291,               /* INCREMENT  */
    DECREMENT = 292,               /* DECREMENT  */
    ARROW = 293                    /* ARROW  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define INT 258
#define CHAR 259
#define UINT 260
#define FLOAT 261
#define NAME 262
#define TYPENAME 263
#define BLOCKNAME 264
#define STRING 265
#define STRUCT 266
#define UNION 267
#define ENUM 268
#define SIZEOF 269
#define UNSIGNED 270
#define COLONCOLON 271
#define SIGNED 272
#define LONG 273
#define SHORT 274
#define INT_KEYWORD 275
#define LAST 276
#define REGNAME 277
#define VARIABLE 278
#define ASSIGN_MODIFY 279
#define THIS 280
#define ABOVE_COMMA 281
#define OR 282
#define AND 283
#define EQUAL 284
#define NOTEQUAL 285
#define LEQ 286
#define GEQ 287
#define LSH 288
#define RSH 289
#define UNARY 290
#define INCREMENT 291
#define DECREMENT 292
#define ARROW 293

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 118 "expread.y"

    LONGEST lval;
    unsigned LONGEST ulval;
    double dval;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  

#line 308 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);



/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_CHAR = 4,                       /* CHAR  */
  YYSYMBOL_UINT = 5,                       /* UINT  */
  YYSYMBOL_FLOAT = 6,                      /* FLOAT  */
  YYSYMBOL_NAME = 7,                       /* NAME  */
  YYSYMBOL_TYPENAME = 8,                   /* TYPENAME  */
  YYSYMBOL_BLOCKNAME = 9,                  /* BLOCKNAME  */
  YYSYMBOL_STRING = 10,                    /* STRING  */
  YYSYMBOL_STRUCT = 11,                    /* STRUCT  */
  YYSYMBOL_UNION = 12,                     /* UNION  */
  YYSYMBOL_ENUM = 13,                      /* ENUM  */
  YYSYMBOL_SIZEOF = 14,                    /* SIZEOF  */
  YYSYMBOL_UNSIGNED = 15,                  /* UNSIGNED  */
  YYSYMBOL_COLONCOLON = 16,                /* COLONCOLON  */
  YYSYMBOL_SIGNED = 17,                    /* SIGNED  */
  YYSYMBOL_LONG = 18,                      /* LONG  */
  YYSYMBOL_SHORT = 19,                     /* SHORT  */
  YYSYMBOL_INT_KEYWORD = 20,               /* INT_KEYWORD  */
  YYSYMBOL_LAST = 21,                      /* LAST  */
  YYSYMBOL_REGNAME = 22,                   /* REGNAME  */
  YYSYMBOL_VARIABLE = 23,                  /* VARIABLE  */
  YYSYMBOL_ASSIGN_MODIFY = 24,             /* ASSIGN_MODIFY  */
  YYSYMBOL_THIS = 25,                      /* THIS  */
  YYSYMBOL_26_ = 26,                       /* ','  */
  YYSYMBOL_ABOVE_COMMA = 27,               /* ABOVE_COMMA  */
  YYSYMBOL_28_ = 28,                       /* '='  */
  YYSYMBOL_29_ = 29,                       /* '?'  */
  YYSYMBOL_OR = 30,                        /* OR  */
  YYSYMBOL_AND = 31,                       /* AND  */
  YYSYMBOL_32_ = 32,                       /* '|'  */
  YYSYMBOL_33_ = 33,                       /* '^'  */
  YYSYMBOL_34_ = 34,                       /* '&'  */
  YYSYMBOL_EQUAL = 35,                     /* EQUAL  */
  YYSYMBOL_NOTEQUAL = 36,                  /* NOTEQUAL  */
  YYSYMBOL_37_ = 37,                       /* '<'  */
  YYSYMBOL_38_ = 38,                       /* '>'  */
  YYSYMBOL_LEQ = 39,                       /* LEQ  */
  YYSYMBOL_GEQ = 40,                       /* GEQ  */
  YYSYMBOL_LSH = 41,                       /* LSH  */
  YYSYMBOL_RSH = 42,                       /* RSH  */
  YYSYMBOL_43_ = 43,                       /* '@'  */
  YYSYMBOL_44_ = 44,                       /* '+'  */
  YYSYMBOL_45_ = 45,                       /* '-'  */
  YYSYMBOL_46_ = 46,                       /* '*'  */
  YYSYMBOL_47_ = 47,                       /* '/'  */
  YYSYMBOL_48_ = 48,                       /* '%'  */
  YYSYMBOL_UNARY = 49,                     /* UNARY  */
  YYSYMBOL_INCREMENT = 50,                 /* INCREMENT  */
  YYSYMBOL_DECREMENT = 51,                 /* DECREMENT  */
  YYSYMBOL_ARROW = 52,                     /* ARROW  */
  YYSYMBOL_53_ = 53,                       /* '.'  */
  YYSYMBOL_54_ = 54,                       /* '['  */
  YYSYMBOL_55_ = 55,                       /* '('  */
  YYSYMBOL_56_ = 56,                       /* '!'  */
  YYSYMBOL_57_ = 57,                       /* '~'  */
  YYSYMBOL_58_ = 58,                       /* ']'  */
  YYSYMBOL_59_ = 59,                       /* ')'  */
  YYSYMBOL_60_ = 60,                       /* '{'  */
  YYSYMBOL_61_ = 61,                       /* '}'  */
  YYSYMBOL_62_ = 62,                       /* ':'  */
  YYSYMBOL_YYACCEPT = 63,                  /* $accept  */
  YYSYMBOL_start = 64,                     /* start  */
  YYSYMBOL_exp1 = 65,                      /* exp1  */
  YYSYMBOL_exp = 66,                       /* exp  */
  YYSYMBOL_67_1 = 67,                      /* $@1  */
  YYSYMBOL_arglist = 68,                   /* arglist  */
  YYSYMBOL_block = 69,                     /* block  */
  YYSYMBOL_variable = 70,                  /* variable  */
  YYSYMBOL_ptype = 71,                     /* ptype  */
  YYSYMBOL_abs_decl = 72,                  /* abs_decl  */
  YYSYMBOL_direct_abs_decl = 73,           /* direct_abs_decl  */
  YYSYMBOL_array_mod = 74,                 /* array_mod  */
  YYSYMBOL_func_mod = 75,                  /* func_mod  */
  YYSYMBOL_type = 76,                      /* type  */
  YYSYMBOL_typebase = 77,                  /* typebase  */
  YYSYMBOL_typename = 78,                  /* typename  */
  YYSYMBOL_nonempty_typelist = 79,         /* nonempty_typelist  */
  YYSYMBOL_name = 80,                      /* name  */
  YYSYMBOL_name_not_typename = 81          /* name_not_typename  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  73
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   566

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  63
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  19
/* YYNRULES -- Number of rules.  */
#define YYNRULES  111
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  187

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   293


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,     2,     2,     2,    48,    34,     2,
      55,    59,    46,    44,    26,    45,    53,    47,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    62,     2,
      37,    28,    38,    29,    43,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    54,     2,    58,    33,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    60,    32,    61,    57,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    27,    30,    31,    35,    36,    39,    40,    41,    42,
      49,    50,    51,    52
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   196,   196,   200,   201,   206,   237,   240,   243,   247,
     251,   255,   259,   263,   267,   271,   275,   281,   285,   291,
     295,   302,   299,   309,   312,   316,   320,   326,   332,   338,
     342,   346,   350,   354,   358,   362,   366,   370,   374,   378,
     382,   386,   390,   394,   398,   402,   406,   410,   414,   418,
     422,   428,   438,   450,   457,   464,   467,   473,   479,   502,
     509,   516,   523,   544,   553,   564,   577,   623,   709,   710,
     767,   769,   771,   774,   776,   781,   787,   789,   793,   795,
     799,   803,   804,   806,   808,   810,   816,   819,   821,   823,
     825,   827,   829,   831,   833,   836,   839,   842,   844,   846,
     849,   853,   854,   859,   864,   872,   877,   884,   885,   886,
     889,   890
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "CHAR", "UINT",
  "FLOAT", "NAME", "TYPENAME", "BLOCKNAME", "STRING", "STRUCT", "UNION",
  "ENUM", "SIZEOF", "UNSIGNED", "COLONCOLON", "SIGNED", "LONG", "SHORT",
  "INT_KEYWORD", "LAST", "REGNAME", "VARIABLE", "ASSIGN_MODIFY", "THIS",
  "','", "ABOVE_COMMA", "'='", "'?'", "OR", "AND", "'|'", "'^'", "'&'",
  "EQUAL", "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "LSH", "RSH", "'@'",
  "'+'", "'-'", "'*'", "'/'", "'%'", "UNARY", "INCREMENT", "DECREMENT",
  "ARROW", "'.'", "'['", "'('", "'!'", "'~'", "']'", "')'", "'{'", "'}'",
  "':'", "$accept", "start", "exp1", "exp", "$@1", "arglist", "block",
  "variable", "ptype", "abs_decl", "direct_abs_decl", "array_mod",
  "func_mod", "type", "typebase", "typename", "nonempty_typelist", "name",
  "name_not_typename", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-106)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-64)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     161,  -106,  -106,  -106,  -106,  -106,  -106,    -6,  -106,   139,
     139,   139,   219,    12,   139,   125,     6,     9,  -106,  -106,
    -106,  -106,  -106,   161,   161,   161,   161,   161,   139,   161,
     161,   161,   111,    34,    18,   346,    19,  -106,    33,  -106,
    -106,  -106,  -106,  -106,  -106,  -106,   161,   511,  -106,    35,
      86,  -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,   511,
     511,   511,   511,   511,  -106,   -22,  -106,     2,     5,   511,
     511,   -28,    40,  -106,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,  -106,  -106,    45,
      81,   161,  -106,   139,   139,    38,  -106,  -106,  -106,   111,
     161,    92,    57,     0,    50,  -106,    61,  -106,  -106,   161,
      62,   346,   346,   346,   311,   398,   271,   421,   443,   464,
     483,   483,   146,   146,   146,   146,   204,   204,   495,   505,
     505,   511,   511,   511,   161,  -106,   161,  -106,   -20,   161,
      94,  -106,   277,   102,   511,  -106,  -106,    63,  -106,  -106,
      76,    77,  -106,  -106,   511,   161,   511,   511,  -106,   346,
      -9,    74,  -106,  -106,   373,   161,  -106,    80,   346,    87,
      28,  -106,    96,    66,   111,  -106,    96
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    51,    53,    52,    54,   110,    86,   111,    60,     0,
       0,     0,     0,    98,     0,   100,    88,    89,    87,    56,
      57,    58,    61,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     2,     3,     0,    55,     0,    67,
     107,   109,   108,    94,    95,    96,     0,    15,   101,   103,
     104,   102,    97,    66,   103,   104,    99,    90,    92,     7,
       8,     6,    11,    12,     5,     0,    81,     0,    68,     9,
      10,     0,    68,     1,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    13,    14,     0,
       0,     0,    21,     0,     0,     0,    91,    93,    28,     0,
       0,     0,    70,     0,     0,    69,    72,    75,    77,     0,
       0,     4,    50,    49,     0,    47,    46,    45,    44,    43,
      37,    38,    41,    42,    39,    40,    35,    36,    29,    33,
      34,    30,    31,    32,     0,    16,     0,    18,     0,    23,
      64,    65,    59,     0,    27,    82,    71,     0,    78,    80,
       0,     0,    74,    76,    26,     0,    17,    19,    20,    24,
       0,     0,    79,    73,    48,     0,    22,     0,    25,    83,
       0,    84,   105,     0,     0,    85,   106
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -106,  -106,     1,   -12,  -106,  -106,  -106,  -106,  -106,  -105,
    -106,    25,    36,   -30,   -24,   134,  -106,    14,  -106
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    33,    65,    35,   149,   170,    36,    37,    66,   115,
     116,   117,   118,    67,    38,    52,   183,   151,    39
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      47,    34,    71,   157,    74,    68,    74,   156,    72,   160,
     -62,    59,    60,    61,    62,    63,   105,   175,    69,    70,
      48,   111,    68,    43,    44,    45,    57,   109,    53,    58,
      49,    50,    51,   119,    73,   103,     6,   108,   168,     9,
      10,    11,    64,    13,    74,    15,    16,    17,    18,   104,
     176,   112,    40,    41,    42,   106,   120,   109,   158,   113,
     114,   110,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   153,   112,   181,    40,    41,
      42,   144,   184,   109,   113,   114,   112,   152,   154,    40,
      41,    42,   148,   112,   113,   114,   107,   164,   155,   159,
     -63,   113,   114,   145,   147,   113,   161,   150,   171,     6,
     177,   172,     9,    10,    11,   185,    13,   146,    15,    16,
      17,    18,   166,    48,   167,   173,   159,   169,   155,   179,
     154,   162,   180,    54,    55,    51,    40,    41,    42,    56,
     182,   109,   163,   174,   186,     0,    72,     0,     0,     0,
      72,     0,     0,   178,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,     0,    22,    89,    90,    91,
      92,    93,    94,    95,    96,    23,    97,    98,    99,   100,
     101,   102,     0,     0,     0,     0,    24,    25,     0,     0,
       0,    26,    27,     0,    28,     0,    29,    30,    31,     0,
       0,    32,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,     0,    22,     0,     0,    91,    92,    93,
      94,    95,    96,    23,    97,    98,    99,   100,   101,   102,
       0,     0,     0,     0,    24,    25,     0,     0,     0,    26,
      27,     0,    28,     0,    46,    30,    31,     0,     0,    32,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,     0,    22,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
       0,    97,    98,    99,   100,   101,   102,    26,    27,     0,
      28,     0,    29,    30,    31,    75,     0,    32,     0,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
       0,    97,    98,    99,   100,   101,   102,     0,     0,     0,
      75,     0,     0,   165,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,     0,    97,    98,    99,   100,
     101,   102,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,     0,    97,    98,    99,   100,   101,   102,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,     0,    97,    98,
      99,   100,   101,   102,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
       0,    97,    98,    99,   100,   101,   102,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,     0,    97,    98,    99,   100,   101,   102,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,     0,    97,    98,    99,   100,   101,   102,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,     0,    97,    98,    99,   100,   101,   102,    92,
      93,    94,    95,    96,     0,    97,    98,    99,   100,   101,
     102,    94,    95,    96,     0,    97,    98,    99,   100,   101,
     102,    97,    98,    99,   100,   101,   102
};

static const yytype_int16 yycheck[] =
{
      12,     0,    32,     3,    26,    29,    26,   112,    32,   114,
      16,    23,    24,    25,    26,    27,    46,    26,    30,    31,
       8,    16,    46,     9,    10,    11,    20,    55,    14,    20,
      18,    19,    20,    61,     0,    16,     8,    59,    58,    11,
      12,    13,    28,    15,    26,    17,    18,    19,    20,    16,
      59,    46,     7,     8,     9,    20,    16,    55,    58,    54,
      55,    59,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,   109,    46,    59,     7,     8,
       9,    46,    26,    55,    54,    55,    46,    59,   110,     7,
       8,     9,   101,    46,    54,    55,    20,   119,    46,    59,
      16,    54,    55,    99,   100,    54,    55,   103,    16,     8,
      46,    58,    11,    12,    13,    59,    15,    46,    17,    18,
      19,    20,   144,     8,   146,    59,    59,   149,    46,    59,
     152,   116,    55,    18,    19,    20,     7,     8,     9,    15,
     180,    55,   116,   165,   184,    -1,   180,    -1,    -1,    -1,
     184,    -1,    -1,   175,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    -1,    25,    41,    42,    43,
      44,    45,    46,    47,    48,    34,    50,    51,    52,    53,
      54,    55,    -1,    -1,    -1,    -1,    45,    46,    -1,    -1,
      -1,    50,    51,    -1,    53,    -1,    55,    56,    57,    -1,
      -1,    60,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    -1,    25,    -1,    -1,    43,    44,    45,
      46,    47,    48,    34,    50,    51,    52,    53,    54,    55,
      -1,    -1,    -1,    -1,    45,    46,    -1,    -1,    -1,    50,
      51,    -1,    53,    -1,    55,    56,    57,    -1,    -1,    60,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    -1,    25,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    51,    52,    53,    54,    55,    50,    51,    -1,
      53,    -1,    55,    56,    57,    24,    -1,    60,    -1,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    51,    52,    53,    54,    55,    -1,    -1,    -1,
      24,    -1,    -1,    62,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    -1,    50,    51,    52,    53,
      54,    55,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    51,    52,    53,    54,    55,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    50,    51,
      52,    53,    54,    55,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,    51,    52,    53,    54,    55,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    51,    52,    53,    54,    55,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    50,    51,    52,    53,    54,    55,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    50,    51,    52,    53,    54,    55,    44,
      45,    46,    47,    48,    -1,    50,    51,    52,    53,    54,
      55,    46,    47,    48,    -1,    50,    51,    52,    53,    54,
      55,    50,    51,    52,    53,    54,    55
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    25,    34,    45,    46,    50,    51,    53,    55,
      56,    57,    60,    64,    65,    66,    69,    70,    77,    81,
       7,     8,     9,    80,    80,    80,    55,    66,     8,    18,
      19,    20,    78,    80,    18,    19,    78,    20,    20,    66,
      66,    66,    66,    66,    80,    65,    71,    76,    77,    66,
      66,    76,    77,     0,    26,    24,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    50,    51,    52,
      53,    54,    55,    16,    16,    76,    20,    20,    59,    55,
      59,    16,    46,    54,    55,    72,    73,    74,    75,    61,
      16,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    46,    80,    46,    80,    65,    67,
      80,    80,    59,    77,    66,    46,    72,     3,    58,    59,
      72,    55,    74,    75,    66,    62,    66,    66,    58,    66,
      68,    16,    58,    59,    66,    26,    59,    46,    66,    59,
      55,    59,    76,    79,    26,    59,    76
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    63,    64,    65,    65,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    67,    66,    68,    68,    68,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    69,    69,    70,    70,    70,    70,    71,    71,
      72,    72,    72,    73,    73,    73,    73,    73,    74,    74,
      75,    76,    76,    76,    76,    76,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    78,    78,    78,    78,    79,    79,    80,    80,    80,
      81,    81
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     3,     4,     3,     4,
       4,     0,     5,     0,     1,     3,     4,     4,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     5,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       1,     1,     1,     3,     3,     3,     2,     1,     1,     2,
       1,     2,     1,     3,     2,     1,     2,     1,     2,     3,
       2,     1,     3,     6,     8,     9,     1,     1,     1,     1,
       2,     3,     2,     3,     2,     2,     2,     2,     1,     2,
       1,     1,     1,     1,     1,     1,     3,     1,     1,     1,
       1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 4: /* exp1: exp1 ',' exp  */
#line 202 "expread.y"
                        { write_exp_elt_opcode (BINOP_COMMA); }
#line 1569 "y.tab.c"
    break;

  case 5: /* exp: '.' name  */
#line 207 "expread.y"
                        { struct symbol *sym;
			  int is_a_field_of_this;

			  sym = lookup_symbol ("BLNK_",
					       expression_context_block,
					       VAR_NAMESPACE,
					       &is_a_field_of_this);
			  if (sym)
			    {
			      switch (sym->class)
				{
				case LOC_REGISTER:
				case LOC_ARG:
				case LOC_LOCAL:
				  if (innermost_block == 0 ||
				      contained_in (block_found,
						    innermost_block))
				    innermost_block = block_found;
				}
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			    }
			  else
			      ui_badnews(-1, "blank common not found");
			  write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string ((yyvsp[0].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
#line 1602 "y.tab.c"
    break;

  case 6: /* exp: '*' exp  */
#line 238 "expread.y"
                        { write_exp_elt_opcode (UNOP_IND); }
#line 1608 "y.tab.c"
    break;

  case 7: /* exp: '&' exp  */
#line 241 "expread.y"
                        { write_exp_elt_opcode (UNOP_ADDR); }
#line 1614 "y.tab.c"
    break;

  case 8: /* exp: '-' exp  */
#line 244 "expread.y"
                        { write_exp_elt_opcode (UNOP_NEG); }
#line 1620 "y.tab.c"
    break;

  case 9: /* exp: '!' exp  */
#line 248 "expread.y"
                        { write_exp_elt_opcode (UNOP_ZEROP); }
#line 1626 "y.tab.c"
    break;

  case 10: /* exp: '~' exp  */
#line 252 "expread.y"
                        { write_exp_elt_opcode (UNOP_LOGNOT); }
#line 1632 "y.tab.c"
    break;

  case 11: /* exp: INCREMENT exp  */
#line 256 "expread.y"
                        { write_exp_elt_opcode (UNOP_PREINCREMENT); }
#line 1638 "y.tab.c"
    break;

  case 12: /* exp: DECREMENT exp  */
#line 260 "expread.y"
                        { write_exp_elt_opcode (UNOP_PREDECREMENT); }
#line 1644 "y.tab.c"
    break;

  case 13: /* exp: exp INCREMENT  */
#line 264 "expread.y"
                        { write_exp_elt_opcode (UNOP_POSTINCREMENT); }
#line 1650 "y.tab.c"
    break;

  case 14: /* exp: exp DECREMENT  */
#line 268 "expread.y"
                        { write_exp_elt_opcode (UNOP_POSTDECREMENT); }
#line 1656 "y.tab.c"
    break;

  case 15: /* exp: SIZEOF exp  */
#line 272 "expread.y"
                        { write_exp_elt_opcode (UNOP_SIZEOF); }
#line 1662 "y.tab.c"
    break;

  case 16: /* exp: exp ARROW name  */
#line 276 "expread.y"
                        { write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string ((yyvsp[0].sval));
			  write_exp_elt_opcode (STRUCTOP_PTR); }
#line 1670 "y.tab.c"
    break;

  case 17: /* exp: exp ARROW '*' exp  */
#line 282 "expread.y"
                        { write_exp_elt_opcode (STRUCTOP_MPTR); }
#line 1676 "y.tab.c"
    break;

  case 18: /* exp: exp '.' name  */
#line 286 "expread.y"
                        { write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ((yyvsp[0].sval));
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
#line 1684 "y.tab.c"
    break;

  case 19: /* exp: exp '.' '*' exp  */
#line 292 "expread.y"
                        { write_exp_elt_opcode (STRUCTOP_MEMBER); }
#line 1690 "y.tab.c"
    break;

  case 20: /* exp: exp '[' exp1 ']'  */
#line 296 "expread.y"
                        { write_exp_elt_opcode (BINOP_SUBSCRIPT); }
#line 1696 "y.tab.c"
    break;

  case 21: /* $@1: %empty  */
#line 302 "expread.y"
                        { start_arglist (); }
#line 1702 "y.tab.c"
    break;

  case 22: /* exp: exp '(' $@1 arglist ')'  */
#line 304 "expread.y"
                        { write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); }
#line 1710 "y.tab.c"
    break;

  case 24: /* arglist: exp  */
#line 313 "expread.y"
                        { arglist_len = 1; }
#line 1716 "y.tab.c"
    break;

  case 25: /* arglist: arglist ',' exp  */
#line 317 "expread.y"
                        { arglist_len++; }
#line 1722 "y.tab.c"
    break;

  case 26: /* exp: '{' type '}' exp  */
#line 321 "expread.y"
                        { write_exp_elt_opcode (UNOP_MEMVAL);
			  write_exp_elt_type ((yyvsp[-2].tval));
			  write_exp_elt_opcode (UNOP_MEMVAL); }
#line 1730 "y.tab.c"
    break;

  case 27: /* exp: '(' type ')' exp  */
#line 327 "expread.y"
                        { write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type ((yyvsp[-2].tval));
			  write_exp_elt_opcode (UNOP_CAST); }
#line 1738 "y.tab.c"
    break;

  case 28: /* exp: '(' exp1 ')'  */
#line 333 "expread.y"
                        { }
#line 1744 "y.tab.c"
    break;

  case 29: /* exp: exp '@' exp  */
#line 339 "expread.y"
                        { write_exp_elt_opcode (BINOP_REPEAT); }
#line 1750 "y.tab.c"
    break;

  case 30: /* exp: exp '*' exp  */
#line 343 "expread.y"
                        { write_exp_elt_opcode (BINOP_MUL); }
#line 1756 "y.tab.c"
    break;

  case 31: /* exp: exp '/' exp  */
#line 347 "expread.y"
                        { write_exp_elt_opcode (BINOP_DIV); }
#line 1762 "y.tab.c"
    break;

  case 32: /* exp: exp '%' exp  */
#line 351 "expread.y"
                        { write_exp_elt_opcode (BINOP_REM); }
#line 1768 "y.tab.c"
    break;

  case 33: /* exp: exp '+' exp  */
#line 355 "expread.y"
                        { write_exp_elt_opcode (BINOP_ADD); }
#line 1774 "y.tab.c"
    break;

  case 34: /* exp: exp '-' exp  */
#line 359 "expread.y"
                        { write_exp_elt_opcode (BINOP_SUB); }
#line 1780 "y.tab.c"
    break;

  case 35: /* exp: exp LSH exp  */
#line 363 "expread.y"
                        { write_exp_elt_opcode (BINOP_LSH); }
#line 1786 "y.tab.c"
    break;

  case 36: /* exp: exp RSH exp  */
#line 367 "expread.y"
                        { write_exp_elt_opcode (BINOP_RSH); }
#line 1792 "y.tab.c"
    break;

  case 37: /* exp: exp EQUAL exp  */
#line 371 "expread.y"
                        { write_exp_elt_opcode (BINOP_EQUAL); }
#line 1798 "y.tab.c"
    break;

  case 38: /* exp: exp NOTEQUAL exp  */
#line 375 "expread.y"
                        { write_exp_elt_opcode (BINOP_NOTEQUAL); }
#line 1804 "y.tab.c"
    break;

  case 39: /* exp: exp LEQ exp  */
#line 379 "expread.y"
                        { write_exp_elt_opcode (BINOP_LEQ); }
#line 1810 "y.tab.c"
    break;

  case 40: /* exp: exp GEQ exp  */
#line 383 "expread.y"
                        { write_exp_elt_opcode (BINOP_GEQ); }
#line 1816 "y.tab.c"
    break;

  case 41: /* exp: exp '<' exp  */
#line 387 "expread.y"
                        { write_exp_elt_opcode (BINOP_LESS); }
#line 1822 "y.tab.c"
    break;

  case 42: /* exp: exp '>' exp  */
#line 391 "expread.y"
                        { write_exp_elt_opcode (BINOP_GTR); }
#line 1828 "y.tab.c"
    break;

  case 43: /* exp: exp '&' exp  */
#line 395 "expread.y"
                        { write_exp_elt_opcode (BINOP_LOGAND); }
#line 1834 "y.tab.c"
    break;

  case 44: /* exp: exp '^' exp  */
#line 399 "expread.y"
                        { write_exp_elt_opcode (BINOP_LOGXOR); }
#line 1840 "y.tab.c"
    break;

  case 45: /* exp: exp '|' exp  */
#line 403 "expread.y"
                        { write_exp_elt_opcode (BINOP_LOGIOR); }
#line 1846 "y.tab.c"
    break;

  case 46: /* exp: exp AND exp  */
#line 407 "expread.y"
                        { write_exp_elt_opcode (BINOP_AND); }
#line 1852 "y.tab.c"
    break;

  case 47: /* exp: exp OR exp  */
#line 411 "expread.y"
                        { write_exp_elt_opcode (BINOP_OR); }
#line 1858 "y.tab.c"
    break;

  case 48: /* exp: exp '?' exp ':' exp  */
#line 415 "expread.y"
                        { write_exp_elt_opcode (TERNOP_COND); }
#line 1864 "y.tab.c"
    break;

  case 49: /* exp: exp '=' exp  */
#line 419 "expread.y"
                        { write_exp_elt_opcode (BINOP_ASSIGN); }
#line 1870 "y.tab.c"
    break;

  case 50: /* exp: exp ASSIGN_MODIFY exp  */
#line 423 "expread.y"
                        { write_exp_elt_opcode (BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode ((yyvsp[-1].opcode));
			  write_exp_elt_opcode (BINOP_ASSIGN_MODIFY); }
#line 1878 "y.tab.c"
    break;

  case 51: /* exp: INT  */
#line 429 "expread.y"
                        { write_exp_elt_opcode (OP_LONG);
			  if ((yyvsp[0].lval) == (int) (yyvsp[0].lval) || (yyvsp[0].lval) == (unsigned int) (yyvsp[0].lval))
			    write_exp_elt_type (builtin_type_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_LONGEST);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (OP_LONG); }
#line 1890 "y.tab.c"
    break;

  case 52: /* exp: UINT  */
#line 439 "expread.y"
                        {
			  write_exp_elt_opcode (OP_LONG);
			  if ((yyvsp[0].ulval) == (unsigned int) (yyvsp[0].ulval))
			    write_exp_elt_type (builtin_type_unsigned_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_UNSIGNED_LONGEST);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[0].ulval));
			  write_exp_elt_opcode (OP_LONG);
			}
#line 1904 "y.tab.c"
    break;

  case 53: /* exp: CHAR  */
#line 451 "expread.y"
                        { write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_char);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (OP_LONG); }
#line 1913 "y.tab.c"
    break;

  case 54: /* exp: FLOAT  */
#line 458 "expread.y"
                        { write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type (builtin_type_double);
			  write_exp_elt_dblcst ((yyvsp[0].dval));
			  write_exp_elt_opcode (OP_DOUBLE); }
#line 1922 "y.tab.c"
    break;

  case 56: /* exp: LAST  */
#line 468 "expread.y"
                        { write_exp_elt_opcode (OP_LAST);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (OP_LAST); }
#line 1930 "y.tab.c"
    break;

  case 57: /* exp: REGNAME  */
#line 474 "expread.y"
                        { write_exp_elt_opcode (OP_REGISTER);
			  write_exp_elt_longcst ((LONGEST) (yyvsp[0].lval));
			  write_exp_elt_opcode (OP_REGISTER); }
#line 1938 "y.tab.c"
    break;

  case 58: /* exp: VARIABLE  */
#line 480 "expread.y"
                        { write_exp_elt_opcode (OP_INTERNALVAR);
			  write_exp_elt_intern ((yyvsp[0].ivar));
			  write_exp_elt_opcode (OP_INTERNALVAR); 
#ifdef GHSFORTRAN
/************
	* 
	* We may want to try this again later.  As it stands now we
	* must use p *$t when writing out an entire temporary fortran array 
	* variable whereas p $t[2][3] works when writing out a single element
 	*
			  blk = get_current_block();

			  if (blk->language == language_fortran)
			if (TYPE_CODE(VALUE_TYPE($1->value)) == TYPE_CODE_PTR)
if ((TYPE_CODE(TYPE_TARGET_TYPE(VALUE_TYPE($1->value))) == TYPE_CODE_ARRAY) ||
(TYPE_CODE(TYPE_TARGET_TYPE(VALUE_TYPE($1->value))) == TYPE_CODE_FORTRAN_ARRAY))
				   write_exp_elt_opcode (UNOP_IND);
***************/
#endif
			}
#line 1963 "y.tab.c"
    break;

  case 59: /* exp: SIZEOF '(' type ')'  */
#line 503 "expread.y"
                        { write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_int);
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH ((yyvsp[-1].tval)));
			  write_exp_elt_opcode (OP_LONG); }
#line 1972 "y.tab.c"
    break;

  case 60: /* exp: STRING  */
#line 510 "expread.y"
                        { write_exp_elt_opcode (OP_STRING);
			  write_exp_string ((yyvsp[0].sval));
			  write_exp_elt_opcode (OP_STRING); }
#line 1980 "y.tab.c"
    break;

  case 61: /* exp: THIS  */
#line 517 "expread.y"
                        { write_exp_elt_opcode (OP_THIS);
			  write_exp_elt_opcode (OP_THIS); }
#line 1987 "y.tab.c"
    break;

  case 62: /* block: BLOCKNAME  */
#line 524 "expread.y"
                        {
			  struct symtab *tem = lookup_symtab (copy_name ((yyvsp[0].sval)));
			  struct symbol *sym;
			  
			  if (tem)
			    (yyval.bval) = BLOCKVECTOR_BLOCK (BLOCKVECTOR (tem), 1);
			  else
			    {
			      sym = lookup_symbol (copy_name ((yyvsp[0].sval)),
						   expression_context_block,
						   VAR_NAMESPACE, 0);
			      if (sym && SYMBOL_CLASS (sym) == LOC_BLOCK)
				(yyval.bval) = SYMBOL_BLOCK_VALUE (sym);
			      else
				ui_badnews(-1,"No file or function \"%s\".",
				       copy_name ((yyvsp[0].sval)));
			    }
			}
#line 2010 "y.tab.c"
    break;

  case 63: /* block: block COLONCOLON name  */
#line 545 "expread.y"
                        { struct symbol *tem
			    = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval), VAR_NAMESPACE, 0);
			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    ui_badnews(-1,"No function \"%s\" in specified context.",
				   copy_name ((yyvsp[0].sval)));
			  (yyval.bval) = SYMBOL_BLOCK_VALUE (tem); }
#line 2021 "y.tab.c"
    break;

  case 64: /* variable: block COLONCOLON name  */
#line 554 "expread.y"
                        { struct symbol *sym;
			  sym = lookup_symbol (copy_name ((yyvsp[0].sval)), (yyvsp[-2].bval), VAR_NAMESPACE, 0);
			  if (sym == 0)
			    ui_badnews(-1,"No symbol \"%s\" in specified context.",
				   copy_name ((yyvsp[0].sval)));
			  write_exp_elt_opcode (OP_VAR_VALUE);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_VALUE); }
#line 2034 "y.tab.c"
    break;

  case 65: /* variable: typebase COLONCOLON name  */
#line 565 "expread.y"
                        {
			  struct type *type = (yyvsp[-2].tval);
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION)
			    ui_badnews(-1,"`%s' is not defined as an aggregate type.",
				   TYPE_NAME (type));

			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string ((yyvsp[0].sval));
			  write_exp_elt_opcode (OP_SCOPE);
			}
#line 2051 "y.tab.c"
    break;

  case 66: /* variable: COLONCOLON name  */
#line 578 "expread.y"
                        {
			  char *name = copy_name ((yyvsp[0].sval));
			  struct symbol *sym;
			  int i;

			  sym = lookup_symbol (name, 0, VAR_NAMESPACE, 0);
			  if (sym)
			    {
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      break;
			    }
			  for (i = 0; i < misc_function_count; i++)
			    if (!strcmp (misc_function_vector[i].name, name))
			      break;

			  if (i < misc_function_count)
			    {
			      enum misc_function_type mft =
				(enum misc_function_type)
				  misc_function_vector[i].type;
			      
			      write_exp_elt_opcode (OP_LONG);
			      write_exp_elt_type (builtin_type_int);
			      write_exp_elt_longcst ((LONGEST) misc_function_vector[i].address);
			      write_exp_elt_opcode (OP_LONG);
			      write_exp_elt_opcode (UNOP_MEMVAL);
			      if (mft == mf_data || mft == mf_bss)
				write_exp_elt_type (builtin_type_int);
			      else if (mft == mf_text)
				write_exp_elt_type (lookup_function_type (builtin_type_int));
			      else
				write_exp_elt_type (builtin_type_char);
			      write_exp_elt_opcode (UNOP_MEMVAL);
			    }
			  else
			    if (symtab_list == 0
				&& partial_symtab_list == 0)
			      ui_badnews(-1,"No symbol table is loaded.  Use the \"symbol-file\" command.");
			    else
			      ui_badnews(-1,"No symbol \"%s\" in current context.", name);
			}
#line 2099 "y.tab.c"
    break;

  case 67: /* variable: name_not_typename  */
#line 624 "expread.y"
                        { struct symbol *sym;
			  int is_a_field_of_this;

			  sym = lookup_symbol (copy_name ((yyvsp[0].sval)),
					       expression_context_block,
					       VAR_NAMESPACE,
					       &is_a_field_of_this);
			  if (sym)
			    {
			      switch (sym->class)
				{
				case LOC_REGISTER:
				case LOC_ARG:
				case LOC_LOCAL:
				  if (innermost_block == 0 ||
				      contained_in (block_found, 
						    innermost_block))
				    innermost_block = block_found;
				}
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
#if GHSFORTRAN
			      blk = get_current_block();

			      if (blk && blk->language == language_fortran)
			      {
				if (TYPE_CODE(SYMBOL_TYPE(sym)) ==TYPE_CODE_PTR)
				   write_exp_elt_opcode (UNOP_IND);
			      }
#endif
			    }
			  else if (is_a_field_of_this)
			    {
			      /* C++: it hangs off of `this'.  Must
			         not inadvertently convert from a method call
				 to data ref.  */
			      if (innermost_block == 0 || 
				  contained_in (block_found, innermost_block))
				innermost_block = block_found;
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			      write_exp_string ((yyvsp[0].sval));
			      write_exp_elt_opcode (STRUCTOP_PTR);
			    }
			  else
			    {
			      register int i;
			      register char *arg = copy_name ((yyvsp[0].sval));

			      for (i = 0; i < misc_function_count; i++)
				if (!strcmp (misc_function_vector[i].name, arg))
				  break;

			      if (i < misc_function_count)
				{
				  enum misc_function_type mft =
				    (enum misc_function_type)
				      misc_function_vector[i].type;
				  
				  write_exp_elt_opcode (OP_LONG);
				  write_exp_elt_type (builtin_type_int);
				  write_exp_elt_longcst ((LONGEST) misc_function_vector[i].address);
				  write_exp_elt_opcode (OP_LONG);
				  write_exp_elt_opcode (UNOP_MEMVAL);
				  if (mft == mf_data || mft == mf_bss)
				    write_exp_elt_type (builtin_type_int);
				  else if (mft == mf_text)
				    write_exp_elt_type (lookup_function_type (builtin_type_int));
				  else
				    write_exp_elt_type (builtin_type_char);
				  write_exp_elt_opcode (UNOP_MEMVAL);
				}
			      else if (symtab_list == 0
				       && partial_symtab_list == 0)
				ui_badnews(-1,"No symbol table is loaded.  Use the \"symbol-file\" command.");
			      else
				ui_badnews(-1,"No symbol \"%s\" in current context.",
				       copy_name ((yyvsp[0].sval)));
			    }
			}
#line 2186 "y.tab.c"
    break;

  case 69: /* ptype: typebase abs_decl  */
#line 711 "expread.y"
                {
		  /* This is where the interesting stuff happens.  */
		  int done = 0;
		  int array_size;
		  struct type *follow_type = (yyvsp[-1].tval);
#ifdef GHSFORTRAN
		  struct type *base_type = (yyvsp[-1].tval);
		  int base_size = 1;
#endif
		  
		  while (!done)
		    switch (pop_type ())
		      {
		      case tp_end:
			done = 1;
			break;
		      case tp_pointer:
#ifdef GHSFORTRAN
			base_type = follow_type = lookup_pointer_type (follow_type);
#else
			follow_type = lookup_pointer_type (follow_type);
#endif
			break;
		      case tp_reference:
			follow_type = lookup_reference_type (follow_type);
			break;
		      case tp_array:
			array_size = (int) pop_type ();
			if (array_size != -1)
#ifdef GHSFORTRAN
{
			  follow_type = create_array_type (follow_type,
							   array_size);
			  if (TYPE_CODE(follow_type) == TYPE_CODE_FORTRAN_ARRAY)
			  {
			  	TYPE_BACK_TYPE(follow_type) = base_type;
			  	TYPE_LENGTH(follow_type) = 
				   TYPE_LENGTH(base_type);
				fix_fortran_lengths(follow_type, array_size);
			  }
}
#else
			  follow_type = create_array_type (follow_type,
							   array_size);
#endif
			else
			  follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_function:
			follow_type = lookup_function_type (follow_type);
			break;
		      }
		  (yyval.tval) = follow_type;
		}
#line 2245 "y.tab.c"
    break;

  case 70: /* abs_decl: '*'  */
#line 768 "expread.y"
                        { push_type (tp_pointer); (yyval.voidval) = 0; }
#line 2251 "y.tab.c"
    break;

  case 71: /* abs_decl: '*' abs_decl  */
#line 770 "expread.y"
                        { push_type (tp_pointer); (yyval.voidval) = (yyvsp[0].voidval); }
#line 2257 "y.tab.c"
    break;

  case 73: /* direct_abs_decl: '(' abs_decl ')'  */
#line 775 "expread.y"
                        { (yyval.voidval) = (yyvsp[-1].voidval); }
#line 2263 "y.tab.c"
    break;

  case 74: /* direct_abs_decl: direct_abs_decl array_mod  */
#line 777 "expread.y"
                        {
			  push_type ((enum type_pieces) (yyvsp[0].lval));
			  push_type (tp_array);
			}
#line 2272 "y.tab.c"
    break;

  case 75: /* direct_abs_decl: array_mod  */
#line 782 "expread.y"
                        {
			  push_type ((enum type_pieces) (yyvsp[0].lval));
			  push_type (tp_array);
			  (yyval.voidval) = 0;
			}
#line 2282 "y.tab.c"
    break;

  case 76: /* direct_abs_decl: direct_abs_decl func_mod  */
#line 788 "expread.y"
                        { push_type (tp_function); }
#line 2288 "y.tab.c"
    break;

  case 77: /* direct_abs_decl: func_mod  */
#line 790 "expread.y"
                        { push_type (tp_function); }
#line 2294 "y.tab.c"
    break;

  case 78: /* array_mod: '[' ']'  */
#line 794 "expread.y"
                        { (yyval.lval) = -1; }
#line 2300 "y.tab.c"
    break;

  case 79: /* array_mod: '[' INT ']'  */
#line 796 "expread.y"
                        { (yyval.lval) = (yyvsp[-1].lval); }
#line 2306 "y.tab.c"
    break;

  case 80: /* func_mod: '(' ')'  */
#line 800 "expread.y"
                        { (yyval.voidval) = 0; }
#line 2312 "y.tab.c"
    break;

  case 82: /* type: typebase COLONCOLON '*'  */
#line 805 "expread.y"
                        { (yyval.tval) = lookup_member_type (builtin_type_int, (yyvsp[-2].tval)); }
#line 2318 "y.tab.c"
    break;

  case 83: /* type: type '(' typebase COLONCOLON '*' ')'  */
#line 807 "expread.y"
                        { (yyval.tval) = lookup_member_type ((yyvsp[-5].tval), (yyvsp[-3].tval)); }
#line 2324 "y.tab.c"
    break;

  case 84: /* type: type '(' typebase COLONCOLON '*' ')' '(' ')'  */
#line 809 "expread.y"
                        { (yyval.tval) = lookup_member_type (lookup_function_type ((yyvsp[-7].tval))); }
#line 2330 "y.tab.c"
    break;

  case 85: /* type: type '(' typebase COLONCOLON '*' ')' '(' nonempty_typelist ')'  */
#line 811 "expread.y"
                        { (yyval.tval) = lookup_member_type (lookup_function_type ((yyvsp[-8].tval)));
			  free ((yyvsp[-1].tvec)); }
#line 2337 "y.tab.c"
    break;

  case 86: /* typebase: TYPENAME  */
#line 817 "expread.y"
                        { (yyval.tval) = lookup_typename (copy_name ((yyvsp[0].sval)),
						expression_context_block, 0); }
#line 2344 "y.tab.c"
    break;

  case 87: /* typebase: INT_KEYWORD  */
#line 820 "expread.y"
                        { (yyval.tval) = builtin_type_int; }
#line 2350 "y.tab.c"
    break;

  case 88: /* typebase: LONG  */
#line 822 "expread.y"
                        { (yyval.tval) = builtin_type_long; }
#line 2356 "y.tab.c"
    break;

  case 89: /* typebase: SHORT  */
#line 824 "expread.y"
                        { (yyval.tval) = builtin_type_short; }
#line 2362 "y.tab.c"
    break;

  case 90: /* typebase: LONG INT_KEYWORD  */
#line 826 "expread.y"
                        { (yyval.tval) = builtin_type_long; }
#line 2368 "y.tab.c"
    break;

  case 91: /* typebase: UNSIGNED LONG INT_KEYWORD  */
#line 828 "expread.y"
                        { (yyval.tval) = builtin_type_unsigned_long; }
#line 2374 "y.tab.c"
    break;

  case 92: /* typebase: SHORT INT_KEYWORD  */
#line 830 "expread.y"
                        { (yyval.tval) = builtin_type_short; }
#line 2380 "y.tab.c"
    break;

  case 93: /* typebase: UNSIGNED SHORT INT_KEYWORD  */
#line 832 "expread.y"
                        { (yyval.tval) = builtin_type_unsigned_short; }
#line 2386 "y.tab.c"
    break;

  case 94: /* typebase: STRUCT name  */
#line 834 "expread.y"
                        { (yyval.tval) = lookup_struct (copy_name ((yyvsp[0].sval)),
					      expression_context_block); }
#line 2393 "y.tab.c"
    break;

  case 95: /* typebase: UNION name  */
#line 837 "expread.y"
                        { (yyval.tval) = lookup_union (copy_name ((yyvsp[0].sval)),
					     expression_context_block); }
#line 2400 "y.tab.c"
    break;

  case 96: /* typebase: ENUM name  */
#line 840 "expread.y"
                        { (yyval.tval) = lookup_enum (copy_name ((yyvsp[0].sval)),
					    expression_context_block); }
#line 2407 "y.tab.c"
    break;

  case 97: /* typebase: UNSIGNED typename  */
#line 843 "expread.y"
                        { (yyval.tval) = lookup_unsigned_typename (copy_name ((yyvsp[0].sval))); }
#line 2413 "y.tab.c"
    break;

  case 98: /* typebase: UNSIGNED  */
#line 845 "expread.y"
                        { (yyval.tval) = builtin_type_unsigned_int; }
#line 2419 "y.tab.c"
    break;

  case 99: /* typebase: SIGNED typename  */
#line 847 "expread.y"
                        { (yyval.tval) = lookup_typename (copy_name ((yyvsp[0].sval)),
						expression_context_block, 0); }
#line 2426 "y.tab.c"
    break;

  case 100: /* typebase: SIGNED  */
#line 850 "expread.y"
                        { (yyval.tval) = builtin_type_int; }
#line 2432 "y.tab.c"
    break;

  case 102: /* typename: INT_KEYWORD  */
#line 855 "expread.y"
                {
		  (yyval.sval).ptr = "int";
		  (yyval.sval).length = 3;
		}
#line 2441 "y.tab.c"
    break;

  case 103: /* typename: LONG  */
#line 860 "expread.y"
                {
		  (yyval.sval).ptr = "long";
		  (yyval.sval).length = 4;
		}
#line 2450 "y.tab.c"
    break;

  case 104: /* typename: SHORT  */
#line 865 "expread.y"
                {
		  (yyval.sval).ptr = "short";
		  (yyval.sval).length = 5;
		}
#line 2459 "y.tab.c"
    break;

  case 105: /* nonempty_typelist: type  */
#line 873 "expread.y"
                { (yyval.tvec) = (struct type **)xmalloc (sizeof (struct type *) * 2);
		  (yyval.tvec)[0] = (struct type *)0;
		  (yyval.tvec)[1] = (yyvsp[0].tval);
		}
#line 2468 "y.tab.c"
    break;

  case 106: /* nonempty_typelist: nonempty_typelist ',' type  */
#line 878 "expread.y"
                { int len = sizeof (struct type *) * ++((yyvsp[-2].ivec)[0]);
		  (yyval.tvec) = (struct type **)xrealloc ((yyvsp[-2].tvec), len);
		  (yyval.tvec)[(yyval.ivec)[0]] = (yyvsp[0].tval);
		}
#line 2477 "y.tab.c"
    break;


#line 2481 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 893 "expread.y"


/* Begin counting arguments for a function call,
   saving the data about any containing call.  */

static void
start_arglist ()
{
  register struct funcall *new = (struct funcall *) xmalloc (sizeof (struct funcall));

  new->next = funcall_chain;
  new->arglist_len = arglist_len;
  arglist_len = 0;
  funcall_chain = new;
}

/* Return the number of arguments in a function call just terminated,
   and restore the data for the containing function call.  */

static int
end_arglist ()
{
  register int val = arglist_len;
  register struct funcall *call = funcall_chain;
  funcall_chain = call->next;
  arglist_len = call->arglist_len;
  free (call);
  return val;
}

/* Free everything in the funcall chain.
   Used when there is an error inside parsing.  */

static void
free_funcalls ()
{
  register struct funcall *call, *next;

  for (call = funcall_chain; call; call = next)
    {
      next = call->next;
      free (call);
    }
}

/* This page contains the functions for adding data to the  struct expression
   being constructed.  */

/* Add one element to the end of the expression.  */

/* To avoid a bug in the Sun 4 compiler, we pass things that can fit into
   a register through here */

static void
write_exp_elt (expelt)
     union exp_element expelt;
{
  if (expout_ptr >= expout_size)
    {
      expout_size *= 2;
      expout = (struct expression *) xrealloc (expout,
					       sizeof (struct expression)
					       + expout_size * sizeof (union exp_element));
    }
  expout->elts[expout_ptr++] = expelt;
}

static void
write_exp_elt_opcode (expelt)
     enum exp_opcode expelt;
{
  union exp_element tmp;

  tmp.opcode = expelt;

  write_exp_elt (tmp);
}

static void
write_exp_elt_sym (expelt)
     struct symbol *expelt;
{
  union exp_element tmp;

  tmp.symbol = expelt;

  write_exp_elt (tmp);
}

static void
write_exp_elt_longcst (expelt)
     LONGEST expelt;
{
  union exp_element tmp;

  tmp.longconst = expelt;

  write_exp_elt (tmp);
}

static void
write_exp_elt_dblcst (expelt)
     double expelt;
{
  union exp_element tmp;

  tmp.doubleconst = expelt;

  write_exp_elt (tmp);
}

static void
write_exp_elt_type (expelt)
     struct type *expelt;
{
  union exp_element tmp;

  tmp.type = expelt;

  write_exp_elt (tmp);
}

static void
write_exp_elt_intern (expelt)
     struct internalvar *expelt;
{
  union exp_element tmp;

  tmp.internalvar = expelt;

  write_exp_elt (tmp);
}

/* Add a string constant to the end of the expression.
   Follow it by its length in bytes, as a separate exp_element.  */

static void
write_exp_string (str)
     struct stoken str;
{
  register int len = str.length;
  register int lenelt
    = (len + sizeof (union exp_element)) / sizeof (union exp_element);

  expout_ptr += lenelt;

  if (expout_ptr >= expout_size)
    {
      expout_size = max (expout_size * 2, expout_ptr + 10);
      expout = (struct expression *)
	xrealloc (expout, (sizeof (struct expression)
			   + (expout_size * sizeof (union exp_element))));
    }
  bcopy (str.ptr, (char *) &expout->elts[expout_ptr - lenelt], len);
  ((char *) &expout->elts[expout_ptr - lenelt])[len] = 0;
  write_exp_elt_longcst ((LONGEST) len);
}

/* During parsing of a C expression, the pointer to the next character
   is in this variable.  */

static char *lexptr;

/* Tokens that refer to names do so with explicit pointer and length,
   so they can share the storage that lexptr is parsing.

   When it is necessary to pass a name to a function that expects
   a null-terminated string, the substring is copied out
   into a block of storage that namecopy points to.

   namecopy is allocated once, guaranteed big enough, for each parsing.  */

static char *namecopy;

/* Current depth in parentheses within the expression.  */

static int paren_depth;

/* Nonzero means stop parsing on first comma (if not within parentheses).  */

static int comma_terminates;

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (olen)
     int olen;
{
  register char *p = lexptr;
  register LONGEST n = 0;
  register int c;
  register int base = 10;
  register int len = olen;
  char *err_copy;
  int unsigned_p = 0;

  extern double atof ();

  for (c = 0; c < len; c++)
    if (p[c] == '.')
      {
	/* It's a float since it contains a point.  */
	yylval.dval = atof (p);
	lexptr += len;
	return FLOAT;
      }

  if (len >= 3 && (!strncmp (p, "0x", 2) || !strncmp (p, "0X", 2)))
    {
      p += 2;
      base = 16;
      len -= 2;
    }
  else if (*p == '0')
    base = 8;

  while (len-- > 0)
    {
      c = *p++;
      if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
      if (c != 'l' && c != 'u')
	n *= base;
      if (c >= '0' && c <= '9')
	n += c - '0';
      else
	{
	  if (base == 16 && c >= 'a' && c <= 'f')
	    n += c - 'a' + 10;
	  else if (len == 0 && c == 'l')
	    ;
	  else if (len == 0 && c == 'u')
	    unsigned_p = 1;
	  else
	    {
	      err_copy = (char *) alloca (olen + 1);
	      bcopy (lexptr, err_copy, olen);
	      err_copy[olen] = 0;
	      ui_badnews(-1,"Invalid number \"%s\".", err_copy);
	    }
	}
    }

  lexptr = p;
  if (unsigned_p)
    {
      yylval.ulval = n;
      return UINT;
    }
  else
    {
      yylval.lval = n;
      return INT;
    }
}

struct token
{
  char *operator;
  int token;
  enum exp_opcode opcode;
};

static struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH}
  };

static struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD},
    {"-=", ASSIGN_MODIFY, BINOP_SUB},
    {"*=", ASSIGN_MODIFY, BINOP_MUL},
    {"/=", ASSIGN_MODIFY, BINOP_DIV},
    {"%=", ASSIGN_MODIFY, BINOP_REM},
    {"|=", ASSIGN_MODIFY, BINOP_LOGIOR},
    {"&=", ASSIGN_MODIFY, BINOP_LOGAND},
    {"^=", ASSIGN_MODIFY, BINOP_LOGXOR},
    {"++", INCREMENT, BINOP_END},
    {"--", DECREMENT, BINOP_END},
    {"->", ARROW, BINOP_END},
    {"&&", AND, BINOP_END},
    {"||", OR, BINOP_END},
    {"::", COLONCOLON, BINOP_END},
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END}
  };

/* assign machine-independent names to certain registers 
 * (unless overridden by the REGISTER_NAMES table)
 */
struct std_regs {
	char *name;
	int regnum;
} std_regs[] = {
#ifndef TEK_HACK

#ifdef PC_REGNUM
	{ "pc", PC_REGNUM },	/* pc is a synthesized (phony) register */
#endif
#ifdef FP_REGNUM
	{ "fp", FP_REGNUM },	/* fp is a synthesized (phony) register */
#endif
#ifdef SP_REGNUM
	{ "sp", SP_REGNUM },	/* sp is a standard register */
#endif
#endif /* TEK_HACK */

	{ "ps", PSR_REGNUM },	/* another name for $ps */
};

#define NUM_STD_REGS (sizeof std_regs / sizeof std_regs[0])

/* Read one token, getting characters through lexptr.  */

static int
yylex ()
{
  register int c;
  register int namelen;
  register int i;
  register char *tokstart;

 retry:

  tokstart = lexptr;
  /* See if it is a special token of length 3.  */
  for (i = 0; i < sizeof tokentab3 / sizeof tokentab3[0]; i++)
    if (!strncmp (tokstart, tokentab3[i].operator, 3))
      {
	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof tokentab2 / sizeof tokentab2[0]; i++)
    if (!strncmp (tokstart, tokentab2[i].operator, 2))
      {
	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
      return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '\'':
      lexptr++;
      c = *lexptr++;
      if (c == '\\')
	c = parse_escape (&lexptr);
      yylval.lval = c;
      c = *lexptr++;
      if (c != '\'')
	ui_badnews(-1,"Invalid character constant.");
      return CHAR;

    case '(':
      paren_depth++;
      lexptr++;
      return c;

    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      lexptr++;
      return c;

    case ',':
      if (comma_terminates && paren_depth == 0)
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] >= '0' && lexptr[1] <= '9')
	break;			/* Falls into number code.  */

    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '@':
    case '<':
    case '>':
    case '[':
    case ']':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
      lexptr++;
      return c;

    case '"':
      for (namelen = 1; (c = tokstart[namelen]) != '"'; namelen++)
	if (c == '\\')
	  {
	    c = tokstart[++namelen];
	    if (c >= '0' && c <= '9')
	      {
		c = tokstart[++namelen];
		if (c >= '0' && c <= '9')
		  c = tokstart[++namelen];
	      }
	  }
      yylval.sval.ptr = tokstart + 1;
      yylval.sval.length = namelen - 1;
      lexptr += namelen + 1;
      return STRING;
    }

  /* Is it a number?  */
  if ((c >= '0' && c <= '9')
      || c == '.')		/* This could only happen if the
				   next thing is a number; see case '.'
				   above.  */
    {
      /* It's a number */
      for (namelen = 0;
	   c = tokstart[namelen],
	   (c == '_' || c == '$' || c == '.' || (c >= '0' && c <= '9')
	    || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
	   namelen++)
	;
      return parse_number (namelen);
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    ui_badnews(-1,"Invalid token in expression.");

  /* It is a name.  See how long it is.  */

  for (namelen = 0;
       c = tokstart[namelen],
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
       namelen++)
    ;

  /* The token "if" terminates the expression and is NOT 
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    {
      return 0;
    }

  lexptr += namelen;

  /* Handle the tokens $digits; also $ (short for $0) and $$ (short for $$1)
     and $$digits (equivalent to $<-digits> if you could type that).
     Make token type LAST, and put the number (the digits) in yylval.  */

  if (*tokstart == '$')
    {
      register int negate = 0;
      c = 1;
      /* Double dollar means negate the number and add -1 as well.
	 Thus $$ alone means -1.  */
      if (namelen >= 2 && tokstart[1] == '$')
	{
	  negate = 1;
	  c = 2;
	}
      if (c == namelen)
	{
	  /* Just dollars (one or two) */
	  yylval.lval = - negate;
	  return LAST;
	}
      /* Is the rest of the token digits?  */
      for (; c < namelen; c++)
	if (!(tokstart[c] >= '0' && tokstart[c] <= '9'))
	  break;
      if (c == namelen)
	{
	  yylval.lval = atoi (tokstart + 1 + negate);
	  if (negate)
	    yylval.lval = - yylval.lval;
	  return LAST;
	}
    }

  /* Handle tokens that refer to machine registers:
     $ followed by a register name.  */

  if (*tokstart == '$') {
#ifdef TEK_HACK
    int num;
    char captok[128];

#endif /* TEK_HACK */
    for (c = 0; c < NUM_REGS; c++)
      if (namelen - 1 == strlen (reg_names[c])
	  && !strncmp (tokstart + 1, reg_names[c], namelen - 1))
	{
	  yylval.lval = c;
	  return REGNAME;
	}
    for (c = 0; c < NUM_STD_REGS; c++)
     if (namelen - 1 == strlen (std_regs[c].name)
	 && !strncmp (tokstart + 1, std_regs[c].name, namelen - 1))
       {
	 yylval.lval = std_regs[c].regnum;
	 return REGNAME;
       }
#ifdef TEK_HACK
    /* try shifting the token to lower case */
    for (c = 0; c < namelen - 1; c++)
      if (isupper(tokstart[c+1]))			/* -rcb 90.12.28 */
        captok[c] = tolower (tokstart[c+1]);
      else						/* -rcb 90.12.28 */
        captok[c] = tokstart[c+1];			/* -rcb 90.12.28 */
    for (c = 0; c < NUM_REGS; c++)
      if (namelen - 1 == strlen (reg_names[c])
	  && !strncmp (captok, reg_names[c], namelen - 1))
	{
	  yylval.lval = c;
	  return REGNAME;
	}
    for (c = 0; c < NUM_STD_REGS; c++)
     if (namelen - 1 == strlen (std_regs[c].name)
	 && !strncmp (captok, std_regs[c].name, namelen - 1))
       {
	 yylval.lval = std_regs[c].regnum;
	 return REGNAME;
       }
    /* try $r<n> */
     if (captok[0] == 'r')
       {
         for (c=2; c < namelen; c++)
           if (!(tokstart[c] >= '0' && tokstart[c] <= '9'))
             break;
         if (c == namelen)
           {
             num = atoi (tokstart + 2);
             if (num < NUM_GENERAL_REGS)
               {
                 yylval.lval = atoi (tokstart + 2);
                 return REGNAME;
               }
           }
       }
#endif /* TEK_HACK */
  }
  /* Catch specific keywords.  Should be done with a data structure.  */
  switch (namelen)
    {
    case 8:
      if (!strncmp (tokstart, "unsigned", 8))
	return UNSIGNED;
      break;
    case 6:
      if (!strncmp (tokstart, "struct", 6))
	return STRUCT;
      if (!strncmp (tokstart, "signed", 6))
	return SIGNED;
      if (!strncmp (tokstart, "sizeof", 6))      
	return SIZEOF;
      break;
    case 5:
      if (!strncmp (tokstart, "union", 5))
	return UNION;
      if (!strncmp (tokstart, "short", 5))
	return SHORT;
      break;
    case 4:
      if (!strncmp (tokstart, "enum", 4))
	return ENUM;
      if (!strncmp (tokstart, "long", 4))
	return LONG;
      if (!strncmp (tokstart, "this", 4)
	  && lookup_symbol ("$this", expression_context_block,
			    VAR_NAMESPACE, 0))
	return THIS;
      break;
    case 3:
      if (!strncmp (tokstart, "int", 3))
	return INT_KEYWORD;
      break;
    default:
      break;
    }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  /* Any other names starting in $ are debugger internal variables.  */

  if (*tokstart == '$')
    {
      yylval.ivar = (struct internalvar *) lookup_internalvar (copy_name (yylval.sval) + 1);
      return VARIABLE;
    }

  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions or symtabs.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    char *tmp = copy_name (yylval.sval);
    struct symbol *sym;

    if (lookup_partial_symtab (tmp))
      return BLOCKNAME;
    sym = lookup_symbol (tmp, expression_context_block,
			 VAR_NAMESPACE, 0);
    if (sym && SYMBOL_CLASS (sym) == LOC_BLOCK)
      return BLOCKNAME;
    if (lookup_typename (copy_name (yylval.sval), expression_context_block, 1))
      return TYPENAME;
    return NAME;
  }
}

static void
yyerror ()
{
  ui_badnews(-1,"Invalid syntax in expression.");
}

/* Return a null-terminated temporary copy of the name
   of a string token.  */

static char *
copy_name (token)
     struct stoken token;
{
  bcopy (token.ptr, namecopy, token.length);
  namecopy[token.length] = 0;
  return namecopy;
}

/* Reverse an expression from suffix form (in which it is constructed)
   to prefix form (in which we can conveniently print or execute it).  */

static void prefixify_subexp ();

static void
prefixify_expression (expr)
     register struct expression *expr;
{
  register int len = sizeof (struct expression) +
				    expr->nelts * sizeof (union exp_element);
  register struct expression *temp;
  register int inpos = expr->nelts, outpos = 0;

  temp = (struct expression *) alloca (len);

  /* Copy the original expression into temp.  */
  bcopy (expr, temp, len);

  prefixify_subexp (temp, expr, inpos, outpos);
}

/* Return the number of exp_elements in the subexpression of EXPR
   whose last exp_element is at index ENDPOS - 1 in EXPR.  */

static int
length_of_subexp (expr, endpos)
     register struct expression *expr;
     register int endpos;
{
  register int oplen = 1;
  register int args = 0;
  register int i;

  if (endpos < 0)
    ui_badnews(-1,"?error in length_of_subexp");

  i = (int) expr->elts[endpos - 1].opcode;

  switch (i)
    {
      /* C++  */
    case OP_SCOPE:
      oplen = 4 + ((expr->elts[endpos - 2].longconst
		    + sizeof (union exp_element))
		   / sizeof (union exp_element));
      break;

    case OP_LONG:
    case OP_DOUBLE:
      oplen = 4;
      break;

    case OP_VAR_VALUE:
    case OP_LAST:
    case OP_REGISTER:
    case OP_INTERNALVAR:
      oplen = 3;
      break;

    case OP_FUNCALL:
      oplen = 3;
      args = 1 + expr->elts[endpos - 2].longconst;
      break;

    case UNOP_CAST:
    case UNOP_MEMVAL:
      oplen = 3;
      args = 1;
      break;

    case STRUCTOP_STRUCT:
    case STRUCTOP_PTR:
      args = 1;
    case OP_STRING:
      oplen = 3 + ((expr->elts[endpos - 2].longconst
		    + sizeof (union exp_element))
		   / sizeof (union exp_element));
      break;

    case TERNOP_COND:
      args = 3;
      break;

    case BINOP_ASSIGN_MODIFY:
      oplen = 3;
      args = 2;
      break;

      /* C++ */
    case OP_THIS:
      oplen = 2;
      break;

    default:
      args = 1 + (i < (int) BINOP_END);
    }

  while (args > 0)
    {
      oplen += length_of_subexp (expr, endpos - oplen);
      args--;
    }

  return oplen;
}

/* Copy the subexpression ending just before index INEND in INEXPR
   into OUTEXPR, starting at index OUTBEG.
   In the process, convert it from suffix to prefix form.  */

static void
prefixify_subexp (inexpr, outexpr, inend, outbeg)
     register struct expression *inexpr;
     struct expression *outexpr;
     register int inend;
     int outbeg;
{
  register int oplen = 1;
  register int args = 0;
  register int i;
  int *arglens;
  enum exp_opcode opcode;

  /* Compute how long the last operation is (in OPLEN),
     and also how many preceding subexpressions serve as
     arguments for it (in ARGS).  */

  opcode = inexpr->elts[inend - 1].opcode;
  switch (opcode)
    {
      /* C++  */
    case OP_SCOPE:
      oplen = 4 + ((inexpr->elts[inend - 2].longconst
		    + sizeof (union exp_element))
		   / sizeof (union exp_element));
      break;

    case OP_LONG:
    case OP_DOUBLE:
      oplen = 4;
      break;

    case OP_VAR_VALUE:
    case OP_LAST:
    case OP_REGISTER:
    case OP_INTERNALVAR:
      oplen = 3;
      break;

    case OP_FUNCALL:
      oplen = 3;
      args = 1 + inexpr->elts[inend - 2].longconst;
      break;

    case UNOP_CAST:
    case UNOP_MEMVAL:
      oplen = 3;
      args = 1;
      break;

    case STRUCTOP_STRUCT:
    case STRUCTOP_PTR:
      args = 1;
    case OP_STRING:
      oplen = 3 + ((inexpr->elts[inend - 2].longconst
		    + sizeof (union exp_element))
		   / sizeof (union exp_element));
		   
      break;

    case TERNOP_COND:
      args = 3;
      break;

    case BINOP_ASSIGN_MODIFY:
      oplen = 3;
      args = 2;
      break;

      /* C++ */
    case OP_THIS:
      oplen = 2;
      break;

    default:
      args = 1 + ((int) opcode < (int) BINOP_END);
    }

  /* Copy the final operator itself, from the end of the input
     to the beginning of the output.  */
  inend -= oplen;
  bcopy (&inexpr->elts[inend], &outexpr->elts[outbeg],
	 oplen * sizeof (union exp_element));
  outbeg += oplen;

  /* Find the lengths of the arg subexpressions.  */
  arglens = (int *) alloca (args * sizeof (int));
  for (i = args - 1; i >= 0; i--)
    {
      oplen = length_of_subexp (inexpr, inend);
      arglens[i] = oplen;
      inend -= oplen;
    }

  /* Now copy each subexpression, preserving the order of
     the subexpressions, but prefixifying each one.
     In this loop, inend starts at the beginning of
     the expression this level is working on
     and marches forward over the arguments.
     outbeg does similarly in the output.  */
  for (i = 0; i < args; i++)
    {
      oplen = arglens[i];
      inend += oplen;
      prefixify_subexp (inexpr, outexpr, inend, outbeg);
      outbeg += oplen;
    }
}

/* This page contains the two entry points to this file.  */

/* Read a C expression from the string *STRINGPTR points to,
   parse it, and return a pointer to a  struct expression  that we malloc.
   Use block BLOCK as the lexical context for variable names;
   if BLOCK is zero, use the block of the selected stack frame.
   Meanwhile, advance *STRINGPTR to point after the expression,
   at the first nonwhite character that is not part of the expression
   (possibly a null character).

   If COMMA is nonzero, stop if a comma is reached.  */

struct expression *
parse_c_1 (stringptr, block, comma)
     char **stringptr;
     struct block *block;
{
  struct cleanup *old_chain;

  lexptr = *stringptr;

  paren_depth = 0;
  type_stack_depth = 0;

  comma_terminates = comma;

  if (lexptr == 0 || *lexptr == 0)
    error_no_arg ("expression to compute");

  old_chain = make_cleanup (free_funcalls, 0);
  funcall_chain = 0;

  expression_context_block_var = block;

  namecopy = (char *) alloca (strlen (lexptr) + 1);
  expout_size = 10;
  expout_ptr = 0;
  expout = (struct expression *)
    xmalloc (sizeof (struct expression)
	     + expout_size * sizeof (union exp_element));
  make_cleanup (free_current_contents, &expout);
  if (yyparse ())
    yyerror ();
  discard_cleanups (old_chain);
  expout->nelts = expout_ptr;
  expout = (struct expression *)
    xrealloc (expout,
	      sizeof (struct expression)
	      + expout_ptr * sizeof (union exp_element));
  prefixify_expression (expout);
  *stringptr = lexptr;
  return expout;
}

/* Parse STRING as an expression, and complain if this fails
   to use up all of the contents of STRING.  */

struct expression *
parse_c_expression (string)
     char *string;
{
  register struct expression *exp;
  exp = parse_c_1 (&string, 0, 0);
  if (*string)
    ui_badnews(-1,"Junk after end of expression.");
  return exp;
}

static void 
push_type (tp)
     enum type_pieces tp;
{
  if (type_stack_depth == type_stack_size)
    {
      type_stack_size *= 2;
      type_stack = (enum type_pieces *)
	xrealloc (type_stack, type_stack_size * sizeof (enum type_pieces));
    }
  type_stack[type_stack_depth++] = tp;
}

static enum type_pieces 
pop_type ()
{
  if (type_stack_depth)
    return type_stack[--type_stack_depth];
  return tp_end;
}

void
_initialize_expread ()
{
  type_stack_size = 80;
  type_stack_depth = 0;
  type_stack = (enum type_pieces *)
    xmalloc (type_stack_size * sizeof (enum type_pieces));
}

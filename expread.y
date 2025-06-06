/* Parse C expressions for GDB.
   Copyright (C) 1986, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/expread.y,v 1.13 90/12/29 16:35:29 robertb Exp $
   $Locker:  $

This file is part of GDB.  */


/* Parse a C expression from text in a string,
   and return the result as a  struct expression  pointer.
   That structure contains arithmetic operations in reverse polish,
   with constants represented by operations that are followed by special data.
   See expression.h for the details of the format.
   What is important here is that it can be built up sequentially
   during the process of parsing; the lower levels of the tree always
   come first in the result.  */
   
%{
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
%}

/* Although the yacc "value" of an expression is not used,
   since the result is stored in the structure being created,
   other node types do have values.  */

%union
  {
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
  }

%type <voidval> exp exp1 start variable
%type <tval> type typebase
%type <tvec> nonempty_typelist
%type <bval> block

/* Fancy type parsing.  */
%type <voidval> func_mod direct_abs_decl abs_decl
%type <tval> ptype
%type <lval> array_mod

%token <lval> INT CHAR
%token <ulval> UINT
%token <dval> FLOAT

/* Both NAME and TYPENAME tokens represent symbols in the input,
   and both convey their data as strings.
   But a TYPENAME is a string that happens to be defined as a typedef
   or builtin type name (such as int or char)
   and a NAME is any other symbol.

   Contexts where this distinction is not important can use the
   nonterminal "name", which matches either NAME or TYPENAME.  */

%token <sval> NAME TYPENAME BLOCKNAME STRING
%type <sval> name name_not_typename typename

%token STRUCT UNION ENUM SIZEOF UNSIGNED COLONCOLON

/* Special type cases, put in to allow the parser to distinguish different
   legal basetypes.  */
%token SIGNED LONG SHORT INT_KEYWORD

%token <lval> LAST REGNAME

%token <ivar> VARIABLE

%token <opcode> ASSIGN_MODIFY

/* C++ */
%token THIS

%left ','
%left ABOVE_COMMA
%right '=' ASSIGN_MODIFY
%right '?'
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQUAL NOTEQUAL
%left '<' '>' LEQ GEQ
%left LSH RSH
%left '@'
%left '+' '-'
%left '*' '/' '%'
%right UNARY INCREMENT DECREMENT
%right ARROW '.' '[' '('
%left COLONCOLON

%%

start   :	exp1
	;

/* Expressions, including the comma operator.  */
exp1	:	exp
	|	exp1 ',' exp
			{ write_exp_elt_opcode (BINOP_COMMA); }
	;

/* Expressions, not including the comma operator.  */
exp	:	'.' name
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
			  write_exp_string ($2);
			  write_exp_elt_opcode (STRUCTOP_PTR); }
	;

exp	:	'*' exp    %prec UNARY
			{ write_exp_elt_opcode (UNOP_IND); }

exp	:	'&' exp    %prec UNARY
			{ write_exp_elt_opcode (UNOP_ADDR); }

exp	:	'-' exp    %prec UNARY
			{ write_exp_elt_opcode (UNOP_NEG); }
	;

exp	:	'!' exp    %prec UNARY
			{ write_exp_elt_opcode (UNOP_ZEROP); }
	;

exp	:	'~' exp    %prec UNARY
			{ write_exp_elt_opcode (UNOP_LOGNOT); }
	;

exp	:	INCREMENT exp    %prec UNARY
			{ write_exp_elt_opcode (UNOP_PREINCREMENT); }
	;

exp	:	DECREMENT exp    %prec UNARY
			{ write_exp_elt_opcode (UNOP_PREDECREMENT); }
	;

exp	:	exp INCREMENT    %prec UNARY
			{ write_exp_elt_opcode (UNOP_POSTINCREMENT); }
	;

exp	:	exp DECREMENT    %prec UNARY
			{ write_exp_elt_opcode (UNOP_POSTDECREMENT); }
	;

exp	:	SIZEOF exp       %prec UNARY
			{ write_exp_elt_opcode (UNOP_SIZEOF); }
	;

exp	:	exp ARROW name
			{ write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string ($3);
			  write_exp_elt_opcode (STRUCTOP_PTR); }
	;

exp	:	exp ARROW '*' exp
			{ write_exp_elt_opcode (STRUCTOP_MPTR); }
	;

exp	:	exp '.' name
			{ write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string ($3);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); }
	;

exp	:	exp '.' '*' exp
			{ write_exp_elt_opcode (STRUCTOP_MEMBER); }
	;

exp	:	exp '[' exp1 ']'
			{ write_exp_elt_opcode (BINOP_SUBSCRIPT); }
	;

exp	:	exp '(' 
			/* This is to save the value of arglist_len
			   being accumulated by an outer function call.  */
			{ start_arglist (); }
		arglist ')'	%prec ARROW
			{ write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); }
	;

arglist	:
	;

arglist	:	exp
			{ arglist_len = 1; }
	;

arglist	:	arglist ',' exp   %prec ABOVE_COMMA
			{ arglist_len++; }
	;

exp	:	'{' type '}' exp  %prec UNARY
			{ write_exp_elt_opcode (UNOP_MEMVAL);
			  write_exp_elt_type ($2);
			  write_exp_elt_opcode (UNOP_MEMVAL); }
	;

exp	:	'(' type ')' exp  %prec UNARY
			{ write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type ($2);
			  write_exp_elt_opcode (UNOP_CAST); }
	;

exp	:	'(' exp1 ')'
			{ }
	;

/* Binary operators in order of decreasing precedence.  */

exp	:	exp '@' exp
			{ write_exp_elt_opcode (BINOP_REPEAT); }
	;

exp	:	exp '*' exp
			{ write_exp_elt_opcode (BINOP_MUL); }
	;

exp	:	exp '/' exp
			{ write_exp_elt_opcode (BINOP_DIV); }
	;

exp	:	exp '%' exp
			{ write_exp_elt_opcode (BINOP_REM); }
	;

exp	:	exp '+' exp
			{ write_exp_elt_opcode (BINOP_ADD); }
	;

exp	:	exp '-' exp
			{ write_exp_elt_opcode (BINOP_SUB); }
	;

exp	:	exp LSH exp
			{ write_exp_elt_opcode (BINOP_LSH); }
	;

exp	:	exp RSH exp
			{ write_exp_elt_opcode (BINOP_RSH); }
	;

exp	:	exp EQUAL exp
			{ write_exp_elt_opcode (BINOP_EQUAL); }
	;

exp	:	exp NOTEQUAL exp
			{ write_exp_elt_opcode (BINOP_NOTEQUAL); }
	;

exp	:	exp LEQ exp
			{ write_exp_elt_opcode (BINOP_LEQ); }
	;

exp	:	exp GEQ exp
			{ write_exp_elt_opcode (BINOP_GEQ); }
	;

exp	:	exp '<' exp
			{ write_exp_elt_opcode (BINOP_LESS); }
	;

exp	:	exp '>' exp
			{ write_exp_elt_opcode (BINOP_GTR); }
	;

exp	:	exp '&' exp
			{ write_exp_elt_opcode (BINOP_LOGAND); }
	;

exp	:	exp '^' exp
			{ write_exp_elt_opcode (BINOP_LOGXOR); }
	;

exp	:	exp '|' exp
			{ write_exp_elt_opcode (BINOP_LOGIOR); }
	;

exp	:	exp AND exp
			{ write_exp_elt_opcode (BINOP_AND); }
	;

exp	:	exp OR exp
			{ write_exp_elt_opcode (BINOP_OR); }
	;

exp	:	exp '?' exp ':' exp	%prec '?'
			{ write_exp_elt_opcode (TERNOP_COND); }
	;
			  
exp	:	exp '=' exp
			{ write_exp_elt_opcode (BINOP_ASSIGN); }
	;

exp	:	exp ASSIGN_MODIFY exp
			{ write_exp_elt_opcode (BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode ($2);
			  write_exp_elt_opcode (BINOP_ASSIGN_MODIFY); }
	;

exp	:	INT
			{ write_exp_elt_opcode (OP_LONG);
			  if ($1 == (int) $1 || $1 == (unsigned int) $1)
			    write_exp_elt_type (builtin_type_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_LONGEST);
			  write_exp_elt_longcst ((LONGEST) $1);
			  write_exp_elt_opcode (OP_LONG); }
	;

exp	:	UINT
			{
			  write_exp_elt_opcode (OP_LONG);
			  if ($1 == (unsigned int) $1)
			    write_exp_elt_type (builtin_type_unsigned_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_UNSIGNED_LONGEST);
			  write_exp_elt_longcst ((LONGEST) $1);
			  write_exp_elt_opcode (OP_LONG);
			}
	;

exp	:	CHAR
			{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_char);
			  write_exp_elt_longcst ((LONGEST) $1);
			  write_exp_elt_opcode (OP_LONG); }
	;

exp	:	FLOAT
			{ write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type (builtin_type_double);
			  write_exp_elt_dblcst ($1);
			  write_exp_elt_opcode (OP_DOUBLE); }
	;

exp	:	variable
	;

exp	:	LAST
			{ write_exp_elt_opcode (OP_LAST);
			  write_exp_elt_longcst ((LONGEST) $1);
			  write_exp_elt_opcode (OP_LAST); }
	;

exp	:	REGNAME
			{ write_exp_elt_opcode (OP_REGISTER);
			  write_exp_elt_longcst ((LONGEST) $1);
			  write_exp_elt_opcode (OP_REGISTER); }
	;

exp	:	VARIABLE
			{ write_exp_elt_opcode (OP_INTERNALVAR);
			  write_exp_elt_intern ($1);
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
	;

exp	:	SIZEOF '(' type ')'	%prec UNARY
			{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_int);
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH ($3));
			  write_exp_elt_opcode (OP_LONG); }
	;

exp	:	STRING
			{ write_exp_elt_opcode (OP_STRING);
			  write_exp_string ($1);
			  write_exp_elt_opcode (OP_STRING); }
	;

/* C++.  */
exp	:	THIS
			{ write_exp_elt_opcode (OP_THIS);
			  write_exp_elt_opcode (OP_THIS); }
	;

/* end of C++.  */

block	:	BLOCKNAME
			{
			  struct symtab *tem = lookup_symtab (copy_name ($1));
			  struct symbol *sym;
			  
			  if (tem)
			    $$ = BLOCKVECTOR_BLOCK (BLOCKVECTOR (tem), 1);
			  else
			    {
			      sym = lookup_symbol (copy_name ($1),
						   expression_context_block,
						   VAR_NAMESPACE, 0);
			      if (sym && SYMBOL_CLASS (sym) == LOC_BLOCK)
				$$ = SYMBOL_BLOCK_VALUE (sym);
			      else
				ui_badnews(-1,"No file or function \"%s\".",
				       copy_name ($1));
			    }
			}
	;

block	:	block COLONCOLON name
			{ struct symbol *tem
			    = lookup_symbol (copy_name ($3), $1, VAR_NAMESPACE, 0);
			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    ui_badnews(-1,"No function \"%s\" in specified context.",
				   copy_name ($3));
			  $$ = SYMBOL_BLOCK_VALUE (tem); }
	;

variable:	block COLONCOLON name
			{ struct symbol *sym;
			  sym = lookup_symbol (copy_name ($3), $1, VAR_NAMESPACE, 0);
			  if (sym == 0)
			    ui_badnews(-1,"No symbol \"%s\" in specified context.",
				   copy_name ($3));
			  write_exp_elt_opcode (OP_VAR_VALUE);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_VALUE); }
	;

variable:	typebase COLONCOLON name
			{
			  struct type *type = $1;
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION)
			    ui_badnews(-1,"`%s' is not defined as an aggregate type.",
				   TYPE_NAME (type));

			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string ($3);
			  write_exp_elt_opcode (OP_SCOPE);
			}
	|	COLONCOLON name
			{
			  char *name = copy_name ($2);
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
	;

variable:	name_not_typename
			{ struct symbol *sym;
			  int is_a_field_of_this;

			  sym = lookup_symbol (copy_name ($1),
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
			      write_exp_string ($1);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			    }
			  else
			    {
			      register int i;
			      register char *arg = copy_name ($1);

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
				       copy_name ($1));
			    }
			}
	;


ptype	:	typebase
	|	typebase abs_decl
		{
		  /* This is where the interesting stuff happens.  */
		  int done = 0;
		  int array_size;
		  struct type *follow_type = $1;
#ifdef GHSFORTRAN
		  struct type *base_type = $1;
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
		  $$ = follow_type;
		}
	;

abs_decl:	'*'
			{ push_type (tp_pointer); $$ = 0; }
	|	'*' abs_decl
			{ push_type (tp_pointer); $$ = $2; }
	|	direct_abs_decl
	;

direct_abs_decl: '(' abs_decl ')'
			{ $$ = $2; }
	|	direct_abs_decl array_mod
			{
			  push_type ((enum type_pieces) $2);
			  push_type (tp_array);
			}
	|	array_mod
			{
			  push_type ((enum type_pieces) $1);
			  push_type (tp_array);
			  $$ = 0;
			}
	| 	direct_abs_decl func_mod
			{ push_type (tp_function); }
	|	func_mod
			{ push_type (tp_function); }
	;

array_mod:	'[' ']'
			{ $$ = -1; }
	|	'[' INT ']'
			{ $$ = $2; }
	;

func_mod:	'(' ')'
			{ $$ = 0; }
	;

type	:	ptype
	|	typebase COLONCOLON '*'
			{ $$ = lookup_member_type (builtin_type_int, $1); }
	|	type '(' typebase COLONCOLON '*' ')'
			{ $$ = lookup_member_type ($1, $3); }
	|	type '(' typebase COLONCOLON '*' ')' '(' ')'
			{ $$ = lookup_member_type (lookup_function_type ($1)); }
	|	type '(' typebase COLONCOLON '*' ')' '(' nonempty_typelist ')'
			{ $$ = lookup_member_type (lookup_function_type ($1));
			  free ($8); }
	;

typebase
	:	TYPENAME
			{ $$ = lookup_typename (copy_name ($1),
						expression_context_block, 0); }
	|	INT_KEYWORD
			{ $$ = builtin_type_int; }
	|	LONG
			{ $$ = builtin_type_long; }
	|	SHORT
			{ $$ = builtin_type_short; }
	|	LONG INT_KEYWORD
			{ $$ = builtin_type_long; }
	|	UNSIGNED LONG INT_KEYWORD
			{ $$ = builtin_type_unsigned_long; }
	|	SHORT INT_KEYWORD
			{ $$ = builtin_type_short; }
	|	UNSIGNED SHORT INT_KEYWORD
			{ $$ = builtin_type_unsigned_short; }
	|	STRUCT name
			{ $$ = lookup_struct (copy_name ($2),
					      expression_context_block); }
	|	UNION name
			{ $$ = lookup_union (copy_name ($2),
					     expression_context_block); }
	|	ENUM name
			{ $$ = lookup_enum (copy_name ($2),
					    expression_context_block); }
	|	UNSIGNED typename
			{ $$ = lookup_unsigned_typename (copy_name ($2)); }
	|	UNSIGNED
			{ $$ = builtin_type_unsigned_int; }
	|	SIGNED typename
			{ $$ = lookup_typename (copy_name ($2),
						expression_context_block, 0); }
	|	SIGNED
			{ $$ = builtin_type_int; }
	;

typename:	TYPENAME
	|	INT_KEYWORD
		{
		  $$.ptr = "int";
		  $$.length = 3;
		}
	|	LONG
		{
		  $$.ptr = "long";
		  $$.length = 4;
		}
	|	SHORT
		{
		  $$.ptr = "short";
		  $$.length = 5;
		}
	;

nonempty_typelist
	:	type
		{ $$ = (struct type **)xmalloc (sizeof (struct type *) * 2);
		  $$[0] = (struct type *)0;
		  $$[1] = $1;
		}
	|	nonempty_typelist ',' type
		{ int len = sizeof (struct type *) * ++($<ivec>1[0]);
		  $$ = (struct type **)xrealloc ($1, len);
		  $$[$<ivec>$[0]] = $3;
		}
	;

name	:	NAME
	|	BLOCKNAME
	|	TYPENAME
	;

name_not_typename :	NAME
	|	BLOCKNAME
	;

%%

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

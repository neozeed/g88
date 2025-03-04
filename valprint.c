/* Print values for GNU debugger gdb.
   Copyright (C) 1986, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/valprint.c,v 1.17 91/01/13 23:49:47 robertb Exp $
   $Locker:  $

This file is part of GDB.  */

#include <stdio.h>
#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "value.h"
#include "ui.h"
#ifdef TEK_PROG_HACK
#include "command.h"
#endif /* TEK_PROG_HACK */

#ifdef TEK_HACK
#ifdef GHSFORTRAN
int fortran_string_length = 0;	/* shared with eval.c */
#endif
#endif

/* Maximum number of chars to print for a string pointer value
   or vector contents.  */

static int print_max;

static void type_print_varspec_suffix ();
static void type_print_varspec_prefix ();
static void type_print_base ();
static void type_print_method_args ();


char **unsigned_type_table;
char **signed_type_table;
char **float_type_table;

/* Print the value VAL in C-ish syntax on stream STREAM.
   FORMAT is a format-letter, or 0 for print in natural format of data type.
   If the object printed is a string pointer, returns
   the number of string bytes printed.  */

int
value_print (val, stream, format, pretty, usmode)
     value val;
     FILE *stream;
     char format;
     enum val_prettyprint pretty;
    int usmode;
{
  register int i, n, typelen;
#ifdef TEK_HACK
  register int retval;
#endif

#ifdef TEK_HACK
  if (VALUE_LVAL (val) == lval_reg_invalid)
    {
      fprintf_filtered (stream, "#%s=%s#", reg_names[VALUE_REGNO (val)],
                        THE_UNKNOWN);
      return;
    }
#endif /* TEK_HACK */      

  /* A "repeated" value really contains several values in a row.
     They are made by the @ operator.
     Print such values as if they were arrays.  */

  if (VALUE_REPEATED (val))
    {
      n = VALUE_REPETITIONS (val);
      typelen = TYPE_LENGTH (VALUE_TYPE (val));
      fprintf_filtered (stream, "{");
      /* Print arrays of characters using string syntax.  */
      if (typelen == 1 && TYPE_CODE (VALUE_TYPE (val)) == TYPE_CODE_INT
	  && format == 0)
	{
	  fprintf_filtered (stream, "\"");
	  for (i = 0; i < n && i < print_max; i++)
	    {
	      QUIT;
	      printchar (VALUE_CONTENTS (val)[i], stream, '"');
	    }
	  if (i < n)
	    fprintf_filtered (stream, "...");
	  fprintf_filtered (stream, "\"");
	}
      else
	{
	  for (i = 0; i < n && i < print_max; i++)
	    {
	      if (i)
		fprintf_filtered (stream, ", ");
	      val_print (VALUE_TYPE (val), VALUE_CONTENTS (val) + typelen * i,
			 VALUE_ADDRESS (val) + typelen * i,
			 stream, format, 1, 0, pretty, usmode);
	    }
	  if (i < n)
	    fprintf_filtered (stream, "...");
	}
      fprintf_filtered (stream, "}");
#ifdef TEK_HACK
#ifdef GHSFORTRAN

      /* must reset this back to zero just in case the last item was a FORTRAN
         array of strings element (this is probably unnecessary here) */

      fortran_string_length = 0;
#endif
#endif
      return n * typelen;
    }
  else
    {
      /* If it is a pointer, indicate what it points to.

	 Print type also if it is a reference.

         C++: if it is a member pointer, we will take care
	 of that when we print it.  */
      if (TYPE_CODE (VALUE_TYPE (val)) == TYPE_CODE_PTR
	  || TYPE_CODE (VALUE_TYPE (val)) == TYPE_CODE_REF)
	{
	  fprintf_filtered (stream, "(");
	  type_print (VALUE_TYPE (val), "", stream, -1);
	  fprintf_filtered (stream, ") ");
	}
#ifdef TEK_HACK
      retval = val_print (VALUE_TYPE (val), VALUE_CONTENTS (val),
		VALUE_ADDRESS (val), stream, format, 1, 0, pretty, usmode);
#ifdef GHSFORTRAN

      /* must reset this back to zero just in case the last item was a FORTRAN
         array of strings element */

      fortran_string_length = 0;
#endif
      return retval;
#else
      return val_print (VALUE_TYPE (val), VALUE_CONTENTS (val),
		VALUE_ADDRESS (val), stream, format, 1, 0, pretty, usmode);
#endif
    }
}

static int prettyprint;	/* Controls prettyprinting of structures.  */
int unionprint;		/* Controls printing of nested unions.  */

/* Print data of type TYPE located at VALADDR (within GDB),
   which came from the inferior at address ADDRESS,
   onto stdio stream STREAM according to FORMAT
   (a letter or 0 for natural format).

   If the data are a string pointer, returns the number of
   sting characters printed.

   if DEREF_REF is nonzero, then dereference references,
   otherwise just print them like pointers.

   The PRETTY parameter controls prettyprinting.  */

int
val_print (type, valaddr, address, stream, format,
	   deref_ref, recurse, pretty, usmode)
     struct type *type;
     char *valaddr;
     CORE_ADDR address;
     FILE *stream;
     char format;
     int deref_ref;
     int recurse;
     enum val_prettyprint pretty;
    int usmode;
{
  register int i;
  int len, n_baseclasses;
  struct type *elttype;
  int eltlen;
  LONGEST val;
  unsigned char c;
#ifdef TEK_HACK
  struct type *ty;
#endif

  if (pretty == Val_pretty_default)
    {
      pretty = prettyprint ? Val_prettyprint : Val_no_prettyprint;
    }
  
  QUIT;

#if 0
  if (pretty && recurse)
    {
      fprintf_filtered (stream, "\n");
      print_spaces_filtered (2 * recurse, stream);
    }
#endif

  if (TYPE_FLAGS (type) & TYPE_FLAG_STUB)
    {
      fprintf_filtered (stream, "<Type not defined in this context>");
      ui_fflush (stream);
      return;
    }
  
  switch (TYPE_CODE (type))
    {
#ifdef TEK_HACK
#ifdef GHSFORTRAN
    case TYPE_CODE_FORTRAN_ARRAY:
#endif
#endif
    case TYPE_CODE_ARRAY:
      if (TYPE_LENGTH (type) >= 0
	  && TYPE_LENGTH (TYPE_TARGET_TYPE (type)) > 0)
	{
#ifdef TEK_HACK
#ifdef GHSFORTRAN
	if (TYPE_CODE(type) == TYPE_CODE_FORTRAN_ARRAY)
	  elttype = TYPE_BACK_TYPE (type);
	else
#endif
#endif
	  elttype = TYPE_TARGET_TYPE (type);
	  eltlen = TYPE_LENGTH (elttype);
	  len = TYPE_LENGTH (type) / eltlen;
#ifndef TEK_HACK
	  fprintf_filtered (stream, "{");
#endif
	  /* For an array of chars, print with string syntax.  */
	  if (eltlen == 1 && TYPE_CODE (elttype) == TYPE_CODE_INT
	      && format == 0)
	    {
#ifdef TEK_HACK
#ifdef GHSFORTRAN
		if (TYPE_CODE(type) == TYPE_CODE_FORTRAN_ARRAY)
		{
		/* In order to handle FORTRAN arrays of strings we
		   must save the string length for val_print.  The
		   string length is the length of the first dimension
		   of the given array.  We then bypass the first
		   dimension when we call value_cast.  A two dimensional (2x3)
		   FORTRAN array of strings of length 4 looks something
		   like this to the debugger

			char arr[4][2][3] */

		  fortran_string_length = len;
		  val_print (TYPE_TARGET_TYPE (type), valaddr,
			     0, stream, format, deref_ref,
			     recurse + 1, pretty, usmode);

		  /* must reset back to zero */

		  fortran_string_length = 0;
		}
		else
		{
	  		fprintf_filtered (stream, "{");
#endif
#endif
	      		fprintf_filtered (stream, "\"");
	      		for (i = 0; i < len && i < print_max; i++)
			{
		  		QUIT;
		  		printchar (valaddr[i], stream, '"');
			}
	      		if (i < len)
				fprintf_filtered (stream, "...");
	      		fprintf_filtered (stream, "\"");
#ifdef TEK_HACK
#ifdef GHSFORTRAN
	  		fprintf_filtered (stream, "}");
		}
#endif
#endif
	    }
	  else
	    {
#ifdef TEK_HACK
#ifdef GHSFORTRAN
	      fprintf_filtered (stream, "{");
#endif
#endif
	      for (i = 0; i < len && i < print_max; i++)
		{
		  if (i) fprintf_filtered (stream, ", ");
#ifdef TEK_HACK
#ifdef GHSFORTRAN
		if (TYPE_CODE(type) == TYPE_CODE_FORTRAN_ARRAY)
		  val_print (TYPE_TARGET_TYPE (type), valaddr + i * eltlen,
			     0, stream, format, deref_ref,
			     recurse + 1, pretty, usmode);
		else
#endif
#endif
		  val_print (elttype, valaddr + i * eltlen,
			     0, stream, format, deref_ref,
			     recurse + 1, pretty, usmode);
		}
	      if (i < len)
		fprintf_filtered (stream, "...");
#ifdef TEK_HACK
#ifdef GHSFORTRAN
	      fprintf_filtered (stream, "}");
#endif
#endif
	    }
	  break;
#ifndef TEK_HACK
	  fprintf_filtered (stream, "}");
#endif
	}
      /* Array of unspecified length: treat like pointer to first */
      /* element.  */
      valaddr = (char *) &address;

    case TYPE_CODE_PTR:
      if (format)
	{
	  print_scalar_formatted (valaddr, type, format, 0, stream);
	  break;
	}
      if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_METHOD)
	{
	  struct type *domain = TYPE_DOMAIN_TYPE (TYPE_TARGET_TYPE (type));
	  struct type *target = TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (type));
	  struct fn_field *f;
	  int j, len2;
	  char *kind = "";

	  val = unpack_long (builtin_type_int, valaddr);
	  if (val < 128)
	    {
	      len = TYPE_NFN_FIELDS (domain);
	      for (i = 0; i < len; i++)
		{
		  f = TYPE_FN_FIELDLIST1 (domain, i);
		  len2 = TYPE_FN_FIELDLIST_LENGTH (domain, i);

		  for (j = 0; j < len2; j++)
		    {
		      QUIT;
		      if (TYPE_FN_FIELD_VOFFSET (f, j) == val)
			{
			  kind = "virtual";
			  goto common;
			}
		    }
		}
	    }
	  else
	    {
	      struct symbol *sym = find_pc_function ((CORE_ADDR) val);
	      if (sym == 0)
		ui_badnews(-1,"invalid pointer to member function");
	      len = TYPE_NFN_FIELDS (domain);
	      for (i = 0; i < len; i++)
		{
		  f = TYPE_FN_FIELDLIST1 (domain, i);
		  len2 = TYPE_FN_FIELDLIST_LENGTH (domain, i);

		  for (j = 0; j < len2; j++)
		    {
		      QUIT;
		      if (!strcmp (SYMBOL_NAME (sym), TYPE_FN_FIELD_PHYSNAME (f, j)))
			goto common;
		    }
		}
	    }
	common:
	  if (i < len)
	    {
	      fprintf_filtered (stream, "&");
	      type_print_varspec_prefix (TYPE_FN_FIELD_TYPE (f, j), stream, 0, 0);
	      ui_fprintf (stream, kind);
	      if (TYPE_FN_FIELD_PHYSNAME (f, j)[0] == '_'
		  && TYPE_FN_FIELD_PHYSNAME (f, j)[1] == '$')
		type_print_method_args
		  (TYPE_FN_FIELD_ARGS (f, j) + 1, "~",
		   TYPE_FN_FIELDLIST_NAME (domain, i), stream);
	      else
		type_print_method_args
		  (TYPE_FN_FIELD_ARGS (f, j), "",
		   TYPE_FN_FIELDLIST_NAME (domain, i), stream);
	      break;
	    }
	  fprintf_filtered (stream, "(");
  	  type_print (type, "", stream, -1);
	  fprintf_filtered (stream, ") %d", (int) val >> 3);
	}
      else if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_MEMBER)
	{
	  struct type *domain = TYPE_DOMAIN_TYPE (TYPE_TARGET_TYPE (type));
	  struct type *target = TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (type));
	  char *kind = "";

	  /* VAL is a byte offset into the structure type DOMAIN.
	     Find the name of the field for that offset and
	     print it.  */
	  int extra = 0;
	  int bits = 0;
	  len = TYPE_NFIELDS (domain);
	  /* @@ Make VAL into bit offset */
	  val = unpack_long (builtin_type_int, valaddr) << 3;
	  for (i = 0; i < len; i++)
	    {
	      int bitpos = TYPE_FIELD_BITPOS (domain, i);
	      QUIT;
	      if (val == bitpos)
		break;
	      if (val < bitpos && i > 0)
		{
		  int ptrsize = (TYPE_LENGTH (builtin_type_char) * TYPE_LENGTH (target));
		  /* Somehow pointing into a field.  */
		  i -= 1;
		  extra = (val - TYPE_FIELD_BITPOS (domain, i));
		  if (extra & 0x3)
		    bits = 1;
		  else
		    extra >>= 3;
		  break;
		}
	    }
	  if (i < len)
	    {
	      fprintf_filtered (stream, "&");
	      type_print_base (domain, stream, 0, 0);
	      fprintf_filtered (stream, "::");
	      fputs_filtered (TYPE_FIELD_NAME (domain, i), stream);
	      if (extra)
		fprintf_filtered (stream, " + %d bytes", extra);
	      if (bits)
		fprintf_filtered (stream, " (offset in bits)");
	      break;
	    }
	  fprintf_filtered (stream, "%d", val >> 3);
	}
      else
	{
	  fprintf_filtered (stream, "0x%x", * (int *) valaddr);
	  /* For a pointer to char or unsigned char,
	     also print the string pointed to, unless pointer is null.  */
	  
	  /* For an array of chars, print with string syntax.  */
	  elttype = TYPE_TARGET_TYPE (type);
	  i = 0;		/* Number of characters printed.  */
	  if (TYPE_LENGTH (elttype) == 1 
	      && TYPE_CODE (elttype) == TYPE_CODE_INT
	      && format == 0
	      && unpack_long (type, valaddr) != 0
	      && print_max)
	    {
	      fprintf_filtered (stream, " ");

	      /* Get first character.  */
	      if (read_memory ( (CORE_ADDR) unpack_long (type, valaddr),
			       &c, 1, usmode))
		{
		  /* First address out of bounds.  */
		  fprintf_filtered (stream, "<Address 0x%x out of bounds>",
			   (* (int *) valaddr));
		  break;
		}
	      else
		{
		  /* A real string.  */
		  int out_of_bounds = 0;
		  
		  fprintf_filtered (stream, "\"");
		  while (c)
		    {
		      QUIT;
		      printchar (c, stream, '"');
		      if (++i >= print_max)
			break;
		      if (read_memory ((CORE_ADDR) unpack_long (type, valaddr)
				       + i, &c, 1, usmode))
			{
			  /* This address was out of bounds.  */
			  fprintf_filtered (stream,
				   "\"*** <Address 0x%x out of bounds>",
				   (* (int *) valaddr) + i);
			  out_of_bounds = 1;
			  break;
			}
		    }
		  if (!out_of_bounds)
		    {
		      fprintf_filtered (stream, "\"");
		      if (i == print_max)
			fprintf_filtered (stream, "...");
		    }
		}
	      ui_fflush (stream);
	    }
	  /* Return number of characters printed, plus one for the
	     terminating null if we have "reached the end".  */
	  return i + (print_max && i != print_max);
	}
      break;

    case TYPE_CODE_MEMBER:
      ui_badnews(-1,"not implemented: member type in val_print");
      break;

    case TYPE_CODE_REF:
      fprintf_filtered (stream, "(0x%x &) = ", * (int *) valaddr);
      /* De-reference the reference.  */
      if (deref_ref)
	{
	  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_UNDEF)
	    {
	      value val = value_at (TYPE_TARGET_TYPE (type), 
                                    * (int *) valaddr,
                                    usmode);
	      val_print (VALUE_TYPE (val), VALUE_CONTENTS (val),
			 VALUE_ADDRESS (val), stream, format,
			 deref_ref, recurse + 1, pretty, usmode);
	    }
	  else
	    fprintf_filtered (stream, "???");
	}
      break;

    case TYPE_CODE_UNION:
      if (recurse && !unionprint)
	{
	  fprintf_filtered (stream, "{...}");
	  break;
	}
      /* Fall through.  */
    case TYPE_CODE_STRUCT:
      fprintf_filtered (stream, "{");
      len = TYPE_NFIELDS (type);
      n_baseclasses = TYPE_N_BASECLASSES (type);
      for (i = 1; i <= n_baseclasses; i++)
	{
#ifdef	DG_HACK
	  struct type* baseclass_type;

#endif	/* DG_HACK */
	  fprintf_filtered (stream, "\n");
	  if (pretty)
	    print_spaces_filtered (2 + 2 * recurse, stream);
#ifdef	DG_HACK
          baseclass_type = TYPE_BASECLASS (type,i);
          if (TYPE_MAIN_VARIANT(baseclass_type))
            baseclass_type = TYPE_MAIN_VARIANT (baseclass_type);
          if (TYPE_TARGET_TYPE (baseclass_type))
            baseclass_type = TYPE_TARGET_TYPE (baseclass_type);
	  fputs_filtered ("<BASE ", stream);
	  fputs_filtered (TYPE_NAME (baseclass_type), stream);
	  fputs_filtered ("> = ", stream);
	  val_print (baseclass_type,
#else /* not DG_HACK */
	  fputs_filtered ("<", stream);
	  fputs_filtered (TYPE_NAME (TYPE_BASECLASS (type, i)), stream);
	  fputs_filtered ("> = ", stream);
	  val_print (TYPE_FIELD_TYPE (type, 0),
#endif	/* not DG_HACK */
		     valaddr + TYPE_FIELD_BITPOS (type, i-1) / 8,
		     0, stream, 0, 0, recurse + 1, pretty, usmode);
	}
#ifdef	DG_HACK
      if (n_baseclasses > 0)
#else /* not DG_HACK */
      if (i > 1)
#endif	/* not DG_HACK */
      {
	fprintf_filtered (stream, "\n");
	print_spaces_filtered (2 + 2 * recurse, stream);
	fputs_filtered ("members of ", stream);
        fputs_filtered (TYPE_NAME (type), stream);
        fputs_filtered (": ", stream);
      }
      if (!len && i == 1)
	fprintf_filtered (stream, "<No data fields>");
      else
	{
	  for (i -= 1; i < len; i++)
	    {
	      if (i > n_baseclasses) fprintf_filtered (stream, ", ");
	      if (pretty)
		{
		  fprintf_filtered (stream, "\n");
		  print_spaces_filtered (2 + 2 * recurse, stream);
		}
	      fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
	      fputs_filtered (" = ", stream);
	      /* check if static field */
	      if (TYPE_FIELD_STATIC (type, i))
		{
		  value v;
		  
		  v = value_static_field (type, TYPE_FIELD_NAME (type, i), i);
		  val_print (TYPE_FIELD_TYPE (type, i),
			     VALUE_CONTENTS (v), 0, stream, format,
			     deref_ref, recurse + 1, pretty, usmode);
		}
	      else if (TYPE_FIELD_PACKED (type, i))
		{
		  char *valp = (char *) & val;
		  union {int i; char c;} test;
		  test.i = 1;
		  if (test.c != 1)
		    valp += sizeof val - TYPE_LENGTH (TYPE_FIELD_TYPE (type, i));
		  val = unpack_field_as_long (type, valaddr, i);
		  val_print (TYPE_FIELD_TYPE (type, i), valp, 0,
		     stream, format, deref_ref, recurse + 1, pretty, usmode);
		}
	      else
		{
		  val_print (TYPE_FIELD_TYPE (type, i), 
			     valaddr + TYPE_FIELD_BITPOS (type, i) / 8,
			     0, stream, format, deref_ref,
			     recurse + 1, pretty, usmode);
		}
	    }
	  if (pretty)
	    {
	      fprintf_filtered (stream, "\n");
	      print_spaces_filtered (2 * recurse, stream);
	    }
	}
      fprintf_filtered (stream, "}");
      break;

    case TYPE_CODE_ENUM:
      if (format)
	{
	  print_scalar_formatted (valaddr, type, format, 0, stream);
	  break;
	}
      len = TYPE_NFIELDS (type);

#ifdef TEK_HACK
      switch (TYPE_LENGTH(type))
      {
      case sizeof(char):
	ty = builtin_type_char;
	break;
      case sizeof(short):
	ty = builtin_type_short;
	break;
      default:
	ty = builtin_type_int;
      }
      val = unpack_long (ty, valaddr);
#else
      val = unpack_long (builtin_type_int, valaddr);
#endif
      for (i = 0; i < len; i++)
	{
	  QUIT;
	  if (val == TYPE_FIELD_BITPOS (type, i))
	    break;
	}
      if (i < len)
	fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
      else
	fprintf_filtered (stream, "%d", (int) val);
      break;

    case TYPE_CODE_FUNC:
      if (format)
	{
	  print_scalar_formatted (valaddr, type, format, 0, stream);
	  break;
	}
      fprintf_filtered (stream, "{");
      type_print (type, "", stream, -1);
      fprintf_filtered (stream, "} ");
      fprintf_filtered (stream, "0x%x", address);
      break;

    case TYPE_CODE_INT:
#ifdef TEK_HACK
#ifdef GHSFORTRAN
      if (fortran_string_length)
      {
          register int i = fortran_string_length;

	  /* we are printing out an element of an array of FORTRAN strings */

	  fprintf_filtered (stream, " '");
	  while (i--)
	  {
	  	printchar ((unsigned char) unpack_long (type, valaddr), 
		     stream, '\'');
		valaddr++;
	  }
	  fprintf_filtered (stream, "'");
	  break;
      }
#endif
#endif
      if (format)
	{
	  print_scalar_formatted (valaddr, type, format, 0, stream);
	  break;
	}
#ifdef PRINT_TYPELESS_INTEGER
      PRINT_TYPELESS_INTEGER (stream, type, unpack_long (type, valaddr));
#else
#ifndef LONG_LONG
      fprintf_filtered (stream,
			TYPE_UNSIGNED (type) ? "%u" : "%d",
			unpack_long (type, valaddr));
#else
      fprintf_filtered (stream,
			TYPE_UNSIGNED (type) ? "%llu" : "%lld",
			unpack_long (type, valaddr));
#endif
#endif
			
      if (TYPE_LENGTH (type) == 1)
	{
	  fprintf_filtered (stream, " '");
	  printchar ((unsigned char) unpack_long (type, valaddr), 
		     stream, '\'');
	  fprintf_filtered (stream, "'");
	}
      break;

    case TYPE_CODE_FLT:
      if (format)
	{
	  print_scalar_formatted (valaddr, type, format, 0, stream);
	  break;
	}
#ifdef IEEE_FLOAT
      if (is_nan ((void *) valaddr, TYPE_LENGTH (type)))
	{
	  fprintf_filtered (stream, "NaN");
	  break;
	}
#endif
      {
	double doub;
	int inv;

	doub = unpack_double (type, valaddr, &inv);
	if (inv)
	  fprintf_filtered (stream, "Invalid float value");
	else
	  fprintf_filtered (stream,
			    TYPE_LENGTH (type) <= 4? "%.7g": "%.16g", doub);
      }
      break;

    case TYPE_CODE_VOID:
      fprintf_filtered (stream, "void");
      break;

    default:
      ui_badnews(-1,"Invalid type code in symbol table.");
    }
  ui_fflush (stream);
}

#ifdef IEEE_FLOAT

/* Nonzero if ARG (a double) is a NAN.  */

int
is_nan (fp, len)
     void *fp;
     int len;
{
  int lowhalf, highhalf;
  union ieee
    {
      long i[2];		/* ASSUMED 32 BITS */
      float f;		/* ASSUMED 32 BITS */
      double d;		/* ASSUMED 64 BITS */
    } *arg;

  arg = (union ieee *)fp;

  /*
   * Single precision float.
   */
  if (len == sizeof(long))
    {
      highhalf = arg->i[0];
      return ((((highhalf >> 23) & 0xFF) == 0xFF) 
	      && 0 != (highhalf & 0x7FFFFF));
    }
  
  /* Separate the high and low words of the double.
     Distinguish big and little-endian machines.  */
#ifdef WORDS_BIG_ENDIAN
    lowhalf = arg->i[1], highhalf = arg->i[0];
#else
    lowhalf = arg->i[0], highhalf = arg->i[1];
#endif
  
  /* Nan: exponent is the maximum possible, and fraction is nonzero.  */
  return (((highhalf>>20) & 0x7ff) == 0x7ff
	  && ! ((highhalf & 0xfffff == 0) && (lowhalf == 0)));
}
#endif

/* Print a description of a type TYPE
   in the form of a declaration of a variable named VARSTRING.
   Output goes to STREAM (via stdio).
   If SHOW is positive, we show the contents of the outermost level
   of structure even if there is a type name that could be used instead.
   If SHOW is negative, we never show the details of elements' types.  */

void
type_print (type, varstring, stream, show)
     struct type *type;
     char *varstring;
     FILE *stream;
     int show;
{
  type_print_1 (type, varstring, stream, show, 0);
}

/* LEVEL is the depth to indent lines by.  */

void
type_print_1 (type, varstring, stream, show, level)
     struct type *type;
     char *varstring;
     FILE *stream;
     int show;
     int level;
{
  register enum type_code code;
  type_print_base (type, stream, show, level);
  code = TYPE_CODE (type);
  if ((varstring && *varstring)
      ||
      /* Need a space if going to print stars or brackets;
	 but not if we will print just a type name.  */
      ((show > 0 || TYPE_NAME (type) == 0)
       &&
       (code == TYPE_CODE_PTR || code == TYPE_CODE_FUNC
	|| code == TYPE_CODE_METHOD
	|| code == TYPE_CODE_ARRAY
#ifdef TEK_HACK
#ifdef GHSFORTRAN
	|| code == TYPE_CODE_FORTRAN_ARRAY
#endif
#endif
	|| code == TYPE_CODE_MEMBER
	|| code == TYPE_CODE_REF)))
    fprintf_filtered (stream, " ");
  type_print_varspec_prefix (type, stream, show, 0);
  fputs_filtered (varstring, stream);
  type_print_varspec_suffix (type, stream, show, 0);
}

/* Print the method arguments ARGS to the file STREAM.  */
static void
type_print_method_args (args, prefix, varstring, stream)
     struct type **args;
     char *prefix, *varstring;
     FILE *stream;
{
  int i;

  fputs_filtered (" ", stream);
  fputs_filtered (prefix, stream);
  fputs_filtered (varstring, stream);
  fputs_filtered (" (", stream);
  if (args[1] && args[1]->code != TYPE_CODE_VOID)
    {
      i = 1;			/* skip the class variable */
      while (1)
	{
	  type_print (args[i++], "", stream, 0);
	  if (!args[i]) 
	    {
	      fprintf_filtered (stream, " ...");
	      break;
	    }
	  else if (args[i]->code != TYPE_CODE_VOID)
	    {
	      fprintf_filtered (stream, ", ");
	    }
	  else break;
	}
    }
  fprintf_filtered (stream, ")");
}
  
/* If TYPE is a derived type, then print out derivation
   information.  Print out all layers of the type heirarchy
   until we encounter one with multiple inheritance.
   At that point, print out that ply, and return.  */
static void
type_print_derivation_info (stream, type)
     FILE *stream;
     struct type *type;
{
  char *name;
  int i, n_baseclasses = TYPE_N_BASECLASSES (type);
  struct type *basetype = 0;

  while (type && n_baseclasses == 1)
    {
      basetype = TYPE_BASECLASS (type, 1);
      if (TYPE_NAME (basetype) && (name = TYPE_NAME (basetype)))
	{
	  while (*name != ' ') name++;
	  fprintf_filtered (stream, ": %s%s ",
		   TYPE_VIA_PUBLIC (basetype) ? "public" : "private",
		   TYPE_VIA_VIRTUAL (basetype) ? " virtual" : "");
	  fputs_filtered (name + 1, stream);
	  fputs_filtered (" ", stream);
	}
      n_baseclasses = TYPE_N_BASECLASSES (basetype);
      type = basetype;
    }

  if (type)
    {
      if (n_baseclasses != 0)
	fprintf_filtered (stream, ": ");
      for (i = 1; i <= n_baseclasses; i++)
	{
	  basetype = TYPE_BASECLASS (type, i);
	  if (TYPE_NAME (basetype) && (name = TYPE_NAME (basetype)))
	    {
	      while (*name != ' ') name++;
	      fprintf_filtered (stream, "%s%s ",
		       TYPE_VIA_PUBLIC (basetype) ? "public" : "private",
		       TYPE_VIA_VIRTUAL (basetype) ? " virtual" : "");
	      fputs_filtered (name + 1, stream);
	    }
	  if (i < n_baseclasses)
	    fprintf_filtered (stream, ", ");
	}
      fprintf_filtered (stream, " ");
    }
}

/* Print any asterisks or open-parentheses needed before the
   variable name (to describe its type).

   On outermost call, pass 0 for PASSED_A_PTR.
   On outermost call, SHOW > 0 means should ignore
   any typename for TYPE and show its details.
   SHOW is always zero on recursive calls.  */

static void
type_print_varspec_prefix (type, stream, show, passed_a_ptr)
     struct type *type;
     FILE *stream;
     int show;
     int passed_a_ptr;
{
  if (type == 0)
    return;

  if (TYPE_NAME (type) && show <= 0)
    return;

  QUIT;

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
      type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0, 1);
      fprintf_filtered (stream, "*");
      break;

    case TYPE_CODE_MEMBER:
      if (passed_a_ptr)
	fprintf_filtered (stream, "(");
      type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0,
				 0);
      fprintf_filtered (stream, " ");
      type_print_base (TYPE_DOMAIN_TYPE (type), stream, 0,
		       passed_a_ptr);
      fprintf_filtered (stream, "::");
      break;

    case TYPE_CODE_METHOD:
      if (passed_a_ptr)
	ui_fprintf (stream, "(");
      type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0,
				 0);
      fprintf_filtered (stream, " ");
      type_print_base (TYPE_DOMAIN_TYPE (type), stream, 0,
		       passed_a_ptr);
      fprintf_filtered (stream, "::");
      break;

    case TYPE_CODE_REF:
      type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0, 1);
      fprintf_filtered (stream, "&");
      break;

    case TYPE_CODE_FUNC:
      type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0,
				 0);
      if (passed_a_ptr)
	fprintf_filtered (stream, "(");
      break;

#ifdef TEK_HACK
#ifdef GHSFORTRAN
    case TYPE_CODE_FORTRAN_ARRAY:
#endif
#endif
    case TYPE_CODE_ARRAY:
      type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0,
				 0);
      if (passed_a_ptr)
	fprintf_filtered (stream, "(");
    }
}

/* Print any array sizes, function arguments or close parentheses
   needed after the variable name (to describe its type).
   Args work like type_print_varspec_prefix.  */

static void
type_print_varspec_suffix (type, stream, show, passed_a_ptr)
     struct type *type;
     FILE *stream;
     int show;
     int passed_a_ptr;
{
  if (type == 0)
    return;

  if (TYPE_NAME (type) && show <= 0)
    return;

  QUIT;

  switch (TYPE_CODE (type))
    {
#ifdef TEK_HACK
#ifdef GHSFORTRAN
    case TYPE_CODE_FORTRAN_ARRAY:
#endif
#endif
    case TYPE_CODE_ARRAY:
      if (passed_a_ptr)
	fprintf_filtered (stream, ")");
      
      fprintf_filtered (stream, "[");
      if (TYPE_LENGTH (type) >= 0
	  && TYPE_LENGTH (TYPE_TARGET_TYPE (type)) > 0)
	fprintf_filtered (stream, "%d",
			  (TYPE_LENGTH (type)
			   / TYPE_LENGTH (TYPE_TARGET_TYPE (type))));
      fprintf_filtered (stream, "]");
      
      type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0,
				 0);
      break;

    case TYPE_CODE_MEMBER:
      if (passed_a_ptr)
	fprintf_filtered (stream, ")");
      type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0, 0);
      break;

    case TYPE_CODE_METHOD:
      if (passed_a_ptr)
	fprintf_filtered (stream, ")");
      type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0, 0);
      if (passed_a_ptr)
	{
	  int i;
	  struct type **args = TYPE_ARG_TYPES (type);

	  fprintf_filtered (stream, "(");
	  if (args[1] == 0)
	    fprintf_filtered (stream, "...");
	  else for (i = 1; args[i] != 0 && args[i]->code != TYPE_CODE_VOID; i++)
	    {
	      type_print_1 (args[i], "", stream, -1, 0);
	      if (args[i+1] == 0)
		fprintf_filtered (stream, "...");
	      else if (args[i+1]->code != TYPE_CODE_VOID)
		fprintf_filtered (stream, ",");
	    }
	  fprintf_filtered (stream, ")");
	}
      break;

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0, 1);
      break;

    case TYPE_CODE_FUNC:
      type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0,
				 passed_a_ptr);
      if (passed_a_ptr)
	fprintf_filtered (stream, ")");
      fprintf_filtered (stream, "()");
      break;
    }
}

/* Print the name of the type (or the ultimate pointer target,
   function value or array element), or the description of a
   structure or union.

   SHOW nonzero means don't print this type as just its name;
   show its real definition even if it has a name.
   SHOW zero means print just typename or struct tag if there is one
   SHOW negative means abbreviate structure elements.
   SHOW is decremented for printing of structure elements.

   LEVEL is the depth to indent by.
   We increase it for some recursive calls.  */

static void
type_print_base (type, stream, show, level)
     struct type *type;
     FILE *stream;
     int show;
     int level;
{
  char *name;
  register int i;
  register int len;
  register int lastval;

  QUIT;

  if (type == 0)
    {
      fprintf_filtered (stream, "type unknown");
      return;
    }

  if (TYPE_NAME (type) && show <= 0)
    {
      fputs_filtered (TYPE_NAME (type), stream);
      return;
    }

  switch (TYPE_CODE (type))
    {
#ifdef TEK_HACK
#ifdef GHSFORTRAN
    case TYPE_CODE_FORTRAN_ARRAY:
#endif
#endif
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_PTR:
    case TYPE_CODE_MEMBER:
    case TYPE_CODE_REF:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      type_print_base (TYPE_TARGET_TYPE (type), stream, show, level);
      break;

    case TYPE_CODE_STRUCT:
      fprintf_filtered (stream, "struct ");
      goto struct_union;

    case TYPE_CODE_UNION:
      fprintf_filtered (stream, "union ");
    struct_union:
      if (TYPE_NAME (type) && (name = TYPE_NAME (type)))
	{
	  while (*name != ' ') name++;
	  fputs_filtered (name + 1, stream);
	  fputs_filtered (" ", stream);
	}
      if (show < 0)
	fprintf_filtered (stream, "{...}");
      else
	{
	  int i;

	  type_print_derivation_info (stream, type);
	  
	  fprintf_filtered (stream, "{");
	  len = TYPE_NFIELDS (type);
	  if (len) fprintf_filtered (stream, "\n");
	  else fprintf_filtered (stream, "<no data fields>\n");

	  /* If there is a base class for this type,
	     do not print the field that it occupies.  */
	  for (i = TYPE_N_BASECLASSES (type); i < len; i++)
	    {
	      QUIT;
	      /* Don't print out virtual function table.  */
	      if (! strncmp (TYPE_FIELD_NAME (type, i),
			   "_vptr$", 6))
		continue;

	      print_spaces_filtered (level + 4, stream);
	      if (TYPE_FIELD_STATIC (type, i))
		{
		  fprintf_filtered (stream, "static ");
		}
	      type_print_1 (TYPE_FIELD_TYPE (type, i),
			    TYPE_FIELD_NAME (type, i),
			    stream, show - 1, level + 4);
	      if (!TYPE_FIELD_STATIC (type, i)
		  && TYPE_FIELD_PACKED (type, i))
		{
		  /* ??? don't know what to put here ??? */;
		}
	      fprintf_filtered (stream, ";\n");
	    }

	  /* C++: print out the methods */
	  len = TYPE_NFN_FIELDS (type);
	  if (len) fprintf_filtered (stream, "\n");
	  for (i = 0; i < len; i++)
	    {
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);
	      int j, len2 = TYPE_FN_FIELDLIST_LENGTH (type, i);

	      for (j = 0; j < len2; j++)
		{
		  QUIT;
		  print_spaces_filtered (level + 4, stream);
		  if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
		    fprintf_filtered (stream, "virtual ");
		  type_print (TYPE_TARGET_TYPE (TYPE_FN_FIELD_TYPE (f, j)), "", stream, 0);
		  if (TYPE_FN_FIELD_PHYSNAME (f, j)[0] == '_'
		      && TYPE_FN_FIELD_PHYSNAME (f, j)[1] == '$')
		    type_print_method_args
		      (TYPE_FN_FIELD_ARGS (f, j) + 1, "~",
		       TYPE_FN_FIELDLIST_NAME (type, i), stream);
		  else
		    type_print_method_args
		      (TYPE_FN_FIELD_ARGS (f, j), "",
		       TYPE_FN_FIELDLIST_NAME (type, i), stream);

		  fprintf_filtered (stream, ";\n");
		}
	      if (len2) fprintf_filtered (stream, "\n");
	    }

	  print_spaces_filtered (level, stream);
	  fprintf_filtered (stream, "}");
	}
      break;

    case TYPE_CODE_ENUM:
      fprintf_filtered (stream, "enum ");
      if (TYPE_NAME (type))
	{
	  name = TYPE_NAME (type);
	  while (*name != ' ') name++;
	  fputs_filtered (name + 1, stream);
	  fputs_filtered (" ", stream);
	}
      if (show < 0)
	fprintf_filtered (stream, "{...}");
      else
	{
	  fprintf_filtered (stream, "{");
	  len = TYPE_NFIELDS (type);
	  lastval = 0;
	  for (i = 0; i < len; i++)
	    {
	      QUIT;
	      if (i) fprintf_filtered (stream, ", ");
	      fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
	      if (lastval != TYPE_FIELD_BITPOS (type, i))
		{
		  fprintf_filtered (stream, " : %d", TYPE_FIELD_BITPOS (type, i));
		  lastval = TYPE_FIELD_BITPOS (type, i);
		}
	      lastval++;
	    }
	  fprintf_filtered (stream, "}");
	}
      break;

    case TYPE_CODE_INT:
      if (TYPE_UNSIGNED (type))
	name = unsigned_type_table[TYPE_LENGTH (type)];
      else
	name = signed_type_table[TYPE_LENGTH (type)];
      fputs_filtered (name, stream);
      break;

    case TYPE_CODE_FLT:
      name = float_type_table[TYPE_LENGTH (type)];
      fputs_filtered (name, stream);
      break;

    case TYPE_CODE_VOID:
      fprintf_filtered (stream, "void");
      break;

    case 0:
      fprintf_filtered (stream, "struct unknown");
      break;

    default:
      ui_badnews(-1,"Invalid type code in symbol table.");
    }
}

static void
set_maximum_command (arg)
     char *arg;
{
  if (!arg) error_no_arg ("value for maximum elements to print");
  print_max = parse_and_eval_address (arg);
}

static void
set_prettyprint_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  prettyprint = parse_binary_operation ("set prettyprint", arg);
}

static void
set_unionprint_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  unionprint = parse_binary_operation ("set unionprint", arg);
}

format_info (arg, from_tty)
     char *arg;
     int from_tty;
{
  if (arg)
    ui_badnews(-1,"\"info format\" does not take any arguments.");
  ui_fprintf(stdout, "Prettyprinting of structures is %s.\n",
	  prettyprint ? "on" : "off");
  ui_fprintf(stdout, "Printing of unions interior to structures is %s.\n",
	  unionprint ? "on" : "off");
  ui_fprintf(stdout, "The maximum number of array elements printed is %d.\n",
	  print_max);
}

extern struct cmd_list_element *setlist;
#ifdef TEK_PROG_HACK
extern struct cmd_list_element *set_cmd;
#endif /* TEK_PROG_HACK */

void
_initialize_valprint ()
{
#ifdef TEK_PROG_HACK
  struct cmd_list_element *new;

  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("array-max", class_vars, set_maximum_command,
	   "Set NUMBER as limit on string chars or array elements to print.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
  
  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("prettyprint", class_support, set_prettyprint_command,
	   "Turn prettyprinting of structures on and off.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
#endif /* TEK_PROG_HACK */
  add_alias_cmd ("pp", "prettyprint", class_support, 1, &setlist);

#ifdef TEK_PROG_HACK
  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("unionprint", class_support, set_unionprint_command,
	   "Turn printing of unions interior to structures on and off.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
#endif /* TEK_PROG_HACK */

  add_info ("format", format_info,
	    "Show current settings of data formatting options.");

  /* Give people the defaults which they are used to.  */
  prettyprint = 0;
  unionprint = 1;

  print_max = 200;

  unsigned_type_table
    = (char **) xmalloc ((1 + sizeof (unsigned LONGEST)) * sizeof (char *));
  bzero (unsigned_type_table, (1 + sizeof (unsigned LONGEST)));
  unsigned_type_table[sizeof (unsigned char)] = "unsigned char";
  unsigned_type_table[sizeof (unsigned short)] = "unsigned short";
  unsigned_type_table[sizeof (unsigned long)] = "unsigned long";
  unsigned_type_table[sizeof (unsigned int)] = "unsigned int";
#ifdef LONG_LONG
  unsigned_type_table[sizeof (unsigned long long)] = "unsigned long long";
#endif

  signed_type_table
    = (char **) xmalloc ((1 + sizeof (LONGEST)) * sizeof (char *));
  bzero (signed_type_table, (1 + sizeof (LONGEST)));
  signed_type_table[sizeof (char)] = "char";
  signed_type_table[sizeof (short)] = "short";
  signed_type_table[sizeof (long)] = "long";
  signed_type_table[sizeof (int)] = "int";
#ifdef LONG_LONG
  signed_type_table[sizeof (long long)] = "long long";
#endif

  float_type_table
    = (char **) xmalloc ((1 + sizeof (double)) * sizeof (char *));
  bzero (float_type_table, (1 + sizeof (double)));
  float_type_table[sizeof (float)] = "float";
  float_type_table[sizeof (double)] = "double";
}











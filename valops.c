/* Perform non-arithmetic operations on values, for GDB.
   Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/valops.c,v 1.26 91/01/13 23:50:18 robertb Exp $
   $Locker:  $

This file is part of GDB.  */

#include "stdio.h"
#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "value.h"
#include "frame.h"
#include "inferior.h"
#include "ui.h"

#ifdef TEK_HACK
CORE_ADDR int_push ();
#endif


/* Cast value ARG2 to type TYPE and return as a value.
   More general than a C cast: accepts any two types of the same length,
   and if ARG2 is an lvalue it can be cast into anything at all.  */

value
value_cast (type, arg2)
     struct type *type;
     register value arg2;
{
  register enum type_code code1;
  register enum type_code code2;
  register int scalar;

  /* Coerce arrays but not enums.  Enums will work as-is
     and coercing them would cause an infinite recursion.  */
  if (TYPE_CODE (VALUE_TYPE (arg2)) != TYPE_CODE_ENUM)
    COERCE_ARRAY (arg2);

  code1 = TYPE_CODE (type);
  code2 = TYPE_CODE (VALUE_TYPE (arg2));
  scalar = (code2 == TYPE_CODE_INT || code2 == TYPE_CODE_FLT
	    || code2 == TYPE_CODE_ENUM);

  if (code1 == TYPE_CODE_FLT && scalar)
    return value_from_double (type, value_as_double (arg2));
  else if ((code1 == TYPE_CODE_INT || code1 == TYPE_CODE_ENUM)
	   && (scalar || code2 == TYPE_CODE_PTR))
    return value_from_long (type, value_as_long (arg2));
  else if (TYPE_LENGTH (type) == TYPE_LENGTH (VALUE_TYPE (arg2)))
    {
      VALUE_TYPE (arg2) = type;
      return arg2;
    }
  else if (VALUE_LVAL (arg2) == lval_memory)
    {
      return value_at (type, 
                       VALUE_ADDRESS (arg2) + VALUE_OFFSET (arg2), 
                       M_NORMAL);
    }
  else
    ui_badnews(-1,"Invalid cast.");
}

/* Create a value of type TYPE that is zero, and return it.  */

value
value_zero (type, lv)
     struct type *type;
     enum lval_type lv;
{
  register value val = allocate_value (type);

  bzero (VALUE_CONTENTS (val), TYPE_LENGTH (type));
  VALUE_LVAL (val) = lv;

  return val;
}

/* Return the value with a specified type located at specified address.  */

value
value_at (type, addr, usmode)
     struct type *type;
     CORE_ADDR addr;
     int usmode;
{
  register value val = allocate_value (type);
  int temp;

#ifdef TEK_HACK
#ifdef GHSFORTRAN
if (TYPE_CODE(type) ==  TYPE_CODE_FORTRAN_ARRAY)
  temp = read_memory (addr, VALUE_CONTENTS (val), fortran_length(type), usmode);
else
#endif
#endif
  temp = read_memory (addr, VALUE_CONTENTS (val), TYPE_LENGTH (type), usmode);
  if (temp)
    {
      if (have_inferior_p ())
	print_sys_errmsg ("ptrace", temp);
      /* Actually, address between addr and addr + len was out of bounds. */
      ui_badnews(-1,"Cannot read memory: address 0x%x out of bounds.", addr);
    }

  VALUE_LVAL (val) = lval_memory;
  VALUE_ADDRESS (val) = addr;

  return val;
}

/* Store the contents of FROMVAL into the location of TOVAL.
   Return a new value with the location of TOVAL and contents of FROMVAL.  */

value
value_assign (toval, fromval)
     register value toval, fromval;
{
  register struct type *type = VALUE_TYPE (toval);
  register value val;
  char raw_buffer[MAX_REGISTER_RAW_SIZE];
  char virtual_buffer[MAX_REGISTER_VIRTUAL_SIZE];
  int use_buffer = 0;

  extern CORE_ADDR find_saved_register ();

  COERCE_ARRAY (fromval);

  if (VALUE_LVAL (toval) != lval_internalvar)
    fromval = value_cast (type, fromval);

  /* If TOVAL is a special machine register requiring conversion
     of program values to a special raw format,
     convert FROMVAL's contents now, with result in `raw_buffer',
     and set USE_BUFFER to the number of bytes to write.  */

  if (VALUE_REGNO (toval) >= 0
      && REGISTER_CONVERTIBLE (VALUE_REGNO (toval)))
    {
      int regno = VALUE_REGNO (toval);
      if (VALUE_TYPE (fromval) != REGISTER_VIRTUAL_TYPE (regno))
	fromval = value_cast (REGISTER_VIRTUAL_TYPE (regno), fromval);
      bcopy (VALUE_CONTENTS (fromval), virtual_buffer,
	     REGISTER_VIRTUAL_SIZE (regno));
      REGISTER_CONVERT_TO_RAW (regno, virtual_buffer, raw_buffer);
      use_buffer = REGISTER_RAW_SIZE (regno);
    }

  switch (VALUE_LVAL (toval))
    {
    case lval_internalvar:
      set_internalvar (VALUE_INTERNALVAR (toval), fromval);
      break;

    case lval_internalvar_component:
      set_internalvar_component (VALUE_INTERNALVAR (toval),
				 VALUE_OFFSET (toval),
				 VALUE_BITPOS (toval),
				 VALUE_BITSIZE (toval),
				 fromval);
      break;

    case lval_memory:
      if (VALUE_BITSIZE (toval))
	{
	  int val;
	  read_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
		       &val, sizeof val, M_NORMAL);
	  modify_field (&val, (int) value_as_long (fromval),
			VALUE_BITPOS (toval), VALUE_BITSIZE (toval));
	  write_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			&val, sizeof val);
	}
      else if (use_buffer)
	write_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
		      raw_buffer, use_buffer);
      else
	write_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
		      VALUE_CONTENTS (fromval), TYPE_LENGTH (type));
      break;

    case lval_register:
      if (VALUE_BITSIZE (toval))
	{
	  int val;

	  read_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			       &val, sizeof val);
	  modify_field (&val, (int) value_as_long (fromval),
			VALUE_BITPOS (toval), VALUE_BITSIZE (toval));
	  write_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
				&val, sizeof val);
	}
      else if (use_buffer)
	write_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			      raw_buffer, use_buffer);
      else
	write_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			      VALUE_CONTENTS (fromval), TYPE_LENGTH (type));
      break;

    case lval_reg_frame_relative:
      {
	/* value is stored in a series of registers in the frame
	   specified by the structure.  Copy that value out, modify
	   it, and copy it back in.  */
	int amount_to_copy = (VALUE_BITSIZE (toval) ? 1 : TYPE_LENGTH (type));
	int reg_size = REGISTER_RAW_SIZE (VALUE_FRAME_REGNUM (toval));
	int byte_offset = VALUE_OFFSET (toval) % reg_size;
	int reg_offset = VALUE_OFFSET (toval) / reg_size;
	int amount_copied;
	char *buffer = (char *) alloca (amount_to_copy);
	int regno;
	FRAME frame;
	CORE_ADDR addr;

	/* Figure out which frame this is in currently.  */
	for (frame = get_current_frame ();
	     frame && FRAME_FP (frame) != VALUE_FRAME (toval);
	     frame = get_prev_frame (frame))
	  ;

	if (!frame)
	  ui_badnews(-1,"Value being assigned to is no longer active.");

	amount_to_copy += (reg_size - amount_to_copy % reg_size);

	/* Copy it out.  */
	for ((regno = VALUE_FRAME_REGNUM (toval) + reg_offset,
	      amount_copied = 0);
	     amount_copied < amount_to_copy;
	     amount_copied += reg_size, regno++)
	  {
	    addr = find_saved_register (frame, regno);
#ifdef TEK_HACK
            if (addr == INVALID_CORE_ADDR)
              ui_badnews(1,"value_assign: register altered and not saved.");
            if (addr < NUM_REGS)
#else /* not TEK_HACK */
	    if (addr == 0)
#endif /* not TEK_HACK */
	      read_register_bytes (REGISTER_BYTE (regno),
				   buffer + amount_copied,
				   reg_size);
	    else
	      read_memory (addr, buffer + amount_copied, reg_size, M_NORMAL);
	  }

	/* Modify what needs to be modified.  */
	if (VALUE_BITSIZE (toval))
	  modify_field (buffer + byte_offset,
			(int) value_as_long (fromval),
			VALUE_BITPOS (toval), VALUE_BITSIZE (toval));
	else if (use_buffer)
	  bcopy (raw_buffer, buffer + byte_offset, use_buffer);
	else
	  bcopy (VALUE_CONTENTS (fromval), buffer + byte_offset,
		 TYPE_LENGTH (type));

	/* Copy it back.  */
	for ((regno = VALUE_FRAME_REGNUM (toval) + reg_offset,
	      amount_copied = 0);
	     amount_copied < amount_to_copy;
	     amount_copied += reg_size, regno++)
	  {
	    addr = find_saved_register (frame, regno);
#ifdef TEK_HACK
	    if (addr == INVALID_CORE_ADDR)
              ui_badnews(1,"value_assign: register altered and not saved.");
	    if (addr < NUM_REGS)
#else /* not TEK_HACK */
	    if (addr == 0)
#endif /* not TEK_HACK */
	      write_register_bytes (REGISTER_BYTE (regno),
				    buffer + amount_copied,
				    reg_size);
	    else
	      write_memory (addr, buffer + amount_copied, reg_size);
	  }
      }
      break;
	

#ifdef TEK_HACK
    case lval_reg_invalid:
      ui_badnews(-1,"Left side of = is a register that has been altered, but not saved.");
      break;
#endif /* TEK_HACK */

    default:
      ui_badnews(-1,"Left side of = operation is not an lvalue.");
    }

  /* Return a value just like TOVAL except with the contents of FROMVAL
     (except in the case of the type if TOVAL is an internalvar).  */

  if (VALUE_LVAL (toval) == lval_internalvar
      || VALUE_LVAL (toval) == lval_internalvar_component)
    {
      type = VALUE_TYPE (fromval);
    }

  val = allocate_value (type);
  bcopy (toval, val, VALUE_CONTENTS (val) - (char *) val);
  bcopy (VALUE_CONTENTS (fromval), VALUE_CONTENTS (val), TYPE_LENGTH (type));
  VALUE_TYPE (val) = type;
  
  return val;
}

/* Extend a value VAL to COUNT repetitions of its type.  */

value
value_repeat (arg1, count)
     value arg1;
     int count;
{
  register value val;

  if (VALUE_LVAL (arg1) != lval_memory)
    ui_badnews(-1,"Only values in memory can be extended with '@'.");
  if (count < 1)
    ui_badnews(-1,"Invalid number %d of repetitions.", count);

  val = allocate_repeat_value (VALUE_TYPE (arg1), count);

  read_memory (VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1),
	       VALUE_CONTENTS (val),
	       TYPE_LENGTH (VALUE_TYPE (val)) * count, M_NORMAL);
  VALUE_LVAL (val) = lval_memory;
  VALUE_ADDRESS (val) = VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1);

  return val;
}

value
value_of_variable (var)
     struct symbol *var;
{
  return read_var_value (var, (FRAME) 0);
}

/* Given a value which is an array, return a value which is
   a pointer to its first element.  */

value
value_coerce_array (arg1)
     value arg1;
{
  register struct type *type;
  register value val;

  if (VALUE_LVAL (arg1) != lval_memory)
    ui_badnews(-1,"Attempt to take address of value not located in memory.");

  /* Get type of elements.  */
#if defined(TEK_HACK) && defined(GHSFORTRAN)
  if ((TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_ARRAY) ||
      (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_FORTRAN_ARRAY))
#else
  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_ARRAY)
#endif
    type = TYPE_TARGET_TYPE (VALUE_TYPE (arg1));
  else
    /* A phony array made by value_repeat.
       Its type is the type of the elements, not an array type.  */
    type = VALUE_TYPE (arg1);

  /* Get the type of the result.  */
  type = lookup_pointer_type (type);
  val = value_from_long (builtin_type_long,
		       (LONGEST) (VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1)));
  VALUE_TYPE (val) = type;
  return val;
}

/* Return a pointer value for the object for which ARG1 is the contents.  */

value
value_addr (arg1)
     value arg1;
{
  register struct type *type;
  register value val, arg1_coerced;

  /* Taking the address of an array is really a no-op
     once the array is coerced to a pointer to its first element.  */
  arg1_coerced = arg1;
  COERCE_ARRAY (arg1_coerced);
  if (arg1 != arg1_coerced)
    return arg1_coerced;

  if (VALUE_LVAL (arg1) != lval_memory)
    ui_badnews(-1,"Attempt to take address of value not located in memory.");

  /* Get the type of the result.  */
  type = lookup_pointer_type (VALUE_TYPE (arg1));
  val = value_from_long (builtin_type_long,
		(LONGEST) (VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1)));
  VALUE_TYPE (val) = type;
  return val;
}

/* Given a value of a pointer type, apply the C unary * operator to it.  */

value
value_ind (arg1)
     value arg1;
{
  /* Must do this before COERCE_ARRAY, otherwise an infinite loop
     will result */
  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_REF)
    return value_at (TYPE_TARGET_TYPE (VALUE_TYPE (arg1)),
		     (CORE_ADDR) value_as_long (arg1), M_NORMAL);

  COERCE_ARRAY (arg1);

  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_MEMBER)
    ui_badnews(-1,"not implemented: member types in value_ind");

  /* Allow * on an integer so we can cast it to whatever we want.  */
  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_INT)
    return value_at (builtin_type_long,
		     (CORE_ADDR) value_as_long (arg1), M_NORMAL);
  else if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_PTR)
    return value_at (TYPE_TARGET_TYPE (VALUE_TYPE (arg1)),
		     (CORE_ADDR) value_as_long (arg1), M_NORMAL);
  else if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_REF)
    return value_at (TYPE_TARGET_TYPE (VALUE_TYPE (arg1)),
		     (CORE_ADDR) value_as_long (arg1), M_NORMAL);
  ui_badnews(-1,"Attempt to take contents of a non-pointer value.");
}

/* Pushing small parts of stack frames.  */

/* Push one word (the size of object that a register holds).  */

CORE_ADDR
push_word (sp, buffer)
     CORE_ADDR sp;
     REGISTER_TYPE buffer;
{
  register int len = sizeof (REGISTER_TYPE);

#if 1 INNER_THAN 2
  sp -= len;
  write_memory (sp, &buffer, len);
#else /* stack grows upward */
  write_memory (sp, &buffer, len);
  sp += len;
#endif /* stack grows upward */

  return sp;
}

/* Push LEN bytes with data at BUFFER.  */

CORE_ADDR
push_bytes (sp, buffer, len)
     CORE_ADDR sp;
     char *buffer;
     int len;
{
#if 1 INNER_THAN 2
  sp -= len;
  write_memory (sp, buffer, len);
#else /* stack grows upward */
  write_memory (sp, buffer, len);
  sp += len;
#endif /* stack grows upward */

  return sp;
}

/* Push onto the stack the specified value VALUE.  */

CORE_ADDR
#ifdef TEK_HACK
value_push (sp, arg, regno)
     register CORE_ADDR sp;
     value arg;
     int regno;
#else
value_push (sp, arg)
     register CORE_ADDR sp;
     value arg;
#endif
{
  register int len = TYPE_LENGTH (VALUE_TYPE (arg));

#if 1 INNER_THAN 2
#if defined(TEK_HACK) && defined(m88k)
  if ((TYPE_CODE(VALUE_TYPE(arg)) == TYPE_CODE_FLT) || 
      (check_struct_align(VALUE_TYPE(arg)) == 8))
     if (sp & 0x7)
	sp += sizeof(int);
  write_memory (sp, VALUE_CONTENTS (arg), len);
  sp += len;
#else
  sp -= len;
  write_memory (sp, VALUE_CONTENTS (arg), len);
#endif
#else /* stack grows upward */
  write_memory (sp, VALUE_CONTENTS (arg), len);
  sp += len;
#endif /* stack grows upward */
#ifdef TEK_HACK
#ifdef m88k
  if (regno < 10)
	write_register_bytes(regno*sizeof(int), VALUE_CONTENTS (arg), len);
#endif
#endif

  return sp;
}
#ifdef TEK_HACK
CORE_ADDR
address_push (sp, arg, regno)
     register CORE_ADDR sp;
     value arg;
     int regno;
{
  register int len = TYPE_LENGTH (VALUE_TYPE (arg));

len = 4;
#if 1 INNER_THAN 2
#if defined(TEK_HACK) && defined(m88k)
  write_memory (sp, &VALUE_ADDRESS (arg), len);
  sp += len;
#else
  sp -= len;
  write_memory (sp, &VALUE_ADDRESS (arg), len);
#endif
#else /* stack grows upward */
  write_memory (sp, &VALUE_ADDRESS (arg), len);
  sp += len;
#endif /* stack grows upward */
#ifdef TEK_HACK
#ifdef m88k
  if (regno < 10)
	write_register_bytes(regno*sizeof(int), &VALUE_ADDRESS (arg), len);
#endif
#endif

  return sp;
}
#endif

/* Perform the standard coercions that are specified
   for arguments to be passed to C functions.  */

value
value_arg_coerce (arg)
     value arg;
{
  register struct type *type;

  COERCE_ENUM (arg);

  type = VALUE_TYPE (arg);

  if (TYPE_CODE (type) == TYPE_CODE_INT
      && TYPE_LENGTH (type) < sizeof (int))
    return value_cast (builtin_type_int, arg);

  if (type == builtin_type_float)
    return value_cast (builtin_type_double, arg);

  return arg;
}

/* Push the value ARG, first coercing it as an argument
   to a C function.  */

CORE_ADDR
#ifdef TEK_HACK
value_arg_push (sp, arg, regno)
     register CORE_ADDR sp;
     value arg;
     int regno;
#else
value_arg_push (sp, arg)
     register CORE_ADDR sp;
     value arg;
#endif
{
#ifdef TEK_HACK
  return value_push (sp, value_arg_coerce (arg), regno);
#else
  return value_push (sp, value_arg_coerce (arg));
#endif
}

/* Perform a function call in the inferior.
   ARGS is a vector of values of arguments (NARGS of them).
   FUNCTION is a value, the function to be called.
   Returns a value representing what the function returned.
   May fail to return, if a breakpoint or signal is hit
   during the execution of the function.  */

value
#ifdef TEK_HACK
call_function (function, nargs, args, forceCcall)
     value function;
     int nargs;
     value *args;
     int forceCcall;	/* If non-zero then we must use C calling conventions */
#else
call_function (function, nargs, args)
     value function;
     int nargs;
     value *args;
#endif
{
  CORE_ADDR sp;
  register int i;
  CORE_ADDR start_sp;
  static REGISTER_TYPE dummy[] = CALL_DUMMY;
  static REGISTER_TYPE remote_dummy[] = REMOTE_CALL_DUMMY;
  REGISTER_TYPE dummy1[sizeof dummy / sizeof (REGISTER_TYPE)];
  CORE_ADDR old_sp;
  struct type *value_type;
  unsigned char struct_return;
  CORE_ADDR struct_addr;
  struct inferior_status inf_status;
  struct cleanup *old_chain;
#ifdef TEK_HACK
 CORE_ADDR old_snip, old_sfip;
 int regno, tlen, len, cur_sp, align;
 char *call_regs = (char *) xmalloc(nargs+1);
 struct str_len
 /* used to save lengths of FORTRAN strings */
 {
	int len;
	int reg;
 } *slen = (struct str_len *)xmalloc((nargs + 1) * sizeof(struct str_len));
 int num_of_strings = 0;
 struct block *blk;
 int fort = 0;		/* If non-zero we are calling a FORTRAN function */
 CORE_ADDR save_pc;
#endif

  if (!have_inferior_p ())
    ui_badnews(-1,"Cannot invoke functions if the inferior is not running.");

  save_inferior_status (&inf_status, 1);
  old_chain = make_cleanup (restore_inferior_status, &inf_status);

  PUSH_DUMMY_FRAME;

  old_sp = sp = read_register (SP_REGNUM);
#ifdef TEK_HACK
  old_snip = read_register(SNIP_REGNUM);
  old_sfip = read_register(SFIP_REGNUM);
#endif

#if 1 INNER_THAN 2		/* Stack grows down */
  sp -= sizeof dummy;
#ifdef TEK_HACK
#ifdef m88k
  sp &= ~0xf;	/* make sure we are on a 16 byte boundary */
#endif
#endif
  start_sp = sp;
#else				/* Stack grows up */
  start_sp = sp;
  sp += sizeof dummy;
#endif

  {
    register CORE_ADDR funaddr;
    register struct type *ftype = VALUE_TYPE (function);
    register enum type_code code = TYPE_CODE (ftype);

    /* If it's a member function, just look at the function
       part of it.  */

    /* Determine address to call.  */
    if (code == TYPE_CODE_FUNC || code == TYPE_CODE_METHOD)
      {
	funaddr = VALUE_ADDRESS (function);
	value_type = TYPE_TARGET_TYPE (ftype);
      }
    else if (code == TYPE_CODE_PTR)
      {
	funaddr = value_as_long (function);
#ifdef GHSFORTRAN
	if (TYPE_CODE (TYPE_TARGET_TYPE (ftype)) == TYPE_CODE_FORTRAN_ARRAY)
    	  ui_badnews(-1,"Cannot invoke a fortran array as a function, use [].");
	else if (TYPE_CODE (TYPE_TARGET_TYPE (ftype)) == TYPE_CODE_FUNC
#else
	if (TYPE_CODE (TYPE_TARGET_TYPE (ftype)) == TYPE_CODE_FUNC
#endif
	    || TYPE_CODE (TYPE_TARGET_TYPE (ftype)) == TYPE_CODE_METHOD)
	  value_type = TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (ftype));
	else
	  value_type = builtin_type_int;
      }
    else if (code == TYPE_CODE_INT)
      {
	/* Handle the case of functions lacking debugging info.
	   Their values are characters since their addresses are char */
	if (TYPE_LENGTH (ftype) == 1)
	  funaddr = value_as_long (value_addr (function));
	else
	  /* Handle integer used as address of a function.  */
	  funaddr = value_as_long (function);

	value_type = builtin_type_int;
      }
    else
      ui_badnews(-1,"Invalid data type for function to be called.");

    /* Are we returning a value using a structure return or a normal
       value return? */

    struct_return = using_struct_return (function, funaddr, value_type);

    if (struct_return)
      ui_badnews(-1,"Functions returning struct are not implemented yet.");

#ifdef TEK_HACK
    save_pc = selected_frame->pc;
#endif
    /* Create a call sequence customized for this function
       and the number of arguments for it.  */
    bcopy (remote_debugging ? remote_dummy : dummy, dummy1, sizeof dummy);
    FIX_CALL_DUMMY (dummy1, start_sp, funaddr, nargs, value_type);

  }

#ifndef CANNOT_EXECUTE_STACK
  write_memory (start_sp, dummy1, sizeof dummy);

#else
  /* Convex Unix prohibits executing in the stack segment. */
  /* Hope there is empty room at the top of the text segment. */
  {
    extern CORE_ADDR text_end;
    static checked = 0;
    if (!checked)
      for (start_sp = text_end - sizeof dummy; start_sp < text_end; ++start_sp)
	if (read_memory_integer (start_sp, 1) != 0)
	  ui_badnews(-1,"text segment full -- no place to put call");
    checked = 1;
    sp = old_sp;
    start_sp = text_end - sizeof dummy;
    write_memory (start_sp, dummy1, sizeof dummy);
  }
#endif /* CANNOT_EXECUTE_STACK */
#ifndef TEK_HACK
#ifdef STACK_ALIGN
  /* If stack grows down, we must leave a hole at the top. */
  {
    int len = 0;

    /* Reserve space for the return structure to be written on the
       stack, if necessary */

    if (struct_return)
      len += TYPE_LENGTH (value_type);
    
    for (i = nargs - 1; i >= 0; i--)
      len += TYPE_LENGTH (VALUE_TYPE (args[i]));
#ifdef CALL_DUMMY_STACK_ADJUST
    len += CALL_DUMMY_STACK_ADJUST;
#endif
#if 1 INNER_THAN 2
    sp -= STACK_ALIGN (len) - len;
#else
    sp += STACK_ALIGN (len) - len;
#endif
  }
#endif /* STACK_ALIGN */
#endif /* !TEK_HACK */

    /* Reserve space for the return structure to be written on the
       stack, if necessary */

    if (struct_return)
      {
#if 1 INNER_THAN 2
	sp -= TYPE_LENGTH (value_type);
	struct_addr = sp;
#else
	struct_addr = sp;
	sp += TYPE_LENGTH (value_type);
#endif
      }
    
#ifdef TEK_HACK

  /* Allocate registers and stack space for parameters */

  len = 0;
  blk = get_current_block();

  /* If the break point has occurred in a FORTRAN module then we want to use
     call by reference.  ForceCcall will override this decision.  Call_function
     is used by gdb to malloc space for string constants and in this situation
     forceCcall will be set to TRUE. */

  if (blk && (BLOCK_LANGUAGE(blk) == language_fortran) && !forceCcall)
	fort = 1;
  regno = 2;
  for (i = 0; i <= nargs - 1; i++)
  {
    if (fort)
    {
      /* Make sure we have an address to push */

      force_lval(args[i], &sp);

      call_regs[i] = regno;
      regno++;
      len += sizeof(int);
      if (len = get_str_length(args[i]))
      {
        /* If we have a string then we must save its length to be passed by
           value */

	slen[num_of_strings++].len = len;
      }
    }
    else
    {
    tlen = TYPE_LENGTH (VALUE_TYPE (args[i]));

    if (tlen > 4)
    {
       if (TYPE_CODE(VALUE_TYPE(args[i])) == TYPE_CODE_FLT)
	 if (len & 0x7)
         {
		/* make sure we are on an 8 byte bddy and that the register
		   is even numbered */

                call_regs[i] = regno + 1;
		regno += 3;
		len += 3 * sizeof(int);
	 }
         else
	 {
                call_regs[i] = regno;
		regno += 2;
		len += 2 * sizeof(int);
	 }
       else
       {
	 align = 4;
         if (TYPE_CODE(VALUE_TYPE(args[i])) == TYPE_CODE_STRUCT)
		align = check_struct_align(VALUE_TYPE(args[i]));
	 if ((align == 8) && (len & 0x7))
	 {
		/* make sure we are on an 8 byte bddy and that the register
		   is even numbered */

		regno++;
		len += sizeof(int);
	 }
         call_regs[i] = regno;
         regno += (tlen % sizeof(int) ? tlen/sizeof(int) + 1: tlen/sizeof(int));
         len += (tlen % sizeof(int) ? tlen/sizeof(int) + 1: tlen/sizeof(int)) 
		* sizeof(int);
       }
    }
    else
    {
      call_regs[i] = regno;
      regno++;
      len += sizeof(int);
    }
    }
  }

  /* Num_of_string can be greater than zero iff we are calling a FORTRAN
     function and there are arguments which are strings.  In this case we
     must allocate registers for passing the legnths.  The lengths are passed
     at the end in the order that the strings were processed. */

  for (i=0; i < num_of_strings; i++)
  {
    len += sizeof(int);
    slen[i].reg = regno++;
  }

  if (len < 32)
    len = 32;
  sp -= len;				/* allow 32 byte call area */
  cur_sp = sp &= ~0x7;			/* align on 8 byte bddy */
#endif

#ifdef TEK_HACK
  for (i = 0; i <= nargs - 1; i++)
    if ((fort) && (TYPE_CODE(VALUE_TYPE(args[i])) != TYPE_CODE_PTR))
      sp = address_push (sp, args[i], call_regs[i]);
    else
      sp = value_arg_push (sp, args[i], call_regs[i]);

  /* Num_of_string can be greater than zero iff we are calling a FORTRAN
     function and there are arguments which are strings.  The lengths are 
     passed by value at the end in the order that the strings were processed. */

  for (i = 0; i < num_of_strings; i++)
  {
     sp = int_push(sp, slen[i].len, slen[i].reg);
  }
  free(slen);
  free(call_regs);
#else
  for (i = nargs - 1; i >= 0; i--)
    sp = value_arg_push (sp, args[i]);
#endif
#ifdef TEK_HACK
  sp = cur_sp;
#endif

#ifdef CALL_DUMMY_STACK_ADJUST
#if 1 INNER_THAN 2
  sp -= CALL_DUMMY_STACK_ADJUST;
#else
  sp += CALL_DUMMY_STACK_ADJUST;
#endif
#endif /* CALL_DUMMY_STACK_ADJUST */

  /* Store the address at which the structure is supposed to be
     written.  Note that this (and the code which reserved the space
     above) assumes that gcc was used to compile this function.  Since
     it doesn't cost us anything but space and if the function is pcc
     it will ignore this value, we will make that assumption.

     Also note that on some machines (like the sparc) pcc uses this
     convention in a slightly twisted way also.  */

  if (struct_return)
    STORE_STRUCT_RETURN (struct_addr, sp);

  /* Write the stack pointer.  This is here because the statement above
     might fool with it */
  write_register (SP_REGNUM, sp);

  /* Figure out the value returned by the function.  */
  {
    char retbuf[GENERAL_REGISTER_BYTES];

    /* Execute the stack dummy routine, calling FUNCTION.
       When it is done, discard the empty frame
       after storing the contents of all regs into retbuf.  */
    run_stack_dummy (start_sp + CALL_DUMMY_START_OFFSET, retbuf);

    do_cleanups (old_chain);

#ifdef TEK_HACK

    /*** Revisions to register restore code by andrew@frip.wv.tek.com:
     *** Don't write the SXIP.  There is NEVER a good reason to write an
     *** SXIP back into the inferior.
     *** Don't turn on the valid bits when writing the SNIP and SFIP.
     *** Doing so may turn a bubble into a bogus instruction execution,
     *** as when the SFIP points to the instruction in the shadow of a
     *** non-delayed branch.
     ***/

    write_register (SP_REGNUM, old_sp);
    write_register (SNIP_REGNUM, old_snip);
    write_register (SFIP_REGNUM, old_sfip);
#endif

#ifdef TEK_HACK
    selected_frame->pc = save_pc;
#endif
    return value_being_returned (value_type, retbuf, struct_return);
  }
}

/* Create a value for a string constant:
   Call the function malloc in the inferior to get space for it,
   then copy the data into that space
   and then return the address with type char *.
   PTR points to the string constant data; LEN is number of characters.  */

value
value_string (ptr, len)
     char *ptr;
     int len;
{
  register value val;
  register struct symbol *sym;
  value blocklen;
  register char *copy = (char *) alloca (len + 1);
  char *i = ptr;
  register char *o = copy, *ibeg = ptr;
  register int c;

  /* Copy the string into COPY, processing escapes.
     We could not conveniently process them in expread
     because the string there wants to be a substring of the input.  */

  while (i - ibeg < len)
    {
      c = *i++;
      if (c == '\\')
	{
	  c = parse_escape (&i);
	  if (c == -1)
	    continue;
	}
      *o++ = c;
    }
  *o = 0;

  /* Get the length of the string after escapes are processed.  */

  len = o - copy;

  /* Find the address of malloc in the inferior.  */

  sym = lookup_symbol ("malloc", 0, VAR_NAMESPACE, 0);
  if (sym != 0)
    {
      if (SYMBOL_CLASS (sym) != LOC_BLOCK)
	ui_badnews(-1,"\"malloc\" exists in this program but is not a function.");
      val = value_of_variable (sym);
    }
  else
    {
      register int i;
      for (i = 0; i < misc_function_count; i++)
	if (!strcmp (misc_function_vector[i].name, "malloc"))
	  break;
      if (i < misc_function_count)
	val = value_from_long (builtin_type_long,
			     (LONGEST) misc_function_vector[i].address);
      else
	ui_badnews(-1,"String constants require the program to have a function \"malloc\".");
    }

  blocklen = value_from_long (builtin_type_int, (LONGEST) (len + 1));
#ifdef TEK_HACK
  val = call_function (val, 1, &blocklen, 1);
#else
  val = call_function (val, 1, &blocklen);
#endif
  if (value_zerop (val))
    ui_badnews(-1,"No memory available for string constant.");
  write_memory ((CORE_ADDR) value_as_long (val), copy, len + 1);
  VALUE_TYPE (val) = lookup_pointer_type (builtin_type_char);
#ifdef TEK_HACK

  /* When we call a FORTRAN function we must know the length of the string
     constant.  This seems to be as good a place as any to place this length.  
     Experimentation showed that this member was always zero for strings anyway
     implying it was not being used for anything. */

  VALUE_ADDRESS(val) = len;
#endif /* TEK_HACK */
  return val;
}

/* Given ARG1, a value of type (pointer to a)* structure/union,
   extract the component named NAME from the ultimate target structure/union
   and return it as a value with its appropriate type.
   ERR is used in the error message if ARG1's type is wrong.

   C++: ARGS is a list of argument types to aid in the selection of
   an appropriate method. Also, handle derived types.

   ERR is an error message to be printed in case the field is not found.  */

value
value_struct_elt (arg1, args, name, err)
     register value arg1, *args;
     char *name;
     char *err;
{
  register struct type *t;
  register int i;
  int found = 0;

  struct type *baseclass;

  COERCE_ARRAY (arg1);

  t = VALUE_TYPE (arg1);

  /* Follow pointers until we get to a non-pointer.  */

  while (TYPE_CODE (t) == TYPE_CODE_PTR || TYPE_CODE (t) == TYPE_CODE_REF)
    {
      arg1 = value_ind (arg1);
      COERCE_ARRAY (arg1);
      t = VALUE_TYPE (arg1);
    }

  if (TYPE_CODE (t) == TYPE_CODE_MEMBER)
    ui_badnews(-1,"not implemented: member type in value_struct_elt");

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    ui_badnews(-1,"Attempt to extract a component of a value that is not a %s.", err);

  baseclass = t;

  if (!args)
    {
      /*  if there are no arguments ...do this...  */

      /*  Try as a variable first, because if we succeed, there
	  is less work to be done.  */
      while (t)
	{
#ifdef	DG_HACK
	  for (i = TYPE_NFIELDS (t) - 1; i >= TYPE_N_BASECLASSES (t); i--)
#else /* not DG_HACK */
	  for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
#endif	/* not DG_HACK */
	    {
	      char *tfn = TYPE_FIELD_NAME (t, i);
	      if (tfn && !strcmp (tfn, name))
		{
		  found = 1;
		  break;
		}
	    }

#ifdef	DG_HACK
	  if (found)
#else /* not DG_HACK */
	  if (i >= 0)
#endif	/* not DG_HACK */
	    return TYPE_FIELD_STATIC (t, i)
	      ? value_static_field (t, name, i) : value_field (arg1, i);

	  if (TYPE_N_BASECLASSES (t) == 0)
	    break;

	  t = TYPE_BASECLASS (t, 1);
	  VALUE_TYPE (arg1) = t; /* side effect! */
	}

      /* C++: If it was not found as a data field, then try to
         return it as a pointer to a method.  */
      t = baseclass;
      VALUE_TYPE (arg1) = t;	/* side effect! */

      if (destructor_name_p (name, t))
	ui_badnews(-1,"use `info method' command to print out value of destructor");

      while (t)
	{
	  for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; --i)
	    {
#ifdef TEK_HACK
	      char *tfn = TYPE_FN_FIELDLIST_NAME (t, i);
	      if (tfn && ! strcmp (tfn, name))
#else
	      if (! strcmp (TYPE_FN_FIELDLIST_NAME (t, i), name))
#endif /* TEK_HACK  */
		{
		  ui_badnews(-1,"use `info method' command to print value of method \"%s\"", name);
		}
	    }

	  if (TYPE_N_BASECLASSES (t) == 0)
	    break;

	  t = TYPE_BASECLASS (t, 1);
	}

      if (found == 0)
	ui_badnews(-1,"there is no field named %s", name);
      return 0;
    }

  if (destructor_name_p (name, t))
    {
      if (!args[1])
	{
	  /* destructors are a special case.  */
	  return (value)value_fn_field (arg1, 0,
					TYPE_FN_FIELDLIST_LENGTH (t, 0));
	}
      else
	{
	  ui_badnews(-1,"destructor should not have any argument");
	}
    }

  /*   This following loop is for methods with arguments.  */
  while (t)
    {
      /* Look up as method first, because that is where we
	 expect to find it first.  */
      for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; i--)
	{
	  struct fn_field *f = TYPE_FN_FIELDLIST1 (t, i);

	  char *tfn = TYPE_FN_FIELDLIST_NAME (t, i);
	  if (tfn && !strcmp (tfn, name))
	    {
	      int j;
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (t, i);

	      found = 1;
	      for (j = TYPE_FN_FIELDLIST_LENGTH (t, i) - 1; j >= 0; --j)
		{
		  if (!typecmp (TYPE_FN_FIELD_ARGS (f, j), args))
		    {
		      if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
			return (value)value_virtual_fn_field (arg1, f, j, t);
		      else
			return (value)value_fn_field (arg1, i, j);
		    }
		}
	    }
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;
      
      t = TYPE_BASECLASS (t, 1);
      VALUE_TYPE (arg1) = t;	/* side effect! */
    }

  if (found)
    {
      ui_badnews(-1,"Structure method %s not defined for arglist.", name);
      return 0;
    }
  else
    {
      /* See if user tried to invoke data as function */
      t = baseclass;
      while (t)
	{
#ifdef	DG_HACK
	  for (i = TYPE_NFIELDS (t) - 1; i >= TYPE_N_BASECLASSES (t); i--)
#else /* not DG_HACK */
	  for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
#endif	/* not DG_HACK */
	    {
	      char *tfn = TYPE_FIELD_NAME (t, i);
	      if (tfn && !strcmp (tfn, name))
		{
		  found = 1;
		  break;
		}
	    }

	  if (i >= 0)
	    return TYPE_FIELD_STATIC (t, i)
	      ? value_static_field (t, name, i) : value_field (arg1, i);

	  if (TYPE_N_BASECLASSES (t) == 0)
	    break;

	  t = TYPE_BASECLASS (t, 1);
	  VALUE_TYPE (arg1) = t; /* side effect! */
	}
      ui_badnews(-1,"Structure has no component named %s.", name);
    }
}

/* C++: return 1 is NAME is a legitimate name for the destructor
   of type TYPE.  If TYPE does not have a destructor, or
   if NAME is inappropriate for TYPE, an error is signaled.  */
int
destructor_name_p (name, type)
     char *name;
     struct type *type;
{
  /* destructors are a special case.  */
  char *dname = TYPE_NAME (type);

  if (name[0] == '~')
    {
      if (! TYPE_HAS_DESTRUCTOR (type))
	ui_badnews(-1,"type `%s' does not have destructor defined",
	       TYPE_NAME (type));
      /* Skip past the "struct " at the front.  */
      while (*dname++ != ' ') ;
      if (strcmp (dname, name+1))
	ui_badnews(-1,"destructor specification error");
      else
	return 1;
    }
  return 0;
}

/* C++: Given ARG1, a value of type (pointer to a)* structure/union,
   return 1 if the component named NAME from the ultimate
   target structure/union is defined, otherwise, return 0.  */

int
check_field (arg1, name)
     register value arg1;
     char *name;
{
  register struct type *t;
  register int i;
  int found = 0;

  struct type *baseclass;

  COERCE_ARRAY (arg1);

  t = VALUE_TYPE (arg1);

  /* Follow pointers until we get to a non-pointer.  */

  while (TYPE_CODE (t) == TYPE_CODE_PTR || TYPE_CODE (t) == TYPE_CODE_REF)
    {
      arg1 = value_ind (arg1);
      COERCE_ARRAY (arg1);
      t = VALUE_TYPE (arg1);
    }

  if (TYPE_CODE (t) == TYPE_CODE_MEMBER)
    ui_badnews(-1,"not implemented: member type in check_field");

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    ui_badnews(-1,"Internal error: `this' is not an aggregate");

  baseclass = t;

  while (t)
    {
#ifdef	DG_HACK
	for (i = TYPE_NFIELDS (t) - 1; i >= TYPE_N_BASECLASSES (t); i--)
#else /* not DG_HACK */
	for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
#endif	/* not DG_HACK */
	{
	  char *tfn = TYPE_FIELD_NAME (t, i);
	  if (tfn && !strcmp (tfn, name))
	    {
	      return 1;
	    }
	}
      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
      VALUE_TYPE (arg1) = t;	/* side effect! */
    }

  /* C++: If it was not found as a data field, then try to
     return it as a pointer to a method.  */
  t = baseclass;
  VALUE_TYPE (arg1) = t;	/* side effect! */

  /* Destructors are a special case.  */
  if (destructor_name_p (name, t))
    return 1;

  while (t)
    {
      for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; --i)
	{
	  char *tfn = TYPE_FN_FIELDLIST_NAME (t, i);
	  if (tfn && !strcmp (tfn, name))
	    return 1;
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
    }
  return 0;
}

/* C++: Given an aggregate type DOMAIN, and a member name NAME,
   return the address of this member as a pointer to member
   type.  If INTYPE is non-null, then it will be the type
   of the member we are looking for.  This will help us resolve
   pointers to member functions.  */

value
value_struct_elt_for_address (domain, intype, name)
     struct type *domain, *intype;
     char *name;
{
  register struct type *t = domain;
  register int i;
  int found = 0;
  value v;

  struct type *baseclass;

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    ui_badnews(-1,"Internal error: non-aggregate type to value_struct_elt_for_address");

  baseclass = t;

  while (t)
    {
#ifdef	DG_HACK
      for (i = TYPE_NFIELDS (t) - 1; i >= TYPE_N_BASECLASSES (t); i--)
#else /* not DG_HACK */
      for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
#endif /* not DG_HACK */
	{
	  char *tfn = TYPE_FIELD_NAME (t, i);
	  if (tfn && !strcmp (tfn, name))
	    {
	      if (TYPE_FIELD_PACKED (t, i))
		ui_badnews(-1,"pointers to bitfield members not allowed");

	      v = value_from_long (builtin_type_int,
				   (LONGEST) (TYPE_FIELD_BITPOS (t, i) >> 3));
	      VALUE_TYPE (v) = lookup_pointer_type (
		      lookup_member_type (TYPE_FIELD_TYPE (t, i), baseclass));
	      return v;
	    }
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
    }

  /* C++: If it was not found as a data field, then try to
     return it as a pointer to a method.  */
  t = baseclass;

  /* Destructors are a special case.  */
  if (destructor_name_p (name, t))
    {
      ui_badnews(-1,"pointers to destructors not implemented yet");
    }

  /* Perform all necessary dereferencing.  */
  while (intype && TYPE_CODE (intype) == TYPE_CODE_PTR)
    intype = TYPE_TARGET_TYPE (intype);

  while (t)
    {
      for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; --i)
	{
	  char *tfn = TYPE_FN_FIELDLIST_NAME (t, i);
	  if (tfn && !strcmp (tfn, name))
	    {
	      int j = TYPE_FN_FIELDLIST_LENGTH (t, i);
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (t, i);

	      if (intype == 0 && j > 1)
		ui_badnews(-1,"non-unique member `%s' requires type instantiation", name);
	      if (intype)
		{
		  while (j--)
		    if (TYPE_FN_FIELD_TYPE (f, j) == intype)
		      break;
		  if (j < 0)
		    ui_badnews(-1,"no member function matches that type instantiation");
		}
	      else
		j = 0;

	      if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
		{
		  v = value_from_long (builtin_type_long,
				       (LONGEST) TYPE_FN_FIELD_VOFFSET (f, j));
		}
	      else
		{
		  struct symbol *s = lookup_symbol (TYPE_FN_FIELD_PHYSNAME (f, j),
						    0, VAR_NAMESPACE, 0);
		  v = locate_var_value (s, 0);
		}
	      VALUE_TYPE (v) = lookup_pointer_type (lookup_member_type (TYPE_FN_FIELD_TYPE (f, j), baseclass));
	      return v;
	    }
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
    }
  return 0;
}

/* Compare two argument lists and return the position in which they differ,
   or zero if equal. Note that we ignore the first argument, which is
   the type of the instance variable. This is because we want to handle
   derived classes. This is not entirely correct: we should actually
   check to make sure that a requested operation is type secure,
   shouldn't we? */
int typecmp(t1, t2)
     struct type *t1[];
     value t2[];
{
  int i;

  if (t1[0]->code == TYPE_CODE_VOID) return 0;
  if (!t1[1]) return 0;
  for (i = 1; t1[i] && t1[i]->code != TYPE_CODE_VOID; i++)
    {
      if (! t2[i]
	  || t1[i]->code != t2[i]->type->code
	  || t1[i]->target_type != t2[i]->type->target_type)
	{
	  return i+1;
	}
    }
  if (!t1[i]) return 0;
  return t2[i] ? i+1 : 0;
}

/* C++: return the value of the class instance variable, if one exists.
   Flag COMPLAIN signals an error if the request is made in an
   inappropriate context.  */
value
value_of_this (complain)
     int complain;
{
  extern FRAME selected_frame;
  struct symbol *func, *sym;
  char *funname = 0;
  struct block *b;
  int i;

  if (selected_frame == 0)
    if (complain)
      ui_badnews(-1,"no frame selected");
    else return 0;

  func = get_frame_function (selected_frame);
  if (func)
    funname = SYMBOL_NAME (func);
  else
    if (complain)
      ui_badnews(-1,"no `this' in nameless context");
    else return 0;

  b = SYMBOL_BLOCK_VALUE (func);
  i = BLOCK_NSYMS (b);
  if (i <= 0)
    if (complain)
      ui_badnews(-1,"no args, no `this'");
    else return 0;

  sym = BLOCK_SYM (b, 0);
  if (strncmp ("$this", SYMBOL_NAME (sym), 5))
    if (complain)
      ui_badnews(-1,"current stack frame not in method");
    else return 0;

  return read_var_value (sym, selected_frame);
}
#ifdef TEK_HACK
/*
 *
 * Returns 8 if any field of the structure requires 8 byte alignment, 4 
 * otherwise.  This function calls itself if any fields are structs.
 *
 */

int
check_struct_align(ty)
struct type *ty;
{
	int retval;

	int n = ty->nfields;
	int i;

	for (i = 0; i < n; i++)
	{
		if (TYPE_CODE(ty->fields[i].type) == TYPE_CODE_FLT)
			return 8;
		else if (TYPE_CODE(ty->fields[i].type) == TYPE_CODE_STRUCT)
			retval = check_struct_align(ty->fields[i].type);
		else
			retval = 4;
		if (retval == 8)
			break;
	}
	return retval;
}

/*
 *
 * Make sure we have an l-val so that we can pass its address to a FORTRAN
 * function.  If it is not an l-val we must copy its contents onto the stack
 * and use the stack address as its address.  We set VALUE_ADDRESS(val) to
 * this stack address.  We must also update sp if anything is pushed onto
 * the stack.
 *
 */

force_lval(val, sp)
register CORE_ADDR *sp;
register value val;
{
	register int len;
	register char *addr;

	if (TYPE_CODE(VALUE_TYPE(val)) == TYPE_CODE_PTR)
		return;

	if (VALUE_LVAL(val) == not_lval)
	{
		len = TYPE_LENGTH(VALUE_TYPE(val));
		addr = VALUE_CONTENTS(val);

		if (len == 8)
			*sp &= ~0x7;
		*sp -= len;
  		write_memory (*sp, addr, len);
		VALUE_LVAL(val) = lval_memory;
		VALUE_ADDRESS(val) = *sp;
	}
}

/*
 *
 * If the given VALUE, val, represents a string variable or string constant, 
 * this function returns its length.  If it is not a string then we return 0.
 *
 */

int
get_str_length(val)
register value val;
{
	register struct type *base, *next;
	register int len;

	base = VALUE_TYPE(val);

	if (TYPE_CODE(base) != TYPE_CODE_PTR)
		return 0;
	next = TYPE_TARGET_TYPE(base);

	/* Now find base type */

	while (next)
	{
		len = TYPE_LENGTH(base);
		base = next;
		next = TYPE_TARGET_TYPE(next);
	}

	/* Do we really have a string? */

	if ((TYPE_CODE(base) == TYPE_CODE_INT) && 
	    (strcmp(TYPE_NAME(base),"char") == 0) &&
	    (TYPE_LENGTH(base) == 1))
	{
		if (VALUE_ADDRESS(val))

		/* !!! STRING CONSTANT !!!
		   In the case of a string constant, we cheated and placed its
		   length in the address member of val.  We are assuming that
		   that in all other cases that this member will be 0. */

			return VALUE_ADDRESS(val);
		else
			return len;
	}
	else
		return 0;
}

/* Push onto the stack the specified constant, val.  If regno is <= r9 then
   place argument into regno.  This routine is used by call_function to
   pass string lengths to FORTRAN functions.  */

CORE_ADDR
int_push (sp, val, regno)
     register CORE_ADDR sp;
     int val;
     int regno;
{
  register int len = sizeof(int);

#if 1 INNER_THAN 2
#if defined(TEK_HACK) && defined(m88k)
  write_memory (sp, val, len);
  sp += len;
#else
  sp -= len;
  write_memory (sp, &val, len);
#endif
#else /* stack grows upward */
  write_memory (sp, &val, len);
  sp += len;
#endif /* stack grows upward */
  if (regno < 10)
	write_register_bytes(regno*sizeof(int), &val, len);
  return sp;
}
#endif

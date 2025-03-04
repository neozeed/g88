/* Find a variable's value in memory, for GDB, the GNU debugger.
   Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/findvar.c,v 1.18 90/06/30 17:31:08 robertb Exp $
   $Locker:  $

 */

#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "frame.h"
#include "value.h"
#include "ui.h"
#include "inferior.h"


#ifdef TEK_HACK

/* Return the address in which frame FRAME's value of register REGNUM has been
   saved in memory.  Or, if the value is still in a register, return that
   register's number.  Return INVALID_CORE_ADDR if REGNUM has been altered, but
   not saved. If REGNUM specifies the SP, the value we return is actually
   the SP value, not an address where it was saved.

   If regnum >= NUM_REGS it means that we are to fetch the value of a
   parameter passed in a register.  In this case we call our sister routine
   find_saved_register_on_entry().  */

CORE_ADDR
find_saved_register(frame, regnum)
  FRAME frame;
  int regnum;
{
  CORE_ADDR addr, find_saved_register_on_entry();
  static int recursion_count = 0;

  if (recursion_count++ > 500) {
    recursion_count = 0;
    ui_badnews(-1, "find_saved_register: recursion loop! regnum=%d", 
                                               regnum);
  }

  if (regnum >= NUM_REGS) {
    addr =  find_saved_register_on_entry(frame, regnum - (NUM_REGS - 2));
  } else {
    if (get_current_frame() == frame || 
        frame == (FRAME)0 || 
        frame->next == (FRAME)0) {
      addr = regnum;
    } else {
  
       /* If we are asked for the pc of a frame other than the current
          one, return the saved value of r1, which points to the instruction
          that else { will execute first when the frame is resumed. -rcb 3/90 */
    
      if (regnum == SYNTH_PC_REGNUM) {
        regnum = R1_REGNUM;
      }
    
      addr = FRAME_FIND_SINGLE_SAVED_REG(get_frame_info(frame->next), regnum);
      if (1 < addr && addr < NUM_GENERAL_REGS) {
        addr = find_saved_register(frame->next, addr);
      }
    }
  }
  recursion_count--;
  return addr;
}

/* Return the address in which frame FRAME's value of register REGNUM has been
   saved in memory.  Or, if the value is still in a register, return that
   register's number.  Return INVALID_CORE_ADDR if REGNUM has been altered, but
   not saved. If REGNUM specifies the SP, the value we return is actually
   the SP value, not an address where it was saved.  

   return the address at which regnum has been saved on
   entry to the function that created frame.   */

CORE_ADDR
find_saved_register_on_entry(frame, regnum)
     FRAME frame;
     int regnum;
{
  register CORE_ADDR addr;

  if (regnum >= NUM_REGS) {
    ui_badnews(-1, "find_saved_register_on_entry(0x%x, %d)\n", frame, regnum);
  }

   /* If we are asked for the pc of a frame other than the current
      one, return the saved value of r1, which points to the instruction
      that will execute first when the frame is resumed. -rcb 3/90 */

  if (regnum == SYNTH_PC_REGNUM) {
    regnum = R1_REGNUM;
  }

  addr = FRAME_FIND_SINGLE_SAVED_REG(get_frame_info(frame), regnum);
  if (1 < addr && addr < NUM_GENERAL_REGS) {
    addr = find_saved_register(frame, addr);
  }
  return addr;
}
#endif

/* Copy the bytes of register REGNUM, relative to the current stack frame,
   into our memory at MYADDR.
   The number of bytes copied is REGISTER_RAW_SIZE (REGNUM).  */

#ifdef TEK_HACK
int
#else /* not TEK_HACK*/
void
#endif /* not TEK_HACK */
read_relative_register_raw_bytes (regnum, myaddr)
     int regnum;
     char *myaddr;
{
  register CORE_ADDR addr;

  if (regnum == SYNTH_FP_REGNUM)
    {
      bcopy (&FRAME_FP(selected_frame), myaddr, sizeof (CORE_ADDR));
#ifdef TEK_HACK
      return 0;
#else /* not TEK_HACK */
      return;
#endif /* not TEK_HACK */
    }

  addr = find_saved_register (selected_frame, regnum);

#ifdef TEK_HACK
  if (addr == INVALID_CORE_ADDR)
    return 1;
  if (addr >= NUM_REGS)
#else /* not TEK_HACK */
  if (addr)
#endif /* not TEK_HACK */
    {
      if (regnum == SP_REGNUM)
	{
	  CORE_ADDR buffer = addr;
	  bcopy (&buffer, myaddr, sizeof (CORE_ADDR));
	}
      else
	read_memory (addr, myaddr, REGISTER_RAW_SIZE (regnum), M_NORMAL);
      return;
    }
#ifdef TEK_HACK
  read_register_bytes (REGISTER_BYTE (addr),
		       myaddr, REGISTER_RAW_SIZE (addr));
  return 0;
#else /* not TEK_HACK */
  read_register_bytes (REGISTER_BYTE (regnum),
                       myaddr, REGISTER_RAW_SIZE (regnum));
#endif /* not TEK_HACK */
}

/* Return a `value' with the contents of register REGNUM
   in its virtual format, with the type specified by
   REGISTER_VIRTUAL_TYPE.  */

value
value_of_register (regnum)
     int regnum;
{
  register CORE_ADDR addr;
  register value val;
  char raw_buffer[MAX_REGISTER_RAW_SIZE];
  char virtual_buffer[MAX_REGISTER_VIRTUAL_SIZE];

  if (! (have_inferior_p () || have_core_file_p ()))
    ui_badnews(-1,"Can't get value of register without inferior or core file");
  
  addr =  find_saved_register (selected_frame, regnum);
#ifdef TEK_HACK
  if (addr == INVALID_CORE_ADDR)
    {
      val = allocate_value (REGISTER_VIRTUAL_TYPE (regnum));
      VALUE_LVAL (val) = lval_reg_invalid;
      VALUE_REGNO (val) = regnum;
      return val;
    }
  if (addr >= NUM_REGS)
#else /* not TEK_HACK */
  if (addr != 0)
#endif /* not TEK_HACK */
    {
      if (regnum == SP_REGNUM)
	return value_from_long (builtin_type_int, (LONGEST) addr);
#ifdef TEK_HACK
      switch (read_memory (addr, raw_buffer, REGISTER_RAW_SIZE (regnum), M_NORMAL))
        {
          char bummer_msg[128];

        case 0: break;
        case 1:
          ui_badnews(-1, "couldn't read address 0x%x from core file", addr);
          break;
        default:
          sprintf(bummer_msg, "ptrace couldn't read address 0x%x", addr);
          perror_with_name (bummer_msg);
        }
#else /* not TEK_HACK */
      read_memory (addr, raw_buffer, REGISTER_RAW_SIZE (regnum), M_NORMAL);
#endif /* not TEK_HACK */
    }
  else
#ifdef TEK_HACK
  {
    regnum = addr;
    addr = 0;
    read_register_bytes (REGISTER_BYTE (regnum), raw_buffer,
			 REGISTER_RAW_SIZE (regnum));
  }
#else
    read_register_bytes (REGISTER_BYTE (regnum), raw_buffer,
			 REGISTER_RAW_SIZE (regnum));
#endif

  REGISTER_CONVERT_TO_VIRTUAL (regnum, raw_buffer, virtual_buffer);
  val = allocate_value (REGISTER_VIRTUAL_TYPE (regnum));
  bcopy (virtual_buffer, VALUE_CONTENTS (val), REGISTER_VIRTUAL_SIZE (regnum));
  VALUE_LVAL (val) = addr ? lval_memory : lval_register;
  VALUE_ADDRESS (val) = addr ? addr : REGISTER_BYTE (regnum);
  VALUE_REGNO (val) = regnum;
  return val;
}

/* Replacement for obsolete method of reading registers.
   Copy "len" bytes of consecutive data from registers
   starting with the "regbyte"'th byte of register data
   into memory at "myaddr". */
read_register_bytes (regbyte, myaddr, len)
  int regbyte;
  int *myaddr;
  int len;
{
  int regno;
  int count;

  if (regbyte&3 || ((int)myaddr)&3 || len&3)
    ui_badnews(-1, "read_register_bytes(%d,%d,%d): odd arguments.",
	       regbyte, myaddr, len);
  regno = regbyte>>2;
  count = len>>2;
  while (count--)
    *myaddr++ = read_register(regno++);
}

/* Replacement for obsolete method of writing registers.
   Copy "len" bytes of consecutive data from memory at "myaddr"
   into registers starting with the "regbyte"'th byte of register data.  */
write_register_bytes (regbyte, myaddr, len)
  int regbyte;
  int *myaddr;
  int len;
{
  int regno;
  int count;

  if (regbyte&3 || ((int)myaddr)&3 || len&3)
    ui_badnews(-1, "read_register_bytes(%d,%d,%d): odd arguments.",
	       regbyte, myaddr, len);
  regno = regbyte>>2;
  count = len>>2;
  while (count--)
    write_register(regno++, *myaddr++);
}


/* Given a struct symbol for a variable,
   and a stack frame id, read the value of the variable
   and return a (pointer to a) struct value containing the value.  */

value
read_var_value (var, frame)
     register struct symbol *var;
     FRAME frame;
{
  register value v;

  struct frame_info *fi;

  struct type *type = SYMBOL_TYPE (var);
  register CORE_ADDR addr = 0;
  int val = SYMBOL_VALUE (var);
  register int len;

  v = allocate_value (type);
  VALUE_LVAL (v) = lval_memory;	/* The most likely possibility.  */
#ifdef TEK_HACK
#ifdef GHSFORTRAN
if (TYPE_CODE(type) == TYPE_CODE_FORTRAN_ARRAY)
  len = fortran_length(type);
else
#endif
#endif
  len = TYPE_LENGTH (type);

  if (frame == 0) frame = selected_frame;

  switch (SYMBOL_CLASS (var))
    {
    case LOC_CONST:
    case LOC_LABEL:
      bcopy (&val, VALUE_CONTENTS (v), len);
      VALUE_LVAL (v) = not_lval;
      return v;

    case LOC_CONST_BYTES:
      bcopy (val, VALUE_CONTENTS (v), len);
      VALUE_LVAL (v) = not_lval;
      return v;

    case LOC_STATIC:
      addr = val;
      break;

    case LOC_ARG:
      fi = get_frame_info (frame);
      addr = val + FRAME_ARGS_ADDRESS (fi);
      break;
    
    case LOC_REF_ARG:
      fi = get_frame_info (frame);
      addr = val + FRAME_ARGS_ADDRESS (fi);
      addr = read_memory_integer (addr, sizeof (CORE_ADDR));
      break;
      
    case LOC_LOCAL:
      fi = get_frame_info (frame);
      addr = val + FRAME_LOCALS_ADDRESS (fi);
      break;

    case LOC_TYPEDEF:
      ui_badnews(-1,"Cannot look up value of a typedef");

    case LOC_BLOCK:
      VALUE_ADDRESS (v) = BLOCK_START (SYMBOL_BLOCK_VALUE (var));
      return v;

    case LOC_REGISTER:
    case LOC_REGPARM:
      v = value_from_register (type, val, frame);
      return v;
    }

  read_memory (addr, VALUE_CONTENTS (v), len, M_NORMAL);
  VALUE_ADDRESS (v) = addr;
  return v;
}

/* Return a value of type TYPE, stored in register REGNUM, in frame
   FRAME. */

value
value_from_register (type, regnum, frame)
     struct type *type;
     int regnum;
     FRAME frame;
{
  char raw_buffer [MAX_REGISTER_RAW_SIZE];
  char virtual_buffer[MAX_REGISTER_VIRTUAL_SIZE];
  CORE_ADDR addr;
  value v = allocate_value (type);
  int len = TYPE_LENGTH (type);
  char *value_bytes = 0;
  int value_bytes_copied = 0;
  int num_storage_locs;

  VALUE_REGNO (v) = regnum;

  num_storage_locs = (len > REGISTER_VIRTUAL_SIZE (regnum) ?
		      ((len - 1) / REGISTER_RAW_SIZE (regnum)) + 1 :
		      1);

  if (num_storage_locs > 1)
    {
      /* Value spread across multiple storage locations.  */
      
      int local_regnum, current_regnum;
      int mem_stor = 0, reg_stor = 0;
      int mem_tracking = 1;
      CORE_ADDR last_addr = 0;

      value_bytes = (char *) alloca (len + MAX_REGISTER_RAW_SIZE);

      /* Copy all of the data out, whereever it may be.  */

      for (local_regnum = regnum;
	   value_bytes_copied < len;
	   (value_bytes_copied += REGISTER_RAW_SIZE (local_regnum),
	    ++local_regnum))
	{
	  int register_index = local_regnum - regnum;
	  addr = find_saved_register (frame, local_regnum);
#ifdef TEK_HACK
          if (addr == INVALID_CORE_ADDR)
            {
              VALUE_LVAL (v) = lval_reg_invalid;
              return v;
            }
	  if (addr < NUM_REGS) 
            {
              current_regnum = addr;
#else /* not TEK_HACK */
	  if (addr == 0) 
            {
              current_regnum = local_regnum
#endif /* not TEK_HACK */
	      read_register_bytes (REGISTER_BYTE (current_regnum),
				   value_bytes + value_bytes_copied,
				   REGISTER_RAW_SIZE (local_regnum));
	      reg_stor++;
	    }
	  else
	    {
	      read_memory (addr, value_bytes + value_bytes_copied,
			   REGISTER_RAW_SIZE (local_regnum), M_NORMAL);
	      mem_stor++;
	      mem_tracking =
		(mem_tracking
		 && (regnum == local_regnum
		     || addr == last_addr));
	    }
	  last_addr = addr;
	}

      if ((reg_stor && mem_stor)
	  || (mem_stor && !mem_tracking))
	/* Mixed storage; all of the hassle we just went through was
	   for some good purpose.  */
	{
	  VALUE_LVAL (v) = lval_reg_frame_relative;
	  VALUE_FRAME (v) = FRAME_FP (frame);
	  VALUE_FRAME_REGNUM (v) = regnum;
	}
      else if (mem_stor)
	{
	  VALUE_LVAL (v) = lval_memory;
	  VALUE_ADDRESS (v) = 
                      find_saved_register (frame, regnum);
	}
      else if (reg_stor)
	{
	  VALUE_LVAL (v) = lval_register;
	  VALUE_ADDRESS (v) = REGISTER_BYTE (regnum);
	}
      else
	ui_badnews(1,"value_from_register: Value not stored anywhere!");
      
      /* Any structure stored in more than one register will always be
	 an integral number of registers.  Otherwise, you'd need to do
	 some fiddling with the last register copied here for little
	 endian machines.  */

      /* Copy into the contents section of the value.  */
      bcopy (value_bytes, VALUE_CONTENTS (v), len);

      return v;
    }

  /* Data is completely contained within a single register.  Locate the
     register's contents in a real register or in core;
     read the data in raw format.  */
  
  addr = find_saved_register (frame, regnum);
#ifdef TEK_HACK
  if (addr == INVALID_CORE_ADDR)
    {
      VALUE_LVAL (v) = lval_reg_invalid;
      return v;
    }
  if (addr < NUM_REGS)
    regnum = addr;
  if (addr < NUM_REGS)
#else /* not TEK_HACK */
  if (addr == 0)
#endif /* not TEK_HACK */
    {
      /* Value is really in a register.  */
      
      VALUE_LVAL (v) = lval_register;
      VALUE_ADDRESS (v) = REGISTER_BYTE (regnum);
      
      read_register_bytes (REGISTER_BYTE (regnum),
			   raw_buffer, REGISTER_RAW_SIZE (regnum));
    }
  else
    {
      /* Value was in a register that has been saved in memory.  */
      
      read_memory (addr, raw_buffer, REGISTER_RAW_SIZE (regnum), M_NORMAL);
      VALUE_LVAL (v) = lval_memory;
      VALUE_ADDRESS (v) = addr;
    }
  
  /* Convert the raw contents to virtual contents.
     (Just copy them if the formats are the same.)  */
  
  REGISTER_CONVERT_TO_VIRTUAL (regnum, raw_buffer, virtual_buffer);
  
  if (REGISTER_CONVERTIBLE (regnum))
    {
      /* When the raw and virtual formats differ, the virtual format
	 corresponds to a specific data type.  If we want that type,
	 copy the data into the value.
	 Otherwise, do a type-conversion.  */
      
      if (type != REGISTER_VIRTUAL_TYPE (regnum))
	{
	  /* eg a variable of type `float' in a 68881 register
	     with raw type `extended' and virtual type `double'.
	     Fetch it as a `double' and then convert to `float'.  */
	  v = allocate_value (REGISTER_VIRTUAL_TYPE (regnum));
	  bcopy (virtual_buffer, VALUE_CONTENTS (v), len);
	  v = value_cast (type, v);
	}
      else
	bcopy (virtual_buffer, VALUE_CONTENTS (v), len);
    }
  else
    {
      /* Raw and virtual formats are the same for this register.  */

#ifdef BYTES_BIG_ENDIAN
      if (len < REGISTER_RAW_SIZE (regnum))
	{
  	  /* Big-endian, and we want less than full size.  */
	  VALUE_OFFSET (v) = REGISTER_RAW_SIZE (regnum) - len;
	}
#endif

      bcopy (virtual_buffer + VALUE_OFFSET (v),
	     VALUE_CONTENTS (v), len);
    }
  
  return v;
}

/* Given a struct symbol for a variable,
   and a stack frame id, 
   return a (pointer to a) struct value containing the variable's address.  */

value
locate_var_value (var, frame)
     register struct symbol *var;
     FRAME frame;
{
  register CORE_ADDR addr = 0;
  int val = SYMBOL_VALUE (var);
  struct frame_info *fi;
  struct type *type = SYMBOL_TYPE (var);
  struct type *result_type;

  if (frame == 0) frame = selected_frame;

  switch (SYMBOL_CLASS (var))
    {
    case LOC_CONST:
    case LOC_CONST_BYTES:
      ui_badnews(-1,"Address requested for identifier \"%s\" which is a constant.",
	     SYMBOL_NAME (var));

    case LOC_REGISTER:
    case LOC_REGPARM:
      addr = find_saved_register (frame, val);
#ifdef TEK_HACK
      if (addr == INVALID_CORE_ADDR)
        ui_badnews(-1,"Address requested for identifier \"%s\" which is in a register that has been altered and not saved.",
	     SYMBOL_NAME (var));
      if (addr >= NUM_REGS)
#else /* not TEK_HACK */
      if (addr != 0)
#endif /* not TEK_HACK */
	{
	  int len = TYPE_LENGTH (type);
#ifdef BYTES_BIG_ENDIAN
	  if (len < REGISTER_RAW_SIZE (val))
	    /* Big-endian, and we want less than full size.  */
	    addr += REGISTER_RAW_SIZE (val) - len;
#endif
	  break;
	}
      ui_badnews(-1,"Address requested for identifier \"%s\" which is in a register.",
	     SYMBOL_NAME (var));

    case LOC_STATIC:
    case LOC_LABEL:
      addr = val;
      break;

    case LOC_ARG:
      fi = get_frame_info (frame);
      addr = val + FRAME_ARGS_ADDRESS (fi);
      break;

    case LOC_REF_ARG:
      fi = get_frame_info (frame);
      addr = val + FRAME_ARGS_ADDRESS (fi);
      addr = read_memory_integer (addr, sizeof (CORE_ADDR));
      break;

    case LOC_LOCAL:
      fi = get_frame_info (frame);
      addr = val + FRAME_LOCALS_ADDRESS (fi);
      break;

    case LOC_TYPEDEF:
      ui_badnews(-1,"Address requested for identifier \"%s\" which is a typedef.",
	     SYMBOL_NAME (var));

    case LOC_BLOCK:
      addr = BLOCK_START (SYMBOL_BLOCK_VALUE (var));
      break;
    }

  /* Address of an array is of the type of address of it's elements.  */
  result_type =
#if defined(TEK_HACK) && defined(GHSFORTRAN)
    lookup_pointer_type ((TYPE_CODE (type) == TYPE_CODE_ARRAY) || 
		         (TYPE_CODE (type) == TYPE_CODE_FORTRAN_ARRAY) ?
#else
    lookup_pointer_type (TYPE_CODE (type) == TYPE_CODE_ARRAY ?
#endif
			 TYPE_TARGET_TYPE (type) : type);

  return value_cast (result_type,
		     value_from_long (builtin_type_long, (LONGEST) addr));
}


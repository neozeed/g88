/* Tektronix programming extensions to GDB, the GNU debugger.
   Copyright (C) 1989 Free Software Foundation, Inc.


This file is part of GDB.

GDB is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GDB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GDB; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */



/* 
 * Changes made by Tektronix are marked by TEK_HACK, TEK_PROG_HACK, 
 * and GHSFORTRAN.
 * Changes made by Data General are marked by DG_HACK.  Compiling without these
 * #defines should be equivalent to compiling vanilla 3.2, more or less.
 *
 * The changes done by Tektronix fit into three catagories:
 *	TEK_HACK -- these were done just to get GDB to work in our environment,
 *		    including work done to support the Green Hills C compiler.
 *	TEK_PROG_HACK -- These were done to extend GDB by adding programming
 *			 support: things like if-elif-else-endif, while, and
 *			 arguments to user defined commands.
 *	GHSFORTRAN -- These changes make (will make) GDB work with the Green
 *		      Hills Fortran compiler.
 *
 *
 * In addition, I/O routines were renamed so that I/O could be directed
 * to/from the X interface if used.  See the files ui.c and ui.h for
 * more information.  Here are the routines renamed:
 *    Old name:			Renamed to:
 *	fprintf			ui_fprintf
 *	printf			ui_fprintf(stdout
 *	putchar			ui_putchar
 *	putc			ui_putc
 *	fputc			ui_putc
 *	fputs			ui_fputs
 *	puts			ui_puts
 *	gets			ui_gets
 *	fgets			ui_fgets
 *	fflush			ui_fflush
 *	system			ui_system
 *	wait			ui_wait
 *	error			ui_badnews(-1
 *	fatal			ui_badnews(1
 *	getc, fgetc		replaced with ui_gets, ui_fgets
 * These changes are not demarcated by ifdef.   
 *
 *					November 16, 1989
 */



#include <stdio.h>
#define GENERATE_TABLE
#include "m88k-opcode.h"
#include "defs.h"
#include "symtab.h"
#include "ui.h"

INSTAB  *hashtab[HASHVAL] = {0};

void sprint_address ();

/*
*		Disassemble an M88000 Instruction
*
*
*       This module decodes the first instruction in inbuf.  It uses the pc
*	to display pc-relative displacements.  It writes the disassembled
*	instruction in outbuf.
*
*			Revision History
*
*       Revision 1.0    11/08/85        Creation date by Motorola
*			05/11/89	R. Trawick adapted to GDB interface.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/m88k-pinsn.c,v 1.14 90/04/23 13:22:21 andrew Exp $
   $Locker:  $

*/
#define MAXLEN 20

print_insn (memaddr, stream, usmode)
     CORE_ADDR memaddr;
     FILE *stream;
     int usmode;
{
  unsigned char buffer[MAXLEN];
  /* should be expanded if disassembler prints symbol names */
  char outbuf[100];
  int n;

  /* Instruction addresses may have low two bits set. Clear them.	*/
  memaddr&= 0xfffffffc;
  read_memory (memaddr, buffer, MAXLEN, usmode);

  n = m88kdis ((int)memaddr, buffer, outbuf);

  ui_fputs (outbuf, stream);

  return (n);
}

print_insn_imm (value, stream)
  long value;
  FILE *stream;
{
  char outbuf[100];

  /* Arbitrarily disassemble as though instruction is at location 0. */
  (void) m88kdis (0, &value, outbuf);

  ui_fputs (outbuf, stream);
}

#ifdef TEK_HACK

/* Find a given instruction in the instruction table. */

INSTAB *
m88k_inst_find (instruction)
     int instruction;
{
  static ihashtab_initialized = 0;
  INSTAB *entry_ptr;
  unsigned int opcode;
  int opmask;
  int class;

  if (!ihashtab_initialized) {
    init_disasm();
    ihashtab_initialized = 1;
  }
  
  /* create a the appropriate mask to isolate the opcode */
  opmask= DEFMASK;
  class= instruction & DEFMASK;
  if ((class >= SFU0) && (class <= SFU7)) {
    if (instruction < SFU1) {
      opmask= CTRLMASK;
    } else {
      opmask= SFUMASK;
    }
  } else if (class == RRR) {
    opmask= RRRMASK;
  } else if (class == RRI10) {
    opmask= RRI10MASK;
  }
  
  /* isolate the opcode */
  opcode= instruction & opmask;
  
  /* search the hash table with the isolated opcode */
  for (entry_ptr= hashtab[ opcode % HASHVAL ];
       (entry_ptr != NULL) && (entry_ptr->opcode != opcode);
       entry_ptr= entry_ptr->next) {
  }
  
  return entry_ptr;
}
#endif

/*
 * disassemble the first instruction in 'inbuf'.
 * 'pc' should be the address of this instruction, it will
 *   be used to print the target address if this is a relative jump or call
 * 'outbuf' gets filled in with the disassembled instruction.  It should
 *   be long enough to hold the longest disassembled instruction.
 *   100 bytes is certainly enough, unless symbol printing is added later
 * The function returns the length of this instruction in bytes.
 */

int m88kdis( pc, inbuf, outbuf )

    int		pc;
    int	        *inbuf;
    char	*outbuf;

{
#ifndef TEK_HACK
    static		ihashtab_initialized = 0;
#endif /* not TEK_HACK */
    int			instruction;
#ifndef TEK_HACK
    unsigned int	opcode;
#endif /* not TEK_HACK */
    INSTAB		*entry_ptr;
#ifndef TEK_HACK
    int	        	opmask;
    int			class;
#endif /* not TEK_HACK */

    instruction= *inbuf;

#ifdef TEK_HACK
    entry_ptr = m88k_inst_find (instruction);
#else /* not TEK_HACK */
    if (!ihashtab_initialized) {
	init_disasm();
    }

    /* create a the appropriate mask to isolate the opcode */
    opmask= DEFMASK;
    class= instruction & DEFMASK;
    if ((class >= SFU0) && (class <= SFU7)) {
	if (instruction < SFU1) {
	    opmask= CTRLMASK;
	} else {
	    opmask= SFUMASK;
	}
    } else if (class == RRR) {
	opmask= RRRMASK;
    } else if (class == RRI10) {
	opmask= RRI10MASK;
    }

    /* isolate the opcode */
    opcode= instruction & opmask;

    /* search the hash table with the isolated opcode */
    for (entry_ptr= hashtab[ opcode % HASHVAL ];
	 (entry_ptr != NULL) && (entry_ptr->opcode != opcode);
	 entry_ptr= entry_ptr->next) {
    }
#endif /* not TEK_HACK */

    if (entry_ptr == NULL) {
	sprintf( outbuf, "word\t%08x", instruction );
    } else {
#ifdef TEK_HACK
	sprintf( outbuf, "0x%08x  ", instruction );
	sprintf( &outbuf[strlen(outbuf)], "%s", entry_ptr->mnemonic );
#else /* not TEK_HACK */
	sprintf( outbuf, "%s\t", entry_ptr->mnemonic );
#endif /* not TEK_HACK */
	sprintop( &outbuf[strlen(outbuf)], &(entry_ptr->op1), instruction, pc, 1 );
	sprintop( &outbuf[strlen(outbuf)], &(entry_ptr->op2), instruction, pc, 0 );
	sprintop( &outbuf[strlen(outbuf)], &(entry_ptr->op3), instruction, pc, 0 );
    }


    return 4;
}


/*
*                      Decode an Operand of an Instruction
*
*			Functional Description
*
*       This module formats and writes an operand of an instruction to buf
*       based on the operand specification.  When the first flag is set this
*       is the first operand of an instruction.  Undefined operand types
*       cause a <dis error> message.
*
*			Parameters
*	char	 *buf		buffer where the operand may be printed
*       OPSPEC   *opptr         Pointer to an operand specification
*       unsigned inst           Instruction from which operand is extracted
*	unsigned pc		PC of instruction; used for pc-relative disp.
*       int      first          Flag which if nonzero indicates the first
*                               operand of an instruction
*
*			Output
*
*       The operand specified is extracted from the instruction and is
*       written to buf in the format specified. The operand is preceded
*       by a comma if it is not the first operand of an instruction and it
*       is not a register indirect form.  Registers are preceded by 'r' and
*       hex values by '0x'.
*
*			Revision History
*
*       Revision 1.0    11/08/85        Creation date
*/

sprintop( buf, opptr, inst, pc, first )

   char   *buf;
   OPSPEC *opptr;
   unsigned int   inst;
   int	  pc;
   int    first;

{  int	  extracted_field;
   char	  *cond_mask_sym;
   char	  cond_mask_sym_buf[6];

   if (opptr->width == 0)
      return;

   switch(opptr->type) {
      case CRREG:
		       if (!first)
			   *buf++= ',';
		       sprintf( buf, "cr%d", UEXT(inst,opptr->offset,opptr->width));
		       break;

      case FCRREG:
		       if (!first)
			   *buf++= ',';
		       sprintf( buf, "fcr%d", UEXT(inst,opptr->offset,opptr->width));
		       break;

      case REGSC:
		       sprintf( buf, "[r%d]", UEXT(inst,opptr->offset,opptr->width));
		       break;

      case REG:
		       if (!first)
			   *buf++= ',';
		       sprintf( buf, "r%d", UEXT(inst,opptr->offset,opptr->width));
		       break;

      case HEX:
		        if (!first)
			   *buf++= ',';
		        extracted_field= UEXT(inst, opptr->offset, opptr->width);
		        if (extracted_field == 0) {
			    sprintf( buf, "0" );
		        } else {
			    sprintf( buf, "0x%02x", extracted_field );
		        }
		        break;

      case CONDMASK:
		        if (!first)
			   *buf++= ',';
		        extracted_field= UEXT(inst, opptr->offset, opptr->width);
		        switch (extracted_field & 0x0f) {
			  case 0x1:	cond_mask_sym= "gt0";
					break;
			  case 0x2:	cond_mask_sym= "eq0";
					break;
			  case 0x3:	cond_mask_sym= "ge0";
					break;
			  case 0xc:	cond_mask_sym= "lt0";
					break;
			  case 0xd:	cond_mask_sym= "ne0";
					break;
			  case 0xe:	cond_mask_sym= "le0";
					break;
			  default:	cond_mask_sym= cond_mask_sym_buf;
					sprintf( cond_mask_sym_buf,
						 "0x%x",
						 extracted_field );
					break;
			}
			strcpy( buf, cond_mask_sym );
			break;
			
      case CMPRSLT:
		        if (!first)
			   *buf++= ',';
		        extracted_field= UEXT(inst, opptr->offset, opptr->width);
		        switch (extracted_field & 0x0f) {
			  case  2:	cond_mask_sym= "eq(2)";  break;
			  case  3:	cond_mask_sym= "ne(3)";  break;
			  case  4:	cond_mask_sym= "gt(4)";  break;
			  case  5:	cond_mask_sym= "le(5)";  break;
			  case  6:	cond_mask_sym= "lt(6)";  break;
			  case  7:	cond_mask_sym= "ge(7)";  break;
			  case  8:	cond_mask_sym= "hi(8)";  break;
			  case  9:	cond_mask_sym= "ls(9)";  break;
			  case 10:	cond_mask_sym= "lo(10)";  break;
			  case 11:	cond_mask_sym= "hs(11)";  break;

			  default:	cond_mask_sym= cond_mask_sym_buf;
					sprintf( cond_mask_sym_buf,
						 "%d",
						 extracted_field );
			}
			strcpy( buf, cond_mask_sym );
			break;
			
      case PCREL:
		        if (!first)
			   *buf++= ',';
			sprint_address( pc + 4*(SEXT(inst,opptr->offset,opptr->width)),
					buf, 1 );
		        break;

#ifdef NOTDEF /* apparently unused */
      case CONT:
		       sprintf( buf,
				"%d,r%d",
				UEXT(inst,opptr->offset,5),
			        UEXT(inst,(opptr->offset)+5,5) );
		       break;
#endif

      case BF:
		       if (!first)
			   *buf++= ',';
		       sprintf( buf,
				"%d<%d>",
				UEXT(inst,(opptr->offset)+5,5),
			        UEXT(inst,opptr->offset,5));
		       break;

      default:
		       sprintf( buf, "<dis error: %08x>", inst );
    }

}

/*
*                 Initialize the Disassembler Instruction Table
*
*       Initialize the hash table and instruction table for the disassembler.
*       This should be called once before the first call to disasm().
*
*			Parameters
*
*			Output
*
*       If the debug option is selected, certain statistics about the hashing
*       distribution are written to stdout.
*
*			Revision History
*
*       Revision 1.0    11/08/85        Creation date
*/

init_disasm()
{
   int i,size;

   for (i=0 ; i < HASHVAL ; i++)
      hashtab[i] = NULL;

   for (i=0, size =  sizeof(instructions) / sizeof(INSTAB) ; i < size ;
       install(&instructions[i++]));

}

/*
*       Insert an instruction into the disassembler table by hashing the
*       opcode and inserting it into the linked list for that hash value.
*
*			Parameters
*
*       INSTAB *instptr         Pointer to the entry in the instruction table
*                               to be installed
*
*       Revision 1.0    11/08/85        Creation date
*			05/11/89	R. TRAWICK ADAPTED FROM MOTOROLA
*/

install(instptr)
   INSTAB *instptr;
{
   unsigned int i;

   i = (instptr->opcode) % HASHVAL;
   instptr->next = hashtab[i];
   hashtab[i] = instptr;
}


void
sprint_address (addr, buffer, display_raw)
     CORE_ADDR  addr;
     char	*buffer;
     int	display_raw;	/* nonzero to print hex value after symbolic */
{
	register int	i;
	struct symbol	*fs;
	char		*name;
	int		name_location;

	fs = find_pc_function (addr);

	if (!fs) {
	    i = find_pc_misc_function (addr);

	    if (i < 0) {
		if (display_raw)
		  sprintf (buffer, "0x%x", addr);
		return;
	    }

	    name = misc_function_vector[i].name;
	    name_location = misc_function_vector[i].address;
	} else {
	    name = fs->name;
	    name_location = BLOCK_START (SYMBOL_BLOCK_VALUE (fs));
	}

	if (display_raw) {
	  if (addr - name_location)
	    sprintf (buffer, "<%s+%d> (0x%x)", name, addr - name_location, addr);
	  else
	    sprintf (buffer, "<%s> (0x%x)", name, addr);
	} else {
	  if (addr - name_location)
	    sprintf (buffer, "<%s+%d>", name, addr - name_location);
	  else
	    sprintf (buffer, "<%s>", name);
	}
}

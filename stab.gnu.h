/*
   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/stab.gnu.h,v 1.2 89/09/21 10:21:08 paulg Exp $
   $Locker:  $
 */
#ifndef __GNU_STAB__

/* Indicate the GNU stab.h is in use.  */

#define __GNU_STAB__

#define __define_stab(NAME, CODE, STRING) NAME=CODE,

enum __stab_debug_code
{
#include "stab.def"
};

#undef __define_stab

#endif /* __GNU_STAB_ */


/* Stubs for when we build gdb w/o the 88k simulator

   Copyright (C) 1986, 1987, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/sim.c,v 1.2 90/04/29 23:28:18 robertb Exp $
   $Locker:  $
 */

#include "remote.h"

sim_copymem() {}
sim_searchmem() {}
check_sim() {}
_initialize_simgdb() {}
sim_v_to_p() {}
int sim_errno, sim_exception, remote_debugging;
sim_interrupt() {}
sim_readfile() {}
sim_zeromem() {}
read_sim_b() {}
read_sim_h() {}
read_sim_w() {}
write_sim_b() {}
write_sim_h() {}
write_sim_w() {}
sim_init() 
{
  simulator = 0;
  remote_debugging = 0;
  error("This copy of gdb was not linked with the simulator");
}
do_sim_reset() {}
reg_sim() {}
setreg_sim() {}
sim() {}

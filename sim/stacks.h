
/*
 * (c) 1987, 1988 Tektronix
 *
 * "$Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/stacks.h,v 1.5 88/06/24 18:05:15 robertb Exp $";
 */

#ifndef stacks_h
#define stacks_h
Address get_return_address(/* frame */);
int nargspassed(/* frame */);
void getframesize(/* frame */);
void get_frame_pointer(/* frame */);
void get_parameter_registers(/* frame */);
Frame nextframe(/* frame */);
#endif

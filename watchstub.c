/* Stub file for getting gdb to link w/o Tek  watchpoint code.
   Copyright (C) 1989, 1990 Tektronix, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/watchstub.c,v 1.1 90/06/30 17:39:29 robertb Exp $
   $Locker:  $

*/

int silent_watchpoint;
int last_exception_was_precise;
int stop_watchpoint;
struct watchpoint { int x; } *watchpoint_chain;

/*
 * commands_watchpoint_by_num -- attach a list of commands to a watchpoint.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 *
 * Called from either_commands_command() in breakpoint.c.  It duplicates
 * that module, with b being a pointer to struct watchpoint here instead
 * of a struct breakpoint.
 */
int
commands_watchpoint_by_num()
{}

/*
 * condition_watchpoint_by_num -- set condition for a watchpoint.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 */
int
condition_watchpoint_by_num()
{}	

/*
 * delete_watchpoint_by_num -- delete a watchpoint.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 */
int
delete_watchpoint_by_num()
{}

/*
 * disable_watchpoint_by_num -- disable a watchpoint.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 */
int
disable_watchpoint_by_num()
{}

/*
 * enable_watchpoint_by_num -- enable a watchpoint.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 */
int
enable_watchpoint_by_num()
{}

/*
 * enable_delete_watchpoint_by_num -- enable a watchpoint for deletion.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 */
int
enable_delete_watchpoint_by_num()
{}

/*
 * enable_once_watchpoint_by_num -- enable a watchpoint for one hit.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 */
int
enable_once_watchpoint_by_num()
{ }

/*
 * ignore_watchpoint_by_num -- set the ignore count for a watchpoint.
 * Returns 1 if successful, 0 if no watchpoint has this number.
 */
int
ignore_watchpoint_by_num()
{}	

/*
 * delete_all_watchpoints -- delete all watchpoints.
 * This is done when new symbol tables are loaded.
 */
delete_all_watchpoints()
{ }

/*
 * Comments on insert_watchpoints() and remove_watchpoints().
 * All watchpoints are removed when the inferior is stopped, and inserted before
 * the inferior is resumed.  Most of the time this is unnecessary, because,
 * unlike breakpoints, the presence of a watchpoint does not alter the
 * inferior's memory image.  But this approach is much simpler than the
 * alternative, of inserting watchpoints only when new or when a new inferior
 * is forked, because this way we don't have to keep track of watchpoints with
 * overlapping pages.  (Under the alternative, to remove a watchpoint when it
 * is deleted would require checking that no page covered by another watchpoint
 * was involved.)
 * Whether or not these unnecessary system calls are a hindrance to performance
 * has not been determined.
 */

/*
 * insert_watchpoints -- tell the kernel to turn on watchpoints.
 */
insert_watchpoints()
{}

/*
 * remove_watchpoints -- tell the kernel to turn off watchpoints.
 */
remove_watchpoints()
{}

/*
 * mark_watchpoints_out -- clear the "inserted" flag for all watchpoints.
 * This is done when the inferior is loaded.
 */
mark_watchpoints_out()
{}

/*
 * user_watchpoint_hit -- see if a user-mode page-write hit a watchpoint.
 * Called when the kernel has sent SIGTRAP, code TRAP_WPUSER, indicating that
 * the user process has written to a watched page.
 * Resolves whether one or more watchpoints were hit; evaluates watchpoint
 * conditions; executes watchpoint commands.
 * Returns 0 if process execution should not stop, else the number of the
 * watchpoint that caused the stop.  (If more than one watchpoint caused a stop,
 * the last of those watchpoint numbers is returned.)
 */
int
user_watchpoint_hit()
{}

/*
 * kernel_watchpoint_hit -- see if a kernel-mode page-write hit a watchpoint.
 * Called when the kernel has sent SIGTRAP, code TRAP_WPSYS, indicating that a
 * system call by the inferior caused the kernel to write to a page containing
 * a user watchpoint.
 * Resolves whether one or more watchpoints were hit; evaluates watchpoint
 * conditions; executes watchpoint commands.
 * Returns 0 if process execution should not stop, else the number of the
 * watchpoint that caused the stop.  (If more than one watchpoint caused a stop,
 * the last of those watchpoint numbers is returned.)
 */
int
kernel_watchpoint_hit()
{}

/*
 * init_watchpoint
 * This is not called "_initialize_watchpoint" because it is explicitly
 * called from _initialize_breakpoint, so that the order of the lines printed
 * by "help breakpoints" is as desired.
 */
init_watchpoint()
{}

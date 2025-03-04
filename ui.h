/*
  
   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/ui.h,v 1.7 90/08/19 21:18:28 robertb Exp $
   $Locker:  $
 
  Tektronix programming extensions to GDB, the GNU debugger.
  Copyright (C) 1989 Free Software Foundation, Inc.

  This file is part of GDB.  */



#ifndef ui_h
#define ui_h

typedef struct {
    char    *uiv_name;
    int     (*uiv_initialize)();
    int     (*uiv_newprompt)();
    int     (*uiv_fprintf)();
    int     (*uiv_fputc)();
    int     (*uiv_puts)();
    int     (*uiv_fputs)();
    char    *(*uiv_gets)();
    char    *(*uiv_fgets)();
    int     (*uiv_fflush)();
    void    (*uiv_help)();
    void    (*uiv_gripe)();
    int     (*uiv_call)();
    int     (*uiv_wait)();
    void    (*uiv_strtSubProc)();
    void    (*uiv_endSubProc)();
    void    (*uiv_edit)();
    void    (*uiv_getClientInfo)();
    void    (*uiv_doButtons)();
    void    (*uiv_byebye)();
    void    (*uiv_badnews)();
    void    (*uiv_procflush)();
    void    (*uiv_tickleMe)();
    int     (*uiv_system)();
    void    (*uiv_button)();
    void    (*uiv_unbutton)();
    int     (*uiv_query)();
    void    (*uiv_startRedirectOut)();
    void    (*uiv_endRedirectOut)();
    void    (*uiv_doneinit)();

} *UIVector, UIVecStruct;

#define ui_initialize      (*(uiVector->uiv_initialize))
#define ui_newprompt       (*(uiVector->uiv_newprompt))
#define ui_fprintf         (*(uiVector->uiv_fprintf))
#define ui_putchar(ch)     (*(uiVector->uiv_fputc))(ch,stdout)
#define ui_putc(ch,f)      (*(uiVector->uiv_fputc))(ch,f)
#define ui_puts            (*(uiVector->uiv_puts))
#define ui_fputs           (*(uiVector->uiv_fputs))
#define ui_gets            (*(uiVector->uiv_gets))
#define ui_fgets           (*(uiVector->uiv_fgets))
#define ui_fflush          (*(uiVector->uiv_fflush))
#define ui_help            (*(uiVector->uiv_help))
#define ui_gripe           (*(uiVector->uiv_gripe))
#define ui_call            (*(uiVector->uiv_call))
#define ui_wait            (*(uiVector->uiv_wait))
#define ui_strtSubProc     (*(uiVector->uiv_strtSubProc))
#define ui_endSubProc      (*(uiVector->uiv_endSubProc))
#define ui_edit            (*(uiVector->uiv_edit))
#define ui_getClientInfo   (*(uiVector->uiv_getClientInfo))
#define ui_doButtons       (*(uiVector->uiv_doButtons))
#define ui_byebye          (*(uiVector->uiv_byebye))
#define ui_badnews         (*(uiVector->uiv_badnews))
#define ui_procflush       (*(uiVector->uiv_procflush))
#define ui_tickleMe        (*(uiVector->uiv_tickleMe))
#define ui_system          (*(uiVector->uiv_system))
#define ui_button          (*(uiVector->uiv_button))
#define ui_unbutton        (*(uiVector->uiv_unbutton))
#define ui_query           (*(uiVector->uiv_query))
#define ui_startRedirectOut (*(uiVector->uiv_startRedirectOut))
#define ui_endRedirectOut  (*(uiVector->uiv_endRedirectOut))
#define ui_doneinit        (*(uiVector->uiv_doneinit))

#ifndef	ui_c
UIVector    uiVector;
#endif	/* ui_c */

initUserInterface (/* choice, argc, argv */);
#endif	/* ui_h */

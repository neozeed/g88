/* vi_keymap.c -- the keymap for vi_mode in readline (). */

/* $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/readline/RCS/vi_keymap.c,v 1.2 89/11/30 14:42:27 andrew Exp $ */

/* Copyright (C) 1987,1989 Free Software Foundation, Inc.

This file is part of GNU Bash, the Bourne Again SHell.

Bash is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

Bash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with Bash; see the file LICENSE.  If not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "readline.h"

/* An array of function pointers, one for each possible key.  Meta
   keys are ASCII + 128. */
Function *vi_movement_keymap[256] = {

  /* The regular control keys come first. */

  (Function *)0x0,		/* Control-@ */
  (Function *)0x0,		/* Control-a */
  (Function *)0x0,		/* Control-b */
  (Function *)0x0,		/* Control-c */
  rl_vi_eof_maybe,		/* Control-d */
  (Function *)0x0,		/* Control-e */
  (Function *)0x0,		/* Control-f */
  rl_abort,			/* Control-g */
  rl_backward,			/* Control-h */
  (Function *)0x0,		/* Control-i */
  rl_newline,			/* Control-j */
  rl_kill_line,			/* Control-k */
  rl_clear_screen,		/* Control-l */
  rl_newline,			/* Control-m */
  rl_get_next_history,		/* Control-n */
  (Function *)0x0,		/* Control-o */
  rl_get_previous_history,	/* Control-p */
  rl_quoted_insert,		/* Control-q */
  rl_reverse_search_history,	/* Control-r */
  rl_forward_search_history,	/* Control-s */
  rl_transpose_chars,		/* Control-t */
  rl_unix_line_discard,		/* Control-u */
  rl_quoted_insert,		/* Control-v */
  rl_unix_word_rubout,		/* Control-w */
  (Function *)0x0,		/* Control-x */
  rl_yank,			/* Control-y */
  (Function *)0x0,		/* Control-z */

  rl_prefix_meta,		/* Control-[ */
  (Function *)0x0,		/* Control-\ */
  (Function *)0x0,		/* Control-] */
  (Function *)0x0,		/* Control-^ */
  rl_do_undo,			/* Control-_ */

  /* The start of printing characters. */
  rl_forward,			/* SPACE */
  (Function *)0x0,		/* ! */
  (Function *)0x0,		/* " */
  (Function *)0x0,		/* # */
  rl_end_of_line,		/* $ */
  (Function *)0x0,		/* % */
  (Function *)0x0,		/* & */
  (Function *)0x0,		/* ' */
  (Function *)0x0,		/* ( */
  (Function *)0x0,		/* ) */
  (Function *)0x0,		/* * */
  (Function *)0x0,		/* + */
  (Function *)0x0,		/* , */
  rl_vi_arg_digit,		/* - */
  (Function *)0x0,		/* . */
  (Function *)0x0,		/* / */

  /* Regular digits. */
  rl_vi_arg_digit,		/* 0 */
  rl_vi_arg_digit,		/* 1 */
  rl_vi_arg_digit,		/* 2 */
  rl_vi_arg_digit,		/* 3 */
  rl_vi_arg_digit,		/* 4 */
  rl_vi_arg_digit,		/* 5 */
  rl_vi_arg_digit,		/* 6 */
  rl_vi_arg_digit,		/* 7 */
  rl_vi_arg_digit,		/* 8 */
  rl_vi_arg_digit,		/* 9 */

  /* A little more punctuation. */
  (Function *)0x0,		/* : */
  (Function *)0x0,		/* ; */
  (Function *)0x0,		/* < */
  (Function *)0x0,		/* = */
  (Function *)0x0,		/* > */
  (Function *)0x0,		/* ? */
  (Function *)0x0,		/* @ */

  /* Uppercase alphabet. */
  (Function *)0x0,		/* A */
  (Function *)0x0,		/* B */
  (Function *)0x0,		/* C */
  (Function *)0x0,		/* D */
  (Function *)0x0,		/* E */
  (Function *)0x0,		/* F */
  (Function *)0x0,		/* G */
  (Function *)0x0,		/* H */
  (Function *)0x0,		/* I */
  (Function *)0x0,		/* J */
  (Function *)0x0,		/* K */
  (Function *)0x0,		/* L */
  (Function *)0x0,		/* M */
  (Function *)0x0,		/* N */
  (Function *)0x0,		/* O */
  (Function *)0x0,		/* P */
  (Function *)0x0,		/* Q */
  (Function *)0x0,		/* R */
  (Function *)0x0,		/* S */
  (Function *)0x0,		/* T */
  (Function *)0x0,		/* U */
  (Function *)0x0,		/* V */
  (Function *)0x0,		/* W */
  (Function *)0x0,		/* X */
  (Function *)0x0,		/* Y */
  (Function *)0x0,		/* Z */

  /* Some more punctuation. */
  (Function *)0x0,		/* [ */
  (Function *)0x0,		/* \ */
  (Function *)0x0,		/* ] */
  (Function *)0x0,		/* ^ */
  (Function *)0x0,		/* _ */
  (Function *)0x0,		/* ` */

  /* Lowercase alphabet. */
  (Function *)0x0,		/* a */
  rl_vi_prev_word,		/* b */
  rl_vi_change,			/* c */
  (Function *)0x0,		/* d */
  (Function *)0x0,		/* e */
  rl_vi_char_search,		/* f */
  (Function *)0x0,		/* g */
  rl_backward,			/* h */
  rl_vi_insertion_mode,		/* i */
  rl_get_next_history,		/* j */
  rl_get_previous_history,	/* k */
  rl_forward,			/* l */
  (Function *)0x0,		/* m */
  (Function *)0x0,		/* n */
  (Function *)0x0,		/* o */
  (Function *)0x0,		/* p */
  (Function *)0x0,		/* q */
  (Function *)0x0,		/* r */
  (Function *)0x0,		/* s */
  (Function *)0x0,		/* t */
  (Function *)0x0,		/* u */
  (Function *)0x0,		/* v */
  rl_vi_next_word,		/* w */
  rl_delete,			/* x */
  (Function *)0x0,		/* y */
  (Function *)0x0,		/* z */

  /* Final punctuation. */
  (Function *)0x0,		/* { */
  (Function *)0x0,		/* | */
  (Function *)0x0,		/* } */
  (Function *)0x0,		/* ~ */
  rl_backward,			/* RUBOUT */

  /* Meta keys.  Just like above, but the high bit is set. */
  /* The regular control keys come first. */

  (Function *)0x0,		/* Meta-Control-@ */
  (Function *)0x0,		/* Meta-Control-a */
  (Function *)0x0,		/* Meta-Control-b */
  (Function *)0x0,		/* Meta-Control-c */
  (Function *)0x0,		/* Meta-Control-d */
  (Function *)0x0,		/* Meta-Control-e */
  (Function *)0x0,		/* Meta-Control-f */
  (Function *)0x0,		/* Meta-Control-g */
  (Function *)0x0,		/* Meta-Control-h */
  (Function *)0x0,		/* Meta-Control-i */
  rl_emacs_editing_mode,	/* Meta-Control-j */
  (Function *)0x0,		/* Meta-Control-k */
  (Function *)0x0,		/* Meta-Control-l */
  rl_emacs_editing_mode,	/* Meta-Control-m */
  (Function *)0x0,		/* Meta-Control-n */
  (Function *)0x0,		/* Meta-Control-o */
  (Function *)0x0,		/* Meta-Control-p */
  (Function *)0x0,		/* Meta-Control-q */
  rl_revert_line,		/* Meta-Control-r */
  (Function *)0x0,		/* Meta-Control-s */
  (Function *)0x0,		/* Meta-Control-t */
  (Function *)0x0,		/* Meta-Control-u */
  (Function *)0x0,		/* Meta-Control-v */
  (Function *)0x0,		/* Meta-Control-w */
  (Function *)0x0,		/* Meta-Control-x */
  (Function *)0x0,		/* Meta-Control-y */
  (Function *)0x0,		/* Meta-Control-z */

  (Function *)0x0,		/* Meta-Control-[ */
  (Function *)0x0,		/* Meta-Control-\ */
  (Function *)0x0,		/* Meta-Control-] */
  (Function *)0x0,		/* Meta-Control-^ */
  (Function *)0x0,		/* Meta-Control-_ */

  /* The start of printing characters. */
  (Function *)0x0,		/* Meta-SPACE */
  (Function *)0x0,		/* Meta-! */
  (Function *)0x0,		/* Meta-" */
  (Function *)0x0,		/* Meta-# */
  (Function *)0x0,		/* Meta-$ */
  (Function *)0x0,		/* Meta-% */
  (Function *)0x0,		/* Meta-& */
  (Function *)0x0,		/* Meta-' */
  (Function *)0x0,		/* Meta-( */
  (Function *)0x0,		/* Meta-) */
  (Function *)0x0,		/* Meta-* */
  (Function *)0x0,		/* Meta-+ */
  (Function *)0x0,		/* Meta-, */
  rl_digit_argument,		/* Meta-- */
  (Function *)0x0,		/* Meta-. */
  (Function *)0x0,		/* Meta-/ */

  /* Regular digits. */
  rl_digit_argument,		/* Meta-0 */
  rl_digit_argument,		/* Meta-1 */
  rl_digit_argument,		/* Meta-2 */
  rl_digit_argument,		/* Meta-3 */
  rl_digit_argument,		/* Meta-4 */
  rl_digit_argument,		/* Meta-5 */
  rl_digit_argument,		/* Meta-6 */
  rl_digit_argument,		/* Meta-7 */
  rl_digit_argument,		/* Meta-8 */
  rl_digit_argument,		/* Meta-9 */

  /* A little more punctuation. */
  (Function *)0x0,		/* Meta-: */
  (Function *)0x0,		/* Meta-; */
  rl_beginning_of_history,	/* Meta-< */
  (Function *)0x0,		/* Meta-= */
  rl_end_of_history,		/* Meta-> */
  rl_possible_completions,	/* Meta-? */
  (Function *)0x0,		/* Meta-@ */

  /* Uppercase alphabet. */
  (Function *)0x0,		/* Meta-A */
  rl_backward_word,		/* Meta-B */
  rl_capitalize_word,		/* Meta-C */
  rl_kill_word,			/* Meta-D */
  (Function *)0x0,		/* Meta-E */
  rl_forward_word,		/* Meta-F */
  (Function *)0x0,		/* Meta-G */
  (Function *)0x0,		/* Meta-H */
  (Function *)0x0,		/* Meta-I */
  (Function *)0x0,		/* Meta-J */
  (Function *)0x0,		/* Meta-K */
  rl_downcase_word,		/* Meta-L */
  (Function *)0x0,		/* Meta-M */
  (Function *)0x0,		/* Meta-N */
  (Function *)0x0,		/* Meta-O */
  (Function *)0x0,		/* Meta-P */
  (Function *)0x0,		/* Meta-Q */
  rl_revert_line,		/* Meta-R */
  (Function *)0x0,		/* Meta-S */
  rl_transpose_words,		/* Meta-T */
  rl_upcase_word,		/* Meta-U */
  (Function *)0x0,		/* Meta-V */
  (Function *)0x0,		/* Meta-W */
  (Function *)0x0,		/* Meta-X */
  rl_yank_pop,			/* Meta-Y */
  (Function *)0x0,		/* Meta-Z */

  /* Some more punctuation. */
  (Function *)0x0,		/* Meta-[ */
  (Function *)0x0,		/* Meta-\ */
  (Function *)0x0,		/* Meta-] */
  (Function *)0x0,		/* Meta-^ */
  (Function *)0x0,		/* Meta-_ */
  (Function *)0x0,		/* Meta-` */

  /* Lowercase alphabet. */
  rl_do_uppercase_version,	/* Meta-a */
  rl_do_uppercase_version,	/* Meta-b */
  rl_do_uppercase_version,	/* Meta-c */
  rl_do_uppercase_version,	/* Meta-d */
  rl_do_uppercase_version,	/* Meta-e */
  rl_do_uppercase_version,	/* Meta-f */
  rl_do_uppercase_version,	/* Meta-g */
  rl_do_uppercase_version,	/* Meta-h */
  rl_do_uppercase_version,	/* Meta-i */
  rl_do_uppercase_version,	/* Meta-j */
  rl_do_uppercase_version,	/* Meta-k */
  rl_do_uppercase_version,	/* Meta-l */
  rl_do_uppercase_version,	/* Meta-m */
  rl_do_uppercase_version,	/* Meta-n */
  rl_do_uppercase_version,	/* Meta-o */
  rl_do_uppercase_version,	/* Meta-p */
  rl_do_uppercase_version,	/* Meta-q */
  rl_do_uppercase_version,	/* Meta-r */
  rl_do_uppercase_version,	/* Meta-s */
  rl_do_uppercase_version,	/* Meta-t */
  rl_do_uppercase_version,	/* Meta-u */
  rl_do_uppercase_version,	/* Meta-v */
  rl_do_uppercase_version,	/* Meta-w */
  rl_do_uppercase_version,	/* Meta-x */
  rl_do_uppercase_version,	/* Meta-y */
  rl_do_uppercase_version,	/* Meta-z */

  /* Final punctuation. */
  (Function *)0x0,		/* Meta-{ */
  (Function *)0x0,		/* Meta-| */
  (Function *)0x0,		/* Meta-} */
  (Function *)0x0,		/* Meta-~ */
  rl_backward_kill_word		/* Meta-RUBOUT */
};

Function *vi_insertion_keymap[256] = {

  /* The regular control keys come first. */

  (Function *)0x0,		/* Control-@ */
  rl_insert,			/* Control-a */
  rl_insert,			/* Control-b */
  rl_insert,			/* Control-c */
  rl_vi_eof_maybe,		/* Control-d */
  rl_insert,			/* Control-e */
  rl_insert,			/* Control-f */
  rl_insert,			/* Control-g */
  rl_insert,			/* Control-h */
  rl_insert,			/* Control-i */
  rl_newline,			/* Control-j */
  rl_insert,			/* Control-k */
  rl_insert,			/* Control-l */
  rl_newline,			/* Control-m */
  rl_insert,			/* Control-n */
  rl_insert,			/* Control-o */
  rl_insert,			/* Control-p */
  rl_insert,			/* Control-q */
  rl_reverse_search_history,	/* Control-r */
  rl_forward_search_history,	/* Control-s */
  rl_transpose_chars,		/* Control-t */
  rl_unix_line_discard,		/* Control-u */
  rl_quoted_insert,		/* Control-v */
  rl_unix_word_rubout,		/* Control-w */
  rl_insert,			/* Control-x */
  rl_yank,			/* Control-y */
  rl_insert,			/* Control-z */

  rl_vi_movement_mode,		/* Control-[ */
  rl_insert,			/* Control-\ */
  rl_insert,			/* Control-] */
  rl_insert,			/* Control-^ */
  rl_do_undo,			/* Control-_ */

  /* The start of printing characters. */
  rl_insert,			/* SPACE */
  rl_insert,			/* ! */
  rl_insert,			/* " */
  rl_insert,			/* # */
  rl_insert,			/* $ */
  rl_insert,			/* % */
  rl_insert,			/* & */
  rl_insert,			/* ' */
  rl_insert,			/* ( */
  rl_insert,			/* ) */
  rl_insert,			/* * */
  rl_insert,			/* + */
  rl_insert,			/* , */
  rl_insert,			/* - */
  rl_insert,			/* . */
  rl_insert,			/* / */

  /* Regular digits. */
  rl_insert,			/* 0 */
  rl_insert,			/* 1 */
  rl_insert,			/* 2 */
  rl_insert,			/* 3 */
  rl_insert,			/* 4 */
  rl_insert,			/* 5 */
  rl_insert,			/* 6 */
  rl_insert,			/* 7 */
  rl_insert,			/* 8 */
  rl_insert,			/* 9 */

  /* A little more punctuation. */
  rl_insert,			/* : */
  rl_insert,			/* ; */
  rl_insert,			/* < */
  rl_insert,			/* = */
  rl_insert,			/* > */
  rl_insert,			/* ? */
  rl_insert,			/* @ */

  /* Uppercase alphabet. */
  rl_insert,			/* A */
  rl_insert,			/* B */
  rl_insert,			/* C */
  rl_insert,			/* D */
  rl_insert,			/* E */
  rl_insert,			/* F */
  rl_insert,			/* G */
  rl_insert,			/* H */
  rl_insert,			/* I */
  rl_insert,			/* J */
  rl_insert,			/* K */
  rl_insert,			/* L */
  rl_insert,			/* M */
  rl_insert,			/* N */
  rl_insert,			/* O */
  rl_insert,			/* P */
  rl_insert,			/* Q */
  rl_insert,			/* R */
  rl_insert,			/* S */
  rl_insert,			/* T */
  rl_insert,			/* U */
  rl_insert,			/* V */
  rl_insert,			/* W */
  rl_insert,			/* X */
  rl_insert,			/* Y */
  rl_insert,			/* Z */

  /* Some more punctuation. */
  rl_insert,			/* [ */
  rl_insert,			/* \ */
  rl_insert,			/* ] */
  rl_insert,			/* ^ */
  rl_insert,			/* _ */
  rl_insert,			/* ` */

  /* Lowercase alphabet. */
  rl_insert,			/* a */
  rl_insert,			/* b */
  rl_insert,			/* c */
  rl_insert,			/* d */
  rl_insert,			/* e */
  rl_insert,			/* f */
  rl_insert,			/* g */
  rl_insert,			/* h */
  rl_insert,			/* i */
  rl_insert,			/* j */
  rl_insert,			/* k */
  rl_insert,			/* l */
  rl_insert,			/* m */
  rl_insert,			/* n */
  rl_insert,			/* o */
  rl_insert,			/* p */
  rl_insert,			/* q */
  rl_insert,			/* r */
  rl_insert,			/* s */
  rl_insert,			/* t */
  rl_insert,			/* u */
  rl_insert,			/* v */
  rl_insert,			/* w */
  rl_insert,			/* x */
  rl_insert,			/* y */
  rl_insert,			/* z */

  /* Final punctuation. */
  rl_insert,			/* { */
  rl_insert,			/* | */
  rl_insert,			/* } */
  rl_insert,			/* ~ */
  rl_rubout,			/* RUBOUT */

  /* Meta keys.  Just like above, but the high bit is set. */
  /* The regular control keys come first. */

  (Function *)0x0,		/* Meta-Control-@ */
  (Function *)0x0,		/* Meta-Control-a */
  (Function *)0x0,		/* Meta-Control-b */
  (Function *)0x0,		/* Meta-Control-c */
  (Function *)0x0,		/* Meta-Control-d */
  (Function *)0x0,		/* Meta-Control-e */
  (Function *)0x0,		/* Meta-Control-f */
  (Function *)0x0,		/* Meta-Control-g */
  (Function *)0x0,		/* Meta-Control-h */
  (Function *)0x0,		/* Meta-Control-i */
  rl_emacs_editing_mode,	/* Meta-Control-j */
  (Function *)0x0,		/* Meta-Control-k */
  (Function *)0x0,		/* Meta-Control-l */
  rl_emacs_editing_mode,	/* Meta-Control-m */
  (Function *)0x0,		/* Meta-Control-n */
  (Function *)0x0,		/* Meta-Control-o */
  (Function *)0x0,		/* Meta-Control-p */
  (Function *)0x0,		/* Meta-Control-q */
  rl_revert_line,		/* Meta-Control-r */
  (Function *)0x0,		/* Meta-Control-s */
  (Function *)0x0,		/* Meta-Control-t */
  (Function *)0x0,		/* Meta-Control-u */
  (Function *)0x0,		/* Meta-Control-v */
  (Function *)0x0,		/* Meta-Control-w */
  (Function *)0x0,		/* Meta-Control-x */
  (Function *)0x0,		/* Meta-Control-y */
  (Function *)0x0,		/* Meta-Control-z */

  (Function *)0x0,		/* Meta-Control-[ */
  (Function *)0x0,		/* Meta-Control-\ */
  (Function *)0x0,		/* Meta-Control-] */
  (Function *)0x0,		/* Meta-Control-^ */
  (Function *)0x0,		/* Meta-Control-_ */

  /* The start of printing characters. */
  (Function *)0x0,		/* Meta-SPACE */
  (Function *)0x0,		/* Meta-! */
  (Function *)0x0,		/* Meta-" */
  (Function *)0x0,		/* Meta-# */
  (Function *)0x0,		/* Meta-$ */
  (Function *)0x0,		/* Meta-% */
  (Function *)0x0,		/* Meta-& */
  (Function *)0x0,		/* Meta-' */
  (Function *)0x0,		/* Meta-( */
  (Function *)0x0,		/* Meta-) */
  (Function *)0x0,		/* Meta-* */
  (Function *)0x0,		/* Meta-+ */
  (Function *)0x0,		/* Meta-, */
  rl_digit_argument,		/* Meta-- */
  (Function *)0x0,		/* Meta-. */
  (Function *)0x0,		/* Meta-/ */

  /* Regular digits. */
  rl_digit_argument,		/* Meta-0 */
  rl_digit_argument,		/* Meta-1 */
  rl_digit_argument,		/* Meta-2 */
  rl_digit_argument,		/* Meta-3 */
  rl_digit_argument,		/* Meta-4 */
  rl_digit_argument,		/* Meta-5 */
  rl_digit_argument,		/* Meta-6 */
  rl_digit_argument,		/* Meta-7 */
  rl_digit_argument,		/* Meta-8 */
  rl_digit_argument,		/* Meta-9 */

  /* A little more punctuation. */
  (Function *)0x0,		/* Meta-: */
  (Function *)0x0,		/* Meta-; */
  rl_beginning_of_history,	/* Meta-< */
  (Function *)0x0,		/* Meta-= */
  rl_end_of_history,		/* Meta-> */
  rl_possible_completions,	/* Meta-? */
  (Function *)0x0,		/* Meta-@ */

  /* Uppercase alphabet. */
  (Function *)0x0,		/* Meta-A */
  rl_backward_word,		/* Meta-B */
  rl_capitalize_word,		/* Meta-C */
  rl_kill_word,			/* Meta-D */
  (Function *)0x0,		/* Meta-E */
  rl_forward_word,		/* Meta-F */
  (Function *)0x0,		/* Meta-G */
  (Function *)0x0,		/* Meta-H */
  (Function *)0x0,		/* Meta-I */
  (Function *)0x0,		/* Meta-J */
  (Function *)0x0,		/* Meta-K */
  rl_downcase_word,		/* Meta-L */
  (Function *)0x0,		/* Meta-M */
  (Function *)0x0,		/* Meta-N */
  (Function *)0x0,		/* Meta-O */
  (Function *)0x0,		/* Meta-P */
  (Function *)0x0,		/* Meta-Q */
  rl_revert_line,		/* Meta-R */
  (Function *)0x0,		/* Meta-S */
  rl_transpose_words,		/* Meta-T */
  rl_upcase_word,		/* Meta-U */
  (Function *)0x0,		/* Meta-V */
  (Function *)0x0,		/* Meta-W */
  (Function *)0x0,		/* Meta-X */
  rl_yank_pop,			/* Meta-Y */
  (Function *)0x0,		/* Meta-Z */

  /* Some more punctuation. */
  (Function *)0x0,		/* Meta-[ */
  (Function *)0x0,		/* Meta-\ */
  (Function *)0x0,		/* Meta-] */
  (Function *)0x0,		/* Meta-^ */
  (Function *)0x0,		/* Meta-_ */
  (Function *)0x0,		/* Meta-` */

  /* Lowercase alphabet. */
  rl_do_uppercase_version,	/* Meta-a */
  rl_do_uppercase_version,	/* Meta-b */
  rl_do_uppercase_version,	/* Meta-c */
  rl_do_uppercase_version,	/* Meta-d */
  rl_do_uppercase_version,	/* Meta-e */
  rl_do_uppercase_version,	/* Meta-f */
  rl_do_uppercase_version,	/* Meta-g */
  rl_do_uppercase_version,	/* Meta-h */
  rl_do_uppercase_version,	/* Meta-i */
  rl_do_uppercase_version,	/* Meta-j */
  rl_do_uppercase_version,	/* Meta-k */
  rl_do_uppercase_version,	/* Meta-l */
  rl_do_uppercase_version,	/* Meta-m */
  rl_do_uppercase_version,	/* Meta-n */
  rl_do_uppercase_version,	/* Meta-o */
  rl_do_uppercase_version,	/* Meta-p */
  rl_do_uppercase_version,	/* Meta-q */
  rl_do_uppercase_version,	/* Meta-r */
  rl_do_uppercase_version,	/* Meta-s */
  rl_do_uppercase_version,	/* Meta-t */
  rl_do_uppercase_version,	/* Meta-u */
  rl_do_uppercase_version,	/* Meta-v */
  rl_do_uppercase_version,	/* Meta-w */
  rl_do_uppercase_version,	/* Meta-x */
  rl_do_uppercase_version,	/* Meta-y */
  rl_do_uppercase_version,	/* Meta-z */

  /* Final punctuation. */
  (Function *)0x0,		/* Meta-{ */
  (Function *)0x0,		/* Meta-| */
  (Function *)0x0,		/* Meta-} */
  (Function *)0x0,		/* Meta-~ */
  rl_backward_kill_word		/* Meta-RUBOUT */
};

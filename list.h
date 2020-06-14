/****************************************************************************/
/* Copyright 2001 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * List macros
 */

struct list_head {
   struct list_head *next;
   struct list_head *prev;
};

#define offsetof(type_, f_) ((int)&(((type_ *)0)->f_))

#define list_entry(lh_, type_, f_)  ((type_ *)(((char*)(lh_)) - offsetof(type_, f_)))

#define list_add_tail(tail_, lh_) ((tail_)->next = (lh_)->next, (lh_)->next = (tail_), (tail_)->prev = (lh_))
#define list_del(lh_) ((lh_)->prev->next = (lh_)->next, (lh_)->next->prev = (lh_)->prev)

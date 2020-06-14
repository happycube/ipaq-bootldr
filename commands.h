/****************************************************************************/
/* Copyright 2002 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * commands.h
 *
 * 2002-03-07 Sacha Chua <sachac@iname.com>
 *   Started work, basing it loosely on params.h.
 *   This is supposed to make maintaining bootldr documentation easier as well as
 *     simplifying command execution
 */
#ifndef _COMMANDS_H_
#define _COMMANDS_H_
#include "bootldr.h"

struct bootldr_command {
  const char *cmdstr;
  const char *subcmd;
  void (*cmdfunc)(int argc, const char **);
  const char *helpstr;
  enum bootblk_flags flags;
  int offset;   /* number of args to remove from the left side */
};

#define __commanddata	__attribute__ ((__section__ (".data.commands")))

#define COMMAND(cmd_, func_, help_, flags_) \
  void func_(int argc, const char **); \
  struct bootldr_command __commanddata bootldr_cmd_ ## cmd_ = { #cmd_, NULL, func_, #cmd_ " " help_, flags_, 0 } 

#define SUBCOMMAND(cmd_, sub_, func_, help_, flags_, offset_) \
  void func_(int argc, const char **); \
  struct bootldr_command __commanddata bootldr_cmd_ ## cmd_ ## sub_ = { #cmd_, #sub_, func_, #cmd_ " " #sub_ " " help_, flags_, offset_ } 

extern struct bootldr_command __commands_begin;
extern struct bootldr_command __commands_end;

extern struct bootldr_command *get_command(const char *cmd);
extern struct bootldr_command *get_sub_command(const char *cmd, const char *subcmd);
extern int do_command(int argc, const char **argv);

#endif // _COMMANDS_H_

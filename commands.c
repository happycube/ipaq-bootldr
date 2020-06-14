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
 * commands.c
 *
 * 2002-03-07 Sacha Chua <sachac@iname.com>
 *   Started work, basing it loosely on params.h.
 *   This is supposed to make maintaining bootldr documentation easier as well as
 *     simplifying command execution
 */

#include "params.h"
#include "commands.h"
#include "bootldr.h"
#include "serial.h"
#include <string.h>

PARAM(use_new_commands, PT_INT, PF_DECIMAL, 0, NULL);


COMMAND(help, command_new_help, "[command] -- Displays help text", BB_RUN_FROM_RAM);
void command_new_help(int argc, const char **argv)
{
  struct bootldr_command *command;
  const char *search;
  if (argc <= 1) { // all help
	command = (struct bootldr_command *)&__commands_begin;
	while (command < (struct bootldr_command *)&__commands_end) {
	  putstr(command->helpstr);
	  putstr("\r\n");
	  command++;
	}
  } else {
	if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0) {
	  search = argv[1];
	} else {
	  search = argv[0];
	}
	command = (struct bootldr_command *)&__commands_begin;
	while (command < (struct bootldr_command *)&__commands_end) {
	  if (strcmp(command->cmdstr, search) == 0) {
		putstr(command->helpstr);
		putstr("\r\n");
	  }
	  command++;
	}
  } 
}

COMMAND(apropos, command_apropos, "<keyword> -- searches for text in help descriptions", BB_RUN_FROM_RAM);
void command_apropos(int argc, const char **argv)
{
  struct bootldr_command *command;
  if (argc < 2) { 
	putstr("apropos <keyword>\r\n");
  } else {
	command = (struct bootldr_command *)&__commands_begin;
	while (command < (struct bootldr_command *)&__commands_end) {
	  if (strstr(command->helpstr, argv[1]) != NULL) {
		putstr(command->helpstr);
		putstr("\r\n");
	  }
	  command++;
	}
  } 
}


struct bootldr_command *get_command(const char *cmd)
{
   struct bootldr_command *command = (struct bootldr_command *)&__commands_begin;
   while (command < (struct bootldr_command *)&__commands_end) {
	 if (strcmp(cmd, command->cmdstr) == 0 && command->subcmd == NULL) {
	   return command;
	 }
	 command++;
   }
   return NULL;
}

struct bootldr_command *get_sub_command(const char *cmd, const char *subcmd)
{
   struct bootldr_command *command = (struct bootldr_command *)&__commands_begin;
   while (command < (struct bootldr_command *)&__commands_end) {
	 if (command->subcmd != NULL && strcmp(cmd, command->cmdstr) == 0 && strcmp(subcmd, command->subcmd) == 0) {
	   return command;
	 }
	 command++;
   }
   return get_command(cmd);
}

int has_sub_commands(const char *cmd)
{
   struct bootldr_command *command = (struct bootldr_command *)&__commands_begin;
   while (command < (struct bootldr_command *)&__commands_end) {
	 if (command->subcmd != NULL && strcmp(cmd, command->cmdstr) == 0) {
	   return 1;
	 }
	 command++;
   }
   return 0;
}

int do_command(int argc, const char **argv)
{
  struct bootldr_command *command = NULL;

  if (strcmp(argv[argc - 1], "?") == 0 ||
	  strcmp(argv[argc - 1], "help") == 0 ||
	  strcmp(argv[0], "?") == 0 ||
	  strcmp(argv[0], "help") == 0)
  {
	putstr("Trying to load help...\r\n");
	command_new_help(argc, argv);
	return 1;
  }
  
  if (argc >= 2)
	command = get_sub_command(argv[0], argv[1]);
  if (command == NULL) command = get_command(argv[0]);

  if (command != NULL) {
	if (command->cmdfunc != NULL) {
	  if (!amRunningFromRam() || (command->flags & BB_RUN_FROM_RAM)){
		(*command->cmdfunc)(argc - command->offset, argv + command->offset);
		return 1;
	  }
	  else {
		putstr("you can't execute the cmd <");
		putstr(argv[0]);
		putstr("> while running from ram\r\n");
		return 0;
	  }
	}
	return 0;
  }
  else {
    if (has_sub_commands(argv[0])) {
      argv[2] = NULL;
      argv[1] = argv[0];
      argv[0] = "help";
      argc = 2;
      command_new_help(argc, argv);
    } else {
	putstr("Don't understand command ");
	putstr(argv[0]);
	putstr("\r\n");
	return 0;
    }
  }
  return 0;
}




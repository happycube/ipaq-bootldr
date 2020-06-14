/****************************************************************************/
/* Copyright 2001,2002 Compaq Computer Corporation.                         */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * h3600 sleeve driver
 *
 */

struct sleeve_driver;
void h3600_sleeve_insert(void); 
void h3600_sleeve_eject(void); 
int h3600_current_sleeve( void );
int h3600_sleeve_register_driver(struct sleeve_driver *drv);
int h3600_sleeve_unregister_driver(struct sleeve_driver *drv);
int h3600_sleeve_init_module(void);
void h3600_sleeve_cleanup_module(void);
void command_sleeve(int argc, const char* argv[]);

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
 * Generic io and filesystem support
 */

struct iohandle;
struct iohandle_ops {
   int (*len)(struct iohandle *ioh);
   int (*read)(struct iohandle *ioh, char *buf, size_t offset, size_t len);
   int (*prewrite)(struct iohandle *ioh, size_t offset, size_t len);
   int (*write)(struct iohandle *ioh, char *buf, size_t offset, size_t len);
   int (*close)(struct iohandle *ioh);
   void *pdata; /* can point to filesystem or io device */
};

struct iohandle {
  struct iohandle_ops *ops;
  int sector_size;
  void *pdata; /* can point to particular file data */
};

struct filesystem;

struct filesystem_ops {
  int (*mount)(struct filesystem *fs, struct iohandle *ioh);
  int mounted;
  struct iohandle *(*open)(struct filesystem *fs, const char *name);
  int (*close)(struct filesystem *fs, const char *name);
};

struct filesystem {
  struct filesystem_ops *ops;
  void *fsdata;
  struct iohandle *fsdev;
};

extern struct iohandle *open_iohandle(const char *iohandle_spec);

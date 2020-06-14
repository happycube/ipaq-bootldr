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
 * PCMCIA/CF card support
 *
 */


#ifndef _PCMCIA_H_
#define _PCMCIA_H_ 1

#ifdef CONFIG_PCMCIA

enum cis_tuple_type {
   CIS_TUPLE_NULL		= 0x00,
   CIS_TUPLE_DEVICE		= 0x01,
   CIS_TUPLE_LONGLINK_CB	= 0x02,
   CIS_TUPLE_INDIRECT		= 0x03,
   CIS_TUPLE_CONFIG_CB		= 0x04,
   CIS_TUPLE_CFTABLE_ENTRY_CB	= 0x05,
   CIS_TUPLE_LONGLINK_MFC	= 0x06,
   CIS_TUPLE_BAR		= 0x07,
   CIS_TUPLE_PWR_MGMNT		= 0x08,
   CIS_TUPLE_EXTDEVICE		= 0x09,
   CIS_TUPLE_CHECKSUM		= 0x10,
   CIS_TUPLE_LONGLINK_A		= 0x11,
   CIS_TUPLE_LONGLINK_C		= 0x12,
   CIS_TUPLE_LINKTARGET		= 0x13,
   CIS_TUPLE_NO_LINK		= 0x14,
   CIS_TUPLE_VERS_1		= 0x15,
   CIS_TUPLE_ALTSTR		= 0x16,
   CIS_TUPLE_DEVICE_A		= 0x17,
   CIS_TUPLE_JEDEC_C		= 0x18,
   CIS_TUPLE_JEDEC_A		= 0x19,
   CIS_TUPLE_CONFIG		= 0x1a,
   CIS_TUPLE_CFTABLE_ENTRY	= 0x1b,
   CIS_TUPLE_DEVICE_OC		= 0x1c,
   CIS_TUPLE_DEVICE_OA		= 0x1d,
   CIS_TUPLE_DEVICE_GEO		= 0x1e,
   CIS_TUPLE_DEVICE_GEO_A	= 0x1f,
   CIS_TUPLE_MANFID		= 0x20,
   CIS_TUPLE_FUNCID		= 0x21,
   CIS_TUPLE_FUNCE		= 0x22,
   CIS_TUPLE_SWIL		= 0x23,
   CIS_TUPLE_END		= 0xff
};

enum cis_funcid {
   CIS_FUNCID_MULTI	= 0x00,
   CIS_FUNCID_MEMORY	= 0x01,
   CIS_FUNCID_SERIAL	= 0x02,
   CIS_FUNCID_PARALLEL	= 0x03,
   CIS_FUNCID_FIXED	= 0x04,
   CIS_FUNCID_VIDEO	= 0x05,
   CIS_FUNCID_NETWORK	= 0x06,
   CIS_FUNCID_AIMS	= 0x07,
   CIS_FUNCID_SCSI	= 0x08
};



struct pcmcia_socket_state {
   int vs;
   int cd;
   int reset;
   int vcc;
   int vpp;
};

struct card_info {
  short manfid[2];
  short funcid;
  char *name;
};

struct pcmcia_ops {
   char *name;
   int (*card_detect)( u8 socket, u8 *detect);
   int (*card_insert)( u8 socket);
   int (*card_eject)( u8 socket);
   int (*get_socket_state)( u8 socket, struct pcmcia_socket_state *state);
   int (*set_socket_state)( u8 socket, struct pcmcia_socket_state *state);
   int (*map_mem)( u8 socket, size_t len, int cis, /* out */ char **mapping);
   int (*map_io)( u8 socket, size_t len, /* out */ char **mapping);
};

extern struct pcmcia_ops *pcmcia_ops;
extern struct pcmcia_ops *generic_pcmcia_ops;

void pcmcia_register_ops(struct pcmcia_ops *ops);


void command_pcmcia(int argc, const char* argv[]);

#define CALL_PCMCIA(f, args...) \
        { return ( pcmcia_ops && pcmcia_ops->f ? pcmcia_ops->f(args) : -EIO ); }
#define CALL_PCMCIA_GENERIC(f, args...) \
{ \
   if (pcmcia_ops && pcmcia_ops->f) \
      return pcmcia_ops->f(args); \
   else if (generic_pcmcia_ops->f) \
      return generic_pcmcia_ops->f(args);\
   else \
      return -EIO; \
}

#define HFUNC  static __inline__ int

HFUNC pcmcia_card_detect( u8 socket, u8 *detect )  CALL_PCMCIA_GENERIC(card_detect, socket, detect);
HFUNC pcmcia_card_insert( u8 socket )  CALL_PCMCIA_GENERIC(card_insert, socket);
HFUNC pcmcia_card_eject( u8 socket )  CALL_PCMCIA_GENERIC(card_eject, socket);
HFUNC pcmcia_get_socket_state( u8 socket, struct pcmcia_socket_state *state )  CALL_PCMCIA(get_socket_state, socket, state);
HFUNC pcmcia_set_socket_state( u8 socket, struct pcmcia_socket_state *state )  CALL_PCMCIA(set_socket_state, socket, state);
HFUNC pcmcia_map_mem( u8 socket, size_t len, int cis, char **mapping)  CALL_PCMCIA_GENERIC(map_mem, socket, len, cis, mapping);
HFUNC pcmcia_map_io( u8 socket, size_t len, char **mapping)  CALL_PCMCIA_GENERIC(map_io, socket, len, mapping);

/* detect all sockets */
int pcmcia_detect(u8 *detect);
/* insert all sockets */
int pcmcia_insert(void);
/* eject all sockets */
int pcmcia_eject(void);

#else /* not CONFIG_PCMCIA */

/* detect all sockets */
#define pcmcia_detect(detect) (*detect = 0)
/* insert all sockets */
#define pcmcia_insert()  
/* eject all sockets */
#define pcmcia_eject()

#endif /* CONFIG_PCMCIA */
#endif 

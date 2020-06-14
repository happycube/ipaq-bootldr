/****************************************************************************/
/* Copyright 2000 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/* 
 * OCHI structures, copied from the OCHI spec
 * -Jamey 12/11/98
 */

typedef struct _ListEntry {
   struct _ListEntry *flink; /* virtual forward pointer to next structure */
   struct _ListEntry *blink; /* virtual forward pointer to next structure */
} ListEntry;

typedef dword HcEndpointControl;

/* see section 4.2: Endpoint Descriptor */
typedef struct _HcEndpointDescriptor {
   HcEndpointControl control;
   volatile long tailp; /* physical pointer to OhciTransferDescriptor */
   volatile long headp; /* flags + physical pointer to OhciTransferDescriptor */
   volatile long nextED; /* phys ptr to next OhciEndpointDescriptor */
} HcEndpointDescriptor;

#define OhciEDHeadpHalt 0x00000001 /* hardware stopped bit */
#define OhciEDHeadpCarry 0x00000002 /* hardware stopped bit */

typedef unsigned long HcTransferControl;

typedef struct _HCTransferDescriptor {
   HcTransferControl control;
   void * cbp;
   volatile dword nextTD;
   void *BE;
} HcTransferDescriptor;


typedef struct _HcdEndpointDescriptor {
   unsigned char listIndex;
   unsigned char pausedFlag;
   unsigned char reserved[2];
   unsigned long physicalAddress;
   ListEntry link;
   struct _HcdEndpoint *endpoint;
   unsigned long reclamationFrame;
   ListEntry pausedLink;
   HcEndpointDescriptor HcED;
} HcdEndpointDescriptor;

typedef struct _HcdTransferDescriptor {
  int pad;
} HcdTransferDescriptor;

typedef struct _HCCA {
   dword  interruptED[32];
   word frameNumber; /* updated by the HC */
   word pad1; /* this is set to zero when HC updates frame number */
   dword  doneHead;
   char   reservedForHc[116];
} HCCA;


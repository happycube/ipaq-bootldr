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
 *
 */

#define BLOCKHEAD_SIGNATURE 0x0000F00D

typedef long Int32;
typedef char Bool;
#define NULL 0
#define TRUE (1 == 1)
#define FALSE (0 == 1)

typedef struct blockhead_t {
  Int32 signature;
  Bool allocated;
  unsigned long size;
  struct blockhead_t *next;
  struct blockhead_t *prev;
}blockhead;

static blockhead *gHeapBase = NULL;

int mmalloc_init(unsigned char *heap, unsigned long size) {
  if (gHeapBase != NULL) return -1;

#ifdef DEBUG_HEAP
  putstr("malloc_init\r\n");
  putLabeledWord("  heap=", heap);
  putLabeledWord("  heapsize=", size);
#endif
  gHeapBase = (blockhead *)(heap);
  gHeapBase->allocated=FALSE;
  gHeapBase->signature=BLOCKHEAD_SIGNATURE;
  gHeapBase->next=NULL;
  gHeapBase->prev=NULL;
  gHeapBase->size = size - sizeof(blockhead);

  return 0;
}

int compact_heap() {
  // return non-zero if heap was compacted
  return 0;
}

void *mmalloc(unsigned long size) 
{
  blockhead *blockptr = gHeapBase;
  blockhead *newblock;
  Bool compacted = FALSE;

  size = (size+7)&~7; /* unsigned long align the size */
#ifdef DEBUG_HEAP
  putLabeledWord("malloc: size=", size);
#endif
  while (blockptr != NULL) {
    if (blockptr->allocated == FALSE) {
      if (blockptr->size >= size) {
	blockptr->allocated=TRUE;
	if ((blockptr->size - size) > sizeof(blockhead)) {
	  newblock = (blockhead *)((unsigned char *)(blockptr) + sizeof(blockhead) + size);
	  newblock->signature = BLOCKHEAD_SIGNATURE;
	  newblock->prev = blockptr;
	  newblock->next = blockptr->next;
	  newblock->size = blockptr->size - size - sizeof(blockhead);
	  newblock->allocated = FALSE;
	  blockptr->next = newblock;
	  blockptr->size = size;
	} else {
	}
	break;
      }
      else {
	if ((blockptr->next == NULL) && (compacted == FALSE)) {
	  if (compact_heap()) {
	    compacted=TRUE;
	    blockptr = gHeapBase;
	    continue;
	  }
	}
      }
    }
    blockptr = blockptr->next;
  }
#ifdef DEBUG_HEAP
  putLabeledWord("   returning blockptr=", blockptr);
#endif
  if (blockptr == NULL) {
    putstr("\r\n\r\n******** malloc out of storage ********\r\n");
    putLabeledWord("  size=", size);
  }
  return (blockptr != NULL) ? ((unsigned char *)(blockptr)+
			       sizeof(blockhead)) : NULL;
}

void mfree(void *block) {
  blockhead *blockptr;

  if (block == NULL) return;

  blockptr = (blockhead *)((unsigned char *)(block) - sizeof(blockhead));


  if (blockptr->signature != BLOCKHEAD_SIGNATURE) return;

  blockptr->allocated=FALSE;
  return;
}


#if 1

void * zalloc(void* opaque,unsigned int items,unsigned int size)
{
    unsigned char *p;
    int i;
    
    //putLabeledWord("zalloc called with items = ",items);
    //putLabeledWord("zalloc called with size = ",size);
    
    p =  (unsigned char *) mmalloc(items*size); // hopefully, this is what they mean by items.
    for (i=0; i < items*size; i++)
	p[i] = 0x0;
    
    //putLabeledWord("zalloc items  = 0x",items);
    //putLabeledWord("zalloc size  = 0x",size);
    //putLabeledWord("zalloc returns address  = 0x",p);
    return p;
    
}

    
		   
void zfree(void* opaque,void *address)
{
    //putLabeledWord("zfree called with address = ",address);
    mfree(address);
    
}
#endif


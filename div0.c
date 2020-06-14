#include "bootldr.h"

#ifdef __arm__

void
__div0 (void)
{
  putstr ("Division by zero!\r\n");
}

#endif

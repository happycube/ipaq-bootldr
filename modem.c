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
 * modem.c - an implementation of the xmodem protocol from the June 20, 1986
 * specification,  by Peter Boswell.
 *
 * George France <france@crl.dec.com>
 * 
 *
 *  WARNING: - This code has mixed coding styles. 
 */

#include "bootconfig.h"
#include "bootldr.h"
#include "btflash.h"
#if defined(CONFIG_SA1100)
#include "sa1100.h"
#endif
#include "serial.h"
#include "params.h"

/* XMODEM  parameters */
#define RETRIES		  10               /* maximum number of retries */

#if defined(CONFIG_SA1100) || defined(CONFIG_PXA)
#define GET_BYTE_TIMEOUT  90000000         /* magic number which is close to
                                              thirty seconds for the timeout
                                              value. (it is 34secs)         */
#endif

#ifdef CONFIG_MACH_SKIFF
#define GET_BYTE_TIMEOUT  20000000
#endif

/* Line control codes */
#define SOH			0x01	/* start of header */
#define ACK			0x06	/* Acknowledge */
#define NAK			0x15	/* Negative acknowledge */
#define CAN			0x18	/* Cancel */
#define EOT			0x04	/* end of text */
#define CTRLZ                   0x1a    /* CTRL-Z */


/* Return Codes - I know non-negitive error codes */
enum {
     errOK = 0,            /* OK - no error */
     errMalfunctioned,     /* Malfunctioned - recoverable error */
     errFailed,            /* Failed - unrecoverable error */
     errBadPacket,
     errEOT,               /* End of Text */
     errEndOfSession,
     errTimeout,
     errTransmission,
     errSerial,             /* problem with the serial port */
     errBuffer,             /* buffer overflow */
     errNumErrorCodes
};

static int errorCounts[errNumErrorCodes];
#define ERROR(code) (((err == code) || errorCounts[code]++), err = code)
#define _COUNT_PACKET() (errorCounts[errOK]++)

     int        flagCrc,       /* Does the sender support FLAGCRC? */
	        longPacket,    /* Does the sender support 1K Packets? */
	        batch,         /* Does the sender support batch? */
                g;             /* Does the sender support 'G' */
static int        timeout;       /* Timeout in number of seconds */
#ifdef UNUSED_CODE
static int        retries;       /* Number of times to retry a packet */
#endif
static int        userAbort;     /* Does the user want to Abort? */ 
static int        packetNumber;  /* Current Packet Number */
static long       fileLength;     
static long       transferred;   /* Number of bytes transferred */
#ifdef UNUSED_CODE
static int        check_value;   /* one byte checksum or two byte crc */
#endif
static int        err;           /* error code */


/************************************************************************
 *
 *   Global Vars.
 *
 ************************************************************************/


/************************************************************************
 *
 *   Debugging Routines
 *
 ***********************************************************************/

#define DisplayMsg()  DsplyMsg()  /* change to (err) for no 
					       debugging mesages       */

void DsplyMsg(void) {

     switch (err)
     {
     case errOK:                                                  break;
     case errMalfunctioned: putstr("Malfunctioned\r\n");          break;
     case errFailed:        putstr("Failed\r\n");                 break;
     case errBadPacket:     putstr("BadPacket\r\n");              break;
     case errEOT:           putstr("End of Text\r\n");            break;
     case errEndOfSession:  putstr("End of Session\r\n");         break;
     case errTimeout:       putstr("Timed Out\r\n");              break;
     case errTransmission:  putstr("Transmission Breakdown\r\n"); break;
     case errBuffer:        putstr("Buffer Overflow");            break;
     default:              putstr("Unknown Error\r\n ");          break;
     }
     return;
}

/************************************************************************
 *
 *   Utility Routines
 *
 *   init_crc_16 pre-computes a table of constants, using the bitwise
 *               algorithm with the x^16+x^12+x^5+1 polynomial = 0x11021
 *               binary. The leading term is implied. 
 ************************************************************************/

unsigned int crc_table[256];

static void InitCRC16(void)
{
     unsigned i, j, crc;

     for (i=0; i<256; i++) {
	  crc = (i << 8);
	  for (j=0; j<8; j++) 
	       crc = (crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0);
	  crc_table[i] = crc & 0xffff;
     }
}

/*************************************************************************
 *
 *  Interface to Serial Port
 *
 *************************************************************************/

extern struct serial_device* serial;

static byte getBytesWithTimeout()
{
   byte c = 0, rxstat;
   
   dword i;

   err = 0; /* reset errno */
   if (userAbort ) {
      ERROR(errFailed);
      return(c);
   }

   i = timeout;
   while (!serial->char_ready(serial) && i)
   	i--;
   if (!i) {
   	ERROR(errTimeout);
	return(c);
   } else {
	c=serial->read_char(serial);
   }
   
   return(c);
}


/*****************************************************************************
 *   Subroutines
 *      ReceivePacket         --  gets a packet
 *      ReceivePacketReliable --  retries bad packets
 *      ReceiveFile           --  gets the file
 *      ReceiveFallback       --  Fallback to the next lower protocol
 *      SendHandshake         --  Sends a handshake to the sender
 *      ReadBytesWithCheck    --  Reads a Packet and verifies Check Sum
 *      ReadBytesWithCRC      --  Reads a Packet and verifies CRC 16
 *      Gobble                --  Eats bytes from the serial port
 *
 ***************************************************************************/

#ifdef UNUSED_CODE
static void Gobble(void) 
{

   int  i;
   int junkLength = 64;
   byte c;

   do {
       for (i=0; i < junkLength; i++ )
          c = getBytesWithTimeout();
       if (err == errSerial)  /* ignore framing errors */
          err = errOK;
   } while ( err == errOK );
   if ( err == errTimeout)
      err = errOK;
   return;
}
#endif

#ifdef UNUSED_CODE
static void ReadBytesWithCheck( byte *pBuffer, int buf_length ) 
{

   int i;
   byte c;

   check_value = 0;
   for (i=0; i < buf_length; i++) {
     c = getBytesWithTimeout();
     if (err != errOK ) 
        return;
     *(byte *)(pBuffer+i) = c;
     check_value += c;
   }
   c = getBytesWithTimeout();
   if (err != errOK ) 
      return;
   if ( (check_value & 0xFF) != c )
      ERROR(errBadPacket);
   return;
}
#endif

#ifdef UNUSED_CODE
static void ReadBytesWithCRC( byte *pBuffer, int buf_length ) 
{

   int i,
       received_crc;
   byte c;

   check_value = 0;
   for (i=0; i < buf_length; i++) {
     c = getBytesWithTimeout();
     if (err != errOK ) 
        return;
     *(byte *)(pBuffer+i) = c;
     check_value = crc_table[((check_value >> 8 ) ^ c)
                      & 0xFF] ^ (check_value << 8);
   }
   check_value &= 0xFFFF;
   c = getBytesWithTimeout();
   if (err != errOK ) 
      return;
   received_crc = ( c & 0xFF ) << 8;
   c = getBytesWithTimeout();
   if (err != errOK ) 
      return;
   received_crc |= ( c & 0xFF );   
   if ( check_value != received_crc )
      ERROR(errBadPacket);
   return;
}
#endif

#ifdef UNUSED_CODE
static void ReceiveFallback(void) 
{

    if (g) {
       g = 0;
       return;
    }
    if (flagCrc) {
       flagCrc = batch = longPacket = 0;
       return;
    }
    /* g = flagCrc = batch = longPacket = -1; */
    return;
}
#endif

#ifdef UNUSED_CODE
static void SendHandshake(void)
{

   if (g) {
      putc('G');
      return;
   }
   if (flagCrc) {
      putc('C');
      return;
   }
   putc(NAK);
   return;
}
#endif

#ifdef UNUSED_CODE
static void ReceivePacket(byte *pBuffer, int *packetNumber, int *pLength) 
{

   byte startOfPacket = 0;
   byte packet = 0;
   byte packetCheck = 0;
   int  packetLength=0;
 
   packetCheck = getBytesWithTimeout();
   if ( err != errOK )
      return;
   if (packetCheck == EOT ) {
      ERROR(EOT); 
      return;
   }
   do {
      startOfPacket = packet;
      packet = packetCheck;
      if ((packetCheck == CAN) && (packet == CAN)) {
         ERROR(errFailed);
         return;
      }
      packetCheck = getBytesWithTimeout();
      if ( err != errOK )
         return;
    } while ( ( ( startOfPacket != SOH ) && (startOfPacket != 0x02) ) || ( ( (packet ^ packetCheck) & 0xFF) != 0xFF) );


   /* a vaild start of packet */
   if (startOfPacket == SOH)
      packetLength = 128;
   else
      packetLength = 1024;
   
   *packetNumber = packet;
   *pLength = packetLength;

   if (flagCrc) {
        ReadBytesWithCRC( pBuffer, packetLength);
   }
   else {
       ReadBytesWithCheck( pBuffer, packetLength); 
   }
   return;
}
#endif

#ifdef UNUSED_CODE
static void ReceivePacketReliable(byte *pBuffer, int *packetNumber, int *pLength) 
{

   int retries = retries;
   int eotCount = 0;

     do {
        ReceivePacket( pBuffer, packetNumber, pLength );
        if (err == EOT) {  
	     if ( g) {
                err = EOT;
                return;
	     }
             eotCount += 3;
             if ( eotCount >= 6) {
                err = EOT;
                return;
             }
	} else if ((err == errTimeout) && (eotCount > 0 )) {
                  eotCount++;
		  if (eotCount >= 6) {
                     err = EOT;
                     return;
		  }
	} else {  /* Data packet or timeout seen */
             eotCount = 0;
             if ((err == errBadPacket) && ( err != errTimeout))
                return;
             else if (g) {
                     ERROR(errMalfunctioned);
                     return;
	     }
	}
        putc(NAK);
     } while (retries-- > 0 );
     ERROR(errMalfunctioned);
     return;

}
#endif

#ifdef UNUSED_CODE
static void ReceiveFile(dword dldaddr, dword img_size) 
{

   int retries = retries / 2 + 1;
   int totalRetries = ( retries * 3 ) / 2 + 1;
   int packetNumber, packetLength;
   byte *pBuffer;

   /* Try different handshakes until we get the first packet */

   err = errOK;
   pBuffer = (byte *)dldaddr;
   do {
      if (--retries == 0 ) {
	 ReceiveFallback();
         retries = retries / 3;
      }
      if (totalRetries-- == 0) {
         ERROR(errFailed); 
         return;
      }
      SendHandshake();
      ReceivePacketReliable( pBuffer, &packetNumber, &packetLength);
      if ((err == errEOT) || (err == errBadPacket)) {
         Gobble();
      }
   } while ( (err == errTimeout)   ||
             (err == errBadPacket) ||
             (err == errMalfunctioned) ||
             (err == errEOT) );

   if ( ( packetNumber != 0 ) && ( packetNumber != 1 ) ) {
      ERROR(errFailed);
      return;
   }

   /* the first packet tells if the sender is in batch mode */
   if ( packetNumber == 0 ) {
      batch = -1;
      putc(ACK);
      _COUNT_PACKET();
      SendHandshake();  /* get next packet */
      ReceivePacketReliable( pBuffer, &packetNumber, &packetLength );
   } else {
      batch = 0;
      g = 0;      /* g is always batch */
   }
     
   
   packetNumber = 1;
   transferred  = 0;

	 
   /* we have the first packet and the correct protocol */
   /* Now we get the remaining packets */
   while (err == errOK) {
      if (packetNumber == ( packetNumber & 0xFF )) {
         packetNumber++;
         transferred += packetLength;
         if (!g) {
            putc(ACK);       /* ACK correct packet */
            _COUNT_PACKET();
         }
      } else if (packetNumber == (packetNumber - 1) & 0xFF) {
         putc(ACK);  /* ACK repeat of previous packet */
      }
      pBuffer += packetLength;
      if (pBuffer >= (byte *)(dldaddr + img_size - 1024)) {
         ERROR(errBuffer);
         return;
      }
      ReceivePacketReliable( pBuffer, &packetNumber, &packetLength );
   }
   /* ACK the EOT.  */
   if (err == EOT) {
      putc(ACK);
      _COUNT_PACKET();
   }
   return;    
}
#endif


#ifdef UNUSED_CODE
static int init_xmodem_dld(void) 
{

     InitCRC16();  /* init the CRC 16 table */
     

     /* non of these are certain yet,
	each of these setting implies a lower
	protocol  
     */
#ifdef CONFIG_MODEM_CRC
     flagCrc = -1;
#else
     flagCrc = 0;
#endif
#ifdef CONFIG_MODEM_CRC
     longPacket = -1;
#else
     longPacket = 0;
#endif
     batch = 0;
#ifdef CONFIG_MODEM_G
     g = -1;
#else
     g = 0;
#endif
     timeout = GET_BYTE_TIMEOUT;
     retries = RETRIES;
     userAbort = 0;
     packetNumber = 0;
     fileLength = 0;
     transferred = 0;
     err = errOK;
     memset(errorCounts, 0, sizeof(errorCounts));
     return ( errOK );
}
#endif

#ifdef UNUSED_CODE
static void cleanup_xmodem_dld(void) {
}
#endif

#ifdef UNUSED_CODE
/* this implementation of xmodem does not work */
dword xmodem_receive(dword dldaddr, size_t img_size)
{

     if (init_xmodem_dld() == errOK) { /* do the init stuff */
         putstr("ready for xmodem download..\r\n"); 
         ReceiveFile(dldaddr, img_size);
         if (err ==  errOK) {
            putstr("Download Successful\r\n");
         } else {
            putstr("\r\n");
            putstr("Download Failed!\r\n");
            DisplayMsg();

            putLabeledWord("Packets=", errorCounts[errOK]);
            putLabeledWord("Malfunctions=", errorCounts[errMalfunctioned]);
            putLabeledWord("Failed=", errorCounts[errFailed]);
            putLabeledWord("BadPackets=", errorCounts[errBadPacket]);
            putLabeledWord("EndOfText=", errorCounts[errEOT]);
            putLabeledWord("EndOfSession=", errorCounts[errEndOfSession]);
            putLabeledWord("Timeout=", errorCounts[errTimeout]);
            putLabeledWord("Transmissions=", errorCounts[errTransmission]);
            putLabeledWord("SerialErrors=", errorCounts[errSerial]);
            putLabeledWord("BufferOverflows=", errorCounts[errBuffer]);

            transferred = 0;  /* return failure */
        }
        cleanup_xmodem_dld(); /* do the cleanup stuff */
     }
     return transferred;
}
#endif

int  packetLength=128;

#ifdef UNUSED_CODE
static unsigned  updateCrc( byte c, unsigned int crc ) 
{

  int i;

  for (i = 8; --i >= 0; ) {
    if (crc & 0x8000 ) {
      crc <<= 1;
      crc += (((c<<1) & 0400) != 0 );
    }
    else {
      crc <<= 1;
      crc += (((c<<=1) & 0400) != 0);
    }
  }
  return crc;
}
#endif


#ifdef UNUSED_CODE
static void sendPacketTest(byte *pBuffer, int packetNumber, int Length) 
{

   int  i;
   int  csum = 0;
   int temp;

   putLabeledWord("csum=", csum );
   putLabeledWord( "pBuffer=", (int)pBuffer );
   putLabeledWord( "SOH=", SOH );
   putLabeledWord( "packet num & 0xff=", (packetNumber & 0xff) );
   temp = 255 - ( packetNumber & 0xff );
   putLabeledWord( "cmpl of packet num=", temp); 
   putLabeledWord("packetNumber=", packetNumber );

   for (i=1; i <= Length; i++ ) {
     putLabeledWord("i=", i );
     putLabeledWord("pBuffer=", (int)pBuffer);
     putLabeledWord("*pBuffer=", *pBuffer);      
     csum = (csum + *pBuffer) & 0xff;
     pBuffer++;
     transferred++;
   }
   for (i=Length+1; i <= packetLength; i++ ) {
     putc(CTRLZ); 
     csum = (csum + CTRLZ) & 0xff;
   } 

   putLabeledWord("csum=", csum);
 
   return;
}
#endif

static void sendPacket(byte *pBuffer, int packetNumber, int Length) 
{

   int  i;
   int  csum = 0;

   putc( SOH ); 
   putc( packetNumber & 0xff );
   putc(255 - ( packetNumber & 0xff )); 
   
   for (i=1; i <= Length; i++ ) {
     putc( *pBuffer ); 
     csum = (csum + *pBuffer) & 0xff;
     pBuffer++;
     transferred++;
   }
   for (i=Length+1; i <= packetLength; i++ ) {
     putc(CTRLZ); 
     csum = (csum + CTRLZ) & 0xff;
   } 

   putc(csum);
 
   return;
}


static void sendPacketCrc(byte *pBuffer, int packetNumber, int Length) 
{

   int  i;
   int  checkValue = 0;

   putc( SOH ); 
   putc( packetNumber & 0xff );
   putc(255 - ( packetNumber & 0xff )); 
   
   for (i=1; i <= Length; i++ ) {
     putc( *pBuffer ); 
     /* compute the crc */
     checkValue = crc_table[((checkValue >> 8 ) ^ *pBuffer)
                      & 0xFF] ^ (checkValue << 8);
     /*
     for (j = 0; i < 8; ++i ) {
       if (crc & 0x8000) {
         crc <<= 1;
         crc += ((((byte)*pBuffer<<=1) & 0400) != 0);
	 crc = crc << 1 ^ 0x1021;
       }
       else {
         crc <<= 1;
         crc += ((((byte)*pBuffer<<=1) & 0400) != 0);
	 } 
     } */
     pBuffer++;
     transferred++;
   }
   for (i=Length+1; i <= packetLength; i++ ) {
     putc(CTRLZ); 
     /*  crc = crc ^ (int)*pBuffer << 8;
     for (j = 0; i < 8; ++i )
       if (crc & 0x8000)
	 crc = crc << 1 ^ 0x1021;
       else
       crc = crc << 1; */
      checkValue = crc_table[((checkValue >> 8 ) ^ *pBuffer)
                      & 0xFF] ^ (checkValue << 8);
   } 
      
   putc( (checkValue >> 8 ) & 0xff );   
   putc( checkValue  & 0xff );

 
   return;
}

#ifdef UNUSED_CODE
static void sendPacketZero(byte *pBuffer, int packetNumber, int Length) 
{

   int  i;
   int  csum = 0;
   byte ch;

   timeout = GET_BYTE_TIMEOUT;
   ch = 'b';
   while ( ch != NAK ) {
   ch = getBytesWithTimeout(); 
   }
   putc( SOH ); 
   putc( packetNumber & 0xff );
   putc(255 - ( packetNumber & 0xff )); 
   
   for (i=1; i <= Length; i++ ) {
     putc( 0 ); 
     csum = (csum + 0) & 0xff;
     pBuffer++;
     transferred++;
   }
   for (i=Length+1; i <= packetLength; i++ ) {
     putc(CTRLZ); 
     csum = (csum + CTRLZ) & 0xff;
   } 

   putc(csum);
 
   return;
}
#endif

dword modem_send(dword dldaddr, dword img_size)
{
     int  length;
     int  totalPackets;
     int  retries = 0;
     byte ch = 0;
     int debug = 0;
     int ackcnt = 0;
     int nakcnt = 0;

     flagCrc = 0;
     longPacket = 0;
     batch = 0;
     g = 0;
     timeout = GET_BYTE_TIMEOUT;
     retries = RETRIES;
     userAbort = 0;
     packetNumber = 1;
     fileLength = 0;
     transferred = 0;
     err = errOK;

 
     totalPackets = img_size / packetLength;
     if ( img_size % packetLength != 0 )
        totalPackets++;
     putLabeledWord("totalPackets=", totalPackets );

     while (packetNumber <= totalPackets ) {
        ch = getBytesWithTimeout(); 
       /*       if (temp == 0) {
	 ch = NAK;
         temp++;
       }
       else {
         ch = ACK;
        
	 } */ 
	 if ( err == errTimeout ) {
	   putstr( "\r\nTransfer timed out\r\n" );
           return 0;   
	 }
	 if ( ch == CAN ) {
            putstr( "\r\nTransfer canceled by reciever\r\n" );
           putLabeledWord("ackcnt=", ackcnt );
           putLabeledWord("nakcnt=", nakcnt );
            return 0;
	 }
         if ( ch == ACK ) {
	    ackcnt++;
	    if (debug)
	      putstr("GOT a ACK\r\n");
	    retries = 0;
            packetNumber++;
            if ( packetNumber <= totalPackets ) {
               dldaddr += packetLength;
               if ( packetNumber == totalPackets ) {
                  if ( img_size % packetLength == 0 )
                     length = packetLength;
                  else
	             length = img_size % packetLength;
               }
               else
                  length = packetLength;
             if ( flagCrc )
                sendPacketCrc((byte *)dldaddr, packetNumber, length);
             else
               sendPacket( (byte *)dldaddr, packetNumber, length);
	    }
	 }
         if ( ch == NAK ) {
	   nakcnt++;
	     if (debug)
	       putstr("GOT a NAK\r\n" );
	     retries++;
             if ( retries == RETRIES ) {
               putc( CAN );
               putc( CAN );
	       putstr( "\r\nTransfer canceled - Maxuim error count exceeded\r\n");
		return 0;
	     }
             if ( packetNumber == totalPackets ) {
                if ( img_size % packetLength == 0 )
                   length = packetLength;
                else
	           length = img_size % packetLength;
             }
             else
                length = packetLength;
             if ( flagCrc )
                sendPacketCrc((byte *)dldaddr, packetNumber, length);
             else
                sendPacket((byte *)dldaddr, packetNumber, length);
	 }
         if ( (ch == 'C') && ( packetNumber == 1 ) ) {
	    flagCrc = -1;
	    /* packetLength=1024; */ 
            InitCRC16();  /* init the CRC 16 table */
	     retries++;
             if ( packetNumber == totalPackets ) {
                if ( img_size % packetLength == 0 )
                   length = packetLength;
                else
	           length = img_size % packetLength;
             }
             else
                length = packetLength;
             sendPacketCrc((byte *)dldaddr, packetNumber, length);
         }
     }
     putc(EOT); 
     ch = getBytesWithTimeout();
     if ( err == errTimeout ) {
        putc( CAN ); putc( CAN );
	putstr( "\r\nTransfer timed out\r\n" );
        return 0;
     }
     if ( ch == ACK ) 
       putstr ( "\r\nUpload Successful\r\n" );
     else 
        putstr( "\r\nTransfer may not have competed\r\n" ); 
     putLabeledWord("\r\nBytes Transferred=", transferred); 
     putLabeledWord("ackcnt=", ackcnt );
     putLabeledWord("nakcnt=", nakcnt );
     return transferred;
}




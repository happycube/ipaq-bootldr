/* MECR: Expansion Memory Configuration Register
 * (SA-1100 Developers Manual, p.10-13; SA-1110 Developers Manual, p.10-24)
 *
 * MECR layout is:  
 *
 *   FAST1 BSM1<4:0> BSA1<4:0> BSIO1<4:0> FAST0 BSM0<4:0> BSA0<4:0> BSIO0<4:0>
 *
 * (This layout is actually true only for the SA-1110; the FASTn bits are
 * reserved on the SA-1100.)
 */

#define MCXX_SETUP_MASK     (0x7f)
#define MCXX_ASST_MASK      (0x1f)
#define MCXX_HOLD_MASK      (0x3f)
#define MCXX_SETUP_SHIFT    (0)
#define MCXX_ASST_SHIFT     (7)
#define MCXX_HOLD_SHIFT     (14)


#define MECR_SET(mecr, sock, shift, mask, bs) \
((mecr)=((mecr)&~(((mask)<<(shift))<<\
                  ((sock)==0?MECR_SOCKET_0_SHIFT:MECR_SOCKET_1_SHIFT)))|\
        (((bs)<<(shift))<<((sock)==0?MECR_SOCKET_0_SHIFT:MECR_SOCKET_1_SHIFT)))

#define MECR_GET(mecr, sock, shift, mask) \
((((mecr)>>(((sock)==0)?MECR_SOCKET_0_SHIFT:MECR_SOCKET_1_SHIFT))>>\
 (shift))&(mask))

#define MECR_BSIO_SET(mecr, sock, bs) \
MECR_SET((mecr), (sock), MECR_BSIO_SHIFT, MECR_BS_MASK, (bs))

#define MECR_BSIO_GET(mecr, sock) \
MECR_GET((mecr), (sock), MECR_BSIO_SHIFT, MECR_BS_MASK)

#define MECR_BSA_SET(mecr, sock, bs) \
MECR_SET((mecr), (sock), MECR_BSA_SHIFT, MECR_BS_MASK, (bs))

#define MECR_BSA_GET(mecr, sock) \
MECR_GET((mecr), (sock), MECR_BSA_SHIFT, MECR_BS_MASK)

#define MECR_BSM_SET(mecr, sock, bs) \
MECR_SET((mecr), (sock), MECR_BSM_SHIFT, MECR_BS_MASK, (bs))

#define MECR_BSM_GET(mecr, sock) \
MECR_GET((mecr), (sock), MECR_BSM_SHIFT, MECR_BS_MASK)

#define MECR_FAST_SET(mecr, sock, fast) \
MECR_SET((mecr), (sock), MECR_FAST_SHIFT, MECR_FAST_MODE_MASK, (fast))

#define MECR_FAST_GET(mecr, sock) \
MECR_GET((mecr), (sock), MECR_FAST_SHIFT, MECR_FAST_MODE_MASK)


/* This function implements the BS value calculation for setting the MECR
 * using integer arithmetic:
 */
static inline unsigned int pxa_pcmcia_mecr_bs(unsigned int pcmcia_cycle_ns,
						 unsigned int cpu_clock_khz){
  unsigned int t = ((pcmcia_cycle_ns * cpu_clock_khz) / 6) - 1000000;
  return (t / 1000000) + (((t % 1000000) == 0) ? 0 : 1);
}

static inline unsigned int pxa_mcxx_hold(unsigned int pcmcia_cycle_ns,
					    unsigned int mem_clk_10khz){
  unsigned int code = pcmcia_cycle_ns * mem_clk_10khz;
  return (code / 300000) + ((code % 300000) ? 1 : 0) - 1;
}

static inline unsigned int pxa_mcxx_asst(unsigned int pcmcia_cycle_ns,
					    unsigned int mem_clk_10khz){
  unsigned int code = pcmcia_cycle_ns * mem_clk_10khz;
  return (code / 300000) + ((code % 300000) ? 1 : 0) - 1;
}

static inline unsigned int pxa_mcxx_setup(unsigned int pcmcia_cycle_ns,
					    unsigned int mem_clk_10khz){
  unsigned int code = pcmcia_cycle_ns * mem_clk_10khz;
  return (code / 100000) + ((code % 100000) ? 1 : 0) - 1;
}

/* This function returns the (approxmiate) command assertion period, in
 * nanoseconds, for a given CPU clock frequency and MCXX_ASST value:
 */

static inline unsigned int pxa_pcmcia_cmd_time(unsigned int mem_clk_10khz,
					   unsigned int pcmcia_mcxx_asst){
  return (300000 * (pcmcia_mcxx_asst + 1) / mem_clk_10khz);
}


/* SA-1100 PCMCIA Memory and I/O timing
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The SA-1110 Developer's Manual, section 10.2.5, says the following:
 *
 *  "To calculate the recommended BS_xx value for each address space:
 *   divide the command width time (the greater of twIOWR and twIORD,
 *   or the greater of twWE and twOE) by processor cycle time; divide
 *   by 2; divide again by 3 (number of BCLK's per command assertion);
 *   round up to the next whole number; and subtract 1."
 *
 * The PC Card Standard, Release 7, section 4.13.4, says that twIORD
 * has a minimum value of 165ns. Section 4.13.5 says that twIOWR has
 * a minimum value of 165ns, as well. Section 4.7.2 (describing
 * common and attribute memory write timing) says that twWE has a
 * minimum value of 150ns for a 250ns cycle time (for 5V operation;
 * see section 4.7.4), or 300ns for a 600ns cycle time (for 3.3V
 * operation, also section 4.7.4). Section 4.7.3 says that taOE
 * has a maximum value of 150ns for a 300ns cycle time (for 5V
 * operation), or 300ns for a 600ns cycle time (for 3.3V operation).
 *
 * When configuring memory maps, Card Services appears to adopt the policy
 * that a memory access time of "0" means "use the default." The default
 * PCMCIA I/O command width time is 165ns. The default PCMCIA 5V attribute
 * and memory command width time is 150ns; the PCMCIA 3.3V attribute and
 * memory command width time is 300ns.
 */
#define PXA_PCMCIA_IO_ACCESS      (165)
#define PXA_PCMCIA_5V_MEM_ACCESS  (150)
#define PXA_PCMCIA_3V_MEM_ACCESS  (300)


/* The socket driver actually works nicely in interrupt-driven form,
 * so the (relatively infrequent) polling is "just to be sure."
 */
#define PXA_PCMCIA_POLL_PERIOD    (2*HZ)

/* Crystal Frequency to Memory Frequency Multiplier (L) */
static unsigned char L_clk_mult[32] = { 0, 27, 32, 36, 40, 45, 0, };

/* Crystal clock */
#define BASE_CLK	3686400

/*
 * Return the current lclk requency in units of 10kHz
 */
unsigned int get_lclk_frequency_10khz(void)
{
	return L_clk_mult[(CCCR >> 0) & 0x1f] * BASE_CLK / 10000;
}


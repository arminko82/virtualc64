/*!
 * @header      VIA.h
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   2008 - 2016 Dirk W. Hoffmann
 * @brief       Declares VC1541 class
 * @details     The implementation is mainly based on the document
 *              "R6522 VERSATILE INTERFACE ADAPTER" by Frank Kontros [F. K.]
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _VIA6522_INC
#define _VIA6522_INC

#include "VirtualComponent.h"

class VC1541;

#define VIACountA0       (1ULL << 0) // Timer 1 decrements every cycle
#define VIACountA1       (1ULL << 1)
#define VIACountB0       (1ULL << 2) // Timer 2 decrements every cycle
#define VIACountB1       (1ULL << 3)
#define VIAReloadA0      (1ULL << 4) // Forces timer 1 to reload (free-run mode)
#define VIAReloadA1      (1ULL << 5)
#define VIAReloadA2      (1ULL << 6)
#define VIAPostOneShotA0 (1ULL << 7) // Indicates that timer 1 has fired in one shot mode
#define VIAPostOneShotB0 (1ULL << 8) // Indicates that timer 2 has fired in one shot mode
#define VIAInterrupt0    (1ULL << 9)
#define VIAInterrupt1    (1ULL << 10)

#define VIAClearBits   ~((1ULL << 11) | VIACountA0 | VIACountB0 | VIAReloadA0 | VIAPostOneShotA0 | VIAPostOneShotB0 | VIAInterrupt0)

/*! @brief    Virtual VIA6522 controller
    @details  The VC1541 drive contains two VIAs on its logic board.
 */
class VIA6522 : public VirtualComponent {
	
protected:
    
    //
    // Peripheral interface
    //
    
    //! @brief    Peripheral port A
    /*! @details  "The Peripheral A port consists of 8 lines which can be
     *             individually programmed to act as an input or an output
     *             under control of a Data Direction Register. The polarity
     *             of output pins is controlled by an Output Register and
     *             input data can be latched into an internal register under
     *             control of the CA1 line."
     */
    uint8_t pa;
    
    //! @brief    Peripheral A control lines
    /*! @details  "The two peripheral A control lines act as interrupt inputs
     *             or ashandshake outputs. Each linecon­ trols an internal
     *             interrupt flag with a corresponding interrupt enable bit.
     *             In addition, CA1controls the latching of data on
     *             Peripheral A Port input lines. The various modes of
     *             operation are controlled by the system processor through
     *             the internal control registers."
     */
    bool ca1;
    bool ca2;

    //! @brief    Peripheral port B
    /*! @details  "The Peripheral B port consists of 8 lines which can be
     *             individually programmed to act as an input or an output
     *             under control of a Data Direction Register. The polarity
     *             of output pins is controlled by an Output Register and
     *             input data can be latched into an internal register under
     *             control of the CA1 line."
     */
    uint8_t pb;
    
    //! @brief
    /*! @details  "The Peripheral B control lines act as interrupt inputs or
     *             as handshake outputs. As with CA1 and CA2, each line
     *             controls an interrupt flag with a corresponding interrupt
     *             enable bit. In addition, these lines act as a serial port
     *             under control of the Shift Register."
     */
    bool cb1;
    bool cb2;
     
    //
    // Port registers
    //
    
    //! @brief    Data direction registers
    /*! @details  "Each port has a Data Direction Register (DDRA, DDRB) for
     *             specifying whether the peripheral pins are to act as
     *             inputs or outputs. A 0 in a bit of the Data Direction
     *             Register causes the corresponding peripheral pin to act
     *             as an input. A 1 causes the pin to act as an output."
     */
    uint8_t ddra;
    uint8_t ddrb;

    //! @brief    Output registers
public:
    uint8_t ora;
    uint8_t orb;

    //! @brief    Input registers
    uint8_t ira;
    uint8_t irb;
protected:
    
    //
    // Timers
    //
    
	/*! @brief    VIA timer 1
	 *  @details  "Interval Timer T1 consists of two 8-bit latches and a
     *             16-bit counter. The latches store data which is to be
     *             loaded into the counter. After loading, the counter
     *             decrements at 02 clock rate. Upon reaching zero, an
     *             interrupt flag is set, and IRQ goes low if the T1
     *             interrupt is enabled. Timer 1 then disables any further
     *             interrupts or automatically transfers the contents of
     *             the latches into the counter and continues to decrement.
     *             In addition, the timer may be programmed to invert the
     *             output signal on a peripheral pin (PB7) each time it
     *             "times-out."
     */
    uint16_t t1; // T1C
    uint8_t t1_latch_lo; // T1L_L
    uint8_t t1_latch_hi; // T1L_H

	/*! @brief    VIA timer 2
	 *  @details  "Timer 2 operates as an interval timer (in the "one-shot"
     *             mode only), or as a counter for counting negative pulses
     *             on the PB6 peripheral pin. A single control bit in the
     *             Auxiliary Control Register selects between these two
     *             modes. This timer is comprised of a "write-only" low-order
     *             latch (T2L-L), a "read-only" low-order counter (T2C-L) and
     *             a read/write high order counter (T2C-H). The counter
     *             registers act as a 16-bit counter which decrements at
     *             02 rate."
     */
    uint16_t t2; // T1C
    uint8_t t2_latch_lo; // T2L_L
	    
    bool pb7toggle;
    bool pb7timerOut;
    
    //! @brief    Peripheral control register
    uint8_t pcr;

    //! @brief    Auxiliary register
    uint8_t acr;

    //! @brief    Interrupt enable register
    uint8_t ier;

    //! @brief    Interrupt flag register
    uint8_t ifr;

    //! @brief    Shift register
    uint8_t sr;
    
    //! @brief    Event triggering queue
    uint64_t delay;
    
    //! @brief    New bits to feed in
    //! @details  Bits set in this variable makes a trigger event persistent.
    uint64_t feed;
    
public:	
	//! @brief    Constructor
	VIA6522();
	
	//! @brief    Destructor
	~VIA6522();
		
	//! @brief    Brings the VIA back to its initial state.
	void reset();

    //! @brief    Dumps debug information.
    void dumpState();

    //! @brief    Executes the virtual VIA for one cycle.
    void execute(); 

    //! @brief    Executes timer 1 for one cycle.
    void executeTimer1();

    //! @brief    Executes timer 2 for one cycle.
    void executeTimer2();
	
	/*! @brief    Special peek function for the I/O memory range
	 *  @details  The peek function only handles those registers that are treated
     *            similarly by both VIA chips
     */
	virtual uint8_t peek(uint16_t addr);
	
    //! @brief    Same as peek, but without side effects
    virtual uint8_t read(uint16_t addr);
    
	/*! @brief    Special poke function for the I/O memory range
	 *  @details  The poke function only handles those registers that are treated
     *            similarly by both VIA chips
     */
	virtual void poke(uint16_t addr, uint8_t value);

    
    // ----------------------------------------------------------------------------------------
    //                                Internal Configuration
    // ----------------------------------------------------------------------------------------

    //! @brief    Returns true iff timer 1 is in free-run mode (continous interrupts)
    bool freeRunMode1() { return (acr & 0x40) != 0; }

    //! @brief    Checks if input latching is enabled
    bool inputLatchingEnabledA() { return (GET_BIT(acr,0)); }

    //! @brief    Checks if input latching is enabled
    bool inputLatchingEnabledB() { return (GET_BIT(acr,1)); }

    //! @brief    Returns the current value of the peripheral control register
    uint8_t getPcr() { return pcr & 0x20; }
    
    
    // ----------------------------------------------------------------------------------------
    //                                        Ports
    // ----------------------------------------------------------------------------------------

    //! @brief   Bit values driving port A from inside the chip
    virtual uint8_t portAinside() = 0;

    //! @brief   Bit values driving port A from outside the chip
    virtual uint8_t portAoutside() = 0;

    /*! @brief   Computes the current bit values visible at port A
     *  @details Value is stored in variable pa
     */
    virtual void updatePA() = 0;

    //! @brief   Bit values driving port B from inside the chip
    virtual uint8_t portBinside() = 0;
    
    //! @brief   Bit values driving port B from outside the chip
    virtual uint8_t portBoutside() = 0;
    
    /*! @brief   Computes the current bit values visible at port B
     *  @details Value is stored in variable pb
     */
    virtual void updatePB() = 0;
    
    //! Returns the current value on chip pin CA2
    bool CA2() {
        switch ((pcr >> 1) & 0x07) {
            case 6: return false; // LOW OUTPUT
            case 7: return true; // HIGH OUTPUT
            default:
                warn("UNUSAL OPERATION MODE FOR CA2 DETECTED");
                return false;
        }
    }
    
    // ----------------------------------------------------------------------------------------
    //                                   Interrupt handling
    // ----------------------------------------------------------------------------------------

    /*! @brief    Returns the value of the IRQ pin
     *  @details  This method updates the IRQ pin of the connected CPU as a side effect and is therefore
     *            invoked on every change in register IFR or register IER.
     */
    bool IRQ();

    //
    // |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
    // ---------------------------------------------------------------------------------
    // |   IRQ   | Timer 1 | Timer 2 |   CB1   |   CB2   |Shift Reg|   CA1   |   CA2   |

    // Timer 1 - Set by:     Time-out of T1
    //           Cleared by: Read t1 low or write t1 high
    
    void setInterruptFlag_T1() {
        SET_BIT(ifr,6); IRQ(); }
    void clearInterruptFlag_T1() {
        CLR_BIT(ifr,6); IRQ(); }

    // Timer 2 - Set by:     Time-out of T2
    //           Cleared by: Read t2 low or write t2 high
    
    void setInterruptFlag_T2() {
        SET_BIT(ifr,5); IRQ(); }
    void clearInterruptFlag_T2() {
        CLR_BIT(ifr,5); IRQ(); }

    // CB1 - Set by:     Active edge on CB1
    //       Cleared by: Read or write to register 0 (ORB)
    
    void setInterruptFlag_CB1() {
        SET_BIT(ifr,4); IRQ(); }
    void clearInterruptFlag_CB1() {
        CLR_BIT(ifr,4); IRQ(); }

    // CB2 - Set by:     Active edge on CB2
    //       Cleared by: Read or write to register 0 (ORB) (only if CB2 is not selected as "INDEPENDENT")
    
    void setInterruptFlag_CB2() {
        SET_BIT(ifr,3); IRQ(); }
    void clearInterruptFlag_CB2() {
        CLR_BIT(ifr,3); IRQ(); }
    //bool CB2selectedAsIndependent() {
    //     uint8_t b765 = (pcr >> 5) & 0x07; return (b765 == 0x01) || (b765 == 0x03); }
    
    //! @brief  Indicates if interrupt bit CB2 in IFR is to be cleared when reading from ORB
    bool shouldClearCB2onRead() {
        uint8_t b765 = (pcr >> 5) & 0x7;
        return b765 == 0x0 || b765 == 0x2;
    }
    
    //! @brief  Indicates if interrupt bit CB2 in IFR is to be cleared when writing to ORB
    bool shouldClearCB2onWrite() {
        uint8_t b765 = (pcr >> 5) & 0x7;
        return b765 == 0x0 || b765 == 0x2 || b765 == 0x4 || b765 == 0x5;
    }
    
    // Shift register - Set by:     8 shifts completed
    //                  Cleared by: Read or write to register 10 (0xA) (shift register)
    
    void setInterruptFlag_SR() {
        SET_BIT(ifr,2); IRQ(); }
    void clearInterruptFlag_SR() {
        CLR_BIT(ifr,2); IRQ(); }

    // CA1 - Set by:     Active edge on CA1
    //       Cleared by: Read or write to register 1 (ORA)
    
    void setInterruptFlag_CA1() {
        SET_BIT(ifr,1); IRQ(); }
    void clearInterruptFlag_CA1() {
        CLR_BIT(ifr,1); IRQ(); }

    // CA2 - Set by:     Active edge on CA2
    //       Cleared by: Read or write to register 1 (ORA) (only if CA2 is not selected as "INDEPENDENT")
    
    void setInterruptFlag_CA2() {
        SET_BIT(ifr,0); IRQ(); }
    void clearInterruptFlag_CA2() {
        CLR_BIT(ifr,0); IRQ(); }
    // bool CA2selectedAsIndependent() {
    //     uint8_t b321 = (pcr >> 1) & 0x07; return (b321 == 0x01) || (b321 == 0x03); }
    
    //! @brief  Indicates if interrupt bit CB2 in IFR is to be cleared when reading from ORA
    bool shouldClearCA2onRead() {
        uint8_t b321 = (pcr >> 1) & 0x7;
        return b321 == 0x0 || b321 == 0x2 || b321 == 0x4 || b321 == 0x5;
    }
    
    //! @brief  Indicates if interrupt bit CB2 in IFR is to be cleared when writing to ORA
    bool shouldClearCA2onWrite() {
        uint8_t b321 = (pcr >> 1) & 0x7;
        return b321 == 0x0 || b321 == 0x2 || b321 == 0x4 || b321 == 0x5;
    }
};


/*! @brief   First virtual VIA6522 controller
 *  @details VIA1 serves as hardware interface between the VC1541 CPU and the IEC bus.
 */
class VIA1 : public VIA6522 {
	
public:

	VIA1();
	~VIA1();
    
    uint8_t portAinside();
    uint8_t portAoutside();
    void updatePA();
    uint8_t portBinside();
    uint8_t portBoutside();
    void updatePB();
    
	uint8_t peek(uint16_t addr);
    uint8_t read(uint16_t addr);
    void poke(uint16_t addr, uint8_t value);
	
    //! @brief    Returns true iff a change of the atn line can trigger interrups
	bool atnInterruptsEnabled() { return ier & 0x02; }

    //! @brief    Indicates that an ATN interrupt has occured.
    void indicateAtnInterrupt() { ifr |= 0x02; }

    //! @brief    Clears the ATN interrupt indication bit.
    void clearAtnIndicator() { ifr &= ~0x02; }
};

//! The second versatile interface adapter (VIA2)
class VIA2 : public VIA6522 {
	
public:

	VIA2();
	~VIA2();
    // void executeTimer1();
    // void executeTimer2();
  
    uint8_t portAinside();
    uint8_t portAoutside();
    void updatePA();
    uint8_t portBinside();
    uint8_t portBoutside();
    void updatePB();
    
	uint8_t peek(uint16_t addr);
    uint8_t read(uint16_t addr);
	void poke(uint16_t addr, uint8_t value);

    //! @brief    Returns bit 0 of output register B
	bool stepperActive0() { return (orb & 0x01) != 0; }

    //! @brief    Returns bit 1 of output register B
	bool stepperActive1() { return (orb & 0x02) != 0; }

    //! @brief    Returns bit 2 of output register B
	bool engineRunning() { return (orb & 0x04) != 0; }

    //! @brief    Returns bit 3 of output register B
    bool redLEDshining() { return (orb & 0x08) != 0; }
    
    bool overflowEnabled() { return (pcr & 0x02); }

    void debug0xC();
};

#endif

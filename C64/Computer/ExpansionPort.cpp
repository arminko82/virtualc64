/*
 * Written by Dirk Hoffmann based on the original code by A. Carl Douglas.
 *
 * This program is free software; you can redistribute it and/or modify
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

#include "C64.h"

ExpansionPort::ExpansionPort()
{
    setDescription("Expansion port");
    debug(3, "  Creating expansion port at address %p...\n", this);
}

ExpansionPort::~ExpansionPort()
{
    debug(3, "  Releasing expansion port...\n");
    detachCartridge();
}

void
ExpansionPort::reset()
{
    VirtualComponent::reset();
    
    if (cartridge) {
        cartridge->reset();
        cartridge->resetCartConfig();
    } else {
        setGameLine(1);
        setExromLine(1);
    }
    
    // debug("Resetting port: game = %d exrom = %d\n", cartridge->getInitialGameLine(), cartridge->getInitialExromLine());
}

void
ExpansionPort::ping()
{
    VirtualComponent::ping();
    c64->putMessage(cartridge ? MSG_CARTRIDGE : MSG_NO_CARTRIDGE);
}

size_t
ExpansionPort::stateSize()
{
    return 6 + (cartridge ? cartridge->stateSize() : 0);
}

void
ExpansionPort::loadFromBuffer(uint8_t **buffer)
{
    uint8_t *old = *buffer;
    
    // Delete old cartridge (if any)
    if (cartridge != NULL) {
        delete cartridge;
        cartridge = NULL;
    }
    
    // Read cartridge type
    CartridgeType cartridgeType = (CartridgeType)read16(buffer);
    gameLinePhi1 = read8(buffer);
    gameLinePhi2 = read8(buffer);
    exromLinePhi1 = read8(buffer);
    exromLinePhi2 = read8(buffer);
    
    // Read cartridge data (if any)
    if (cartridgeType != CRT_NONE) {
        cartridge = Cartridge::makeWithType(c64, cartridgeType);
        cartridge->loadFromBuffer(buffer);
    }
    
    debug(2, "  Expansion port state loaded (%d bytes)\n", *buffer - old);
    assert(*buffer - old == stateSize());
}

void
ExpansionPort::saveToBuffer(uint8_t **buffer)
{
    uint8_t *old = *buffer;
    
    write16(buffer, cartridge ? cartridge->getCartridgeType() : CRT_NONE);
    write8(buffer, gameLinePhi1);
    write8(buffer, gameLinePhi2);
    write8(buffer, exromLinePhi1);
    write8(buffer, exromLinePhi2);

    // Write cartridge data (if any)
    if (cartridge != NULL)
        cartridge->saveToBuffer(buffer);
 
    debug(4, "  Expansion port state saved (%d bytes)\n", *buffer - old);
    assert(*buffer - old == stateSize());
}

void
ExpansionPort::dump()
{
    msg("Expansion port\n");
    msg("--------------\n");
    
    msg(" Game line (phi1 / phi2):  %d / %d\n", gameLinePhi1, gameLinePhi2);
    msg("Exrom line (phi1 / phi2):  %d / %d\n", exromLinePhi1, exromLinePhi2);

    if (cartridge == NULL) {
        msg("No cartridge attached\n");
    } else {
        cartridge->dump();
    }
}

CartridgeType
ExpansionPort::getCartridgeType()
{
    return cartridge ? cartridge->getCartridgeType() : CRT_NONE;
}

uint8_t
ExpansionPort::peek(uint16_t addr)
{
    return cartridge ? cartridge->peek(addr) : 0;
}

uint8_t
ExpansionPort::spypeek(uint16_t addr)
{
    return cartridge ? cartridge->spypeek(addr) : 0;
}

/*
uint8_t
ExpansionPort::peek(uint16_t addr)
{
    assert((addr >= 0x8000 && addr <= 0x9FFF) ||
           (addr >= 0xA000 && addr <= 0xBFFF) ||
           (addr >= 0xE000 && addr <= 0xFFFF));
    
    if (cartridge) {
        if (addr <= 0x9FFF) {
            return cartridge->peekRomLabs(addr);
        } else {
            return cartridge->peekRomHabs(addr);
        }
    }
    return 0;
}
*/

uint8_t
ExpansionPort::peekIO1(uint16_t addr)
{
    /* "Die beiden mit "I/O 1" und "I/O 2" bezeichneten Bereiche
     *  sind für Erweiterungskarten reserviert und normalerweise ebenfalls offen,
     *  ein Lesezugriff liefert auch hier "zufällige" Daten (dass diese Daten gar
     *  nicht so zufällig sind, wird in Kapitel 4 noch ausführlich erklärt. Ein
     *  Lesen von offenen Adressen liefert nämlich auf vielen C64 das zuletzt vom
     *  VIC gelesene Byte zurück!)" [C.B.]
     */
    return cartridge ? cartridge->peekIO1(addr) : c64->vic.getDataBusPhi1();
}

uint8_t
ExpansionPort::spypeekIO1(uint16_t addr)
{
    return cartridge ? cartridge->spypeekIO1(addr) : c64->vic.getDataBusPhi1();
}

uint8_t
ExpansionPort::peekIO2(uint16_t addr)
{
    return cartridge ? cartridge->peekIO2(addr) : c64->vic.getDataBusPhi1();
}

uint8_t
ExpansionPort::spypeekIO2(uint16_t addr)
{
    return cartridge ? cartridge->spypeekIO2(addr) : c64->vic.getDataBusPhi1();
}

void
ExpansionPort::poke(uint16_t addr, uint8_t value)
{
    if (cartridge != NULL) cartridge->poke(addr, value);

    if (!c64->getUltimax())
        c64->mem.ram[addr] = value;
}

void
ExpansionPort::pokeIO1(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDE00 && addr <= 0xDEFF);
    if (cartridge != NULL)
        cartridge->pokeIO1(addr, value);
}

void
ExpansionPort::pokeIO2(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDF00 && addr <= 0xDFFF);
    if (cartridge != NULL)
        cartridge->pokeIO2(addr, value);
}

void
ExpansionPort::setGameLinePhi1(bool value)
{
    gameLinePhi1 = value;
    c64->vic.setUltimax(!gameLinePhi1 && exromLinePhi1);
}

void
ExpansionPort::setGameLinePhi2(bool value)
{
    gameLinePhi2 = value;
    c64->mem.updatePeekPokeLookupTables();
}

void
ExpansionPort::setExromLinePhi1(bool value)
{
    exromLinePhi1 = value;
    c64->vic.setUltimax(!gameLinePhi1 && exromLinePhi1);
}

void
ExpansionPort::setExromLinePhi2(bool value)
{
    exromLinePhi2 = value;
    c64->mem.updatePeekPokeLookupTables();
}

void
ExpansionPort::updatePeekPokeLookupTables()
{
    if (cartridge) {
        cartridge->updatePeekPokeLookupTables();
    }
}

bool
ExpansionPort::attachCartridge(Cartridge *c)
{
    assert(c != NULL);
    
    // Remove old cartridge (if any) and assign new one
    detachCartridge();
    cartridge = c;
    
    // Reset cartridge to update exrom and game line on the expansion port
    cartridge->reset();
    
    c64->putMessage(MSG_CARTRIDGE);
    debug(1, "Cartridge attached to expansion port");
    cartridge->dump();

    return true;
}

bool
ExpansionPort::attachCartridgeAndReset(CRTFile *file)
{
    assert(file != NULL);
    
    Cartridge *cartridge = Cartridge::makeWithCRTFile(c64, file);
    
    if (cartridge) {
        
        suspend();
        attachCartridge(cartridge);
        c64->reset();
        resume();
        return true;
    }
    
    return false;
}

bool
ExpansionPort::attachGeoRamCartridge(uint32_t capacity)
{
    switch (capacity) {
        case 64: case 128: case 256: case 512: case 1024: case 2048: case 4096:
            break;
        default:
            warn("Cannot create GeoRAM cartridge of size %d\n", capacity);
            return false;
    }
    
    Cartridge *geoRAM = Cartridge::makeWithType(c64, CRT_GEO_RAM);
    uint32_t capacityInBytes = capacity * 1024;
    geoRAM->setRamCapacity(capacityInBytes);
    debug("Created GeoRAM cartridge (%d KB)\n", capacity);
    
    return attachCartridge(geoRAM);
}

void
ExpansionPort::detachCartridge()
{
    if (cartridge) {
        
        suspend();
        
        delete cartridge;
        cartridge = NULL;
        
        setGameLine(1);
        setExromLine(1);
        
        debug(1, "Cartridge detached from expansion port");
        
        c64->putMessage(MSG_NO_CARTRIDGE);
        resume();
    }
}

void
ExpansionPort::detachCartridgeAndReset()
{
    suspend();
    detachCartridge();
    c64->reset();
    resume();
}

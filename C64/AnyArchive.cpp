/*!
 * @file        AnyArchive.cpp
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   Dirk W. Hoffmann, all rights reserved.
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

#include "T64File.h"
#include "D64File.h"
#include "PRGFile.h"
#include "P00File.h"
#include "G64File.h"

AnyArchive *
AnyArchive::makeArchiveWithFile(const char *path)
{
    assert(path != NULL);
    
    if (T64File::isT64File(path)) {
        return T64File::makeT64ArchiveWithFile(path);
    }
    if (D64File::isD64File(path)) {
        return D64File::makeObjectWithFile(path);
    }
    if (PRGFile::isPRGFile(path)) {
        return PRGFile::makePRGArchiveWithFile(path);
    }
    if (P00File::isP00File(path)) {
        return P00File::makeP00ArchiveWithFile(path);
    }
    if (G64File::isG64File(path)) {
        return G64File::makeG64ArchiveWithFile(path);
    }
    return NULL;
}

const char *
AnyArchive::getNameOfItem()
{
    return "FILE";
}

const unsigned short *
AnyArchive::getUnicodeNameOfItem()
{
    const char *name = getNameOfItem();
    translateToUnicode(name, unicode, 0xE000, sizeof(unicode) / 2);
    return unicode;
}

size_t
AnyArchive::getSizeOfItem()
{
    int size = 0;
    
    seek(0);
    while (getByte() != EOF)
        size++;

    return size;
}

void
AnyArchive::flashItem(uint8_t *buffer)
{
    uint16_t addr = getDestinationAddrOfItem();
    flash(buffer, (size_t)addr);
}

void
AnyArchive::dumpDirectory()
{
    int numItems = numberOfItems();
    
    msg("Archive:           %s\n", getName());
    msg("-------\n");
    msg("  Path:            %s\n", getPath());
    msg("  Items:           %d\n", numItems);

    for (unsigned i = 0; i < numItems; i++) {
        
        selectItem(i);
        msg("  Item %2d:      %s (%d bytes, load address: %d)\n",
                i, getNameOfItem(), getSizeOfItem(), getDestinationAddrOfItem());
        msg("                 ");
        selectItem(i);
        for (unsigned j = 0; j < 8; j++) {
            int byte = getByte();
            if (byte != -1)
                msg("%02X ", byte);
        }
        msg("\n");
    }
}

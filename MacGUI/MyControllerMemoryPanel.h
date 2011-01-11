/*
 * (C) 2011 Dirk W. Hoffmann. All rights reserved.
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

#ifndef INC_MYCONTROLLER_MEMORY_PANEL
#define INC_MYCONTROLLER_MEMORY_PANEL

#import "MyController.h"

@interface MyController(MemoryPanel)

// Debug panel (Memory)
- (IBAction)searchAction:(id)sender;

- (Memory::MemoryType)currentMemSource;
- (IBAction)setMemSourceToRAM:(id)sender;
- (IBAction)setMemSourceToROM:(id)sender;
- (IBAction)setMemSourceToIO:(id)sender;

- (id)objectValueForMemTableColumn:(NSTableColumn *)aTableColumn row:(int)row;
- (void)changeMemValue:(uint16_t)addr value:(int16_t)v memtype:(Memory::MemoryType)t;
- (void)setMemObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;

- (void)doubleClickInMemTable:(id)sender;

- (void)refreshMemory;

@end

#endif
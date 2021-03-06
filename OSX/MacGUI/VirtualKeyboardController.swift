//
// This file is part of VirtualC64 - A user-friendly Commodore 64 emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
//

import Foundation

class VirtualKeyboardController : UserDialogController, NSWindowDelegate
{
    /// Array holding a reference to the view of each key
    var keyView = Array(repeating: nil as NSButton?, count: 66)

    /// Array holding a reference to the image of each key
    var keyImage = Array(repeating: nil as NSImage?, count: 66)

    /// Indicates if the left Shift key is pressed
    var lshift = false

    /// Indicates if the right Shift key is pressed
    var rshift = false

    /// Indicates if the shift lock key is pressed
    var shiftLock = false

    /// Indicates if the Control key is pressed
    var control = false
    
    /// Indicates if the Commodore key is pressed
    var commodore = false
    
    /// Indicates if the lower case character set is currently in use
    var lowercase = false
    
    /// Indicates if the window should close itself when a key has been pressed.
    /// If the virtual keyboard is opened as a sheet, this variable is set to
    /// true. If it is opened as a seperate window, it is set to false.
    var autoClose = true
    
    func showWindow(withParent controller: MyController) {
        
        track()
        
        parent = controller
        parentWindow = parent.window
        c64 = parent.mydocument.c64
        autoClose = false
        
        showWindow(self)
    }
    
    override func windowDidLoad() {
        
        track()
        
        // Setup key references
        for tag in 0 ... 65 {
            keyView[tag] = window!.contentView!.viewWithTag(tag) as? NSButton
        }

        updateImages()
    }
    
    func windowWillClose(_ notification: Notification) {
    
        track()
        releaseSpecialKeys()
    }
    
    func refresh() {
        
        var needsUpdate = false;
        
        if lshift != c64.keyboard.leftShiftIsPressed() {
            lshift = c64.keyboard.leftShiftIsPressed()
            needsUpdate = true
        }
        if rshift != c64.keyboard.rightShiftIsPressed() {
            rshift = c64.keyboard.rightShiftIsPressed()
            needsUpdate = true
        }
        if shiftLock != c64.keyboard.shiftLockIsHoldDown() {
            shiftLock = c64.keyboard.shiftLockIsHoldDown()
            needsUpdate = true
        }
        if control != c64.keyboard.controlIsPressed() {
            control = c64.keyboard.controlIsPressed()
            needsUpdate = true
        }
        if commodore != c64.keyboard.commodoreIsPressed() {
            commodore = c64.keyboard.commodoreIsPressed()
            needsUpdate = true
        }
        if lowercase != !c64.keyboard.inUpperCaseMode() {
            lowercase = !c64.keyboard.inUpperCaseMode()
            needsUpdate = true
        }
        
        if needsUpdate {
            updateImages()
        }
    }
    
    func updateImages() {
                
        for nr in 0 ... 65 {
            
            let shiftLock = c64.keyboard.shiftLockIsHoldDown()
            let pressed =
                (nr == 17 && control) ||
                (nr == 34 && shiftLock) ||
                (nr == 49 && commodore) ||
                (nr == 50 && lshift) ||
                (nr == 61 && rshift)
            let shift = lshift || rshift || shiftLock
            
            keyView[nr]!.image = C64Key(nr).image(pressed: pressed,
                                                  shift: shift,
                                                  control: control,
                                                  commodore: commodore,
                                                  lowercase: lowercase)
        }
    }
    
    func releaseSpecialKeys() {
        
        self.parent.c64.keyboard.releaseKey(atRow: C64Key.control.row,
                                            col: C64Key.control.col)
        self.parent.c64.keyboard.releaseKey(atRow: C64Key.commodore.row,
                                            col: C64Key.commodore.col)
        self.parent.c64.keyboard.releaseKey(atRow: C64Key.shift.row,
                                            col: C64Key.shift.col)
        self.parent.c64.keyboard.releaseKey(atRow: C64Key.rightShift.row,
                                            col: C64Key.rightShift.col)
    }
    
    @IBAction func pressVirtualC64Key(_ sender: Any!) {
        
        let tag = (sender as! NSButton).tag
        let key = C64Key(tag)
        
        func press() {
            if key.nr == 31 {
                parent.c64.keyboard.pressRestoreKey()
            } else {
                parent.c64.keyboard.pressKey(atRow: key.row, col: key.col)
            }
        }
        func release() {
            if key.nr == 31 {
                parent.c64.keyboard.releaseRestoreKey()
            } else {
                parent.c64.keyboard.releaseKey(atRow: key.row, col: key.col)
            }
        }
        
        switch (key.nr) {
            
        case 34: // Shift Lock
            shiftLock ? c64.keyboard.unlockShift() : c64.keyboard.lockShift()

        case 17: // Control
            control ? release() : press()

        case 49: // Commodore
            commodore ? release() : press()
            
        case 50: // Left Shift
            lshift ? release() : press()
            
        case 61: // Right Shift
            rshift ? release() : press()
            
        default:
            
            DispatchQueue.global().async {
                
                press()
                usleep(useconds_t(20000))
                release()
                if (self.autoClose) {
                    self.releaseSpecialKeys()
                }
            }
            
            if (autoClose) {
                cancelAction(self)
            }
        }
    }
}

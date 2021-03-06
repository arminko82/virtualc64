//
// This file is part of VirtualC64 - A user-friendly Commodore 64 emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
//

import Foundation

class TapeMountController : UserDialogController {
    
    var tape: TAPFileProxy!
    
    // Outlets
    @IBOutlet weak var autoLoad: NSButton!
    @IBOutlet weak var autoPress: NSButton!
    
    override func showSheet(withParent controller: MyController,
                   completionHandler:(() -> Void)? = nil) {
        
        track()
        tape = controller.mydocument.attachment as? TAPFileProxy
        super.showSheet(withParent: controller, completionHandler: completionHandler)
    }
    
    //
    // Action methods
    //

    @IBAction func autoLoadAction(_ sender: Any!) {
        
        let sender = sender as! NSButton
        autoPress.isEnabled = (sender.integerValue == 1)
    }
    
    @IBAction func okAction(_ sender: Any!) {
        
        // Insert tape
        c64.datasette.insertTape(tape)
        parent.metalScreen.rotateBack()
        
        // Process options
        if autoLoad.integerValue == 1 {
            let kb = parent.keyboardcontroller!
            if autoPress.integerValue == 1 {
                kb.typeOnKeyboard(string: "LOAD\n", completion: c64.datasette.pressPlay)
            } else {
                kb.typeOnKeyboard(string: "LOAD\n", completion: nil)
            }
        }
        
        hideSheet()
    }
}


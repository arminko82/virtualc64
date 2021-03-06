//
// This file is part of VirtualC64 - A user-friendly Commodore 64 emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
//

import Foundation

extension NSDraggingInfo
{
    var url: URL? {
        let pasteBoard = draggingPasteboard()
        let types = [NSPasteboard.PasteboardType.compatibleFileURL]
        if let _ = pasteBoard.availableType(from: types) {
            return NSURL.init(from: pasteBoard) as URL?
        }
        return nil
    }
}

class RomDropView : NSImageView
{
    @IBOutlet var dialogController: RomDialogController!
    
    override func awakeFromNib()
    {
        registerForDraggedTypes([NSPasteboard.PasteboardType.compatibleFileURL])
    }
    
    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation
    {
        if let url = sender.url {
            if dialogController.c64.isRom(url) {
                image = dialogController.romImageMedium
                return .copy
            }
        }
        return NSDragOperation()
    }
    
    override func draggingExited(_ sender: NSDraggingInfo?)
    {
        dialogController.refresh()
    }
    
    override func prepareForDragOperation(_ sender: NSDraggingInfo) -> Bool
    {
        return true
    }

    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool
    {
        guard let url = sender.url else {
            return false
        }
        if !dialogController.parent.mydocument.loadRom(url) {
            return false
        }
        
        // Check if all ROMs are loaded
        if dialogController.c64.isRunnable() {
            dialogController.hideSheet()
        }
        return true
    }
    
    override func concludeDragOperation(_ sender: NSDraggingInfo?)
    {
        track()
        dialogController.refresh()
    }
}


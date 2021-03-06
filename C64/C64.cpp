/*!
 * @file        C64.cpp
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   Dirk W. Hoffmann. All rights reserved.
 */
/*
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

//
// Emulator thread
//

void 
threadCleanup(void* thisC64)
{
    assert(thisC64 != NULL);
    
    C64 *c64 = (C64 *)thisC64;
    c64->threadCleanup();
    c64->sid.halt();
    
    c64->debug(2, "Execution thread terminated\n");
    c64->putMessage(MSG_HALT);
}

void 
*runThread(void *thisC64) {
        
    assert(thisC64 != NULL);
    
    C64 *c64 = (C64 *)thisC64;
    bool success = true;
    
    c64->debug(2, "Execution thread started\n");
    c64->putMessage(MSG_RUN);
    
    // Configure thread properties...
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push(threadCleanup, thisC64);
    
    // Prepare to run...
    c64->cpu.clearErrorState();
    c64->drive1.cpu.clearErrorState();
    c64->drive2.cpu.clearErrorState();
    c64->restartTimer();
    
    while (likely(success)) {
        pthread_testcancel();
        success = c64->executeOneFrame();
    }
    
    pthread_cleanup_pop(1);
    pthread_exit(NULL);    
}


//
// Class methods
//

C64::C64()
{
    setDescription("C64");
    debug("Creating virtual C64[%p]\n", this);

    p = NULL;
    warp = false;
    alwaysWarp = false;
    warpLoad = false;
    
    // Register sub components
    VirtualComponent *subcomponents[] = {
        
        &mem,
        &cpu,
        &processorPort,
        &cia1, &cia2,
        &vic,
        &sid,
        &keyboard,
        &port1,
        &port2,
        &expansionport,
        &iec,
        &drive1,
        &drive2,
        &datasette,
        &mouse1350,
        &mouse1351,
        &neosMouse,
        NULL };
    
    registerSubComponents(subcomponents, sizeof(subcomponents));
    setC64(this);
    
    // Register snapshot items
    SnapshotItem items[] = {
 
        { &frame,              sizeof(frame),              CLEAR_ON_RESET },
        { &rasterLine,         sizeof(rasterLine),         CLEAR_ON_RESET },
        { &rasterCycle,        sizeof(rasterCycle),        CLEAR_ON_RESET },
        { &frequency,          sizeof(frequency),          KEEP_ON_RESET  },
        { &durationOfOneCycle, sizeof(durationOfOneCycle), KEEP_ON_RESET  },
        { &warp,               sizeof(warp),               CLEAR_ON_RESET },
        // { &alwaysWarp,         sizeof(alwaysWarp),         CLEAR_ON_RESET },
        // { &warpLoad,           sizeof(warpLoad),           KEEP_ON_RESET  },
        { &ultimax,            sizeof(ultimax),            CLEAR_ON_RESET },
        
        { NULL,             0,                       0 }};
    
    registerSnapshotItems(items, sizeof(items));

    // Set initial hardware configuration
    mouse = &mouse1350;
    mousePort = 0;
    vic.setModel(PAL_8565);
    drive1.powerOn();
    drive2.powerOff();
    
    // Initialize mach timer info
    mach_timebase_info(&timebase);

    snapshotInterval = 3;

    reset();
}

C64::~C64()
{
    debug(1, "Destroying virtual C64[%p]\n", this);
    
    halt();
}

void
C64::reset()
{
    debug(1, "Resetting virtual C64[%p]\n", this);
    
    // Reset all sub components
    VirtualComponent::reset();
    
    // Initialize processor port data direction register and processor port
    mem.poke(0x0000, 0x2F);  // Data direction
    mem.poke(0x0001, 0x1F);  // IO port, set default memory layout

    // Initialize program counter
    bool takeFromRom = mem.getPeekSource(0xFFFC) == M_ROM;
    cpu.regPC = takeFromRom ? 0xFCE2 : LO_HI(mem.spypeek(0xFFFC), mem.spypeek(0xFFFD));
    
    rasterCycle = 1;
    nanoTargetTime = 0UL;
    ping();
}

void C64::ping()
{
    debug (2, "Pinging virtual C64[%p]\n", this);

    VirtualComponent::ping();
    putMessage(warp ? MSG_WARP_ON : MSG_WARP_OFF);
    putMessage(alwaysWarp ? MSG_ALWAYS_WARP_ON : MSG_ALWAYS_WARP_OFF);
}

void
C64::setClockFrequency(uint32_t value)
{
    VirtualComponent::setClockFrequency(value);
    
    frequency = value;
    durationOfOneCycle = 10000000000 / value;
    debug(2, "Duration of a C64 CPU cycle is %lld 1/10 nsec.\n", durationOfOneCycle);
}

void
C64::suspend()
{
    debug(2, "Suspending...(%d)\n", suspendCounter);
    
    if (suspendCounter == 0 && isHalted())
    return;
    
    halt();
    suspendCounter++;
}

void
C64::resume()
{
    debug(2, "Resuming (%d)...\n", suspendCounter);
    
    if (suspendCounter == 0)
    return;
    
    if (--suspendCounter == 0)
    run();
}

void 
C64::dump() {
    msg("C64:\n");
    msg("----\n\n");
    msg("              Machine type : %s\n", vic.isPAL() ? "PAL" : "NTSC");
    msg("         Frames per second : %f\n", vic.getFramesPerSecond());
    msg("     Rasterlines per frame : %d\n", vic.getRasterlinesPerFrame());
    msg("     Cycles per rasterline : %d\n", vic.getCyclesPerRasterline());
    msg("             Current cycle : %llu\n", cpu.cycle);
    msg("             Current frame : %d\n", frame);
    msg("        Current rasterline : %d\n", rasterLine);
    msg("  Current rasterline cycle : %d\n", rasterCycle);
    msg("              Ultimax mode : %s\n\n", getUltimax() ? "YES" : "NO");
    msg("warp, warpLoad, alwaysWarp : %d %d %d\n", warp, warpLoad, alwaysWarp);
    msg("\n");
}

C64Model
C64::getModel()
{
    // Look for known configurations
    for (unsigned i = 0; i < sizeof(configurations) / sizeof(C64Configuration); i++) {
        if (vic.getModel() == configurations[i].vic &&
            vic.emulateGrayDotBug == configurations[i].grayDotBug &&
            cia1.getModel() == configurations[i].cia &&
            cia1.getEmulateTimerBBug() == configurations[i].timerBBug &&
            sid.getModel() == configurations[i].sid &&
            sid.getAudioFilter() == configurations[i].sidFilter &&
            vic.getGlueLogic() == configurations[i].glue &&
            mem.getRamInitPattern() == configurations[i].pattern) {
            return (C64Model)i;
        }
    }
    
    // We've got a non-standard configuration
    return C64_CUSTOM; 
}

void
C64::setModel(C64Model m)
{
    if (m != C64_CUSTOM) {
        
        suspend();
        vic.setModel(configurations[m].vic);
        vic.emulateGrayDotBug = configurations[m].grayDotBug;
        cia1.setModel(configurations[m].cia);
        cia2.setModel(configurations[m].cia);
        cia1.setEmulateTimerBBug(configurations[m].timerBBug);
        cia2.setEmulateTimerBBug(configurations[m].timerBBug);
        sid.setModel(configurations[m].sid);
        sid.setAudioFilter(configurations[m].sidFilter);
        vic.setGlueLogic(configurations[m].glue);
        mem.setRamInitPattern(configurations[m].pattern);
        resume();
    }
}

void
C64::updateVicFunctionTable()
{
    // Assign model independent execution functions
    vicfunc[0] = NULL;
    vicfunc[12] = &VIC::cycle12;
    vicfunc[13] = &VIC::cycle13;
    vicfunc[14] = &VIC::cycle14;
    vicfunc[15] = &VIC::cycle15;
    vicfunc[16] = &VIC::cycle16;
    vicfunc[17] = &VIC::cycle17;
    vicfunc[18] = &VIC::cycle18;
    
    for (unsigned cycle = 19; cycle <= 54; cycle++)
        vicfunc[cycle] = &VIC::cycle19to54;

    vicfunc[56] = &VIC::cycle56;
    
    // Assign model specific execution functions
    switch (vic.getModel()) {
            
        case PAL_6569_R1:
        case PAL_6569_R3:
        case PAL_8565:
            
            vicfunc[1] = &VIC::cycle1pal;
            vicfunc[2] = &VIC::cycle2pal;
            vicfunc[3] = &VIC::cycle3pal;
            vicfunc[4] = &VIC::cycle4pal;
            vicfunc[5] = &VIC::cycle5pal;
            vicfunc[6] = &VIC::cycle6pal;
            vicfunc[7] = &VIC::cycle7pal;
            vicfunc[8] = &VIC::cycle8pal;
            vicfunc[9] = &VIC::cycle9pal;
            vicfunc[10] = &VIC::cycle10pal;
            vicfunc[11] = &VIC::cycle11pal;
            vicfunc[55] = &VIC::cycle55pal;
            vicfunc[57] = &VIC::cycle57pal;
            vicfunc[58] = &VIC::cycle58pal;
            vicfunc[59] = &VIC::cycle59pal;
            vicfunc[60] = &VIC::cycle60pal;
            vicfunc[61] = &VIC::cycle61pal;
            vicfunc[62] = &VIC::cycle62pal;
            vicfunc[63] = &VIC::cycle63pal;
            vicfunc[64] = NULL;
            vicfunc[65] = NULL;
            break;
            
        case NTSC_6567_R56A:
            
            vicfunc[1] = &VIC::cycle1pal;
            vicfunc[2] = &VIC::cycle2pal;
            vicfunc[3] = &VIC::cycle3pal;
            vicfunc[4] = &VIC::cycle4pal;
            vicfunc[5] = &VIC::cycle5pal;
            vicfunc[6] = &VIC::cycle6pal;
            vicfunc[7] = &VIC::cycle7pal;
            vicfunc[8] = &VIC::cycle8pal;
            vicfunc[9] = &VIC::cycle9pal;
            vicfunc[10] = &VIC::cycle10pal;
            vicfunc[11] = &VIC::cycle11pal;
            vicfunc[55] = &VIC::cycle55ntsc;
            vicfunc[57] = &VIC::cycle57ntsc;
            vicfunc[58] = &VIC::cycle58ntsc;
            vicfunc[59] = &VIC::cycle59ntsc;
            vicfunc[60] = &VIC::cycle60ntsc;
            vicfunc[61] = &VIC::cycle61ntsc;
            vicfunc[62] = &VIC::cycle62ntsc;
            vicfunc[63] = &VIC::cycle63ntsc;
            vicfunc[64] = &VIC::cycle64ntsc;
            vicfunc[65] = NULL;
            break;
            
        case NTSC_6567:
        case NTSC_8562:
            
            vicfunc[1] = &VIC::cycle1ntsc;
            vicfunc[2] = &VIC::cycle2ntsc;
            vicfunc[3] = &VIC::cycle3ntsc;
            vicfunc[4] = &VIC::cycle4ntsc;
            vicfunc[5] = &VIC::cycle5ntsc;
            vicfunc[6] = &VIC::cycle6ntsc;
            vicfunc[7] = &VIC::cycle7ntsc;
            vicfunc[8] = &VIC::cycle8ntsc;
            vicfunc[9] = &VIC::cycle9ntsc;
            vicfunc[10] = &VIC::cycle10ntsc;
            vicfunc[11] = &VIC::cycle11ntsc;
            vicfunc[55] = &VIC::cycle55ntsc;
            vicfunc[57] = &VIC::cycle57ntsc;
            vicfunc[58] = &VIC::cycle58ntsc;
            vicfunc[59] = &VIC::cycle59ntsc;
            vicfunc[60] = &VIC::cycle60ntsc;
            vicfunc[61] = &VIC::cycle61ntsc;
            vicfunc[62] = &VIC::cycle62ntsc;
            vicfunc[63] = &VIC::cycle63ntsc;
            vicfunc[64] = &VIC::cycle64ntsc;
            vicfunc[65] = &VIC::cycle65ntsc;
            break;
            
        default:
            assert(false);
    }
}

void
C64::powerUp()
{
    suspend();
    reset();
    resume();
    run();
}

void
C64::run()
{
    if (isHalted()) {
        
        // Check for ROM images
        if (!isRunnable()) {
            putMessage(MSG_ROM_MISSING);
            return;
        }
        
        // Power up sub components
        sid.run();
        
        // Start execution thread
        pthread_create(&p, NULL, runThread, (void *)this);
    }
}

void
C64::halt()
{
    if (isRunning()) {
        
        // Cancel execution thread
        pthread_cancel(p);
        // Wait until thread terminates
        pthread_join(p, NULL);
        // Finish the current command (to reach a clean state)
        step();
    }
}

void
C64::threadCleanup()
{
    p = NULL;
    debug(2, "Execution thread cleanup\n");
}

bool
C64::isRunnable()
{
    return
    mem.basicRomIsLoaded() &&
    mem.charRomIsLoaded() &&
    mem.kernalRomIsLoaded() &&
    drive1.mem.romIsLoaded() &&
    drive2.mem.romIsLoaded();
}

bool
C64::isRunning()
{
    return p != NULL;
}

bool
C64::isHalted()
{
    return p == NULL;
}

void
C64::step()
{
    cpu.clearErrorState();
    drive1.cpu.clearErrorState();
    drive2.cpu.clearErrorState();

    // Wait until the execution of the next command has begun
    while (cpu.inFetchPhase()) executeOneCycle();

    // Finish the command
    while (!cpu.inFetchPhase()) executeOneCycle();
    
    // Execute the first microcycle (fetch phase) and stop there
    executeOneCycle();
}

void
C64::stepOver()
{
    cpu.clearErrorState();
    drive1.cpu.clearErrorState();
    drive2.cpu.clearErrorState();
    
    // If the next instruction is a JSR instruction, ...
    if (mem.spypeek(cpu.getPC()) == 0x20) {
        // set a soft breakpoint at the next memory location.
        cpu.setSoftBreakpoint(cpu.getAddressOfNextInstruction());
        run();
        return;
    }

    // Otherwise, stepOver behaves like step
    step();
}

bool
C64::executeOneLine()
{
    if (rasterCycle == 1)
    beginRasterLine();
    
    int lastCycle = vic.getCyclesPerRasterline();
    for (unsigned i = rasterCycle; i <= lastCycle; i++) {
        if (!_executeOneCycle()) {
            if (i == lastCycle)
            endRasterLine();
            return false;
        }
    }
    endRasterLine();
    return true;
}

bool
C64::executeOneFrame()
{
    do {
        if (!executeOneLine())
        return false;
    } while (rasterLine != 0);
    return true;
}

bool
C64::executeOneCycle()
{
    bool isFirstCycle = rasterCycle == 1;
    bool isLastCycle = vic.isLastCycleInRasterline(rasterCycle);
    
    if (isFirstCycle) beginRasterLine();
    bool result = _executeOneCycle();
    if (isLastCycle) endRasterLine();
    
    return result;
}

bool
C64::_executeOneCycle()
{
    uint8_t result = true;
    uint64_t cycle = ++cpu.cycle;
    
    //  <---------- o2 low phase ----------->|<- o2 high phase ->|
    //                                       |                   |
    // ,-- C64 ------------------------------|-------------------|--,
    // |   ,-----,     ,-----,     ,-----,   |    ,-----,        |  |
    // |   |     |     |     |     |     |   |    |     |        |  |
    // '-->| VIC | --> | CIA | --> | CIA | --|--> | CPU | -------|--'
    //     |     |     |  1  |     |  2  |   |    |     |        |
    //     '-----'     '-----'     '-----'   |    '-----'        |
    //                                       v
    //                                 IEC bus update      IEC bus update
    //                                                           ^
    //                                       |    ,--------,     |
    //                                       |    |        |     |
    // ,-- Drive ----------------------------|--> | VC1541 | ----|--,
    // |                                     |    |        |     |  |
    // |                                     |    '--------'     |  |
    // '-------------------------------------|-------------------|--'
    
    // First clock phase (o2 low)
    (vic.*vicfunc[rasterCycle])();
    if (cycle >= cia1.wakeUpCycle) cia1.executeOneCycle(); else cia1.idleCounter++;
    if (cycle >= cia2.wakeUpCycle) cia2.executeOneCycle(); else cia2.idleCounter++;
    if (iec.isDirtyC64Side) iec.updateIecLinesC64Side();
    
    // Second clock phase (o2 high)
    result &= cpu.executeOneCycle();
    if (drive1.isPoweredOn()) result &= drive1.execute(durationOfOneCycle);
    if (drive2.isPoweredOn()) result &= drive2.execute(durationOfOneCycle);
    // if (iec.isDirtyDriveSide) iec.updateIecLinesDriveSide();
    datasette.execute();
    
    rasterCycle++;
    return result;
}

void
C64::beginRasterLine()
{
    // First cycle of rasterline
    if (rasterLine == 0) {
        vic.beginFrame();
    }
    vic.beginRasterline(rasterLine);
}

void
C64::endRasterLine()
{
    vic.endRasterline();
    rasterCycle = 1;
    rasterLine++;
    
    if (rasterLine >= vic.getRasterlinesPerFrame()) {
        rasterLine = 0;
        endFrame();
    }
}

void
C64::endFrame()
{
    frame++;
    vic.endFrame();
    
    // Increment time of day clocks every tenth of a second
    cia1.incrementTOD();
    cia2.incrementTOD();
    
    // Execute remaining SID cycles
    sid.executeUntil(cpu.cycle);
    
    // Execute other components
    iec.execute();
    expansionport.execute();
    port1.execute();
    port2.execute();

    // Update mouse coordinates
    if (mousePort != 0) mouse->execute();
    
    // Take a snapshot once in a while
    if (snapshotInterval > 0 &&
        frame % ((unsigned)(vic.getFramesPerSecond() * snapshotInterval)) == 0) {
        takeAutoSnapshot();
    }
    
    // Count some sheep (zzzzzz) ...
    if (!getWarp()) {
            synchronizeTiming();
    }
}

void
C64::setMouseModel(MouseModel value)
{
    suspend();
    
    switch(value) {
        case MOUSE1350:
        mouse = &mouse1350;
        break;
        case MOUSE1351:
        mouse = &mouse1351;
        break;
        case NEOSMOUSE:
        mouse = &neosMouse;
        break;
        default:
        warn("Unknown mouse model selected.\n");
        mouse = &mouse1350;
    }
    
    mouse->reset();
    resume();
}

void
C64::connectMouse(unsigned port)
{
    assert(port <= 2);
    mousePort = port;
}

uint8_t
C64::mouseBits(unsigned port)
{
    if (mousePort != port) {
        return 0xFF;
    } else {
        return mouse->readControlPort();
    }
}

uint8_t
C64::potXBits()
{
    if (mousePort != 0) {
        switch (mouse->mouseModel()) {
            case MOUSE1350:
            return mouse1350.rightButton ? 0x00 : 0xFF;
            case MOUSE1351:
            return mouse1351.mouseXBits();
            case NEOSMOUSE:
            return neosMouse.rightButton ? 0xFF : 0x00;
        }
    }
    return 0xFF;
}

uint8_t
C64::potYBits()
{
    if (mousePort != 0 && mouse->mouseModel() == MOUSE1351) {
        return mouse1351.mouseYBits();
    } else {
        return 0xFF;
    }
}

bool
C64::getWarp()
{
    bool newValue = (warpLoad && iec.isBusy()) || alwaysWarp;
    
    if (newValue != warp) {
        warp = newValue;
        
        /* Warping has the unavoidable drawback that audio playback gets out of
         * sync. To cope with this issue, we silence SID during warp mode and
         * fade in smoothly after warping has ended.
         */
        
        if (warp) {
            // Quickly fade out SID
            sid.rampDown();
            
        } else {
            // Smoothly fade in SID
            sid.rampUp();
            restartTimer();
        }
        
        putMessage(warp ? MSG_WARP_ON : MSG_WARP_OFF);
    }
    
    return warp;
}

void
C64::setAlwaysWarp(bool b)
{
    if (alwaysWarp != b) {
        
        alwaysWarp = b;
        putMessage(b ? MSG_ALWAYS_WARP_ON : MSG_ALWAYS_WARP_OFF);
    }
}

void
C64::setWarpLoad(bool b)
{
    warpLoad = b;
}

void
C64::restartTimer()
{
    uint64_t kernelNow = mach_absolute_time();
    uint64_t nanoNow = abs_to_nanos(kernelNow);
    
    nanoTargetTime = nanoNow + vic.getFrameDelay();
}

void
C64::synchronizeTiming()
{
    const uint64_t earlyWakeup = 1500000; /* 1.5 milliseconds */
    
    // Get current time in nano seconds
    uint64_t nanoAbsTime = abs_to_nanos(mach_absolute_time());
    
    // Check how long we're supposed to sleep
    int64_t timediff = (int64_t)nanoTargetTime - (int64_t)nanoAbsTime;
    if (timediff > 200000000 || timediff < -200000000 /* 0.2 sec */) {
        
        // The emulator seems to be out of sync, so we better reset the
        // synchronization timer
        
        debug(2, "Emulator lost synchronization (%lld). Restarting timer.\n", timediff);
        restartTimer();
    }
    
    // Convert nanoTargetTime into kernel unit
    int64_t kernelTargetTime = nanos_to_abs(nanoTargetTime);
    
    // Sleep and update target timer
    // debug(2, "%p Sleeping for %lld\n", this, kernelTargetTime - mach_absolute_time());
    int64_t jitter = sleepUntil(kernelTargetTime, earlyWakeup);
    nanoTargetTime += vic.getFrameDelay();
    
    // debug(2, "Jitter = %d", jitter);
    if (jitter > 1000000000 /* 1 sec */) {
        
        // The emulator did not keep up with the real time clock. Instead of
        // running behind for a long time, we reset the synchronization timer
        
        debug(2, "Jitter exceeds limit (%lld). Restarting synchronization timer.\n", jitter);
        restartTimer();
    }
}

void C64::loadFromSnapshotUnsafe(Snapshot *snapshot)
{    
    uint8_t *ptr;
    
    if (snapshot && (ptr = snapshot->getData())) {
        loadFromBuffer(&ptr);
        keyboard.releaseAll(); // Avoid constantly pressed keys
        ping();
    }
}

void
C64::loadFromSnapshotSafe(Snapshot *snapshot)
{
    debug(2, "C64::loadFromSnapshotSafe\n");

    suspend();
    loadFromSnapshotUnsafe(snapshot);
    resume();
}

bool
C64::restoreSnapshot(vector<Snapshot *> &storage, unsigned nr)
{
    Snapshot *snapshot = getSnapshot(storage, nr);
    
    if (snapshot) {
        loadFromSnapshotSafe(snapshot);
        return true;
    }
    
    return false;
}

size_t
C64::numSnapshots(vector<Snapshot *> &storage)
{
    return storage.size();
}

Snapshot *
C64::getSnapshot(vector<Snapshot *> &storage, unsigned nr)
{
    return nr < storage.size() ? storage.at(nr) : NULL;
    
}

void
C64::takeSnapshot(vector<Snapshot *> &storage)
{
    // Delete oldest snapshot if capacity limit has been reached
    if (storage.size() >= MAX_SNAPSHOTS) {
        deleteSnapshot(storage, MAX_SNAPSHOTS - 1);
    }
    
    Snapshot *snapshot = Snapshot::makeWithC64(this);
    storage.insert(storage.begin(), snapshot);
    putMessage(MSG_SNAPSHOT_TAKEN);
}

void
C64::deleteSnapshot(vector<Snapshot *> &storage, unsigned index)
{
    Snapshot *snapshot = getSnapshot(storage, index);
    
    if (snapshot) {
        delete snapshot;
        storage.erase(storage.begin() + index);
    }
}

bool
C64::flash(AnyC64File *file)
{
    bool result = true;
    
    suspend();
    switch (file->type()) {
        
        case BASIC_ROM_FILE:
        file->flash(mem.rom, 0xA000);
        break;
        
        case CHAR_ROM_FILE:
        file->flash(mem.rom, 0xD000);
        break;
        
        case KERNAL_ROM_FILE:
        file->flash(mem.rom, 0xE000);
        break;
        
        case VC1541_ROM_FILE:
        file->flash(drive1.mem.rom);
        file->flash(drive2.mem.rom);
        break;
        
        case V64_FILE:
        loadFromSnapshotUnsafe((Snapshot *)file);
        break;
        
        default:
        assert(false);
        result = false;
    }
    resume();
    return result;
}

bool
C64::flash(AnyArchive *file, unsigned item)
{
    bool result = true;
    
    suspend();
    switch (file->type()) {
        
        case D64_FILE:
        case T64_FILE:
        case PRG_FILE:
        case P00_FILE:
        file->selectItem(item);
        file->flashItem(mem.ram);
        break;
        
        default:
        assert(false);
        result = false;
    }
    resume();
    return result;
}

bool
C64::loadRom(const char *filename)
{
    bool result;
    bool wasRunnable = isRunnable();
    ROMFile *rom = ROMFile::makeWithFile(filename);
    
    if (!rom) {
        warn("Failed to read ROM image file %s\n", filename);
        return false;
    }
    
    suspend();
    result = flash(rom);
    resume();
    
    if (result) {
        debug(2, "Loaded ROM image %s.\n", filename);
    } else {
        debug(2, "Failed to flash ROM image %s.\n", filename);
    }
    
    if (!wasRunnable && isRunnable())
        putMessage(MSG_READY_TO_RUN);
    
    delete rom;
    return result;
}

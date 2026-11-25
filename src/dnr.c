/*
 * A small glue for the MacTCP domain name resolver (DNR).
 *
 * The resolver is a code resource ('dnrp') in the MacTCP control panel.
 * It is called like a C function whose first argument is a selector.
 * It was built with MPW C, which may change D2; gcc expects D2 to be kept.
 * So all calls go through dnr_call, which saves every register gcc keeps.
 *
 * This does what Apple's dnr.c does, without the MixedMode parts that a
 * 68k program does not need. From HTTPHead, a MacTCP sample for the Snow
 * emulator. Archaic's nmactcp.c uses it for names; C99 (see CMakeLists.txt).
 */
#include <Files.h>
#include <Gestalt.h>
#include <Resources.h>
#include <MacMemory.h>
#include <OSUtils.h>
#include <Events.h>
#include <Traps.h>
#include "dnr.h"

enum { OPENRESOLVER = 1, CLOSERESOLVER = 2, STRTOADDR = 3 };

/* Folder Manager values that Multiversal Interfaces do not name */
enum { gestaltFindFolderPresent = 0 };
#define kOnSystemDisk ((short)0x8000)
#define kDontCreateFolder false

/* OSErr dnr_call(Ptr code, long selector, void *a, void *b, void *c, void *d) */
OSErr dnr_call(Ptr code, long selector, void *a, void *b, void *c, void *d);
__asm__(
    "   .text\n"
    "   .even\n"
    "   .globl dnr_call\n"
    "dnr_call:\n"
    "   movem.l %d2-%d7/%a2-%a6,-(%sp)\n" /* 44 bytes; return address at 44(sp) */
    "   move.l  68(%sp),-(%sp)\n"         /* d */
    "   move.l  68(%sp),-(%sp)\n"         /* c */
    "   move.l  68(%sp),-(%sp)\n"         /* b */
    "   move.l  68(%sp),-(%sp)\n"         /* a */
    "   move.l  68(%sp),-(%sp)\n"         /* selector */
    "   move.l  68(%sp),%a0\n"            /* code */
    "   jsr     (%a0)\n"
    "   lea     20(%sp),%sp\n"
    "   movem.l (%sp)+,%d2-%d7/%a2-%a6\n"
    "   rts\n");

static Handle gCode;

static Boolean trap_available(unsigned short trap)
{
    TrapType type = (trap & 0x0800) ? kToolboxTrapType : kOSTrapType;
    if (type == kToolboxTrapType) {
        /* A small trap table has no traps above 0x1FF */
        Boolean small = NGetTrapAddress(0xA86E, kToolboxTrapType) ==
                        NGetTrapAddress(0xAA6E, kToolboxTrapType);
        if ((trap & 0x07FF) >= (small ? 0x200 : 0x400))
            return false;
    }
    return NGetTrapAddress(trap, type) !=
           NGetTrapAddress(_Unimplemented, kToolboxTrapType);
}

/* Opens the resource file of the first file with this type and creator in a
   folder that has a 'dnrp' resource. dirID 0 means vRefNum is a working
   directory (System 6). Returns the file's refnum, or -1. */
static short open_dnr_file(short vRefNum, long dirID, OSType creator)
{
    Str255 name;
    HParamBlockRec pb;

    for (short i = 1;; i++) {
        pb.fileParam.ioCompletion = NULL;
        pb.fileParam.ioNamePtr = name;
        pb.fileParam.ioVRefNum = vRefNum;
        pb.fileParam.ioFDirIndex = i;
        pb.fileParam.ioDirID = dirID;
        if (PBHGetFInfoSync(&pb) != noErr)
            return -1;
        if (pb.fileParam.ioFlFndrInfo.fdType != 'cdev' ||
            pb.fileParam.ioFlFndrInfo.fdCreator != creator)
            continue;
        short ref = dirID ? HOpenResFile(vRefNum, dirID, name, fsRdPerm)
                          : OpenRFPerm(name, vRefNum, fsRdPerm);
        if (ref == -1)
            continue;
        if (Get1IndResource('dnrp', 1) != NULL)
            return ref;
        CloseResFile(ref);
    }
}

/* Finds the MacTCP control panel: MacTCP 1.1 and later ('ztcp') in Control
   Panels (System 7) or in the System Folder (System 6), else MacTCP 1.0
   ('mtcp') in the System Folder */
static short open_mactcp(void)
{
    short ref = -1;
    long attr;

    if (trap_available(_Gestalt) &&
        Gestalt(gestaltFindFolderAttr, &attr) == noErr &&
        (attr & (1L << gestaltFindFolderPresent))) {
        short vRefNum;
        long dirID;
        if (FindFolder(kOnSystemDisk, kControlPanelFolderType,
                       kDontCreateFolder, &vRefNum, &dirID) == noErr)
            ref = open_dnr_file(vRefNum, dirID, 'ztcp');
    }
    if (ref == -1) {
        SysEnvRec env;
        SysEnvirons(1, &env);
        ref = open_dnr_file(env.sysVRefNum, 0, 'ztcp');
        if (ref == -1)
            ref = open_dnr_file(env.sysVRefNum, 0, 'mtcp');
    }
    return ref;
}

OSErr dnr_open(void)
{
    OSErr err;

    if (gCode != NULL)
        return noErr;
    short saved = CurResFile();
    short ref = open_mactcp();
    /* Without the control panel, the 'dnrp' may be in the System file */
    gCode = ref != -1 ? Get1IndResource('dnrp', 1) : GetIndResource('dnrp', 1);
    if (gCode == NULL) {
        err = ResError();
        if (ref != -1)
            CloseResFile(ref);
        UseResFile(saved);
        return err ? err : resNotFound;
    }
    DetachResource(gCode);
    if (ref != -1)
        CloseResFile(ref);
    UseResFile(saved);

    /* The code must not move while the resolver is open */
    MoveHHi(gCode);
    HLock(gCode);
    err = dnr_call(*gCode, OPENRESOLVER, NULL, NULL, NULL, NULL);
    if (err != noErr) {
        HUnlock(gCode);
        DisposeHandle(gCode);
        gCode = NULL;
    }
    return err;
}

void dnr_close(void)
{
    if (gCode == NULL)
        return;
    dnr_call(*gCode, CLOSERESOLVER, NULL, NULL, NULL, NULL);
    HUnlock(gCode);
    DisposeHandle(gCode);
    gCode = NULL;
}

/* The resolver calls this when the name server answers, maybe at interrupt
   time: it only sets a flag */
static pascal void dnr_done(struct hostInfo *info, Ptr user)
{
    (void)info;
    *(volatile short *)user = 1;
}

OSErr dnr_lookup(const char *host, unsigned long *addr, long timeoutTicks)
{
    /* The resolver writes here after StrToAddr returns: keep it static */
    static struct hostInfo info;
    static volatile short done;
    OSErr err;

    if (gCode == NULL)
        return notOpenErr;
    done = 0;
    err = dnr_call(*gCode, STRTOADDR, (void *)host, &info, (void *)dnr_done,
                   (void *)&done);
    if (err == cacheFault) {
        /* Not in the cache: the resolver asks the name server */
        unsigned long end = TickCount() + timeoutTicks;
        while (!done) {
            if (TickCount() > end)
                return commandTimeout;
            SystemTask();
        }
        err = (OSErr)info.rtnCode;
    } else if (err == noErr) {
        err = (OSErr)info.rtnCode;
    }
    if (err == noErr)
        *addr = info.addr[0];
    return err;
}

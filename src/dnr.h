/*
 * Glue for the MacTCP domain name resolver (DNR).
 * Multiversal Interfaces have the TCP driver types but not the DNR, so its
 * one structure is here. Layout from Apple's AddressXlation.h.
 */
#ifndef DNR_H
#define DNR_H

#include <stddef.h>
#include <MacTypes.h>

enum { NUM_ALT_ADDRS = 4 };

struct hostInfo {
    long rtnCode;
    char cname[255];
    unsigned long addr[NUM_ALT_ADDRS];
};

#ifdef __m68k__ /* the Mac's 2-byte alignment */
_Static_assert(offsetof(struct hostInfo, addr) == 260, "hostInfo layout");
#endif

/* Loads the MacTCP resolver */
OSErr dnr_open(void);
/* Unloads it */
void dnr_close(void);
/* Looks up a host name or a dotted address; waits up to timeoutTicks */
OSErr dnr_lookup(const char *host, unsigned long *addr, long timeoutTicks);

#endif

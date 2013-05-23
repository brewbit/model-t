#ifndef __WLAN_H__
#define __WLAN_H__

#define mkip(a,b,c,d) ((a) << 24) | ((b) << 16) | ((c) << 8) | (d)

void
wlan_init(void);

#endif

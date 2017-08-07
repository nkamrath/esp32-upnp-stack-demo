#ifndef UPNP_STACK__SSDP_H
#define UPNP_STACK__SSDP_H

/**
Simple Device Discovery Protocol (SSDP) start line defines 
UPnP/SSDP requires that the start line be one of the following
**/
#define SSDP_START_LINE__NOTIFY "NOTIFY * HTTP/1.1\r\n"
#define SSDP_START_LINE__SEARCH "M-SEARCH * HTTP/1.1\r\n"
#define SSDP_START_LINE__OK "HTTP/1.1 200 OK\r\n"

#define SSDP_MULTICAST_ADDRESS "239.255.255.250"
#define SSDP_MULTICAST_PORT 1900


#endif
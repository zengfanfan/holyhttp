#ifndef HOLYHTTP_ADDRESS_H
#define HOLYHTTP_ADDRESS_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IPV4_STR_TO_BIN(string) ntohl(inet_addr(string))
#define IPV4_BIN_TO_STR(bin)    ({struct in_addr ia={.s_addr=htonl(bin)};inet_ntoa(ia);})
#define MAC_ADDR_FMT            "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define MAC_ADDR_ARG(mac)       (mac)[0], (mac)[1], (mac)[2], (mac)[3], (mac)[4], (mac)[5]
#define MAC_STR_TO_BIN(string, bin) \
    (6 == sscanf(string, MAC_ADDR_FMT, &bin[0], &bin[1], &bin[2], &bin[3], &bin[4], &bin[5]))
#define MAC_BIN_TO_STR(bin, buf, buf_sz) \
    (17 == snprintf(buf, buf_sz, MAC_ADDR_FMT, MAC_ADDR_ARG(bin)))

#endif // HOLYHTTP_ADDRESS_H

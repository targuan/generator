/* 
 * File:   writer.h
 * Author: targuan
 *
 * Created on 14 juillet 2013, 20:51
 */

#ifndef WRITER_H
#define	WRITER_H

#ifdef	__cplusplus
extern "C" {
#endif

#define WI_PCAP 0
#define WI_IF 1
       #include <netpacket/packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#include "options.h"

    struct writer_info {
        int type; // 0=pcap 1=interface
        // UNION ?
        struct sockaddr_ll socket_address;
        int sockfd;
        pcap_dumper_t *dumper;
        pcap_t *p;
    };
    
    int wopen(struct writer_info *wi,struct options *opts);
    void wclose(struct writer_info *wi);
    int wwrite(struct writer_info *wi,u_char *packet,int len);


#ifdef	__cplusplus
}
#endif

#endif	/* WRITER_H */


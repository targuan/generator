/* 
 * File:   main.c
 * Author: cqdc5422
 *
 * Created on 29 mai 2013, 16:45
 */

#include <stdio.h>
#include <stdlib.h>
#include <resolv.h>
#include <string.h>
#include <sys/time.h>
#include <net/ethernet.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include "etnernet.h"
#include "options.h"
#include "ip.h"
#include "queries.h"
#include "timer.h"
#include "writer.h"

#define UDP_HLEN	8
#define UDP_HDR_LEN     UDP_HLEN

#define IP_HDR_LEN      (ip->ihl * 4)
#define IP6_HDR_LEN     (sizeof(struct ip6_hdr))



/*
 * 
 */
int sleeper;
double pkt_sent;
double speed_limit;
struct timeval tv;
int stop;

static void * readliner(void * arg) {
	char line[1024];
	char c;
   	int i;
	do{
	    fgets(line,1024,stdin);
	    i = atoi(line);

	    if(i != 0)  speed_limit = i;

	    printf("%f\n",speed_limit);
	    c = line[0];
	} while(c != 'q');
}


double difftimeval(struct timeval x , struct timeval y)
{
    double x_ms , y_ms , diff;
     
    x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
    y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;
     
    diff = (double)y_ms - (double)x_ms;

    return diff/1000000;
}

void speed_calc()
{
    struct timeval tv_old;
    double ellapsed;
    double speed;
    if((tv.tv_sec == tv.tv_usec) && (tv.tv_sec == 0)) {
	gettimeofday(&tv,NULL);
	return;
    }
    tv_old.tv_sec = tv.tv_sec;
    tv_old.tv_usec = tv.tv_usec;

    gettimeofday(&tv,NULL);

    ellapsed = difftimeval(tv_old,tv);

    speed = pkt_sent/ellapsed;

    if(speed>speed_limit) {
        sleeper--;
        if(sleeper<1) sleeper = 1;
    }
    else if(speed<speed_limit) {
        sleeper++;
    }
    pkt_sent = 0;
}


int main(int argc, char** argv) {
    u_char *packet;
    int i;
    uint32_t ck;
    
    struct sockaddr *sin_src;
    struct sockaddr *sin_net;
    struct sockaddr *sin_dst;
    struct sockaddr *sin_mask;
    struct ip6_pseudo_hdr ps_hdr;
    int sendsize;
    struct options opts;
    FILE* fp_domain;
    char * domain;
    struct writer_info wi;
    int pkt_len;

    

    int q_class;
    int q_type;


    pthread_t thread;


    pthread_create(&thread,NULL,readliner,NULL);

    sleeper = 50;
    pkt_sent = 0;
    speed_limit = 50000;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    /**
     * 
     * headers
     */
    struct ether_header *eth;
    struct iphdr *ip;
    struct ip6_hdr *ip6;
    struct udphdr *udp;
    u_char *dns;
    u_char dns_opt[] = {00, 00, 0x29, 0x08, 00, 00, 00, 00, 00, 00, 00};
    setoptions(argc, argv, &opts);
    
    


    

    

    

    packet = calloc(sizeof (u_char) * 4096, 1);
    sin_src = calloc(sizeof (struct sockaddr_storage), 1);
    sin_net = calloc(sizeof (struct sockaddr_storage), 1);
    sin_dst = calloc(sizeof (struct sockaddr_storage), 1);
    sin_mask = calloc(sizeof (struct sockaddr_storage), 1);
    domain = calloc(sizeof (char) * 1024, 1);
    //domain = NULL;
    
    if (packet == NULL ||
            sin_src == NULL ||
            sin_net == NULL ||
            sin_dst == NULL ||
            sin_mask == NULL ||
            domain == NULL) {
        return -1;
    }

    /**
     * 
     * ETHERNET static
     */

    eth = (struct ether_header *) packet;

    if (ether_setaddr(opts.smac, &(eth->ether_shost)) < 0) {
        fprintf(stderr, "smac error:%s\n");
        return 1;
    }
    if (ether_setaddr(opts.dmac, &(eth->ether_dhost)) < 0) {
        fprintf(stderr, "dmac error\n");
        return 1;
    }

    /**
     * 
     * IP static
     */
    if (getipaddr(opts.dip, sin_dst, opts.family) < 0) {
        fprintf(stderr, "dip error\n");
        return 2;
    }
    if (getipaddr(opts.snet, sin_net, opts.family) < 0) {
        fprintf(stderr, "sip error\n");
        return 2;
    }

    if (sin_net->sa_family != sin_dst->sa_family) {
        fprintf(stderr, "IP family doesn't match\n");
        return 2;
    }

    set_mask(sin_mask, sin_net->sa_family, opts.smask);

    if (sin_net->sa_family == AF_INET) {
        eth->ether_type = htons(ETHERTYPE_IP);

        ip = (struct iphdr *) (packet + ETHER_HDR_LEN);
        ip->daddr = ((struct sockaddr_in *) sin_dst)->sin_addr.s_addr;


        ip->version = 4;
        ip->ihl = 5; // no opts
        ip->frag_off = 0;
        ip->id = 0;
        ip->protocol = IPPROTO_UDP;
        ip->tos = 0;
        ip->ttl = IPDEFTTL;


        udp = (struct udphdr *) (((void *) ip) + IP_HDR_LEN);
    } else if (sin_net->sa_family == AF_INET6) {
        eth->ether_type = htons(ETHERTYPE_IPV6);

        ip6 = (struct ip6_hdr *) (packet + ETHER_HDR_LEN);
        ip6->ip6_vfc = 6 << 4;
        ip6->ip6_hlim = IPDEFTTL;
        ip6->ip6_nxt = IPPROTO_UDP;

        memcpy(&(ip6->ip6_dst), &(((struct sockaddr_in6 *) sin_dst)->sin6_addr), 16);

        udp = (struct udphdr *) (((void *) ip6) + IP6_HDR_LEN);
    } else {
        fprintf(stderr, "Family unknown\n");
        return 2;
    }

    /**
     * 
     * UDP static
     */
    //udp->check = 0;
    udp->dest = htons(53);

    /**
     * 
     * DNS static
     */
    dns = (u_char *) (((void *) udp) + UDP_HDR_LEN);


    wopen(&wi,&opts);
    
    init_domain_file(&fp_domain, opts.in_file_name);
    if (fp_domain == NULL) {
        fprintf(stderr, "Can't open queries file\n");
        wclose(&wi);
        
        return 4;
    }
    for (i = 0; i < opts.count; i++) {
	if(pkt_sent >= sleeper) {
		speed_calc();
		usleep(1);
	}
        /**
         * 
         * DNS dynamic
         */
        if(nextdomain(fp_domain, &domain, &q_type, &q_class)) {
            printf("Can't read next domain\n");
            exit(1);
        }

        sendsize = res_mkquery(QUERY, domain, q_class, q_type, NULL,
                0, NULL, dns, PACKETSZ);
        dns[11] = 1;
        memcpy(dns + sendsize, dns_opt, 11);
        sendsize += 11;
        /**
         * 
         * UDP dynamic
         */
        udp->source = htons(rand());
        udp->len = htons(sendsize + UDP_HDR_LEN);
        udp->check = 0;
        /**
         * 
         * IP dynamic
         */
        get_rand_addr(sin_net, sin_mask, sin_src);

        if (sin_net->sa_family == AF_INET) {
            ip->saddr = ((struct sockaddr_in *) sin_src)->sin_addr.s_addr;
            ip->tot_len = htons(sendsize + UDP_HDR_LEN + (ip->ihl * 4));
            pkt_len = sendsize + UDP_HDR_LEN + (ip->ihl * 4) + ETHER_HDR_LEN;

            ip->check = 0;
            ip->check = inet_cksum(ip, (ip->ihl * 4), 0);
        } else if (sin_net->sa_family == AF_INET6) {
            memcpy(&(ip6->ip6_src), &(((struct sockaddr_in6 *) sin_src)->sin6_addr), 16);
            ip6->ip6_plen = htons(sendsize + UDP_HDR_LEN);
            pkt_len = sendsize + UDP_HDR_LEN + sizeof (struct ip6_hdr) +ETHER_HDR_LEN;

            ps_hdr.src = ip6->ip6_src;
            ps_hdr.dst = ip6->ip6_dst;
            ps_hdr.len = udp->len;
            ps_hdr.nh = ntohs(17);

            ck = inet_cksum(&ps_hdr, sizeof (ps_hdr), 0);
            udp->check = inet_cksum(udp, sendsize + UDP_HDR_LEN, (~ck)&0xffff);
        }


        
        
        if (pkt_len > opts.mtu) {
            fprintf(stderr, "too long: %s needs %d MTU is %d\n", domain, pkt_len, opts.mtu);
        } else {
            wwrite(&wi,packet,pkt_len);
        }
	pkt_sent++;
    }

    wclose(&wi);
    fclose(fp_domain);

    return (EXIT_SUCCESS);
}

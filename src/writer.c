#include "writer.h"
#include "options.h"

#include <netinet/in.h>


#define PCAP_SNAPLEN 4096

int wopen(struct writer_info *wi, struct options *opts) {
    struct ifreq if_idx;
    if (opts->interface != NULL) {
        wi->type = WI_IF;

        if ((wi->sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
            perror("socket");
            return -1;
        }

        memset(&if_idx, 0, sizeof (struct ifreq));
        strncpy(if_idx.ifr_name, opts->interface, IFNAMSIZ - 1);
        if (ioctl(wi->sockfd, SIOCGIFINDEX, &if_idx) < 0) {
            perror("SIOCGIFINDEX");
            return -1;
        }

        wi->socket_address.sll_ifindex = if_idx.ifr_ifindex;
        wi->socket_address.sll_halen = ETH_ALEN;
    } else {
        wi->type = WI_PCAP;

        wi->p = pcap_open_dead(DLT_EN10MB, PCAP_SNAPLEN);
        wi->dumper = pcap_dump_open(wi->p, opts->out_file_name);
        if (wi->dumper == NULL) {
            fprintf(stderr, "Can't open output file\n");
            pcap_close(wi->p);
            return 3;
        }
    }
    
    return 0;
}

void wclose(struct writer_info *wi) {
    if (wi->type == WI_PCAP) {
        pcap_dump_close(wi->dumper);
        pcap_close(wi->p);
    } else if (wi->type == WI_IF) {
        close(wi->sockfd);
    }

}

int wwrite(struct writer_info *wi, u_char *packet, int len) {
    static struct pcap_pkthdr header = {0};
    if (wi->type == WI_PCAP) {
        header.caplen = header.len = len;
        //set_ts(&(header.ts), i, &opts);
        pcap_dump(wi->dumper, &header, packet);
    } else if (wi->type == WI_IF) {
        if (sendto(wi->sockfd, packet, len, 0, (struct sockaddr*) &(wi->socket_address), sizeof (struct sockaddr_ll)) < 0)
            perror("Send failed\n");
    }
}
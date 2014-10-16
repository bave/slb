
#ifndef __netmap_hpp__
#define __netmap_hpp__

#include "common.hpp"

#include <iostream>
#include <map>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <ifaddrs.h>
#include <pthread.h>

#include <sys/mman.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef POLL
#include <poll.h>
#else
#include <sys/select.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/ethernet.h>

#include <net/netmap.h>
#include <net/netmap_user.h>


class netmap
{
public:
    // constructor
    netmap();
    // destructor
    virtual ~netmap();

    // control methods
    bool open_if(const std::string& ifname);
    bool open_if(const char* ifname);
    inline bool rxsync(int ringid);
    inline bool txsync(int ringid);
    inline bool rxsync_block(int ringid);
    inline bool txsync_block(int ringid);

    // utils methods
    void dump_nmr();
    bool set_promisc();
    bool unset_promisc();

    // netmap getter
    char* get_ifname();
    uint16_t get_tx_qnum();
    uint16_t get_rx_qnum();
    struct ether_addr* get_mac();

    inline void next(struct netmap_ring* ring);

    inline size_t get_ethlen(struct netmap_ring* ring);
    inline void set_ethlen(struct netmap_ring* ring, size_t size);
    inline struct ether_header* get_eth(struct netmap_ring* ring);

    inline uint32_t get_cursor(struct netmap_ring* ring);
    inline struct netmap_slot* get_slot(struct netmap_ring* ring);

    inline int get_fd(int ringid);
    inline char* get_mem(int ringid);
    inline struct netmap_ring* get_tx_ring(int ringid);
    inline struct netmap_ring* get_rx_ring(int ringid);

    inline int get_fd_sw();
    inline char* get_mem();
    inline struct netmap_ring* get_rx_ring_sw();
    inline struct netmap_ring* get_tx_ring_sw();


private:
    uint32_t nm_version;
    uint16_t nm_rx_qnum;
    uint16_t nm_tx_qnum;
    uint32_t nm_memsize;
    char nm_ifname[IFNAMSIZ];
    struct ether_addr nm_mac;
    uint32_t nm_oui;
    uint32_t nm_bui;
    struct nmreq nm_nmr;

    //hard_info
    int* nm_fds;
    char** nm_mem_addrs;
    struct netmap_ring** nm_tx_rings;
    struct netmap_ring** nm_rx_rings;
    //soft_info
    int nm_fd_soft;
    char* nm_mem_addr_soft;
    struct netmap_ring* nm_tx_ring_soft;
    struct netmap_ring* nm_rx_ring_soft;

    bool _create_nmring(int qnum, int swhw);
    bool _remove_hw_ring(int qnum);
    bool _remove_sw_ring();
};

netmap::netmap()
{
    nm_version = 0;
    nm_rx_qnum = 0;
    nm_tx_qnum = 0;
    nm_memsize = 0;
    memset(nm_ifname, 0, sizeof(nm_ifname));
    memset(&nm_mac, 0, sizeof(nm_mac));
    nm_oui = 0;
    nm_bui = 0;
    nm_fds = NULL;
    nm_mem_addrs = NULL;
    nm_tx_rings = NULL;
    nm_rx_rings = NULL;
    nm_mem_addr_soft = NULL;
    nm_fd_soft = 0;
    nm_tx_ring_soft = NULL;
    nm_rx_ring_soft = NULL;
}

netmap::~netmap()
{
    //printf("hoge:%d\n", __LINE__);
    for (int i = 0; i < nm_tx_qnum; i++) {
        if (nm_mem_addrs[i] != NULL) {
            _remove_hw_ring(i);
        }
    }
    if (nm_fds != NULL) free(nm_fds);
    if (nm_mem_addrs != NULL) free(nm_mem_addrs);
    if (nm_rx_rings != NULL) free(nm_rx_rings);
    if (nm_tx_rings != NULL) free(nm_tx_rings);

    _remove_sw_ring();
    nm_fd_soft = 0;
    nm_mem_addr_soft = NULL;
    nm_tx_ring_soft = NULL;
    nm_rx_ring_soft = NULL;
}

bool
netmap::open_if(const std::string& ifname)
{
    return open_if(ifname.c_str());
}

bool
netmap::open_if(const char* ifname)
{
    int fd;
    fd = open("/dev/netmap", O_RDWR);

    if (fd < 0) {
        PERROR("open");
        MESG("Unable to open /dev/netmap");
        return false;
    }

    memset(&nm_nmr, 0, sizeof(nm_nmr));
    nm_version = 4;
    nm_nmr.nr_version = nm_version;
    strncpy(nm_ifname, ifname, strlen(ifname));
    strncpy(nm_nmr.nr_name, ifname, strlen(ifname));

    if (ioctl(fd, NIOCGINFO, &nm_nmr) < 0) {
        PERROR("ioctl");
        MESG("unabe to get interface info for %s", ifname);
        memset(&nm_nmr, 0, sizeof(nm_nmr));
        close(fd);
        return false;
    }

    if (nm_nmr.nr_tx_rings != nm_nmr.nr_rx_rings) {
        MESG("%s NIC cant supported with this netmap class..", ifname);
        memset(&nm_nmr, 0, sizeof(nm_nmr));
        close(fd);
        return false;
    }

    nm_tx_qnum = nm_nmr.nr_tx_rings;
    nm_rx_qnum = nm_nmr.nr_rx_rings;
    nm_memsize = nm_nmr.nr_memsize;
    close(fd);

    nm_fds = (int*)malloc(sizeof(int*)*nm_rx_qnum);
    memset(nm_fds, 0, sizeof(int*)*nm_rx_qnum);

    nm_mem_addrs = (char**)malloc(sizeof(char*)*nm_rx_qnum);
    memset(nm_mem_addrs, 0, sizeof(char*)*nm_rx_qnum);

    nm_rx_rings = 
        (struct netmap_ring**)malloc(sizeof(struct netmap_ring*)*nm_rx_qnum);
    memset(nm_rx_rings, 0, sizeof(struct netmap_rings**)*nm_rx_qnum);

    nm_tx_rings = 
        (struct netmap_ring**)malloc(sizeof(struct netmap_ring*)*nm_tx_qnum);
    memset(nm_tx_rings, 0, sizeof(struct netmap_rings**)*nm_tx_qnum);

    get_mac_addr(ifname, &nm_mac);

    nm_oui = nm_mac.octet[0]<<16 | nm_mac.octet[1]<<8 | nm_mac.octet[2];
    nm_bui = nm_mac.octet[3]<<16 | nm_mac.octet[4]<<8 | nm_mac.octet[5];
    if (debug) printf("%s_mac_address->%06x:%06x\n", nm_ifname, nm_oui, nm_bui);

    for (int i = 0; i < nm_rx_qnum; i++) {
        if (_create_nmring(i, NETMAP_HW_RING) == false) {
            for (int j = 0; j < i; j++) {
                if (nm_mem_addrs[j] != NULL) {
                    _remove_hw_ring(j);
                }
            }
            if (nm_fds != NULL) free(nm_fds);
            if (nm_mem_addrs != NULL) free(nm_mem_addrs);
            if (nm_rx_rings != NULL) free(nm_rx_rings);
            if (nm_tx_rings != NULL) free(nm_tx_rings);
            return false;
        } else {
            if (debug) printf("(%s:%02d) open_fd :%d\n", ifname, i, nm_fds[i]);
            if (debug) printf("(%s:%02d) open_mem:%p\n", ifname, i, nm_mem_addrs[i]);
            if (debug) printf("(%s:%02d) rx_ring :%p\n", ifname, i, nm_rx_rings[i]);
            if (debug) printf("(%s:%02d) tx_ring :%p\n", ifname, i, nm_tx_rings[i]);
        }
    }

    if (_create_nmring(0, NETMAP_SW_RING) == false) {
        _remove_sw_ring();
    } else {
        if (debug) printf("(%s:sw) open_fd :%d\n", ifname, nm_fd_soft);
        if (debug) printf("(%s:sw) open_mem:%p\n", ifname, nm_mem_addr_soft);
        if (debug) printf("(%s:sw) rx_ring :%p\n", ifname, nm_rx_ring_soft);
        if (debug) printf("(%s:sw) tx_ring :%p\n", ifname, nm_tx_ring_soft);
    }


    return true;
}

inline bool
netmap::rxsync(int ringid)
{
    if (ioctl(nm_fds[ringid], NIOCRXSYNC, ringid) == -1) {
        PERROR("ioctl");
        return false;
    } else {
        return true;
    }
}

inline bool
netmap::rxsync_block(int ringid)
{
#ifdef POLL

    int retval;
    struct pollfd x[1];
    x[0].fd = nm_fds[ringid];
    x[0].events = POLLIN;
    retval = poll(x, 1, -1);
    if (retval == 0) {
        // timeout
        return false;
    } else if (retval < 0) {
        PERROR("poll");
        return false;
    } else {
        return true;
    }

#else 

    int retval;
    fd_set s_fd;
    FD_ZERO(&s_fd);
    FD_SET(nm_fds[ringid], &s_fd);
    retval = select(nm_fds[ringid]+1, &s_fd, NULL, NULL, NULL);
    if (retval == 0) {
        //timeout
        return false;
    } else if (retval < 0) {
        PERROR("select");
        return false;
    } else {
        return true;
    }

#endif
}

inline bool
netmap::txsync(int ringid)
{
    if (ioctl(nm_fds[ringid], NIOCTXSYNC, ringid) == -1) {
        PERROR("ioctl");
        return false;
    } else {
        return true;
    }
}

inline bool
netmap::txsync_block(int ringid)
{
#ifdef POLL

    int retval;
    struct pollfd x[1];
    x[0].fd = nm_fds[ringid];
    x[0].events = POLLOUT;
    retval = poll(x, 1, -1);
    if (retval == 0) {
        // timeout
        return false;
    } else if (retval < 0) {
        PERROR("poll");
        return false;
    } else {
        return true;
    }

#else 

    int retval;
    fd_set s_fd;
    FD_ZERO(&s_fd);
    FD_SET(nm_fds[ringid], &s_fd);
    retval = select(nm_fds[ringid]+1, NULL, &s_fd, NULL, NULL);
    if (retval == 0) {
        //timeout
        return false;
    } else if (retval < 0) {
        PERROR("select");
        return false;
    } else {
        return true;
    }

#endif
}

inline uint32_t
netmap::get_cursor(struct netmap_ring* ring)
{
    return ring->cur;
}

inline struct netmap_slot*
netmap::get_slot(struct netmap_ring* ring)
{
    return &ring->slot[ring->cur];
}

inline void
netmap::next(struct netmap_ring* ring)
{
    ring->cur = NETMAP_RING_NEXT(ring, ring->cur);
    return;
}

inline struct ether_header*
netmap::get_eth(struct netmap_ring* ring)
{
    struct netmap_slot* slot = get_slot(ring);
    return (struct ether_header*)NETMAP_BUF(ring, slot->buf_idx);
}

inline size_t
netmap::get_ethlen(struct netmap_ring* ring)
{
    struct netmap_slot* slot = get_slot(ring);
    return slot->len;
}


inline void
netmap::set_ethlen(struct netmap_ring* ring, size_t size)
{
    struct netmap_slot* slot = get_slot(ring);
    slot->len = size;
}

inline struct ether_addr*
netmap::get_mac()
{
    return &nm_mac;
}

inline int
netmap::get_fd_sw()
{
    return nm_fd_soft;
}

inline char*
netmap::get_mem()
{
    return nm_mem_addr_soft;
}

inline struct netmap_ring*
netmap::get_rx_ring_sw()
{
    return nm_rx_ring_soft;
}

inline struct netmap_ring*
netmap::get_tx_ring_sw()
{
    return nm_tx_ring_soft;
}

inline int
netmap::get_fd(int ringid)
{
    if (ringid > 0 && ringid < nm_rx_qnum) {
        return 0;
    }
    return nm_fds[ringid];
}

inline char*
netmap::get_mem(int ringid)
{
    if (ringid > 0 && ringid < nm_rx_qnum) {
        return NULL;
    }
    return nm_mem_addrs[ringid];
}

inline struct netmap_ring*
netmap::get_tx_ring(int ringid)
{
    if (ringid > 0 && ringid < nm_tx_qnum) {
        return NULL;
    }
    return nm_tx_rings[ringid];
}

inline struct netmap_ring*
netmap::get_rx_ring(int ringid)
{
    if (ringid > 0 && ringid < nm_rx_qnum) {
        return NULL;
    }
    return nm_rx_rings[ringid];
}


void
netmap::dump_nmr()
{
    printf("-----\n");
    printf("nr_name     : %s\n", nm_nmr.nr_name);
    printf("nr_varsion  : %d\n", nm_nmr.nr_version);
    printf("nr_offset   : %d\n", nm_nmr.nr_offset);
    printf("nr_memsize  : %d\n", nm_nmr.nr_memsize);
    printf("nr_tx_slots : %d\n", nm_nmr.nr_tx_slots);
    printf("nr_rx_slots : %d\n", nm_nmr.nr_rx_slots);
    printf("nr_tx_rings : %d\n", nm_nmr.nr_tx_rings);
    printf("nr_rx_rings : %d\n", nm_nmr.nr_rx_rings);
    printf("nr_ringid   : %d\n", nm_nmr.nr_ringid);
    printf("nr_cmd      : %d\n", nm_nmr.nr_cmd);
    printf("nr_arg1     : %d\n", nm_nmr.nr_arg1);
    printf("nr_arg2     : %d\n", nm_nmr.nr_arg2);
    printf("nr_spare2[0]: %x\n", nm_nmr.spare2[0]);
    printf("nr_spare2[1]: %x\n", nm_nmr.spare2[1]);
    printf("nr_spare2[2]: %x\n", nm_nmr.spare2[2]);
    printf("-----\n");
    return;
}

char*
netmap::get_ifname()
{
    return nm_ifname;
}

uint16_t
netmap::get_tx_qnum()
{
    return nm_tx_qnum;
}

uint16_t
netmap::get_rx_qnum()
{
    return nm_rx_qnum;
}

bool
netmap::set_promisc()
{
    int fd;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    char* ifname = get_ifname();

    strncpy(ifr.ifr_name, ifname, strlen(ifname));
    if (ioctl(fd, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
        PERROR("ioctl");
        MESG("failed to get interface status");
        close(fd);
        return false;
    }

    //printf("%04x%04x\n", ifr.ifr_flagshigh, ifr.ifr_flags & 0xffff);

    int flags = (ifr.ifr_flagshigh << 16) | (ifr.ifr_flags & 0xffff);

    flags |= IFF_PPROMISC;
    ifr.ifr_flags = flags & 0xffff;
    ifr.ifr_flagshigh = flags >> 16;

    //printf("%04x%04x\n", ifr.ifr_flagshigh, ifr.ifr_flags & 0xffff);

    if (ioctl(fd, SIOCSIFFLAGS, (caddr_t)&ifr) < 0) {
        PERROR("ioctl");
        MESG("failed to set interface to promisc");
        close(fd);
        return false;
    }
    close(fd);

    return true;
}

bool
netmap::unset_promisc()
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    char* ifname = get_ifname();
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, strlen(ifname));
    if (ioctl (fd, SIOCGIFFLAGS, &ifr) != 0) {
        PERROR("ioctl");
        MESG("failed to get interface status");
        close(fd);
        return false;
    }
    
    ifr.ifr_flags &= ~IFF_PROMISC;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) != 0) {
        PERROR("ioctl");
        MESG("failed to set interface to promisc");
        close(fd);
        return false;
    }
    close(fd);
    
    return true;
}

bool netmap::_create_nmring(int ringid, int swhw)
{
    // swhw : soft ring or hard ring
    //NETMAP_HW_RING   0x4000
    //NETMAP_SW_RING   0x2000
    //NETMAP_RING_MASK 0x0fff

    int fd;

    struct nmreq nmr;
    struct netmap_if* nmif;

    fd = open("/dev/netmap", O_RDWR);
    if (fd < 0) {
        perror("open");
        MESG("unable to open /dev/netmap");
        return false;
    }

    memset (&nmr, 0, sizeof(nmr));
    //printf("nm_ifname:%s\n", nm_ifname);
    strncpy (nmr.nr_name, nm_ifname, strlen(nm_ifname));
    nmr.nr_version = nm_version;
    nmr.nr_ringid = (swhw | ringid);

    if (ioctl(fd, NIOCREGIF, &nmr) < 0) {
        perror("ioctl");
        MESG("unable to register interface %s", nm_ifname);
        close(fd);
        return false;
    }

    char* mem = (char*)mmap(NULL, nmr.nr_memsize,
            PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        MESG("unable to mmap");
        close(fd);
        return false;
    }

    nmif = NETMAP_IF(mem, nmr.nr_offset);

    if (swhw == NETMAP_HW_RING) {
        nm_tx_rings[ringid] = NETMAP_TXRING(nmif, ringid);
        nm_rx_rings[ringid] = NETMAP_RXRING(nmif, ringid);
        nm_mem_addrs[ringid] = mem;
        nm_fds[ringid] = fd;
        return true;
    } else if (swhw == NETMAP_SW_RING) {
        nm_rx_ring_soft = NETMAP_RXRING(nmif, nm_rx_qnum);
        nm_tx_ring_soft = NETMAP_TXRING(nmif, nm_tx_qnum);
        nm_mem_addr_soft = mem;
        nm_fd_soft = fd;
        return true;
    } else {
        return false;
    }

    return false;
}

bool
netmap::_remove_hw_ring(int ringid)
{
    if (munmap(nm_mem_addrs[ringid], nm_memsize) != 0) {
        PERROR("munmap");
        return false;
    }
    nm_mem_addrs[ringid] = NULL;

    close(nm_fds[ringid]);
    nm_fds[ringid] = 0;
    nm_rx_rings[ringid] = NULL;
    nm_tx_rings[ringid] = NULL;
    return true;
}


bool
netmap::_remove_sw_ring()
{
    if (munmap(nm_mem_addr_soft, nm_memsize) != 0) {
        PERROR("munmap");
        return false;
    }
    nm_mem_addr_soft = NULL;
    close(nm_fd_soft);
    nm_fd_soft = 0;
    nm_rx_ring_soft = NULL;
    nm_tx_ring_soft = NULL;

    return true;
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#include "common.hpp"
#include "ether.hpp"
#include "netmap.hpp"

inline void
slot_swap(struct netmap_ring* rxring, struct netmap_ring* txring)
{
    struct netmap_slot* rx_slot =
        ((netmap_slot*)&rxring->slot[rxring->cur]);
    struct netmap_slot* tx_slot =
        ((netmap_slot*)&txring->slot[txring->cur]);

#ifdef USERLAND_COPY

    struct ether_header* rx_eth =
        (struct ether_header*)NETMAP_BUF(rxring, rx_slot->buf_idx);
    struct ether_header* tx_eth =
        (struct ether_header*)NETMAP_BUF(txring, tx_slot->buf_idx);
    memcpy(tx_eth, rx_eth, tx_slot->len);

#else

    uint32_t buf_idx;
    buf_idx = tx_slot->buf_idx;
    tx_slot->buf_idx = rx_slot->buf_idx;
    rx_slot->buf_idx = buf_idx;
    tx_slot->flags |= NS_BUF_CHANGED;
    rx_slot->flags |= NS_BUF_CHANGED;

#endif

    tx_slot->len = rx_slot->len;

    if (debug) { 
        uint8_t* tx_eth = (uint8_t*)NETMAP_BUF(txring, tx_slot->buf_idx);
        pktdump(tx_eth, tx_slot->len);
    }

    return;
}

int main(int argc, char** argv)
{
    debug = true;
    if (argc != 2) {
        if (argc != 2) {
            printf("%s [interface_name]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    netmap* nm;
    nm = new netmap();
    if (nm->open_if(argv[1]) == false) exit(1);

    //nm->dump_nmr();

    // max queue number
    int mq = nm->get_rx_qnum();
    struct pollfd pfd[mq];
    memset(pfd, 0, sizeof(pfd));

    for (int i = 0; i < mq; i++) {
        pfd[i].fd = nm->get_fd(i);
        pfd[i].events = POLLIN|POLLOUT;
    }

    
    printf("max_queue:%d\n", mq);
    pfd[mq].fd = nm->get_fd_sw();
    pfd[mq].events = POLLIN|POLLOUT;

    for (int i = 0; i < mq + 1; i++) {
        printf("%d:%d\n", i, pfd[i].fd);
    }

    int retval;
    int loop_count = 0;
    int rx_avail = 0;
    int tx_avail = 0;
    for (;;) {

        retval = poll(pfd, mq+1, -1);

        if (retval <= 0) {
            PERROR("poll error");
            return EXIT_FAILURE;
        }

        // nic -> host
        for (int i = 0; i < mq; i++) {

            if (pfd[i].revents & POLLERR) {

                MESG("rx_hard poll error");

            } else if (pfd[i].revents & POLLIN) {

                struct netmap_ring* rx = nm->get_rx_ring(i);
                struct netmap_ring* tx = nm->get_tx_ring_sw();
                rx_avail = nm->get_avail(rx);
                tx_avail = nm->get_avail(tx);

                while (rx_avail > 0) {

                    if (pfd[mq].revents & POLLOUT && tx_avail > 0) {
                        slot_swap(rx, tx);
                        nm->next(tx);
                        tx_avail--;
                        nm->next(rx);
                        rx_avail--;
                    } else {
                        break;
                    }

                }

            }

        }

        // nic -> host
        if (pfd[mq].revents & POLLERR) {

            MESG("rx_soft poll error");

        } else if (pfd[mq].revents & POLLIN) {

            int dest_ring = loop_count % mq;
            struct netmap_ring* rx = nm->get_rx_ring_sw();
            struct netmap_ring* tx = nm->get_tx_ring(dest_ring);
            rx_avail = nm->get_avail(rx);
            tx_avail = nm->get_avail(tx);

            while (rx_avail > 0) {

                if (pfd[dest_ring].revents & POLLOUT && tx_avail > 0) {
                    slot_swap(rx, tx);
                    nm->next(tx);
                    tx_avail--;
                    nm->next(rx);
                    rx_avail--;
                } else {
                    break;
                }

            }

        }

        /*
        for (int i = 0; i < mq + 1; i++) {
            pfd[i].revents = 0;
        }
        */

        loop_count++;
    }

    return EXIT_SUCCESS;
}




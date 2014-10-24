#define USE_NETMAP_API_11

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
    /*
    */

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
    struct pollfd pfd_rx[mq];
    struct pollfd pfd_tx[mq];
    memset(pfd_rx, 0, sizeof(pfd_rx));
    memset(pfd_tx, 0, sizeof(pfd_tx));

    for (int i = 0; i < mq; i++) {
        pfd_rx[i].fd = nm->get_fd(i);
        pfd_rx[i].events = POLLIN;
    }

    for (int i = 0; i < mq; i++) {
        pfd_tx[i].fd = nm->get_fd(i);
        pfd_tx[i].events = POLLOUT;
    }
    
    printf("max_queue:%d\n", mq);
    pfd_rx[mq].fd = nm->get_fd_sw();
    pfd_rx[mq].events = POLLIN;
    pfd_tx[mq].fd = nm->get_fd_sw();
    pfd_tx[mq].events = POLLOUT;

    for (int i = 0; i < mq + 1; i++) {
        printf("%d:%d\n", i, pfd_rx[i].fd);
    }

    int retval;
    int loop_count = 0;
    int rx_avail = 0;
    int tx_avail = 0;
    struct netmap_ring* rx = NULL;
    struct netmap_ring* tx = NULL;

    retval = poll(pfd_tx, mq+1, -1);
    for (;;) {

        retval = poll(pfd_rx, mq+1, -1);

        if (retval <= 0) {
            PERROR("poll error");
            return EXIT_FAILURE;
        }


        // nic -> host
        for (int i = 0; i < mq; i++) {

            if (pfd_rx[i].revents & POLLERR) {

                MESG("rx_hard poll error");

            } else if (pfd_rx[i].revents & POLLIN) {

                rx = nm->get_rx_ring(i);
                tx = nm->get_tx_ring_sw();
                rx_avail = nm->get_avail(rx);
                tx_avail = nm->get_avail(tx);

                while (rx_avail > 0) {
                    printf("nic->host:rx_avail:%d\n", rx_avail);
                    printf("nic->host:tx_avail:%d\n", tx_avail);
                    if (tx_avail > 0) {
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


        // host -> nic
        if (pfd_rx[mq].revents & POLLERR) {

            MESG("rx_soft poll error");

        } else if (pfd_rx[mq].revents & POLLIN) {

            int dest_ring = loop_count % mq;
            rx = nm->get_rx_ring_sw();
            tx = nm->get_tx_ring(dest_ring);
            rx_avail = nm->get_avail(rx);
            tx_avail = nm->get_avail(tx);

            while (rx_avail > 0) {
                printf("host->nic:rx_avail:%d\n", rx_avail);
                printf("host->nic:tx_avail:%d\n", tx_avail);
                if (tx_avail > 0) {
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

        for (int i = 0; i < mq + 1; i++) {
            pfd_rx[i].revents = 0;
            pfd_tx[i].revents = 0;
        }

        loop_count++;

#ifdef USE_NETMAP_API_11
        printf("%d\n", __LINE__);
        //retval = poll(pfd_tx, mq+1, -1);
        nm->txsync_hw_block(0);
        printf("%d\n", __LINE__);
        nm->txsync_sw_block();
        //nm->txsync_hw(0);
        //nm->txsync_sw();
        printf("hage\n");
#endif
    }

    return EXIT_SUCCESS;
}




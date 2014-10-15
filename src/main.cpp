#include <string.h>
#include <poll.h>
#include <unistd.h>

#include "common.hpp"
#include "ether.hpp"
#include "netmap.hpp"

int main(int argc, char** argv)
{
    debug = true;
    if (argc != 2) {
        if (argc != 2) {
            printf("%s [interface_name]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    /*
    struct ether_addr mac;
    get_mac_addr(argv[1], &mac);
    uint32_t oui;
    uint32_t bui;
    oui = mac.octet[0]<<16 | mac.octet[1]<<8 | mac.octet[2];
    bui = mac.octet[3]<<16 | mac.octet[4]<<8 | mac.octet[5];
    if (debug) printf("%s_mac_address->%06x:%06x\n", argv[1], oui, bui);
    */

    netmap* nm;
    nm = new netmap();
    if (nm->open_if(argv[1]) == false) exit(1);

    nm->dump_nmr();

    /*
    int tx_hard_fd;
    int rx_hard_fd;
    int tx_soft_fd;
    int rx_soft_fd;

    struct netmap_ring* tx_hard_ring = NULL;
    struct netmap_ring* rx_hard_ring = NULL;
    struct netmap_ring* tx_soft_ring = NULL;
    struct netmap_ring* rx_soft_ring = NULL;

    tx_hard_fd = nm->create_nmring_hard_tx(&tx_hard_ring, 0);
    rx_hard_fd = nm->create_nmring_hard_rx(&rx_hard_ring, 0);
    tx_soft_fd = nm->create_nmring_hard_tx(&tx_soft_ring);
    rx_soft_fd = nm->create_nmring_hard_rx(&rx_soft_ring);

    struct pollfd pfd[4];
    memset(pfd, 0, sizeof(pfd));

    pfd[0].fd = rx_hard_fd;
    pfd[1].fd = rx_soft_fd;
    pfd[2].fd = tx_hard_fd;
    pfd[4].fd = tx_soft_fd;

    pfd[0].events = POLLIN;
    pfd[1].events = POLLIN;
    pfd[2].events = POLLOUT;
    pfd[3].events = POLLOUT;

    int retval;
    for (;;) {

        pfd[0].revents = 0;
        pfd[1].revents = 0;
        pfd[2].revents = 0;
        pfd[3].revents = 0;
        retval = poll(pfd, 4, -1);

        if (retval <= 0) {
            PERROR("poll error");
            return EXIT_FAILURE;
        }

        if (pfd[0].revents & POLLERR) {

            MESG("rx_hard poll error");

        } else if (pfd[0].revents & POLLIN) {

            if (pfd[3].revents & POLLERR) {

                MESG("tx_soft poll error");

            } else if (pfd[3].revents & POLLOUT) {
                while () {
                }
            }

        }

        if (pfd[1].revents & POLLERR) {

            MESG("rx_soft poll error");

        } else if (pfd[1].revents & POLLIN) {

            if (pfd[2].revents & POLLERR) {

                MESG("tx_hard poll error");

            } else if (pfd[2].revents & POLLOUT) {
                ;
            }

        }

    }
    */

    return EXIT_SUCCESS;
}

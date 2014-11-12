#define USE_NETMAP_API_11

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <unistd.h>

#include "common.hpp"
#include "ether.hpp"
#include "switch.hpp"
#include "property.hpp"

inline void
slot_swap(struct netmap_ring* rxring, struct netmap_ring* txring)
{
    struct netmap_slot* rx_slot =
        ((netmap_slot*)&rxring->slot[rxring->cur]);
    struct netmap_slot* tx_slot =
        ((netmap_slot*)&txring->slot[txring->cur]);

#ifdef DEEPCOPY

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

    /*
    if (debug) { 
        uint8_t* tx_eth = (uint8_t*)NETMAP_BUF(txring, tx_slot->buf_idx);
        pktdump(tx_eth, tx_slot->len);
    }
    */

    return;
}

void
usage(char* prog_name)
{
    printf("%s\n", prog_name);
    printf("  Must..\n");
    printf("    -f [configuration file]\n");
    printf("  Option..\n");
    printf("    -h/? : help usage information\n");
#ifdef DEBUG
    printf("    -v : verbose mode\n");
#endif
    printf("\n");
    return;
}

int main(int argc, char** argv)
{
    debug = false;
    int opt;
    int option_index;
    std::string opt_f;

    struct option long_options[] = {
        {"config", no_argument,  NULL, 'f'},
        {"help",   no_argument,  NULL, 'h'},
#ifdef DEBUG
        {"verbose", no_argument, NULL, 'v'},
#endif
        {0, 0, 0, 0},
    };

    while ((opt = getopt_long(argc, argv,
                "f:hv?", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'f':
            opt_f = optarg;
            break;

        case 'h':
        case '?':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;

#ifdef DEBUG
        case 'v':
            debug = true;
            break;
#endif

        default:
            exit(EXIT_FAILURE);
        }
    }

    if (opt_f.size() == 0) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    property prop;
    prop.read(opt_f);
    std::string left = prop.get("left");
    std::string right = prop.get("right");

    if (left.size() == 0 || right.size() == 0) {
        MESG("can't find the left/right Network Interface in config");
        exit(EXIT_FAILURE);
    } else {
        if (debug) {
            std::cout << "L-NIC: " << left << std::endl;
            std::cout << "R-NIC: " << right << std::endl;
        }
    }

    /*
    netmap* nm;
    nm = new netmap();
    if (nm->open_if(argv[1]) == false) exit(1);
    }
    */

    return EXIT_SUCCESS;
}




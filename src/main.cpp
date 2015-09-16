#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <getopt.h>

#include "common.hpp"

#include "lbif.hpp"
#include "ether.hpp"
#include "property.hpp"

void usage(char* progname)
{
    printf("%s\n", progname);
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
        {"config",  no_argument, NULL, 'f'},
        {"help",    no_argument, NULL, 'h'},
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

    std::string ifname = prop.get("lb_interface");
    if (ifname.size() == 0) {
        usage(argv[0]);
        MESG("To set \"lb_interface=[interface_name] in conf file.\"");
        exit(EXIT_FAILURE);
    }

    std::string lbip = prop.get("lb_ipaddress");
    if (lbip.size() == 0) {
        usage(argv[0]);
        MESG("To set \"lb_ipaddress=xxx.xxx.xxx.xxx in conf file.\"");
        exit(EXIT_FAILURE);
    }

    try {
        class lbif* lbif = new class lbif(ifname, lbip);
        lbif->run();
    } catch (...) {
        MESG("cant be followed exception.");
    }

    for (;;) {
        sleep(1);
    }

    return EXIT_SUCCESS;
}

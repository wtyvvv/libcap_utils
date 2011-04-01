#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "caputils/caputils.h"
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netdb.h>

static int packet_flag = 0;

#define STPBRIDGES 0x0026
#define CDPVTP 0x016E

void show_usage(){
  printf("capinfo  caputils-" CAPUTILS_VERSION "\n");
  printf("(c) 2011 David Sveningsson\n\n");
  printf("Open a capstream and show information about it.\n");
  printf("Usage: capinfo [OPTIONS] FILENAME..\n\n");
  printf("      --packets              Show how many packets it contain.\n");
  printf("  -h, --help                 Show this help.\n");
}

int show_info(const char* filename){
  struct stream* st;
  long ret = 0;

  if ( (ret=openstream(&st, filename, 0, NULL, 0)) != 0 ){
    fprintf(stderr, "%s: %s\n", filename, caputils_error_string(ret));
    return ret;
  }

  printf("%s: caputils %d.%d stream\n", filename, st->FH.version.major, st->FH.version.minor);
  printf("     mpid: %s\n", st->FH.mpid[0] != 0 ? st->FH.mpid : "(unset)");
  printf("  comment: %s\n", st->comment ? st->comment : "(unset)");

  struct cap_header* cp;
  long int packets = 0;
  long arp = 0;
  long stp = 0;
  long cdpvtp = 0;
  long other = 0;
  long ipproto[UINT8_MAX] = {0,}; /* protocol is defined as 1 octet */

  while ( (ret=read_post(st, (char**)&cp, NULL)) == 0 ){
    packets++;

    struct ethhdr* eth = (struct ethhdr*)cp->payload;
    struct iphdr* ip = NULL;
    uint16_t h_proto = ntohs(eth->h_proto);
    
    switch ( h_proto ){
    case ETHERTYPE_VLAN:
      ip = (struct iphdr*)(cp->payload + sizeof(struct ether_vlan_header));
      /* fallthrough */

    case ETHERTYPE_IP:
      if ( !ip ){
	ip = (struct iphdr*)(cp->payload + sizeof(struct ethhdr));
      }

      ipproto[ip->protocol]++;
      break;

    case ETHERTYPE_ARP:
      arp++;
      break;

    case STPBRIDGES:
      stp++;
      break;

    case CDPVTP:
      cdpvtp++;
      break;

    default:
      other++;
      break;
    }
  }
  
  if ( ret > 0 ){
    fprintf(stderr, "read_post() returned 0x%08lx: %s\n", ret, caputils_error_string(ret));
  }
  
  printf("  packets: %ld\n", packets);
  
  if ( packet_flag ){
    long ipother = 0;
    printf("    IP: ");
    for ( int i = 0; i < UINT8_MAX; i++ ){
      if ( ipproto[i] == 0 ){
	continue;
      }

      struct protoent* protoent = getprotobynumber(i);

      if ( !protoent ){
	ipother += ipproto[i];
	continue;
      }

      printf("%s(%ld) ", protoent->p_name, ipproto[i]);
    }
    if ( ipother > 0 ){
      printf("other(%ld)", other);
    }
    printf("\n");
    if ( arp > 0 ){
      printf("    ARP: %ld\n", arp);
    }
    if ( stp > 0 ){
      printf("    stpbridges: %ld\n", stp);
    }
    if ( cdpvtp > 0 ){
      printf("    cdpvtp: %ld\n", cdpvtp);
    }
    if ( other > 0 ){
      printf("    other: %ld\n", other);
    }
  }

  closestream(st);
  
  return 0;
}

int main(int argc, char* argv[]){
  /* no arguments */
  if ( argc == 1 ){
    show_usage();
    return 0;
  }

  /* parse arguments */
  while (1){
    static struct option long_options[] = {
      {"packets", 0, &packet_flag,   1},
      {"help",    0,            0, 'h'}
    };

    int option_index = 0;
    
    int c = getopt_long(argc, argv, "h", long_options, &option_index);

    if ( c == -1 ){
      break;
    }

    switch (c){
    case 'h':
      show_usage();
      return 0;
    }
  }

  /* no targets */
  if ( optind == argc ){
    show_usage();
    return 0;
  }

  /* visit all targets */
  while ( optind < argc ){
    show_info(argv[optind++]);
    if ( optind < argc ){
      putchar('\n');
    }
  }

  return 0;
}

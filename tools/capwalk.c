/**
 * libcap_utils - DPMI capture utilities
 * Copyright (C) 2003-2013 (see AUTHORS)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "caputils/caputils.h"
#include "caputils/stream.h"
#include "caputils/filter.h"
#include "caputils/packet.h"
#include "caputils/utils.h"
#include "caputils/marker.h"
#include "caputils/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>

static int keep_running = 1;
static unsigned int flags = FORMAT_REL_TIMESTAMP;
static unsigned int max_packets = 0;
static unsigned int max_matched_packets = 0;
static const char* iface = NULL;
static struct timeval timeout = {1,0};
static const char* program_name = NULL;
static const struct stream_stat* stream_stat = NULL;

void handle_sigint(int signum){
	if ( keep_running == 0 ){
		fprintf(stderr, "\rGot SIGINT again, terminating.\n");
		abort();
	}
	fprintf(stderr, "\rAborting capture.\n");
	keep_running = 0;
}

static const char* shortopts = "p:c:i:t:dDar1234xHh";
static struct option longopts[]= {
	{"packets",  required_argument, 0, 'p'},
	{"count",    required_argument, 0, 'c'},
	{"iface",    required_argument, 0, 'i'},
	{"timeout",  required_argument, 0, 't'},
	{"calender", no_argument,       0, 'd'},
	{"localtime",no_argument,       0, 'D'},
	{"absolute", no_argument,       0, 'a'},
	{"relative", no_argument,       0, 'r'},
	{"hexdump",  no_argument,       0, 'x'},
	{"headers",  no_argument,       0, 'H'},
	{"help",     no_argument,       0, 'h'},
	{0, 0, 0, 0} /* sentinel */
};

static void show_usage(void){
	printf("%s-%s\n", program_name, caputils_version(NULL));
	printf("(C) 2004 Patrik Arlos <patrik.arlos@bth.se>\n");
	printf("(C) 2012 David Sveningsson <david.sveningsson@bth.se>\n");
	printf("Usage: %s [OPTIONS] STREAM\n", program_name);
	printf("  -i, --iface          For ethernet-based streams, this is the interface to listen\n"
	       "                       on. For other streams it is ignored.\n"
	       "  -p, --packets=N      Stop after N read packets.\n"
	       "  -c, --count=N        Stop after N matched packets.\n"
	       "                       If both -p and -c is used, what ever happens first will stop.\n"
	       "  -t, --timeout=N      Wait for N ms while buffer fills [default: 1000ms].\n"
	       "  -h, --help           This text.\n"
	       "\n"
	       "Formatting options:\n"
	       "  -1                   Show only DPMI information.\n"
	       "  -2                     .. include link layer.\n"
	       "  -3                     .. include transport layer.\n"
	       "  -4                     .. include application layer. [default]\n"
	       "  -H, --headers        Show layer headers.\n"
	       "  -x, --hexdump        Write full package content as hexdump.\n"
	       "  -d, --calender       Show timestamps in human-readable format (UTC).\n"
	       "  -D, --localtime      Show timestamps in human-readable format (local time).\n"
	       "  -a, --absolute       Show absolute timestamps.\n"
	       "  -r, --relative       Show timestamps relative to first packet. [default]\n"
	       "\n");
	filter_from_argv_usage();
}

static const char* ucfirst(const char* str){
	static char buf[4096];
	if ( !str || str[0] == 0 ) return str;
	if ( str[1] != 0 ){
		snprintf(buf, sizeof(buf), "%c%s", toupper(str[0]), &str[1]);
	} else {
		snprintf(buf, sizeof(buf), "%c", toupper(str[0]));
	}
	return buf;
}

static int handle_packet(const stream_t st, const cap_head* cp){
	printf("Packet %ld\n", stream_stat->read);
	printf("  DPMI\n"
	       "    MAMPid:             %.8s\n"
	       "    Iface:              %.8s\n"
	       "    Timestamp:          %s\n"
	       "    Length:             %d\n"
	       "    Capture length:     %d\n"
	       , cp->mampid, cp->nic, timepico_to_string(&cp->ts, "%Y-%m-%d %H:%M:%S +00:00")
	       , cp->len, cp->caplen);

	struct header_chunk header;
	header_init(&header, cp, 0);
	while ( header_walk(&header) ){
		if ( !header.protocol ){
			printf("Unknown protocol\n");
			continue;
		}

		printf("  %s @ %ld\n", ucfirst(header.protocol->name), header.ptr - cp->payload);
		header_dump(stdout, &header, "    ");
	};

	return 0;
}

int main(int argc, char **argv){
	/* extract program name from path. e.g. /path/to/MArCd -> MArCd */
	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
		program_name = separator + 1;
	} else {
		program_name = argv[0];
	}

	struct filter filter;
	if ( filter_from_argv(&argc, argv, &filter) != 0 ){
		return 0; /* error already shown */
	}

	int op, option_index = -1;
	while ( (op = getopt_long(argc, argv, shortopts, longopts, &option_index)) != -1 ){
		switch (op){
		case 0:   /* long opt */
		case '?': /* unknown opt */
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		{
			const unsigned int mask = (7<<FORMAT_LAYER_BIT);
			flags &= ~mask; /* reset all layer bits */
			flags |= (op-'0')<<FORMAT_LAYER_BIT;
			break;
		}

		case 'd': /* --calender */
			flags |= FORMAT_DATE_STR | FORMAT_DATE_UTC;
			break;

		case 'D': /* --localtime */
			flags |= FORMAT_DATE_STR | FORMAT_DATE_LOCALTIME;
			break;

		case 'a': /* --absolute */
			flags &= ~FORMAT_REL_TIMESTAMP;
			break;

		case 'r': /* --relative */
			flags |= FORMAT_REL_TIMESTAMP;
			break;

		case 'H': /* --headers */
			flags |= FORMAT_HEADER;
			break;

		case 'p': /* --packets */
			max_packets = atoi(optarg);
			break;

		case 'c': /* --packets */
			max_matched_packets = atoi(optarg);
			break;

		case 't': /* --timeout */
		{
			int tmp = atoi(optarg);
			timeout.tv_sec  = tmp / 1000;
			timeout.tv_usec = tmp % 1000 * 1000;
		}
		break;

		case 'x': /* --hexdump */
			flags |= FORMAT_HEXDUMP;
			break;

		case 'i': /* --iface */
			iface = optarg;
			break;

		case 'h': /* --help */
			show_usage();
			return 0;

		default:
			fprintf (stderr, "%s: argument '-%c' declared but not handled\n", argv[0], op);
		}
	}

	int ret;

	/* Open stream(s) */
	struct stream* stream;
	if ( (ret=stream_from_getopt(&stream, argv, optind, argc, iface, "-", program_name, 0)) != 0 ) {
		return ret; /* Error already shown */
	}

	/* handle C-c */
	signal(SIGINT, handle_sigint);

	/* read packets */
	stream_stat = stream_get_stat(stream);
	while ( keep_running && stream_read_cb(stream, handle_packet, &filter, &timeout) == 0 );

	/* Release resources */
	stream_close(stream);
	filter_close(&filter);

	return 0;
}

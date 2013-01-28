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
#endif

#include "format.h"

void print_udp(FILE* fp, const struct cap_header* cp, net_t net, const struct udphdr* udp, unsigned int flags){
	fputs("UDP", fp);
	if ( limited_caplen(cp, udp, sizeof(struct udphdr)) ){
		fprintf(fp, " [Packet size limited during capture]");
		return;
	}

	const size_t header_size = sizeof(struct udphdr);
	const size_t total_size = ntohs(udp->len);
	const size_t payload_size = total_size - header_size;
	if ( flags & FORMAT_HEADER ){
		fprintf(fp, "(HDR[%zd]DATA[%zd])", header_size, payload_size);
	}

	const uint16_t sport = ntohs(udp->source);
	const uint16_t dport = ntohs(udp->dest);

	fprintf(fp, ": %s:%d --> %s:%d",
	        net->net_src, sport,
	        net->net_dst, dport);

	const char* payload = (const char*)udp + header_size;
	if ( sport == PORT_DNS || dport == PORT_DNS ){
		print_dns(fp, cp, payload, payload_size, flags);
	}
}

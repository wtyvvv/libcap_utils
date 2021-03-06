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

#ifndef CAPUTILS_CAPTURE_H
#define CAPUTILS_CAPTURE_H

#include <caputils/picotime.h>
#include <caputils/file.h>

#include <stdint.h>
#include <netinet/if_ether.h>

#ifdef CAPUTILS_EXPORT
#pragma GCC visibility push(default)
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ether_vlan_header{
	uint8_t  ether_dhost[ETH_ALEN];       /* destination eth addr */
	uint8_t  ether_shost[ETH_ALEN];       /* source ether addr    */
	uint16_t vlan_proto;                  /* vlan is present if field begins with 0x8100 */
	uint16_t vlan_tci;                    /* vlan is present if field begins with 0x8100 */
	uint16_t h_proto;                     /* Ethernet payload protocol */
};

// Capture Header. This header is attached to each packet that we keep, i.e. it matched a filter.
#define CAPHEAD_NICLEN 8
struct cap_header {
	char nic[CAPHEAD_NICLEN];             // Identifies the CI where the frame was caught
	char mampid[8];                       // Identifies the MP where the frame was caught
	timepico ts;                          // Identifies when the frame was caught
	uint32_t len;                         // Identifies the lenght of the frame
	uint32_t caplen;                      // Identifies how much of the frame that we find here

	/* convenience accessor (since array size is 0 it won't affect sizeof) */
	union {
		char payload[0];
		struct ethhdr ethhdr[0];
		struct ether_vlan_header ethvlanhdr[0];
	};
} __attribute((packed));
typedef struct cap_header cap_head;
typedef struct cap_header* caphead_t;

#ifdef CAPUTILS_EXPORT
#pragma GCC visibility pop
#endif

#ifdef __cplusplus
}
#endif

#endif /* CAPUTILS_CAPTURE_H */

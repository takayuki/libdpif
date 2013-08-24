/*
 * Copyright (c) 2013 Takayuki Usui
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "odp.h"
#include "utils.h"

static struct port_head ports[OVS_VPORT_TYPE_MAX];
static int dp_ifindex;
static int nl_mmap;
static int flood_mode = 3;

__attribute__((constructor))
void hub_init()
{
	int i;

	for (i = 0; i < OVS_VPORT_TYPE_MAX; i++)
		LIST_INIT(&ports[i]);
}

__attribute__((destructor))
void hub_exit(void)
{
	int i;
	struct port *port;

	for (i = 0; i < OVS_VPORT_TYPE_MAX; i++) {
		while (!LIST_EMPTY(&ports[i])) {
			port = LIST_FIRST(&ports[i]);
			LIST_REMOVE(port, next);
			if (port != 0)
				free(port);
		}
	}
}

static int flood(struct nl *nl, struct buffer *buf, void *arg,
		 struct nl_parser *ignore)
{
	struct packet *packet = packet_downcast(nl);
	struct packet_req req = {
		.dp_ifindex = dp_ifindex,
		.packet = packet,
		.ports = ports,
	};

	switch (ntohs(packet->key.key_ethertype)) {
	case 0x0806: /* ARP */
		if (flood_mode >= 1) {
			if (packet_exec(packet, &req, packet_flood) < 0)
				goto err;
		}
		break;
	default:
		if (flood_mode >= 2) {
			if (packet_exec(packet, &req, packet_flood) < 0)
				goto err;
		}
	}

	if (flood_mode >= 3) {
		struct odp *odp = container_of(packet, struct odp, packet);
		struct flow *flow = &odp->flow;
		struct flow_req req = {
			.dp_ifindex = dp_ifindex,
			.packet = packet,
			.ports = ports,
		};

		if (flow_exec(flow, &req, flow_flood) < 0)
			goto err;
	}
	return 0;
err:
	return -1;
}

static int dispatch(struct nl *nl, struct buffer *buf, void *arg,
		    struct nl_parser *ignore)
{
	struct dp *dp = dp_downcast(nl);
	struct odp *odp = container_of(dp, struct odp, dp);
	struct ovs *ovs = &dp->ovs;
	__u16 nlmsg_type;
	struct nl_parser inner[2] = {};

	assert(ignore->parse == 0);

	nlmsg_type = nl->nlh->nlmsg_type;

	if (nlmsg_type == ovs->ovs_datapath_family) {
		inner[0].parse = dp_parse;
		nl = dp_cast(&odp->dp);
	} else if (nlmsg_type == ovs->ovs_vport_family) {
		inner[0].parse = vport_parse;
		nl = vport_cast(&odp->vport);
	} else if (nlmsg_type == ovs->ovs_flow_family) {
		inner[0].parse = flow_parse;
		nl = flow_cast(&odp->flow);
	} else if (nlmsg_type == ovs->ovs_packet_family) {
		inner[0].parse = packet_parse;
		inner[1].parse = flood;
		nl = packet_cast(&odp->packet);
	} else {
		return -1;
	}

	return ovs_parse(nl, buf, arg, inner);
}

static void help(int status)
{
	fprintf(stderr, "Usage: hub [-M] [-F <mode>] [-d name[,addr=<cidr>]] [-i name[,addr=<cidr>]] [-n name] [-g name,src=<addr>,dst=<addr>] [-G name,src=<addr>,dst=<addr>] [-L list,src=<addr>,dst=<addr>,port=<port>]\n");
	exit(status);
}

static void alloc_port(char *options, __u32 type)
{
	struct port *port;

	port = malloc(sizeof(struct port));
	if (!port) {
		perror("malloc");
		exit(1);
	}

	if (port_options(port, options, type) < 0)
		help(1);

	LIST_INSERT_HEAD(&ports[type], port, next);
}

int main(int argc, char *argv[])
{
	struct odp odp;
	struct nl_parser parser = { .parse = dispatch };
	char *datapath = "dp0";
	__u32  type;
	int ch, status;

	while ((ch = getopt(argc,argv,"MF:d:i:n:g:G:L:h")) != -1) {
		type = OVS_VPORT_TYPE_UNSPEC;
		switch (ch) {
		case 'M':
			nl_mmap = 1;
			break;
		case 'F':
			flood_mode = atoi(optarg);
			break;
		case 'd':
			datapath = optarg;
			break;
		case 'i':
			type = OVS_VPORT_TYPE_INTERNAL;
			break;
		case 'n':
			type = OVS_VPORT_TYPE_NETDEV;
			break;
		case 'g':
			type = OVS_VPORT_TYPE_GRE;
			break;
		case 'G':
			type = OVS_VPORT_TYPE_GRE64;
			break;
		case 'L':
			type = OVS_VPORT_TYPE_LISP;
			break;
		case 'h':
			help(0);
		default:
			help(1);
		}

		if (type != OVS_VPORT_TYPE_UNSPEC)
			alloc_port(optarg, type);
	}
	argc -= optind;
	argv += optind;

	if (argc >= 1)
		help(1);

	alloc_port(datapath, OVS_VPORT_TYPE_INTERNAL);

	dp_ifindex = odp_new(&odp, &ports[0], nl_mmap);
	if (dp_ifindex < 0)
		exit(1);

	status = odp_loop(dp_cast(&odp.dp), &parser);
	odp_free(&odp);
	return status;
}

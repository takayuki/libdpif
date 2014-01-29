/*
 * Copyright 2013 Takayuki Usui
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "odp.h"
#include "timer.h"
#include "utils.h"

static struct odp odp;
static struct port_head ports[__OVS_VPORT_TYPE_MAX+1];
static int dp_ifindex;
static int use_mmap;
static int flood_mode = 2;
static int timer_interval = 0;

__attribute__((constructor))
void hub_init()
{
	int i;

	for (i = 0; i < __OVS_VPORT_TYPE_MAX; i++)
		LIST_INIT(&ports[i]);
}

__attribute__((destructor))
void hub_exit(void)
{
	int i;
	struct port *port;

	for (i = 0; i < __OVS_VPORT_TYPE_MAX; i++) {
		LIST_FOREACH(port, &ports[i], next)
			free(port);
	}
}

static void compact_ports()
{
	int i, j;

	for (i = OVS_VPORT_TYPE_UNSPEC + 1, j = 0;
	     i < __OVS_VPORT_TYPE_MAX; i++) {
		if (!LIST_EMPTY(&ports[i])) {
			ports[j] = ports[i];
			ports[i] = (struct port_head){};
			/* XXX update link from head */
			ports[j].lh_first->next.le_prev = &ports[j].lh_first;
			j++;
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

	switch (ntohs(packet->ovs.family.packet.key.key_ethertype)) {
	case 0x0806: /* ARP */
		if (flood_mode >= 1) {
			if (packet_run(packet, &req, packet_flood) < 0)
				goto err;
		}
		break;
	default:
		if (flood_mode >= 2) {
			if (packet_run(packet, &req, packet_flood) < 0)
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
		if (flood_mode >= 4)
			if (flow_exec(flow, &req, flow_delete) < 0)
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
	fprintf(stderr, "Usage: hub [-Mv] [-F <mode>] [-d name[,addr=<cidr>][,mac=<lladdr>]] [-i name[,addr=<cidr>][,mac=<lladdr>]] [-n name] [-g name,src=<addr>,dst=<addr>] [-G name,src=<addr>,dst=<addr>] [-L name,src=<addr>,dst=<addr>,port=<port>] [-V name,src=<addr>,dst=<addr>,port=<port>]\n");
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

static void timer_handler(int signum)
{
	static struct nl_stats rx_stats = {};
	static struct nl_stats tx_stats = {};
	struct nl_stats rx_delta = {};
	struct nl_stats tx_delta = {};
	static struct timeval last = {};
	long elapsed;

	elapsed = timer_sub(&last);
	if (elapsed <= 0)
		return;

	nl_stats_delta(&odp.dp.ovs.genl.nl.rx_stats, &rx_stats, &rx_delta);
	nl_stats_delta(&odp.packet.ovs.genl.nl.tx_stats, &tx_stats, &tx_delta);

	info("Rx: %.2lf Kfps,%.2lf Mbps Tx: %.2lf Kfps,%.2lf Mbps\t\t\t\t\r",
	     timer_stat(rx_delta.packets, elapsed, 1024),
	     timer_stat(rx_delta.bytes, elapsed, 1024 * 1024),
	     timer_stat(tx_delta.packets, elapsed, 1024),
	     timer_stat(tx_delta.bytes, elapsed, 1024 * 1024));
}

int main(int argc, char *argv[])
{
	struct nl_parser parser = { .parse = dispatch };
	char *datapath = "dp0";
	__u32  type;
	int ch, status;

	while ((ch = getopt(argc,argv,"MF:d:i:n:g:G:L:V:t:vh")) != -1) {
		type = OVS_VPORT_TYPE_UNSPEC;
		switch (ch) {
		case 'M':
			use_mmap = 1;
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
		case 'V':
			type = OVS_VPORT_TYPE_VXLAN;
			break;
		case 't':
			timer_interval = atoi(optarg);
			break;
		case 'v':
			debug_level += 1;
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

	compact_ports();

	dp_ifindex = odp_new(&odp, &ports[0], use_mmap);
	if (dp_ifindex < 0)
		exit(1);

	if (timer_interval > 0)
		timer(timer_handler, timer_interval);

	status = odp_loop(dp_cast(&odp.dp), &parser);
	odp_free(&odp);
	return status;
}

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
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "nl.h"
#include "ovs.h"
#include "odp.h"
#include "utils.h"

struct odp *odp_init(struct odp *odp, int nl_mmap)
{
	struct nl_mmap_req *small = nl_mmap ? &nl_small_map : 0;
	struct nl_mmap_req *medium = nl_mmap ? &nl_medium_map : 0;
	struct nl_mmap_req *large = nl_mmap ? &nl_large_map : 0;


	if (!dp_init(&odp->dp, large))
		goto err_dp;

	if (!vport_init(&odp->vport, small))
		goto err_vport;

	if (!flow_init(&odp->flow, medium))
		goto err_flow;

	if (!packet_init(&odp->packet, small))
		goto err_packet;

	if (!rtnl_init(&odp->rtnl, 0))
		goto err_rtnl;

	return odp;

err_rtnl:
	packet_free(&odp->packet);
err_packet:
	flow_free(&odp->flow);
err_flow:
	vport_free(&odp->vport);
err_vport:
	dp_free(&odp->dp);
err_dp:
	return 0;
}

void odp_free(struct odp *odp)
{

	rtnl_free(&odp->rtnl);

	packet_free(&odp->packet);

	flow_free(&odp->flow);

	vport_free(&odp->vport);

	dp_free(&odp->dp);
}

static int
internal_port(struct odp *odp, int dp_ifindex, struct port_head *ports)
{
	struct vport_req req = {
		.dp_ifindex = dp_ifindex,
		.vport_upcall_pid = dp_cast(&odp->dp)->local.nl_pid,
	};
	struct port *port;
	int i = 0;

	LIST_FOREACH(port, ports, next) {
		if (i++ == 0) {
			port->port_no = 0;
			continue;
		}

		req.vport_name = port->port_name;
		req.vport_type = port->port_type;
		if (vport_exec(&odp->vport, &req, vport_cmd_new) < 0) {
			info("unable to add port %s\n", req.vport_name);
			goto err;
		}

		port->port_no = odp->vport.ovs.family.vport.vport_no;
	}
	return 0;
err:
	return -1;
}

static int
netdev_port(struct odp *odp, int dp_ifindex, struct port_head *ports)
{
	struct vport_req req = {
		.dp_ifindex = dp_ifindex,
		.vport_upcall_pid = dp_cast(&odp->dp)->local.nl_pid,
	};
	struct port *port;

	LIST_FOREACH(port, ports, next) {
		req.vport_name = port->port_name;
		req.vport_type = port->port_type;

		if (vport_exec(&odp->vport, &req, vport_cmd_new) < 0){
			info("unable to add port %s\n", req.vport_name);
			goto err;
		}
		port->port_no = odp->vport.ovs.family.vport.vport_no;;
	}
	return 0;
err:
	return -1;
}

static int
tunnel_port(struct odp *odp, int dp_ifindex, struct port_head *ports)
{
	struct vport_req req = {
		.dp_ifindex = dp_ifindex,
		.vport_upcall_pid = dp_cast(&odp->dp)->local.nl_pid,
	};
	struct port *port = LIST_FIRST(ports);

	switch (port->port_type) {
	case OVS_VPORT_TYPE_GRE:
		req.vport_name = "GRE";
		break;
	case OVS_VPORT_TYPE_VXLAN:
		req.vport_name = "VXLAN";
		break;
	case OVS_VPORT_TYPE_GRE64:
		req.vport_name = "GRE64";
		break;
	case OVS_VPORT_TYPE_LISP:
		req.opt.tun.dst_port = port->opt.tun.dst_port;
		req.vport_name = "LISP";
		break;
	default:
		req.vport_name = "unknown";
		break;
	}
	req.vport_type = port->port_type;

	if (vport_exec(&odp->vport, &req, vport_cmd_new) < 0) {
		info("unable to add port %s\n", req.vport_name);
		goto err;
	}

	LIST_FOREACH(port, ports, next)
		port->port_no = odp->vport.ovs.family.vport.vport_no;

	return 0;
err:
	return -1;
}

static int link_addr(struct rtnl *rtnl, struct port *port)
{
	struct rtnl_addr_req req = {
		.addr	 = port->opt.link.addr,
		.ifindex = port->opt.link.ifindex,
	};

	return rtnl_exec(rtnl, &req, rtnl_addr_add);
}

static int link_up(struct rtnl *rtnl, struct port *port)
{
	struct rtnl_link_req req = {
		.addr	 = port->opt.link.mac,
		.change	 = IFF_UP,
		.flags	 = IFF_UP,
		.ifindex = port->opt.link.ifindex,
	};

	return rtnl_exec(rtnl, &req, rtnl_link_set);
}

static int port_parse(struct nl *nl, struct buffer *buf, void *arg,
		      struct nl_parser *inner)
{
	struct rtnl *rtnl = rtnl_downcast(nl);
	struct port_head *ports = arg;
	struct port *port;
	int ifindex;
	char *ifname;

	ifindex = rtnl->hdr.ifm->ifi_index;
	if (!rtnl->hdr.ifm->ifi_index)
		return 0;

	ifname = (char*)(rtnl->attrs[IFLA_IFNAME] + 1);

	LIST_FOREACH(port, ports, next) {
		if (!strcmp(port->port_name, ifname)) {
			port->opt.link.ifindex = ifindex;
			break;
		}
	}
	return 0;
}

static int odp_link(struct odp *odp, struct port_head *ports)
{
	struct rtnl *rtnl = &odp->rtnl;
	struct nl *nl = rtnl_cast(rtnl);
	struct buffer buf;
	struct memory mem;
	struct nl_parser inner[3] = {
		{ .parse = rtnl_parse, },
		{ .parse = port_parse, .arg = ports },
	};
	struct port *port;

	if (!nl_frame_init(&buf, &mem, &nl->tx_ring))
		return -1;

	rtnl_build(rtnl, &buf, 0, rtnl_link_list);
	if (nl_send(nl, &buf, inner) <= 0)
		goto err;

	if (nl_dispatch(nl, inner) < 0)
		goto err;

	buffer_release(&buf);
	assert(mem.refcnt == 0);

	LIST_FOREACH(port, ports, next) {
		if (port->opt.link.addr) {
			if (link_addr(rtnl, port) < 0) {
				info("unable to add address %s to %s",
				     port->opt.link.addr,
				     port->port_name);
				goto err;
			}
			if (link_up(rtnl, port) < 0) {
				info("unable to set link %s up",
				     port->port_name);
				goto err;
			}
		}
	}
	return 0;
err:
	buffer_release(&buf);
	assert(mem.refcnt == 0);
	return -1;
}

int odp_new(struct odp *odp, struct port_head *ports, int nl_mmap)
{
	struct dp_req req = {};
	struct dp *dp;
	struct port *port = 0;
	int dp_ifindex, i;

	if (!odp_init(odp, nl_mmap))
		return -1;

	dp = &odp->dp;

	for (i = 0; i < OVS_VPORT_TYPE_MAX; i++) {

		if (LIST_EMPTY(&ports[i]))
			return -1;

		port = LIST_FIRST(&ports[i]);
		if (port->port_type == OVS_VPORT_TYPE_INTERNAL)
			break;
	}

	assert((port && port->port_type == OVS_VPORT_TYPE_INTERNAL));

	req.dp_name = port->port_name;
	req.dp_upcall_pid = dp_cast(&odp->dp)->local.nl_pid;

	if (dp_exec(dp, &req, dp_cmd_get) == 0 &&
	    dp_exec(dp, &req, dp_cmd_del) < 0) {
		goto err;
	}

	if (dp_exec(dp, &req, dp_cmd_new) < 0 ||
	    dp_exec(dp, &req, dp_cmd_get) < 0) {
		info("unable to add datapath  %s", port->port_name);
		goto err;
	}

	dp_ifindex = dp->ovs.ovsh->dp_ifindex;

	for (i = 0; i < OVS_VPORT_TYPE_MAX; i++) {
		if (LIST_EMPTY(&ports[i]))
			break;

		port = LIST_FIRST(&ports[i]);

		switch (port->port_type) {
		case OVS_VPORT_TYPE_INTERNAL:
			if (internal_port(odp, dp_ifindex, &ports[i]) < 0)
				goto err;
			if (odp_link(odp, &ports[i]) < 0)
				goto err;
			break;
		case OVS_VPORT_TYPE_NETDEV:
			if (netdev_port(odp, dp_ifindex, &ports[i]) < 0)
				goto err;
			break;
		case OVS_VPORT_TYPE_GRE:
		case OVS_VPORT_TYPE_GRE64:
		case OVS_VPORT_TYPE_LISP:
			if (tunnel_port(odp, dp_ifindex, &ports[i]) < 0)
				goto err;
			break;
		default:
			goto err;
		}
	}

	for (i = 0; i < OVS_VPORT_TYPE_MAX; i++) {
		if (LIST_EMPTY(&ports[i]))
			break;

		LIST_FOREACH(port, &ports[i], next) {
			if (port->opt.link.ifindex)
				info("port #%u: %s, ifindex=%d, addr=%s,%s\n",
				     port->port_no,
				     port->port_name,
				     port->opt.link.ifindex,
				     port->opt.link.addr,
				     port->opt.link.mac);
			else
				info("port #%d: %s\n",
				     port->port_no,
				     port->port_name);

		}
	}

	return dp_ifindex;

err:
	odp_free(odp);
	return -1;
}

int odp_loop(struct nl *nl, struct nl_parser *dispatch)
{
	struct buffer buf;
	struct memory mem;
	struct nl_parser inner[3] = {
		{ .parse = genl_parse, },
		{ .parse = dispatch->parse, },
	};
	struct timeval last, now, diff;
	double elapsed;
	unsigned int rx_packets = 0, rx_bytes = 0;
	int quit = 0, ret = 0;

	if (gettimeofday(&last, 0) < 0) {
		perror("gettimeofday");
		return -1;
	}

	while (!quit) {
		nl_frame_init(&buf, &mem, &nl->rx_ring);

		ret = nl_recv(nl, &buf);
		if (ret == 0) {
			quit = !nl->nl_mmap;
			goto next;
		} else if (ret < 0) {
			quit = 1;
			goto next;
		}

		rx_bytes += ret;
		rx_packets += 1;
		while (!nl_parse(nl, &buf, inner))
			;

		assert(buffer_remaining(&buf) == 0);

		if (rx_bytes < ODP_STAT_BYTES_INTERVAL)
			goto next;

		if (gettimeofday(&now, 0) < 0) {
			perror("gettimeofday");
			quit = 1;
			goto next;
		}

		timersub(&now, &last, &diff);
		elapsed = diff.tv_sec * 1000000 + diff.tv_usec;

		info("%.3lf Kfps, %.3lf Mbps\t\t\t\t\r",
		     rx_packets / elapsed * 1000000 / 1024,
		     rx_bytes / elapsed * 1000000 / (1024 * 1024));

		rx_packets = rx_bytes = 0;
		last = now;
	next:
		nl_frame_release(&buf);
	}

	return ret;
}

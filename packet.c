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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <openvswitch.h>
#include "action.h"
#include "ovs.h"
#include "packet.h"
#include "nla.h"

struct packet *packet_init(struct packet *packet, struct nl_mmap_req *req)
{
	memset(packet, 0, offsetof(struct packet, ovs));

	if (!ovs_init(&packet->ovs, req))
		return 0;

	return packet;
}

void packet_free(struct packet *packet)
{
	ovs_free(&packet->ovs);
}

struct nl *packet_cast(struct packet *packet)
{
	return &packet->ovs.genl.nl;
}

struct packet* packet_downcast(struct nl *nl)
{
	return container_of(ovs_downcast(nl), struct packet, ovs);
}


int packet_parse(struct nl *nl, struct buffer *buf, void *arg,
		 struct nl_parser *inner)
{
	struct packet *p = packet_downcast(nl);
	struct nlattr* nla;
	struct buffer slice;
	int ret = -1;

	memset(p, 0, offsetof(struct packet, ovs));

	while (!nla_parse(buf, &nla, 0)) {
		switch (nla->nla_type) {
		case OVS_PACKET_ATTR_UNSPEC:
			ret = nla_discard(buf, nla);
			break;
		case OVS_PACKET_ATTR_PACKET:
			*(char**)&p->packet_frame.data = buffer_data(buf);
			p->packet_frame.len = nla->nla_len - NLA_HDRLEN;
			ret = nla_discard(buf, nla);
			break;
		case OVS_PACKET_ATTR_KEY:
			*(char**)&p->packet_key.data = buffer_data(buf);
			p->packet_key.len = nla->nla_len - NLA_HDRLEN;
			nla_slice(buf, &slice, nla);
			ret = key_parse(&slice, &p->key, 1);
			nla_slice_end(buf, &slice);
			break;
		default:
			ret = nla_discard(buf, nla);
		}

		if (ret < 0)
			goto err;
	}

	if (inner && inner->parse)
		ret = inner->parse(nl, buf, inner->arg, inner + 1);
	else
		ret = 0;

err:
	return ret;
}

int packet_build(struct packet *packet, struct buffer *buf, void *arg,
		 packet_builder_t build)
{
	return nl_frame_build(buf->memory, build(packet, buf, arg));
}

int packet_exec(struct packet *packet, void *arg, packet_builder_t build)
{
	struct nl *nl = packet_cast(packet);
	struct buffer buf;
	struct memory mem;
	static struct nl_parser inner[4] = {
		{ .parse = genl_parse, },
		{ .parse = ovs_parse, },
		{ .parse = packet_parse, },
	};
	int ret = -1;

	nl_frame_init(&buf, &mem, &nl->tx_ring);
	packet_build(packet, &buf, arg, build);
	ret = nl_send(nl, &buf, inner);
	if (ret <= 0)
		goto err;
	ret = nl_dispatch(nl, inner);
err:
	buffer_release(&buf);
	assert(mem.refcnt == 0);
	return ret;
}

#define _PACKET_BUILDER(builder, flag, cmd)				\
int packet_##builder(struct packet *p, struct buffer *buf, void *arg) 	\
									\
{									\
	return genl_builder(builder, p->ovs.ovs_packet_family,		\
			    flag, cmd, OVS_PACKET_VERSION,		\
			    &p->ovs.genl, buf, arg);			\
}

static int flood(struct buffer *buf, void *arg)
{
	struct packet_req *req = arg;
	struct ovs_header ovsh = {
		.dp_ifindex = req->dp_ifindex,
	};
	struct packet *p = req->packet;
	__u32 in_port = p->key.key_in_port;
	struct port *out;
	struct nlattr *nest[1];
	int i;

	ovs_put_header(buf, &ovsh);

	nla_put_data(buf, p->packet_key.data, p->packet_key.len,
		     OVS_PACKET_ATTR_KEY);

	nla_nest_begin(buf,&nest[0],OVS_PACKET_ATTR_ACTIONS);

	for (i = 0; i < OVS_VPORT_TYPE_MAX; i++) {
		LIST_FOREACH(out, &req->ports[i], next) {

			if (out->port_no == in_port)
				continue;

			if (out->port_type == OVS_VPORT_TYPE_GRE ||
			    out->port_type == OVS_VPORT_TYPE_GRE64 ||
			    out->port_type == OVS_VPORT_TYPE_VXLAN ||
			    out->port_type == OVS_VPORT_TYPE_LISP) {
				action_output_tunnel(buf, &req->ports[i]);
				break;
			}

			action_output(buf, out->port_no);
		}
	}

	nla_nest_end(buf, nest[0]);

	nla_put_data(buf, p->packet_frame.data, p->packet_frame.len,
		     OVS_PACKET_ATTR_PACKET);
	return 0;
}
_PACKET_BUILDER(flood, NLM_F_ACK, OVS_PACKET_CMD_EXECUTE)

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
	struct ovs_packet_family *p = &ovs_downcast(nl)->family.packet;
	struct nlattr* nla;
	struct buffer slice;
	int ret = -1;

	memset(packet_downcast(nl), 0, offsetof(struct packet, ovs));

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

int packet_run(struct packet *packet, void *arg, packet_builder_t build)
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
	struct ovs_packet_family *p = &req->packet->ovs.family.packet;
	__u32 in_port = p->key.key_in_port;
	struct port *out;
	struct nlattr *nest[1];
	int i;

	ovs_put_header(buf, &ovsh);

	nla_put_data(buf, p->packet_key.data, p->packet_key.len,
		     OVS_PACKET_ATTR_KEY);

	nla_nest_begin(buf,&nest[0],OVS_PACKET_ATTR_ACTIONS);

	for (i = 0; i < __OVS_VPORT_TYPE_MAX; i++) {
		if (LIST_EMPTY(&req->ports[i]))
			break;

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
_PACKET_BUILDER(flood, 0, OVS_PACKET_CMD_EXECUTE)

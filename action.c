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
#include "action.h"
#include "nla.h"

static int
key_parse_member(struct buffer *buf, struct key *key, struct nlattr *nla,
		 int level)
{
	struct buffer slice;
	int ret = 0;

	switch (nla->nla_type) {
	case OVS_KEY_ATTR_PRIORITY:
		return nla_discard(buf, nla);
	case OVS_KEY_ATTR_IN_PORT:
		return nla_get_u32(buf, nla, &key->key_in_port);
	case OVS_KEY_ATTR_ETHERNET:
		return nla_get_data(buf, nla, (void**)&key->key_ethernet);
	case OVS_KEY_ATTR_VLAN:
		return nla_get_be16(buf, nla, &key->key_vlan);
	case OVS_KEY_ATTR_ETHERTYPE:
		return nla_get_be16(buf, nla, &key->key_ethertype);
	case OVS_KEY_ATTR_IPV4:
		return nla_get_data(buf, nla, (void**)&key->key_ipv4);
	case OVS_KEY_ATTR_IPV6:
		return nla_get_data(buf, nla, (void**)&key->key_ipv6);
	case OVS_KEY_ATTR_TCP:
		return nla_get_data(buf, nla, (void**)&key->key_tcp);
	case OVS_KEY_ATTR_UDP:
		return nla_get_data(buf, nla, (void**)&key->key_udp);
	case OVS_KEY_ATTR_ICMP:
		return nla_get_data(buf, nla, (void**)&key->key_icmp);
	case OVS_KEY_ATTR_ICMPV6:
		return nla_get_data(buf, nla, (void**)&key->key_icmpv6);
	case OVS_KEY_ATTR_ARP:
		return nla_get_data(buf, nla, (void**)&key->key_arp);
	case OVS_KEY_ATTR_ND:
		return nla_discard(buf, nla);
	case OVS_KEY_ATTR_SKB_MARK:
		return nla_get_u32(buf, nla, &key->key_skb_mark);
	case OVS_KEY_ATTR_TUNNEL:
		nla_slice(buf, &slice, nla);
		ret = key_parse(&slice, key, level + 1);
		nla_slice_end(buf, &slice);
		return ret;
	default:
		return nla_discard(buf, nla);
	}
}

int key_parse(struct buffer *buf, struct key *key, int level)
{
	struct nlattr *nla;
	struct buffer slice;
	int ret = 0;

	while (!nla_parse(buf, &nla, level)) {
		switch (nla->nla_type) {
		case OVS_KEY_ATTR_UNSPEC:
			ret = nla_discard(buf, nla);
			break;
		case OVS_KEY_ATTR_ENCAP:
			nla_slice(buf, &slice, nla);
			ret = key_parse(&slice, key, level + 1);
			nla_slice_end(buf, &slice);
			break;
		default:
			ret = key_parse_member(buf, key, nla, level);
		}
        }
	return ret;
}

int action_parse(struct buffer *buf, int level)
{
	struct nlattr *nla;

	while (!nla_parse(buf, &nla, level))
		nla_discard(buf, nla);

	return 0;
}

int action_output(struct buffer *buf, __u32 port_no)
{
	nla_put_u32(buf, port_no, OVS_ACTION_ATTR_OUTPUT);
	return 0;
}

int action_output_tunnel(struct buffer *buf, struct port_head *ports)
{
	struct port *port;
	struct nlattr *nest[2];

	LIST_FOREACH(port, ports, next) {
		nla_nest_begin(buf, &nest[0],OVS_ACTION_ATTR_SET);
		nla_nest_begin(buf, &nest[1], OVS_KEY_ATTR_TUNNEL);
		nla_put_be32(buf, port->key.src_ipv4,
			     OVS_TUNNEL_KEY_ATTR_IPV4_SRC);
		nla_put_be32(buf, port->key.dst_ipv4,
			     OVS_TUNNEL_KEY_ATTR_IPV4_DST);
		nla_put_u8(buf, 64, OVS_TUNNEL_KEY_ATTR_TTL);
		nla_nest_end(buf, nest[1]);
		nla_nest_end(buf, nest[0]);
		action_output(buf, port->port_no);
	}
	return 0;
}

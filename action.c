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
#include <endian.h>
#include "action.h"
#include "nla.h"
#include "utils.h"

static int
key_parse_tunnel_attr(struct buffer *buf, struct key *key, struct nlattr *nla,
		      int level)
{
	int ret = 0;

	while (!nla_parse(buf, &nla, level)) {
		switch (nla->nla_type) {
		case OVS_TUNNEL_KEY_ATTR_ID:
			ret = nla_get_be64(buf, nla, &key->tun_key.id);
		default:
			ret = nla_discard(buf, nla);
		}
	}
	return ret;
}

static int
key_parse_attr(struct buffer *buf, struct key *key, struct nlattr *nla,
	       int level)
{
	struct buffer slice;
	int ret = 0;

	switch (nla->nla_type) {
	case OVS_KEY_ATTR_PRIORITY:
		return nla_discard(buf, nla);
	case OVS_KEY_ATTR_IN_PORT:
		return nla_get_u32(buf, nla, &key->key_in_port);
	case OVS_KEY_ATTR_FRAG_MAX_SIZE:
		return nla_get_u16(buf, nla, &key->key_frag_max_size);
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
		ret = key_parse_tunnel_attr(&slice, key, nla, level +1);
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
			ret = key_parse_attr(buf, key, nla, level);
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
		nla_put_be64(buf, htobe64(port->opt.tun.id),
			     OVS_TUNNEL_KEY_ATTR_ID);
		nla_put_be32(buf, ip4_addr(port->opt.tun.src_ipv4),
			     OVS_TUNNEL_KEY_ATTR_IPV4_SRC);
		nla_put_be32(buf, ip4_addr(port->opt.tun.dst_ipv4),
			     OVS_TUNNEL_KEY_ATTR_IPV4_DST);
		nla_put_u8(buf, 64, OVS_TUNNEL_KEY_ATTR_TTL);
		nla_nest_end(buf, nest[1]);
		nla_nest_end(buf, nest[0]);
		action_output(buf, port->port_no);
	}
	return 0;
}

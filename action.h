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
#ifndef _ACTION_H
#define _ACTION_H

#include <openvswitch.h>
#include "buffer.h"
#include "port.h"

struct key {
	__u32			 key_in_port;
	struct ovs_key_ethernet *key_ethernet;
	__be16			 key_vlan;
	__be16			 key_ethertype;
	struct ovs_key_ipv4	*key_ipv4;
	struct ovs_key_ipv6	*key_ipv6;
	struct ovs_key_tcp	*key_tcp;
	struct ovs_key_udp	*key_udp;
	struct ovs_key_icmp	*key_icmp;
	struct ovs_key_icmpv6	*key_icmpv6;
	struct ovs_key_arp	*key_arp;
	__u32			 key_skb_mark;
};

int key_parse(struct buffer *, struct key *, int);
int action_parse(struct buffer *, int);
int action_output(struct buffer *, __u32);
int action_output_tunnel(struct buffer *, struct port_head *);

#endif

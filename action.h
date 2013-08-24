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

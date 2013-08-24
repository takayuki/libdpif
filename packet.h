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
#ifndef _PACKET_H
#define _PACKET_H

#include <linux/types.h>
#include "action.h"
#include "ovs.h"
#include "port.h"

struct opaque_data {
	char	*data;
	int	 len;
};

struct packet {
	struct opaque_data	packet_frame;
	struct opaque_data	packet_key;
	struct key		key;

	struct ovs		ovs;
};

struct packet_req {
	struct packet		*packet;
	struct port_head	*ports;

	int			 dp_ifindex;
};

typedef int (*packet_builder_t)(struct packet *, struct buffer *, void *);

struct packet* packet_init(struct packet *, struct nl_mmap_req *);
void packet_free(struct packet *);
struct nl *packet_cast(struct packet *);
struct packet* packet_downcast(struct nl *);
int packet_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int packet_build(struct packet *, struct buffer *, void *, packet_builder_t);
int packet_exec(struct packet *, void *, packet_builder_t);

#define _PACKET_BUILDER_DECL(builder)			\
int packet_##builder(struct packet *, struct buffer *, void *);

_PACKET_BUILDER_DECL(flood)

#endif

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
#ifndef _FLOW_H
#define _FLOW_H

#include "action.h"
#include "ovs.h"
#include "packet.h"

struct flow {
	struct opaque_data	flow_key;

	struct key		key;

	struct ovs		ovs;
};

struct flow_req {
	struct packet		*packet;
	struct port_head	*ports;

	int			 dp_ifindex;
};

typedef int (*flow_builder_t)(struct flow *, struct buffer *, void *);

struct flow *flow_init(struct flow *, struct nl_mmap_req *);
void flow_free(struct flow *);
struct nl *flow_cast(struct flow *);
struct flow *flow_downcast(struct nl *);
int flow_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int flow_build(struct flow *, struct buffer *, void *, flow_builder_t);
int flow_exec(struct flow *, void *, flow_builder_t);

#define _FLOW_BUILDER_DECL(builder)			\
int flow_##builder(struct flow *, struct buffer *, void *);

_FLOW_BUILDER_DECL(flood)

#endif

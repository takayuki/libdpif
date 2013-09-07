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

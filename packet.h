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

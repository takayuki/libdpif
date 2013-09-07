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
#ifndef _ODP_H
#define _ODP_H

#include <openvswitch.h>
#include "dp.h"
#include "flow.h"
#include "ovs.h"
#include "packet.h"
#include "rtnl.h"
#include "vport.h"

#define ODP_STAT_BYTES_INTERVAL		(1024 * 1024 * 100)

struct odp {
	struct dp	dp;
	struct vport	vport;
	struct flow	flow;
	struct packet	packet;
	struct rtnl     rtnl;
};

struct odp *odp_init(struct odp *, int);
void odp_free(struct odp *);
struct odp *odp_downcast(struct nl *);
int odp_new(struct odp *, struct port_head *, int);
int odp_loop(struct nl *, struct nl_parser *);

#endif

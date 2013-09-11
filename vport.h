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
#ifndef _VPORT_H
#define _VPORT_H

#include <linux/types.h>
#include <openvswitch.h>
#include "buffer.h"
#include "dp.h"
#include "ovs.h"

struct vport {
	struct ovs		 ovs;
};

#define DP_MAX_PORTS USHRT_MAX

struct vport_req {
	int			 dp_ifindex;

	char			*vport_name;
	__u32			 vport_no;
	__u32			 vport_type;
	__u32			 vport_upcall_pid;
	union {
		struct {
			__u16	 dst_port;
		} tun;
	} opt;
};

typedef int (*vport_builder_t)(struct vport *, struct buffer *, void *);

struct vport* vport_init(struct vport *, struct nl_mmap_req *);
void vport_free(struct vport *);
struct nl *vport_cast(struct vport *);
struct vport *vport_downcast(struct nl *);
int vport_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int vport_build(struct vport *, struct buffer *, void *, vport_builder_t);
int vport_exec(struct vport *, void *, vport_builder_t);

#define _VPORT_BUILDER_DECL(builder)				\
int vport_##builder(struct vport *, struct buffer *, void *);

_VPORT_BUILDER_DECL(cmd_new)
_VPORT_BUILDER_DECL(cmd_del)
_VPORT_BUILDER_DECL(cmd_list)

#endif

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
#ifndef _OVS_H
#define _OVS_H

#include <linux/types.h>
#include "action.h"
#include "nlctrl.h"

struct opaque_data {
	char	*data;
	int	 len;
};

struct ovs_datapath_family {
	char			*dp_name;
	__u32			 dp_upcall_pid;
	struct ovs_dp_stats	*dp_stats;
};

struct ovs_vport_family {
	char			*vport_name;
	__u32			 vport_no;
	__u32			 vport_type;
	__u32			 vport_upcall_pid;
	struct ovs_vport_stats	*vport_stats;
	union {
		struct {
			__u32	 flags;
			__be32	 dst_ipv4;
			__be32	 src_ipv4;
		} tun;
	} opt;
};

struct ovs_flow_family {
	struct opaque_data	flow_key;
	struct key		key;
};

struct ovs_packet_family {
	struct opaque_data	packet_frame;
	struct opaque_data	packet_key;
	struct key		key;
};

struct ovs {
	union {
		struct ovs_datapath_family	dp;
		struct ovs_vport_family		vport;
		struct ovs_flow_family		flow;
		struct ovs_packet_family	packet;
	} family;

	struct ovs_header	*ovsh;

	__u16			 ovs_datapath_family;
	__u16			 ovs_vport_family;
	__u16			 ovs_flow_family;
	__u16			 ovs_packet_family;

	__u32			 ovs_datapath_mcgroup;
	__u32			 ovs_vport_mcgroup;
	__u32			 ovs_flow_mcgroup;

	struct genl		 genl;
};

struct ovs *ovs_init(struct ovs *, struct nl_mmap_req *);
void ovs_free(struct ovs *);
struct nl *ovs_upcast(struct ovs *);
struct ovs *ovs_downcast(struct nl *);
int ovs_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int ovs_put_header(struct buffer *, void *);

#endif

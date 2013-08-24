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
#ifndef _VPORT_H
#define _VPORT_H

#include <linux/types.h>
#include <openvswitch.h>
#include "buffer.h"
#include "dp.h"
#include "ovs.h"

struct vport {
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
		} tunnel;
	} options;

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

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
#ifndef _DP_H
#define _DP_H

#include <linux/types.h>
#include <openvswitch.h>
#include "buffer.h"
#include "ovs.h"

struct dp {
	char			*dp_name;
	__u32			 dp_upcall_pid;
	struct ovs_dp_stats	*dp_stats;

	struct ovs		 ovs;
};

struct dp_req {
	char	*dp_name;
	__u32	 dp_upcall_pid;
};

typedef int (*dp_builder_t)(struct dp *, struct buffer *, void *);

struct dp *dp_init(struct dp *, struct nl_mmap_req *req);
void dp_free(struct dp *);
struct nl *dp_cast(struct dp *);
struct dp *dp_downcast(struct nl *);
int dp_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int dp_build(struct dp *, struct buffer *, void *, dp_builder_t);
int dp_exec(struct dp *, void *, dp_builder_t);

#define _DP_BUILDER_DECL(builder)			\
int dp_##builder(struct dp *, struct buffer *, void *);

_DP_BUILDER_DECL(cmd_new)
_DP_BUILDER_DECL(cmd_del)
_DP_BUILDER_DECL(cmd_get)
_DP_BUILDER_DECL(cmd_list)

#endif

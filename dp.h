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

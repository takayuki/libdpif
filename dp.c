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
#include <assert.h>
#include <string.h>
#include "dp.h"
#include "genlmsg.h"
#include "nla.h"
#include "nlmsg.h"
#include "ovs.h"

struct dp *dp_init(struct dp* dp, struct nl_mmap_req *req)
{
	memset(dp, 0, offsetof(struct dp, ovs));

	if (!ovs_init(&dp->ovs, req))
		return 0;

	return dp;
}

void dp_free(struct dp *dp)
{
	ovs_free(&dp->ovs);
}

struct nl *dp_cast(struct dp *dp)
{
	return &dp->ovs.genl.nl;
}

struct dp *dp_downcast(struct nl *nl)
{
	struct genl *genl = container_of(nl, struct genl, nl);
	struct ovs *ovs = container_of(genl, struct ovs, genl);

	return container_of(ovs, struct dp, ovs);
}

int dp_parse(struct nl *nl, struct buffer *buf, void *arg,
	     struct nl_parser *inner)
{
	struct dp *dp = dp_downcast(nl);
	struct nlattr* nla;
	int ret = 0;

	memset(dp, 0, offsetof(struct dp, ovs));

	while (!ret && !nla_parse(buf, &nla,0)) {
		switch (nla->nla_type) {
		case OVS_DP_ATTR_UNSPEC:
			ret = nla_discard(buf, nla);
			break;
		case OVS_DP_ATTR_NAME:
			ret = nla_get_str(buf, nla, &dp->dp_name);
			break;
		case OVS_DP_ATTR_UPCALL_PID:
			ret = nla_get_u32(buf, nla, &dp->dp_upcall_pid);
			break;
		case OVS_DP_ATTR_STATS:
			ret = nla_get_data(buf, nla, (void**)&dp->dp_stats);
			break;
		default:
			ret = nla_discard(buf, nla);
		}
	}

	if (!ret && inner && inner->parse)
		ret = inner->parse(nl, buf, inner->arg, inner + 1);

	return ret;
}

int dp_build(struct dp *dp, struct buffer *buf, void *arg, dp_builder_t build)
{
	return nl_frame_build(buf->memory, build(dp, buf, arg));

}

int dp_exec(struct dp *dp, void *arg, dp_builder_t build)
{
	struct nl *nl = dp_cast(dp);
	struct buffer buf;
	struct memory mem;
	static struct nl_parser inner[4] = {
		{ .parse = genl_parse, },
		{ .parse = ovs_parse, },
		{ .parse = dp_parse, },
	};
	int ret = -1;

	nl_frame_init(&buf, &mem, &nl->tx_ring);
	dp_build(dp, &buf, arg, build);
	ret = nl_send(nl, &buf, inner);
	if (ret <= 0)
		goto err;
	ret = nl_dispatch(nl, inner);
err:
	buffer_release(&buf);
	assert(mem.refcnt == 0);
	return ret;
}

#define _DP_BUILDER(builder, flag, cmd)					\
int dp_##builder(struct dp *dp, struct buffer *buf, void *arg)		\
{									\
	return genl_builder(builder,					\
			    dp->ovs.ovs_datapath_family, flag,		\
			    cmd, OVS_DATAPATH_VERSION,			\
			    &dp->ovs.genl, buf, arg);			\
}

static int cmd_new(struct buffer *buf, void *arg)
{
	struct ovs_header ovsh = {};
	struct dp_req *req = arg;

	ovs_put_header(buf, &ovsh);
	nla_put_str(buf, req->dp_name, OVS_DP_ATTR_NAME);
	nla_put_u32(buf, req->dp_upcall_pid, OVS_DP_ATTR_UPCALL_PID);
	return 0;
}
_DP_BUILDER(cmd_new, NLM_F_ACK, OVS_DP_CMD_NEW)

static int cmd_del(struct buffer *buf, void *arg)
{
	struct ovs_header ovsh = {};
	struct dp_req *req = arg;

	ovs_put_header(buf, &ovsh);
	nla_put_str(buf, req->dp_name, OVS_DP_ATTR_NAME);
	return 0;
}
_DP_BUILDER(cmd_del, NLM_F_ACK, OVS_DP_CMD_DEL)

static int cmd_get(struct buffer *buf, void *arg)
{
	struct ovs_header ovsh = {};
	struct dp_req *req = arg;

	ovs_put_header(buf, &ovsh);
	nla_put_str(buf, req->dp_name, OVS_DP_ATTR_NAME);
	return 0;
}
_DP_BUILDER(cmd_get, 0, OVS_DP_CMD_GET)

static int cmd_list(struct buffer *buf, void *arg)
{
	struct ovs_header ovsh = {};

	return ovs_put_header(buf, &ovsh);
}
_DP_BUILDER(cmd_list, NLM_F_DUMP, OVS_DP_CMD_GET)

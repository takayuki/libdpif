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
#include <string.h>
#include "genlmsg.h"
#include "nla.h"
#include "nlctrl.h"
#include "nlmsg.h"

struct nlctrl *
nlctrl_init(struct nlctrl *nlctrl, struct nl_mmap_req *req)
{
	memset(nlctrl, 0, offsetof(struct nlctrl, genl));

	if (!genl_init(&nlctrl->genl, req))
		return 0;

	return nlctrl;
}

void nlctrl_free(struct nlctrl *nlctrl)
{
	genl_free(&nlctrl->genl);
}

static int parse_mcast_group(struct buffer *buf, struct nlctrl *nlctrl)
{
	struct nlattr *nla;
	int ret = -1;

	while (!nla_parse(buf, &nla, 2)) {
		switch (nla->nla_type) {
		case CTRL_ATTR_MCAST_GRP_NAME:
			ret = nla_get_str(buf, nla, &nlctrl->mcgroup_name);
			break;
		case CTRL_ATTR_MCAST_GRP_ID:
			ret = nla_get_u32(buf, nla, &nlctrl->mcgroup_id);
			break;
		default:
			ret = nla_discard(buf, nla);
		}
	}
	return ret;
}

static int parse_mcast_groups(struct buffer *buf, struct nlctrl *nlctrl)
{
	struct nlattr *nla;
	int ret = -1;

	while (!nla_parse(buf, &nla, 1)) {
		struct buffer slice;

		nla_slice(buf, &slice, nla);
		ret = parse_mcast_group(&slice, nlctrl);
		nla_slice_end(buf, &slice);
	}

	return ret;
}

static int parse_new_family(struct buffer *buf, struct nlctrl *nlctrl)
{
	struct nlattr *nla;
	int ret = -1;

	while (!nla_parse(buf, &nla, 0)) {
		switch (nla->nla_type) {
		case CTRL_ATTR_FAMILY_NAME:
			ret = nla_get_str(buf, nla, &nlctrl->family_name);
			break;
		case CTRL_ATTR_FAMILY_ID:
			ret = nla_get_u16(buf, nla, &nlctrl->family_id);
			break;
		case CTRL_ATTR_MCAST_GROUPS: {
			struct buffer slice;

			nla_slice(buf, &slice, nla);
			ret = parse_mcast_groups(&slice, nlctrl);
			nla_slice_end(buf, &slice);
			break;
		}
		default:
			nla_discard(buf, nla);
		}
	}

	return ret;
}

static int parse_get_family(struct buffer *buf, struct nlctrl *nlctrl)
{
	struct nlattr *nla;
	int ret = -1;

	while (!nla_parse(buf, &nla, 0)) {
		if (nla->nla_type == CTRL_ATTR_FAMILY_NAME)
			ret = nla_get_str(buf, nla, &nlctrl->family_name);
		else
			ret = nla_discard(buf, nla);

	}
	return ret;
}

static inline struct nlctrl *downcast(struct nl *nl)
{
	struct genl *genl = container_of(nl, struct genl, nl);

	return container_of(genl, struct nlctrl, genl);
}

int nlctrl_parse(struct nl *nl, struct buffer *buf, void *arg,
		   struct nl_parser *inner)
{
	struct nlctrl *ctrl = downcast(nl);

	if (nl->nlh->nlmsg_type != GENL_ID_CTRL)
		goto err;

	if (ctrl->genl.genlh->cmd == CTRL_CMD_NEWFAMILY)
		return parse_new_family(buf, ctrl);
	else if (ctrl->genl.genlh->cmd == CTRL_CMD_GETFAMILY)
		return parse_get_family(buf, ctrl);
err:
	return -1;
}


#define _NLCTRL_BUILDER(builder, type, flags, cmd, version)		\
int nlctrl_##builder(struct genl *genl, struct buffer *buf, void *arg)	\
{									\
	struct nlctrl *ctrl = downcast(&genl->nl);			\
									\
	return genl_builder(builder, type, flags, cmd, version,		\
			    &ctrl->genl, buf, arg);			\
}

static int get_family(struct buffer *buf, void *arg)
{
	nla_put_str(buf, (char*)arg, CTRL_ATTR_FAMILY_NAME);
	return 0;

}
_NLCTRL_BUILDER(get_family, GENL_ID_CTRL, NLM_F_REQUEST,
		CTRL_CMD_GETFAMILY, 2)

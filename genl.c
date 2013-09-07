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
#include <string.h>
#include "genl.h"
#include "genlmsg.h"
#include "nlmsg.h"

struct genl* genl_init(struct genl *genl, struct nl_mmap_req *req)
{
	memset(genl, 0, offsetof(struct genl, nl));

	if (!nl_init(&genl->nl, NETLINK_GENERIC, req))
		return 0;

	return genl;
}

void genl_free(struct genl *genl)
{
	nl_free(&genl->nl);
}


struct genl *genl_downcast(struct nl *nl)
{
	return container_of(nl, struct genl, nl);
}

int genl_parse(struct nl *nl, struct buffer *buf, void *arg,
	       struct nl_parser *inner)
{
	struct genl *genl = genl_downcast(nl);
	int ret = -1;

	memset(genl, 0, offsetof(struct genl, nl));

	if (genlmsg_parse(buf, &genl->genlh) < 0)
		goto err;

	if (inner && inner->parse)
		ret = inner->parse(nl, buf, inner->arg, inner + 1);
	else
		ret = 0;

err:
	return ret;
}

int genl_build(struct genl *genl, struct buffer *buf, void *arg,
	       int (*build)(struct genl *, struct buffer *, void *))
{
	return nl_frame_build(buf->memory, build(genl, buf, arg));
}

int genl_builder(int (*builder)(struct buffer *, void *),
		 __u16 type, __u16 flags, __u8 cmd, __u8 version,
		 struct genl *genl, struct buffer *buf, void *arg)
{
	struct nl *nl = &genl->nl;

	if (!nlmsg_begin(buf, &nl->nlh, type, (flags|NLM_F_REQUEST),
			 ++nl->seq, 0))
		goto err;

	if (!genlmsg_begin(buf, &genl->genlh, cmd, version))
		goto err;

	if (builder(buf, arg) < 0)
		goto err;

	genlmsg_end(buf, genl->genlh);
	nlmsg_end(buf, nl->nlh);
	buffer_flip(buf);
	return 0;
err:
	return -1;
}

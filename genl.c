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

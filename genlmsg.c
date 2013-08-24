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
#include <stdio.h>
#include <string.h>
#include "genlmsg.h"
#include "utils.h"

struct buffer *
genlmsg_begin(struct buffer *buf, struct genlmsghdr **gnlhp,
	      __u8 cmd, __u8 version)
{
	struct genlmsghdr *gnlh;

	if (buffer_remaining(buf) < GENL_HDRLEN)
		return 0;

	gnlh = *gnlhp = (struct genlmsghdr *)buffer_data(buf);
	memset(gnlh, 0, GENL_HDRLEN);
	gnlh->cmd = cmd;
	gnlh->version = version;
	gnlh->reserved = 0;

	buf->position += GENL_HDRLEN;
	return buf;
}

void genlmsg_end(struct buffer *buf, struct genlmsghdr *gnlh)
{
}

int genlmsg_parse(struct buffer *buf, struct genlmsghdr **gnlhp)
{
	struct genlmsghdr *gnlh;

	if (buffer_remaining(buf) < GENL_HDRLEN)
		return -1;

	gnlh = *gnlhp = (struct genlmsghdr *)buffer_data(buf);

	trace("\tcmd:%u,version:%u,reserved:%u\n", gnlh->cmd, gnlh->version, gnlh->reserved);

	buf->position += GENL_HDRLEN;
	return 0;
}

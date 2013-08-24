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
#include "nl.h"
#include "nlmsg.h"
#include "utils.h"

struct buffer* nlmsg_begin(struct buffer *buf, struct nlmsghdr **nlhp,
			   __u16 type, __u16 flags, __u32 seq, __u32 pid)
{
	struct nlmsghdr *nlh;

	if (buffer_remaining(buf) < NLMSG_HDRLEN)
		return 0;

	if (nl_pollout(container_of(nlhp, struct nl, nlh), buf) < 0)
		return 0;

	nlh = *nlhp = (struct nlmsghdr *)buffer_data(buf);
	memset(nlh, 0, NLMSG_HDRLEN);
	nlh->nlmsg_type	= type;
	nlh->nlmsg_flags = flags;
	nlh->nlmsg_seq = seq;
	nlh->nlmsg_pid = pid;

	buf->position += NLMSG_HDRLEN;
	return buf;
}

void nlmsg_end(struct buffer *buf, struct nlmsghdr *nlh)
{
	nlh->nlmsg_len = NLMSG_ALIGN(buffer_data(buf) - (void*)nlh);
}

static inline int nlmsg_payload(struct nlmsghdr* nlh)
{
	return nlh->nlmsg_len - NLMSG_HDRLEN;
}

void
nlmsg_slice(struct buffer *buf, struct buffer *slice, struct nlmsghdr *nlh)
{
	int limit = buffer_position(buf) + NLMSG_ALIGN(nlmsg_payload(nlh));

	buffer_clone(slice, buf);
	buffer_set_limit(slice, limit);
}

void nlmsg_slice_end(struct buffer *buf, struct buffer *slice)
{
	buffer_set_position(buf, buffer_position(slice));
	buffer_release(slice);
}

int nlmsg_parse(struct buffer* buf, struct nlmsghdr **nlhp)
{
	struct nlmsghdr *nlh;

	if (buffer_remaining(buf) < NLMSG_HDRLEN)
		return -1;

	nlh = *nlhp = (struct nlmsghdr*)buffer_data(buf);
	if (buffer_remaining(buf) < NLMSG_ALIGN(nlh->nlmsg_len))
		return -1;

	trace("%snlmsg_len:%u,nlmsg_type:0x%x,nlmsg_flags:%u,nlmsg_seq:%u,nlmsg_pid:%u\n",
	      buffer_note(buf),
	      nlh->nlmsg_len,
	      nlh->nlmsg_type,
	      nlh->nlmsg_flags,
	      nlh->nlmsg_seq,
	      nlh->nlmsg_pid);

	buf->position += NLMSG_HDRLEN;
	return 0;
}

int nlmsg_discard(struct buffer *buf, struct nlmsghdr **nlhp)
{
	int len = NLMSG_ALIGN(nlmsg_payload(*nlhp));

	if (buffer_remaining(buf) < len)
		return -1;

	buf->position += len;
	return 0;
}

int nlmsg_err(struct buffer *buf, struct nlmsgerr **nlep)
{
	int len = NLMSG_ALIGN(sizeof(**nlep));
	struct nlmsgerr *nle;

	if (buffer_remaining(buf) < len)
		return -1;

	nle = *nlep = (struct nlmsgerr*)buffer_data(buf);
	if (nle->error < 0)
		len += NLMSG_ALIGN(nlmsg_payload(&nle->msg));
	if (buffer_remaining(buf) < len)
		return -1;

	trace("\terror:%d(%s),nlmsg_len:%u,nlmsg_type:0x%x,nlmsg_flags:%u,nlmsg_seq:%u,nlmsg_pid:%u\n",
	      nle->error,
	      (nle->error < 0 ? strerror(-nle->error) : "Ack"),
	      nle->msg.nlmsg_len,
	      nle->msg.nlmsg_type,
	      nle->msg.nlmsg_flags,
	      nle->msg.nlmsg_seq,
	      nle->msg.nlmsg_pid);

	buf->position += len;
	return 0;
}

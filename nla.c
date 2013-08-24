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
#include "nla.h"
#include "utils.h"

struct nlattr *nla_begin(struct buffer *buf, int type)
{
	struct nlattr *nla;

	if (buffer_remaining(buf) < NLA_HDRLEN)
		return 0;

	nla = (typeof(nla))buffer_data(buf);
	nla->nla_len = 0;
	nla->nla_type = type;

	buf->position += NLA_HDRLEN;
	return nla;
}

void nla_end(struct buffer *buf, struct nlattr *nla, int len)
{
	nla->nla_len = NLA_HDRLEN + len;
}


static inline int nla_payload(struct nlattr *nla)
{
	return nla->nla_len - NLA_HDRLEN;
}

void nla_slice(struct buffer *buf, struct buffer *slice, struct nlattr *nla)
{
	int limit = buffer_position(buf) + NLA_ALIGN(nla_payload(nla));

	buffer_clone(slice, buf);
	buffer_set_limit(slice, limit);
}

void nla_slice_end(struct buffer *buf,struct buffer *slice)
{
	buffer_set_position(buf,buffer_position(slice));
	buffer_release(slice);
}

struct buffer*
nla_nest_begin(struct buffer *buf, struct nlattr **nlap, int type)
{
	struct nlattr *nla;

	nla = *nlap = nla_begin(buf,type);
	if (nla == 0)
		return 0;

	return buf;
}

void nla_nest_end(struct buffer *buf, struct nlattr* nla)
{
	nla->nla_len = NLA_ALIGN(buffer_data(buf) - (void*)nla);
}

int nla_parse(struct buffer *buf, struct nlattr **nlap, int level)
{
	struct nlattr *nla;

	if (buffer_remaining(buf) < NLA_HDRLEN)
		return -1;

	nla = *nlap = (typeof(nla))buffer_data(buf);
	if (buffer_remaining(buf) < NLA_ALIGN(nla->nla_len))
		return -1;

	trace("%.*snla_len:%u,nla_type:%u\n",
	      level + 1, "\t\t\t\t\t\t",
	      nla->nla_len,
	      nla->nla_type);

	buf->position += NLA_HDRLEN;
	return 0;
}

int nla_discard(struct buffer *buf,struct nlattr *nla)
{
	int len = NLA_ALIGN(nla_payload(nla));

	if (buffer_remaining(buf) < len)
		return -1;

	buf->position += len;
	return 0;
}

int nla_get_data(struct buffer *buf, struct nlattr *nla, void **value)
{
	int len = NLA_ALIGN(nla_payload(nla));

	if (buffer_remaining(buf) < len)
		return -1;

	*value = buffer_data(buf);

	buf->position += len;
	return 0;
}


int nla_put_data(struct buffer *buf, void *src, int len, int type)
{
	struct nlattr* nla;
	void* dst;

	nla = nla_begin(buf,type);
	if (nla == 0)
		return -1;

	if (buffer_remaining(buf) < NLA_ALIGN(len))
		return -1;

	dst = buffer_reserve(buf, NLA_ALIGN(len));
	if (dst == 0)
		return -1;

	memcpy(dst, src, len);

	nla_end(buf, nla, len);
	return 0;
}

int nla_get_str(struct buffer *buf, struct nlattr *nla, char **value)
{
	int len = NLA_ALIGN(nla_payload(nla));

	if (buffer_remaining(buf) < len)
		return -1;

	*value = (char*)buffer_data(buf);

	buf->position += len;
	return 0;
}

int nla_put_str(struct buffer *buf, char *value, int type)
{
	struct nlattr *nla;
	int len;
	char* dst;

	nla = nla_begin(buf, type);
	if (nla == 0)
		return -1;

	len = strlen(value) + 1;
	if (buffer_remaining(buf) < NLA_ALIGN(len))
		return -1;

	dst = buffer_reserve(buf, NLA_ALIGN(len));
	if (dst == 0)
		return -1;

	strcpy(dst, value);

	nla_end(buf, nla, len);
	return 0;
}

int nla_put_empty(struct buffer *buf, int type)
{
	struct nlattr *nla;

	nla = nla_begin(buf, type);
	if (nla == 0)
		return -1;

	nla_end(buf, nla, 0);
	return 0;
}

#define _NLA_GET(type)						\
int nla_get_##type(struct buffer *buf, struct nlattr *nla, __##type *value) \
{								\
	int len = NLA_ALIGN(sizeof(*value));			\
	if (buffer_remaining(buf) < len)			\
		return -1;					\
	*value = *(typeof(value))buffer_data(buf);		\
	buf->position += len;					\
	return 0;						\
}

_NLA_GET(u8)
_NLA_GET(u16)
_NLA_GET(u32)
_NLA_GET(u64)
_NLA_GET(s8)
_NLA_GET(s16)
_NLA_GET(s32)
_NLA_GET(s64)
_NLA_GET(be16)
_NLA_GET(be32)
_NLA_GET(be64)

#define _NLA_PUT(type)						\
int nla_put_##type(struct buffer *buf, __##type value, int type)	\
{								\
	int len = NLA_ALIGN(sizeof(value));			\
	struct nlattr* nla;					\
	nla = nla_begin(buf, type);				\
	if (nla == 0)						\
		return -1;					\
	if (buffer_remaining(buf) < sizeof(value))		\
		return -1;					\
	if (sizeof(value) % NLA_ALIGNTO)			\
		memset(buffer_data(buf), 0, len);		\
	*(typeof(&value))buffer_data(buf) = value;		\
	buf->position += len;					\
	nla_end(buf, nla, sizeof(value));			\
	return 0;						\
}

_NLA_PUT(u8)
_NLA_PUT(u16)
_NLA_PUT(u32)
_NLA_PUT(u64)
_NLA_PUT(s8)
_NLA_PUT(s16)
_NLA_PUT(s32)
_NLA_PUT(s64)
_NLA_PUT(be16)
_NLA_PUT(be32)
_NLA_PUT(be64)

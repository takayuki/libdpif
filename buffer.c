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
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "memory.h"

struct buffer *buffer_init(struct buffer *buf, struct memory *mem)
{
	if (mem->size < mem->offset)
		return 0;

	memory_get(mem);

	buf->memory   = mem;
	buf->capacity = mem->size - mem->offset;
	buf->position = 0;
	buf->limit    = buf->capacity;
	buf->data     = mem->addr + mem->offset;
	buf->note     = "?";
	return buf;
}

void buffer_release(struct buffer* buf)
{
	memory_put(buf->memory);
}

struct buffer *buffer_clone(struct buffer* dst, struct buffer* src)
{
	memcpy(dst,src,sizeof(*src));
	memory_get(dst->memory);
	return dst;
}

int buffer_capacity(struct buffer *buf)
{
	return buf->capacity;
}

int buffer_position(struct buffer *buf)
{
	return buf->position;
}

void buffer_set_position(struct buffer *buf, int position)
{
	buf->position = position;
}

int buffer_limit(struct buffer *buf)
{
	return buf->limit;
}

void buffer_set_limit(struct buffer *buf, int limit)
{
	buf->limit = limit;
}

void* buffer_data(struct buffer *buf)
{
	return (buf->data + buf->position);
}

int buffer_remaining(struct buffer *buf)
{
	return buf->limit - buf->position;
}

void buffer_flip(struct buffer *buf)
{
	buf->limit = buf->position;
	buf->position = 0;
}

void buffer_rewind(struct buffer *buf)
{
	buf->position = 0;
}

struct buffer *buffer_clear(struct buffer *buf)
{
	buf->position = 0;
	buf->limit = buf->capacity;
	return buf;
}

struct buffer *buffer_compact(struct buffer *buf)
{
	memmove(buf->data,buffer_data(buf),buffer_remaining(buf));

	buf->position = buffer_remaining(buf);
	buf->limit = buf->capacity;
	return buf;
}

void *buffer_reserve(struct buffer *buf, int len)
{
	void* data;

	if (buffer_remaining(buf) < len)
		return 0;

	data = buffer_data(buf);
	buf->position += len;
	return data;
}

int buffer_discard(struct buffer *buf, int len)
{
	if (buffer_remaining(buf) < len)
		return 0;

	buf->position += len;
	return len;
}

const char *buffer_note(struct buffer *buf)
{
	return buf->note;
}

void buffer_set_note(struct buffer *buf, const char *note)
{
	buf->note = note;
}

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

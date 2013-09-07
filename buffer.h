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
#ifndef _BUFFER_H
#define _BUFFER_H

#include "memory.h"

struct buffer {
	struct memory	*memory;
	int		 capacity;
	int		 position;
	int		 limit;
	void		*data;
	const char	*note;
};

struct buffer *buffer_init(struct buffer *,struct memory *);
void buffer_release(struct buffer *);
struct buffer *buffer_clone(struct buffer *,struct buffer *);
int buffer_capacity(struct buffer *);
int buffer_position(struct buffer *);
void buffer_set_position(struct buffer *, int);
int buffer_limit(struct buffer *);
void buffer_set_limit(struct buffer *, int);
void *buffer_data(struct buffer *);
int buffer_remaining(struct buffer *);
void buffer_flip(struct buffer *);
void buffer_rewind(struct buffer *);
void *buffer_reserve(struct buffer *, int);
int buffer_discard(struct buffer *, int);
const char* buffer_note(struct buffer *);
void buffer_set_note(struct buffer *, const char*);

#endif

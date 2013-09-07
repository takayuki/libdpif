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
#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdlib.h>
#include <sys/mman.h>

enum memory_type {
	MEMORY_STATIC,
	MEMORY_STACK,
	MEMORY_MALLOC,
	MEMORY_MMAP,
};

struct memory {
	enum memory_type	type;
	void*			addr;
	size_t			size;
	off_t			offset;
	int			refcnt;
};

void memory_init(struct memory *, enum memory_type, void*, size_t, off_t);
void memory_get(struct memory *);
void memory_put(struct memory *);

#endif

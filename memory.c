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
#include "memory.h"

void memory_init(struct memory *mem, enum memory_type type,
		 void *addr, size_t size, off_t offset)
{
	mem->type   = type;
	mem->addr   = addr;
	mem->size   = size;
	mem->offset = offset;
	mem->refcnt = 0;
}

void memory_get(struct memory *mem)
{
	mem->refcnt++;
}

void memory_put(struct memory *mem)
{
	if (mem->refcnt > 1) {
		mem->refcnt--;
	} else 	if (mem->refcnt == 1) {
		if (mem->type == MEMORY_MALLOC)
			free(mem->addr);
		mem->refcnt = 0;
	}
}

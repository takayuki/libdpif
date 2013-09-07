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
#ifndef _GENL_H
#define _GENL_H

#include "nl.h"

struct genl {
	struct genlmsghdr	*genlh;

	struct nl		 nl;
};

typedef int (*genl_builder_t)(struct genl *, struct buffer *, void *);

struct genl *genl_init(struct genl *, struct nl_mmap_req *);
void genl_free(struct genl *);
struct genl *genl_downcast(struct nl *);
int genl_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int genl_build(struct genl *, struct buffer *, void *, genl_builder_t);
int genl_builder(int (*)(struct buffer *, void *), __u16, __u16, __u8, __u8,
		 struct genl *, struct buffer *, void *);

#endif

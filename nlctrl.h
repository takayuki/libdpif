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
#ifndef _NLCTRL_H
#define _NLCTRL_H

#include "genl.h"

struct nlctrl {
	char		*family_name;
	__u16		 family_id;

	char		*mcgroup_name;
	__u32		 mcgroup_id;

	struct genl	 genl;
};


struct nlctrl *nlctrl_init(struct nlctrl *, struct nl_mmap_req *);
void nlctrl_free(struct nlctrl *);
int nlctrl_parse(struct nl *, struct buffer *, void *, struct nl_parser *);

#define _NLCTRL_BUILDER_DECL(builder)				\
int nlctrl_##builder(struct genl *, struct buffer *, void *);

_NLCTRL_BUILDER_DECL(get_family)

#endif

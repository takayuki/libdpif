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
#ifndef _RTNL_H
#define _RTNL_H

#include "buffer.h"
#include "nl.h"

#ifndef IFF_UP
#define IFF_UP          0x1
#endif

#define RTNL_ATTR_MAX	32

struct rtnl {
	union {
		struct ifinfomsg	*ifm;
		struct ifaddrmsg	*ifa;
		struct rtmsg		*rtm;
	} hdr;
	struct rtattr			*attrs[RTNL_ATTR_MAX];

	struct nl			 nl;
};

struct rtnl_link_req {
	char		*addr;
	int		 ifindex;
	unsigned int	 change;
	unsigned int	 flags;
};

struct rtnl_addr_req {
	char		*addr;
	int		 ifindex;
};

struct rtnl_route_req {
	char		*addr;
	char		*via;
	int		 ifindex;
};

typedef int (*rtnl_builder_t)(struct rtnl *, struct buffer *, void *);

struct rtnl *rtnl_init(struct rtnl *, struct nl_mmap_req *);
void rtnl_free(struct rtnl *);
struct nl *rtnl_cast(struct rtnl *);
struct rtnl* rtnl_downcast(struct nl *);
int rtnl_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int rtnl_build(struct rtnl *, struct buffer *, void *, rtnl_builder_t);
int rtnl_exec(struct rtnl *, void *, rtnl_builder_t);

#define _RTNL_BUILDER_DECL(builder) \
int rtnl_##builder(struct rtnl *, struct buffer *, void *);

_RTNL_BUILDER_DECL(link_set)
_RTNL_BUILDER_DECL(link_list)
_RTNL_BUILDER_DECL(addr_add)
_RTNL_BUILDER_DECL(route_add)

#define NLA_DATA(nla) (((void*)nla)+NLA_HDRLEN)

#endif

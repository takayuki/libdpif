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

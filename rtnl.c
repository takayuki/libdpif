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
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include "nla.h"
#include "nlmsg.h"
#include "rtnl.h"
#include "utils.h"

struct rtnl* rtnl_init(struct rtnl *rtnl, struct nl_mmap_req *req)
{
	memset(rtnl, 0, offsetof(struct rtnl, nl));

	if (!nl_init(&rtnl->nl, NETLINK_ROUTE, req))
		return 0;

	return rtnl;
}

void rtnl_free(struct rtnl *rtnl)
{
	nl_free(&rtnl->nl);
}

struct nl *rtnl_cast(struct rtnl *rtnl)
{
	return &rtnl->nl;
}
struct rtnl* rtnl_downcast(struct nl *nl)
{
	return container_of(nl, struct rtnl, nl);
}

int rtnl_exec(struct rtnl *rtnl, void *arg, rtnl_builder_t build)
{
	struct nl *nl = rtnl_cast(rtnl);
	struct buffer buf;
	struct memory mem;
	static struct nl_parser inner[2] = {
		{ .parse = rtnl_parse, },
	};
	int ret = -1;

	nl_frame_init(&buf, &mem, &nl->tx_ring);
	rtnl_build(rtnl, &buf, arg, build);
	ret = nl_send(nl, &buf, inner);
	if (ret <= 0)
		goto err;
	ret = nl_dispatch(nl, inner);
err:
	buffer_release(&buf);
	assert(mem.refcnt == 0);
	return ret;
}

#define _RTNL_GET_STRUCT(type)						\
int rtnl_get_##type(struct buffer *buf, struct type **dst)	 	\
{									\
	if (buffer_remaining(buf) < NLMSG_ALIGN(sizeof(**dst)))		\
		return -1;						\
	*dst = (typeof(*dst))buffer_data(buf);				\
	buf->position += NLMSG_ALIGN(sizeof(**dst));			\
	return 0;							\
}

_RTNL_GET_STRUCT(ifinfomsg)
_RTNL_GET_STRUCT(ifaddrmsg)
_RTNL_GET_STRUCT(rtmsg)

#define _RTNL_PUT_STRUCT(type)						\
int rtnl_put_##type(struct buffer *buf, struct type *src)		\
{									\
	char *dst = buffer_reserve(buf, NLMSG_ALIGN(sizeof(*src)));	\
	if (dst == 0)							\
		return -1;						\
	memcpy(dst, src, sizeof(*src));					\
	return 0;							\
}

_RTNL_PUT_STRUCT(ifinfomsg)
_RTNL_PUT_STRUCT(ifaddrmsg)
_RTNL_PUT_STRUCT(rtmsg)

int rtnl_parse(struct nl *nl, struct buffer *buf, void* arg,
	       struct nl_parser *inner)
{
	struct rtnl *rtnl = rtnl_downcast(nl);
	struct rtattr *rta;
	__u16 nlmsg_type = nl->nlh->nlmsg_type;
	int ret = -1;

	memset(rtnl, 0, offsetof(struct rtnl, nl));

	if (RTM_NEWLINK <= nlmsg_type && nlmsg_type <= RTM_SETLINK) {
		if (rtnl_get_ifinfomsg(buf, &rtnl->hdr.ifm) < 0)
			goto err;
	} else if (RTM_NEWADDR <= nlmsg_type && nlmsg_type <= RTM_GETADDR) {
		if (rtnl_get_ifaddrmsg(buf, &rtnl->hdr.ifa) < 0)
			goto err;
	} else if (RTM_NEWROUTE <= nlmsg_type && nlmsg_type <= RTM_GETROUTE) {
		if (rtnl_get_rtmsg(buf, &rtnl->hdr.rtm) < 0)
			goto err;
	} else {
		goto err;
	}

	assert(sizeof(struct rtattr) == sizeof(struct nlattr));

	while (!nla_parse(buf,(struct nlattr **)&rta, 0)) {
		if (rta->rta_type < RTNL_ATTR_MAX)
			rtnl->attrs[rta->rta_type] = rta;
		if (nla_discard(buf, (struct nlattr *)rta) < 0)
			goto err;
	}

	if (inner && inner->parse)
		ret = inner->parse(nl, buf, inner->arg, inner + 1);
	else
		ret = 0;

err:
	return ret;
}

int rtnl_build(struct rtnl *rtnl, struct buffer *buf, void *arg,
	       rtnl_builder_t build)
{
	return nl_frame_build(buf->memory, build(rtnl, buf, arg));
}

static int rtnl_builder(int (*builder)(struct buffer *, void *),
			__u16 type, __u16 flags,
			struct rtnl *rtnl, struct buffer *buf, void *arg)
{
	struct nl *nl = &rtnl->nl;

	if (!nlmsg_begin(buf, &nl->nlh, type, (flags|NLM_F_REQUEST),
			 ++nl->seq, 0))
		goto err;

	if (builder(buf, arg) < 0)
		goto err;

	nlmsg_end(buf, nl->nlh);
	buffer_flip(buf);
	return 0;
err:
	return -1;
}


#define _RTNL_BUILDER(builder, type, flags)				\
int									\
rtnl_##builder(struct rtnl *rtnl, struct buffer *buf, void *arg)	\
{									\
	return rtnl_builder(builder, type, flags, rtnl, buf, arg);	\
}

static int link_list(struct buffer *buf, void *arg)
{
	struct ifinfomsg ifm;

	memset(&ifm, 0, sizeof(ifm));
	if (rtnl_put_ifinfomsg(buf, &ifm) < 0)
		return -1;
	return 0;
}
_RTNL_BUILDER(link_list, RTM_GETLINK, NLM_F_DUMP)

static int link_set(struct buffer *buf, void *arg)
{
	struct rtnl_link_req *req = arg;
	struct ifinfomsg ifm;
	char addr[6];

	memset(&ifm, 0, sizeof(ifm));
	ifm.ifi_index = req->ifindex;
	ifm.ifi_change = req->change;
	ifm.ifi_flags = req->flags;
	if (rtnl_put_ifinfomsg(buf, &ifm) < 0)
		return -1;
	if (req->addr && eth_addr(req->addr, addr))
		nla_put_data(buf, addr, sizeof(addr), IFLA_ADDRESS);
	return 0;
}
_RTNL_BUILDER(link_set, RTM_NEWLINK, NLM_F_ACK)

static int addr_add(struct buffer *buf, void *arg)
{
	struct rtnl_addr_req *req = arg;
	struct ifaddrmsg ifa;

	memset(&ifa, 0, sizeof(ifa));
	ifa.ifa_family = AF_INET;
	ifa.ifa_prefixlen = ip4_netmask(req->addr);
	ifa.ifa_index = req->ifindex;
	if (rtnl_put_ifaddrmsg(buf, &ifa) < 0)
		return -1;
	nla_put_u32(buf, ip4_addr(req->addr), IFA_LOCAL);
	return 0;
}
_RTNL_BUILDER(addr_add, RTM_NEWADDR, (NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK))

static int route_add(struct buffer *buf, void *arg)
{
	struct rtnl_route_req *req = arg;
	struct rtmsg rtm;

	memset(&rtm, 0, sizeof(rtm));
	rtm.rtm_family = AF_INET;
	rtm.rtm_dst_len = ip4_netmask(req->addr);
	rtm.rtm_table = RT_TABLE_MAIN;
	rtm.rtm_protocol = RTPROT_BOOT;
	rtm.rtm_type = RTN_UNICAST;
	if (rtnl_put_rtmsg(buf, &rtm) < 0)
		return -1;
	nla_put_u32(buf, ip4_addr(req->addr), RTA_DST);
	nla_put_u32(buf, ip4_addr(req->via), RTA_GATEWAY);
	nla_put_s32(buf, req->ifindex, RTA_OIF);
	return 0;
}
_RTNL_BUILDER(route_add, RTM_NEWROUTE, (NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK))

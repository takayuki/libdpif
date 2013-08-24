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
#ifndef _NL_H
#define _NL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <linux/rtnetlink.h>
#include <linux/types.h>
#include <poll.h>
#include <stddef.h>
#include "buffer.h"
#include "memory.h"

#ifndef NL_MMAP_HDRLEN
#define NL_MMAP_HDRLEN		NLMSG_ALIGN(sizeof(struct nl_mmap_hdr))
#endif

#ifndef NETLINK_RECV_NO_ENOBUFS
#define NETLINK_RECV_NO_ENOBUFS	5
#endif
#ifndef NETLINK_RX_RING
#define NETLINK_RX_RING		6
#endif
#ifndef NETLINK_TX_RING
#define NETLINK_TX_RING		7
#endif

#ifndef HAVE_STRUCT_NL_MMAP_REQ
struct nl_mmap_req {
	unsigned int	nm_block_size;
	unsigned int	nm_block_nr;
	unsigned int	nm_frame_size;
	unsigned int	nm_frame_nr;
};
#endif

#ifndef HAVE_STRUCT_NL_MMAP_HDR
struct nl_mmap_hdr {
	unsigned int	nm_status;
	unsigned int	nm_len;
	__u32		nm_group;
	__u32		nm_pid;
	__u32		nm_uid;
	__u32		nm_gid;
};
#endif

extern struct nl_mmap_req nl_small_map;
extern struct nl_mmap_req nl_medium_map;
extern struct nl_mmap_req nl_large_map;

#ifndef HAVE_ENUM_NL_MMAP_STATUS
enum nl_mmap_status {
	NL_MMAP_STATUS_UNUSED,
	NL_MMAP_STATUS_RESERVED,
	NL_MMAP_STATUS_VALID,
	NL_MMAP_STATUS_COPY,
	NL_MMAP_STATUS_SKIP,
};
#endif

#ifndef SOL_NETLINK
#define SOL_NETLINK		270
#endif

struct nl;

#define NL_SNDBUF_SIZE		(1024 * 1024 * 2)
#define NL_RCVBUF_SIZE		(1024 * 1024 * 2)

struct nl_ring {
	struct nl	*nl;
	void		*addr;
	size_t		 ring_size;
	size_t		 frame_size;
	unsigned int	 offset;
};

struct nl {
	struct nlmsghdr		*nlh;
	struct nlmsgerr		*nle;

	unsigned long		 rx_poll;
	unsigned long		 tx_poll;

	__u32			 seq;

	int			 fd;
	struct sockaddr_nl	 local;

	int			 nl_mmap;

	struct nl_ring		 rx_ring;
	struct nl_ring		 tx_ring;
};

struct nl_parser {
	int (*parse)(struct nl *, struct buffer *, void*, struct nl_parser *);
	void *arg;
};

struct nl *nl_init(struct nl *, int, struct nl_mmap_req *);
void nl_free(struct nl *);
struct buffer* nl_frame_init(struct buffer *, struct memory *, struct nl_ring *);
void nl_frame_release(struct buffer *);
int nl_frame_build(struct memory *, int);
int nl_subscribe(struct nl *,  __u32);
int nl_recv(struct nl *, struct buffer *);
int nl_pollout(struct nl *, struct buffer *);
int nl_parse(struct nl *, struct buffer *, struct nl_parser *);
int nl_send(struct nl *, struct buffer *, struct nl_parser *);
int nl_dispatch(struct nl *, struct nl_parser *);
int nl_exec(struct nl *, struct buffer *, struct nl_parser *);

#ifndef container_of
#define container_of(ptr, type, member)					\
        ((type *)(void *)((char *)(ptr) - offsetof(type, member)))
#endif

#endif

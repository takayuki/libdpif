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
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "nl.h"
#include "nlmsg.h"
#include "utils.h"

struct nl_mmap_req nl_small_map = {
	.nm_block_size		      = 65536,
	.nm_block_nr		      = 1,
	.nm_frame_size		      = 65536,
	.nm_frame_nr		      = 1,
};

struct nl_mmap_req nl_medium_map = {
	.nm_block_size		      = 65536,
	.nm_block_nr		      = 4,
	.nm_frame_size		      = 65536,
	.nm_frame_nr		      = 4,
};

struct nl_mmap_req nl_large_map = {
	.nm_block_size		      = 65536,
	.nm_block_nr		      = 64,
	.nm_frame_size		      = 16384,
	.nm_frame_nr		      = 64 * 65536 / 16384,
};

static void nl_ring_init(struct nl_ring *ring, void *addr,
			 struct nl_mmap_req *req, struct nl *nl)
{
	ring->addr	 = addr;
	ring->ring_size	 = req->nm_block_size * req->nm_block_nr;
	ring->frame_size = req->nm_frame_size;
	ring->offset	 = 0;
	ring->nl	 = nl;
}

static int nl_sndbuf(struct nl *nl, int size)
{
	int ret;

	ret = setsockopt(nl->fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	if (ret < 0)
		perror("setsockopt");

	return ret;
}

static int nl_rcvbuf(struct nl *nl, int size)
{
	int ret;

	ret = setsockopt(nl->fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	if (ret < 0)
		perror("setsockopt");

	return ret;
}

static int nl_no_enobufs(struct nl *nl)
{
	int ret;
	unsigned int val = 1;

	ret = setsockopt(nl->fd, SOL_NETLINK, NETLINK_RECV_NO_ENOBUFS,
			 &val, sizeof(val));
	if (ret < 0)
		perror("setsockopt");

	return ret;
}

static struct nl *nl_mmap(struct nl *nl, struct nl_mmap_req *req)
{
	unsigned int size;
	void *addr;

	if (req)
		nl->nl_mmap = 1;
	else {
		req = &nl_small_map;
		nl->nl_mmap = 0;
	}

	if (nl->nl_mmap) {
		if (setsockopt(nl->fd, SOL_NETLINK, NETLINK_RX_RING,
			       req, sizeof(*req)) < 0) {
			perror("setsockopt");
			return 0;
		} else if (setsockopt(nl->fd, SOL_NETLINK, NETLINK_TX_RING,
				      req, sizeof(*req)) < 0) {
			perror("setsockopt");
			return 0;
		}
	}

	if (nl_sndbuf(nl, NL_SNDBUF_SIZE) < 0)
		return 0;

	if (nl_rcvbuf(nl, NL_RCVBUF_SIZE) < 0)
		return 0;

	if (nl_no_enobufs(nl) < 0)
		return 0;

	size = req->nm_block_size * req->nm_block_nr * 2;

	if (nl->nl_mmap)
		addr = mmap(0, size, (PROT_READ | PROT_WRITE),
			    MAP_SHARED, nl->fd, 0);
	else
		addr = mmap(0, size, (PROT_READ | PROT_WRITE),
			    (MAP_PRIVATE | MAP_ANONYMOUS), -1, 0);

	if (addr == MAP_FAILED) {
		perror("mmap");
		return 0;
	}

	nl_ring_init(&nl->rx_ring, addr, req, nl);
	nl_ring_init(&nl->tx_ring, addr + size / 2, req, nl);
	return nl;
}

struct nl *nl_init(struct nl *nl, int protocol, struct nl_mmap_req *req)
{
	struct sockaddr_nl sa;
	socklen_t sa_len;
	int s = -1, ret;

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	s = socket(AF_NETLINK, (SOCK_DGRAM | SOCK_CLOEXEC), protocol);
	if (s < 0)
		return 0;
	ret = bind(s, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0)
		goto error;

	memset(&sa, 0, sizeof(sa));
	sa_len = sizeof(sa);
	ret = getsockname(s, (struct sockaddr *)&sa, &sa_len);
	if (ret < 0)
		goto error;

	memset(nl, 0, offsetof(struct nl, fd));
	nl->fd = s;
	nl->local = sa;

	if (!nl_mmap(nl, req))
		goto error;

	return nl;
error:
	close(s);
	return 0;
}

void nl_free(struct nl *nl)
{
	if (munmap(nl->rx_ring.addr, nl->rx_ring.ring_size) < 0)
		perror("munmap");
	if (munmap(nl->tx_ring.addr, nl->tx_ring.ring_size) < 0)
		perror("munmap");

	close(nl->fd);
}

struct buffer *
nl_frame_init(struct buffer *buf, struct memory *mem, struct nl_ring *ring)
{
	memory_init(mem, MEMORY_MMAP, ring->addr + ring->offset,
		    ring->frame_size, NL_MMAP_HDRLEN);

	/* advance frame */
	ring->offset = (ring->offset + ring->frame_size) % ring->ring_size;

	return buffer_init(buf, mem);
}

void nl_frame_release(struct buffer *buf)
{
	struct nl_mmap_hdr *hdr = buf->memory->addr;

	buffer_release(buf);

	switch (hdr->nm_status) {
	case NL_MMAP_STATUS_VALID:
	case NL_MMAP_STATUS_COPY:
		hdr->nm_status = NL_MMAP_STATUS_UNUSED;
	default:
		break;
	}
}

int nl_frame_build(struct memory *mem, int ret)
{
	struct nl_mmap_hdr *hdr = mem->addr;
	struct nlmsghdr *nlh = mem->addr + mem->offset;

	if (ret < 0)
		return ret;

	memset(hdr, 0 ,sizeof(*hdr));

	hdr->nm_len = nlh->nlmsg_len;
	hdr->nm_status =  NL_MMAP_STATUS_VALID;
	return 0;
}

int nl_subscribe(struct nl *nl, __u32 group)
{
	int ret;

	ret = setsockopt(nl->fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP,
			 &group, sizeof(group));
	if (ret < 0)
		perror("setsockopt");

	return ret;
}

static int nl_poll(struct nl *nl, short int events)
{
	struct pollfd fds[1] = {{
			.fd	= nl->fd,
			.events = events,
		}};
	int ret;

	if ((events & POLLIN))
		nl->rx_poll++;
	else if ((events & POLLOUT))
		nl->tx_poll++;

	do {
		ret = poll(fds, 1, -1);
	} while (ret < 0 && errno == EINTR);

	if (ret < 0)
		perror("poll");

	if ((fds[0].revents & POLLERR)) {
		info("poll: error has occurred in stream\n");
		return -1;
	}

	if ((events & POLLIN) && !(fds[0].revents & POLLIN))
		info("poll: stream not readable\n");
	else if ((events & POLLOUT) && !(fds[0].revents & POLLOUT))
		info("poll: stream not writable\n");

	return ret;
}

int nl_pollout(struct nl *nl, struct buffer *snd)
{
	volatile struct nl_mmap_hdr *hdr;
	int ret = 0;

	if (!nl->nl_mmap)
		return 0;

	for (hdr = snd->memory->addr;
	     hdr->nm_status != NL_MMAP_STATUS_UNUSED;
	     hdr = snd->memory->addr) {
		ret = nl_poll(nl, POLLOUT);
		if (ret <= 0)
			return -1;
	}
	return 0;
}

int nl_pollin(struct nl *nl, struct buffer *rcv)
{
	volatile struct nl_mmap_hdr *hdr;
	int ret = 0;

	for (hdr = rcv->memory->addr;
	     (hdr->nm_status == NL_MMAP_STATUS_UNUSED ||
	      hdr->nm_status == NL_MMAP_STATUS_RESERVED);
	     hdr = rcv->memory->addr) {
		ret = nl_poll(nl, POLLIN);
		if (ret <= 0)
			return ret;
	}
	return 0;
}

static const char *str_nm_status(unsigned int status)
{
	switch (status) {
	case NL_MMAP_STATUS_UNUSED:
		return "Unused";
	case NL_MMAP_STATUS_RESERVED:
		return "Reserved";
	case NL_MMAP_STATUS_VALID:
		return "Valid";
	case NL_MMAP_STATUS_COPY:
		return "Copy";
	case NL_MMAP_STATUS_SKIP:
		return "Skip";
	default:
		return "Unknown";
	}
}

int nl_recv(struct nl *nl, struct buffer *rcv)
{
	volatile struct nl_mmap_hdr *hdr;
	int ret;

	buffer_set_note(rcv, ">");

	if (!nl->nl_mmap)
		goto copy;

	if (nl_pollin(nl, rcv) < 0)
		return -1;

	hdr = rcv->memory->addr;

	trace(">nm_status:%u(%s),nm_len:%u,nm_group:%u,nm_pid:%u,nm_uid:%u,nm_gid:%u\n",
	      hdr->nm_status,
	      str_nm_status(hdr->nm_status),
	      hdr->nm_len,
	      hdr->nm_group,
	      hdr->nm_pid,
	      hdr->nm_uid,
	      hdr->nm_gid);

	switch (hdr->nm_status) {
	case NL_MMAP_STATUS_UNUSED:
		info("frame status: unused\n");
		return 0;
	case NL_MMAP_STATUS_RESERVED:
		info("frame status: reserved\n");
		return 0;
	case NL_MMAP_STATUS_VALID:
		/* release frame back to the kernel later */
		buffer_set_position(rcv, hdr->nm_len);
		buffer_flip(rcv);
		ret = buffer_remaining(rcv);
		return ret;
	case NL_MMAP_STATUS_COPY:
		/* release frame back to the kernel later */
		break;
	case NL_MMAP_STATUS_SKIP:
		info("frame status: skip\n");
		return -1;
	default:
		info("frame status: %u\n", hdr->nm_status);
		return -1;
	}

copy:
	ret = read(nl->fd, buffer_data(rcv), buffer_remaining(rcv));
	if (ret == 0) {
		info("read: empty data");
	} else if (ret < 0) {
		perror("read");
		return ret;
	}

	buffer_set_position(rcv, ret);
	buffer_flip(rcv);
	return buffer_remaining(rcv);
}

int nl_parse(struct nl *nl, struct buffer *buf, struct nl_parser *inner)

{
	struct buffer slice;
	int ret = -1;

	if (nlmsg_parse(buf, &nl->nlh) < 0)
		goto err;

	nlmsg_slice(buf, &slice, nl->nlh);
	{
		struct buffer *buf = &slice;

		switch (nl->nlh->nlmsg_type) {
		case NLMSG_NOOP:
			ret = nlmsg_discard(buf, &nl->nlh);
			break;
		case NLMSG_ERROR:
			ret = nlmsg_err(buf, &nl->nle);
			if (ret < 0)
				goto err;
			break;
		case NLMSG_DONE:
			ret = nlmsg_discard(buf, &nl->nlh);
			break;
		case NLMSG_OVERRUN:
			ret = nlmsg_discard(buf, &nl->nlh);
			break;
		default:
			ret = inner->parse(nl, buf,  inner->arg, inner + 1);
		}
	}
	nlmsg_slice_end(buf, &slice);

err:
	return ret;
}

static int
validate(struct nl *nl, struct buffer *buf, struct nl_parser *inner)
{
#ifndef NDEBUG
	struct buffer tmp;

	if (!buffer_clone(&tmp, buf))
		return -1;

	buffer_set_note(&tmp, "<");

	while (!nl_parse(nl, &tmp, inner))
		;

	assert(buffer_remaining(&tmp) == 0);

	if (buffer_remaining(&tmp) != 0)
		return -1;

	buffer_release(&tmp);
#endif
	return 0;
}

int nl_send(struct nl *nl, struct buffer *snd, struct nl_parser *inner)
{
	int ret;

	if (nl->nl_mmap) {
		struct nl_mmap_hdr *hdr = snd->memory->addr;

		trace("<nm_status:%u,nm_len:%u,nm_group:%u,nm_pid:%u,nm_uid:%u,nm_gid:%u\n",
		      hdr->nm_status,
		      hdr->nm_len,
		      hdr->nm_group,
		      hdr->nm_pid,
		      hdr->nm_uid,
		      hdr->nm_gid);

		ret = validate(nl, snd, inner);
		if (ret < 0)
			return ret;

		ret = write(nl->fd, 0, 0);
	} else {
		ret = validate(nl, snd, inner);
		if (ret < 0)
			return ret;

		ret = write(nl->fd, buffer_data(snd), buffer_remaining(snd));
	}

	return ret;
}

int nl_dispatch(struct nl *nl, struct nl_parser *inner)
{
	struct buffer buf;
	struct memory mem;
	int quit = 0, ret = 0;

	while (!quit) {
		nl_frame_init(&buf, &mem, &nl->rx_ring);

		ret = nl_recv(nl, &buf);
		if (ret == 0) {
			quit = !nl->nl_mmap;
			goto next;
		} else if (ret < 0) {
			quit = 1;
			goto next;
		}

		while (!nl_parse(nl, &buf, inner)) {
			if (nl->nlh->nlmsg_type == NLMSG_ERROR) {
				ret = nl->nle->error < 0 ? -1 : 0;
				errno = -nl->nle->error;
				if (errno)
					perror("nl");
			} else {
				ret = 0;
			}

			if (nl->nlh->nlmsg_seq < nl->seq)
				goto next;

			if (nl->nlh->nlmsg_flags & NLM_F_MULTI) {
				if (nl->nlh->nlmsg_type == NLMSG_DONE)
					quit = 1;
			} else
				quit = 1;
		}

		assert(buffer_remaining(&buf) == 0);

	next:
		nl_frame_release(&buf);
	}
	return ret;
}

int nl_exec(struct nl *nl, struct buffer *buf, struct nl_parser *inner)
{
	int ret;

	ret = nl_send(nl, buf, inner);
	if (ret <= 0)
		return ret;

	return nl_dispatch(nl, inner);
}
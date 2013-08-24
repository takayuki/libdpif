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
#ifndef _OVS_H
#define _OVS_H

#include <linux/types.h>
#include "nlctrl.h"

struct ovs {
	struct ovs_header	*ovsh;

	__u16			 ovs_datapath_family;
	__u16			 ovs_vport_family;
	__u16			 ovs_flow_family;
	__u16			 ovs_packet_family;

	__u32			 ovs_datapath_mcgroup;
	__u32			 ovs_vport_mcgroup;
	__u32			 ovs_flow_mcgroup;

	struct genl		 genl;
};

struct ovs *ovs_init(struct ovs *, struct nl_mmap_req *);
void ovs_free(struct ovs *);
struct nl *ovs_upcast(struct ovs *);
struct ovs *ovs_downcast(struct nl *);
int ovs_parse(struct nl *, struct buffer *, void *, struct nl_parser *);
int ovs_put_header(struct buffer *, void *);

#endif

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
#ifndef _NLMSG_H
#define _NLMSG_H

#include <linux/netlink.h>
#include <linux/types.h>
#include "buffer.h"

struct buffer *nlmsg_begin(struct buffer*,struct nlmsghdr**,
			   __u16, __u16, __u32, __u32);
void nlmsg_end(struct buffer*, struct nlmsghdr*);
void nlmsg_slice(struct buffer*, struct buffer*, struct nlmsghdr*);
void nlmsg_slice_end(struct buffer*, struct buffer*);
int nlmsg_parse(struct buffer*, struct nlmsghdr**);
int nlmsg_discard(struct buffer* buf, struct nlmsghdr**);
int nlmsg_err(struct buffer*, struct nlmsgerr**);

#endif

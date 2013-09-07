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
#ifndef _GENLMSG_H
#define _GENLMSG_H

#include <linux/genetlink.h>
#include "buffer.h"

struct buffer *genlmsg_begin(struct buffer *,struct genlmsghdr **, __u8, __u8);
void genlmsg_end(struct buffer *, struct genlmsghdr *);
int genlmsg_parse(struct buffer *, struct genlmsghdr **);

#endif

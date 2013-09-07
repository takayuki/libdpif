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
#include <stdio.h>
#include <string.h>
#include "genlmsg.h"
#include "utils.h"

struct buffer *
genlmsg_begin(struct buffer *buf, struct genlmsghdr **gnlhp,
	      __u8 cmd, __u8 version)
{
	struct genlmsghdr *gnlh;

	if (buffer_remaining(buf) < GENL_HDRLEN)
		return 0;

	gnlh = *gnlhp = (struct genlmsghdr *)buffer_data(buf);
	memset(gnlh, 0, GENL_HDRLEN);
	gnlh->cmd = cmd;
	gnlh->version = version;
	gnlh->reserved = 0;

	buf->position += GENL_HDRLEN;
	return buf;
}

void genlmsg_end(struct buffer *buf, struct genlmsghdr *gnlh)
{
}

int genlmsg_parse(struct buffer *buf, struct genlmsghdr **gnlhp)
{
	struct genlmsghdr *gnlh;

	if (buffer_remaining(buf) < GENL_HDRLEN)
		return -1;

	gnlh = *gnlhp = (struct genlmsghdr *)buffer_data(buf);

	trace("\tcmd:%u,version:%u,reserved:%u\n", gnlh->cmd, gnlh->version, gnlh->reserved);

	buf->position += GENL_HDRLEN;
	return 0;
}

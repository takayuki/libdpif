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
#include <openvswitch.h>
#include "nlmsg.h"
#include "genlmsg.h"
#include "ovs.h"
#include "utils.h"

static int
get_family(struct nlctrl *ctrl, char *name)
{
	struct nl *nl = &ctrl->genl.nl;
	static struct nl_parser inner[3] = {
		{ .parse = genl_parse, },
		{ .parse = nlctrl_parse, },
	};
	struct buffer buf;
	struct memory mem;

	if (!nl_frame_init(&buf, &mem, &nl->tx_ring))
		return -1;

	genl_build(&ctrl->genl, &buf, name, nlctrl_get_family);
	if (nl_send(nl, &buf, inner) <= 0)
		goto err;

	if (nl_dispatch(nl, inner) < 0)
		goto err;

	buffer_release(&buf);
	return 0;
err:
	buffer_release(&buf);
	return -1;
}

struct ovs *ovs_init(struct ovs *ovs, struct nl_mmap_req *req)
{
	static char* families[] = { OVS_DATAPATH_FAMILY,
				    OVS_VPORT_FAMILY,
				    OVS_FLOW_FAMILY,
				    OVS_PACKET_FAMILY };
	struct nlctrl ctrl;
	int i;

	memset(ovs, 0, offsetof(struct ovs, genl));

	if (!nlctrl_init(&ctrl, req))
		return 0;

	for (i = 0; i < sizeof(families)/sizeof(char*); i++) {
		if (get_family(&ctrl, families[i]) < 0)
			goto err;

		if (!strcmp(ctrl.family_name, OVS_DATAPATH_FAMILY))
			ovs->ovs_datapath_family = ctrl.family_id;
		else if (!strcmp(ctrl.family_name, OVS_VPORT_FAMILY))
			ovs->ovs_vport_family = ctrl.family_id;
		else if (!strcmp(ctrl.family_name, OVS_FLOW_FAMILY))
			ovs->ovs_flow_family = ctrl.family_id;
		else if (!strcmp(ctrl.family_name, OVS_PACKET_FAMILY))
			ovs->ovs_packet_family = ctrl.family_id;

		if (!strcmp(ctrl.mcgroup_name, OVS_DATAPATH_MCGROUP))
			ovs->ovs_datapath_mcgroup = ctrl.mcgroup_id;
		else if (!strcmp(ctrl.mcgroup_name, OVS_VPORT_MCGROUP))
			ovs->ovs_vport_mcgroup = ctrl.mcgroup_id;
		else if (!strcmp(ctrl.mcgroup_name, OVS_FLOW_MCGROUP))
			ovs->ovs_flow_mcgroup = ctrl.mcgroup_id;
	}
	nlctrl_free(&ctrl);

	if (!genl_init(&ovs->genl, req))
		goto err;

	return ovs;
err:
	nlctrl_free(&ctrl);
	return 0;
}

void ovs_free(struct ovs *ovs)
{
	genl_free(&ovs->genl);
}

static int ovs_get_header(struct buffer *buf, struct ovs_header **ovshp)
{
	struct ovs_header *ovsh;

	if (buffer_remaining(buf) < NLMSG_ALIGN(sizeof(*ovsh)))
		return -1;

	ovsh = *ovshp = (struct ovs_header*)buffer_data(buf);

	trace("\tdp_ifindex:%d\n", ovsh->dp_ifindex);

	buf->position += NLMSG_ALIGN(sizeof(*ovsh));
	return 0;
}

struct nl *ovs_upcast(struct ovs *ovs)
{
	return &ovs->genl.nl;
}

struct ovs *ovs_downcast(struct nl *nl)
{
	struct genl *genl = container_of(nl, struct genl, nl);

	return container_of(genl, struct ovs, genl);
}

int ovs_parse(struct nl *nl, struct buffer *buf, void *arg,
	      struct nl_parser *inner)
{
	struct ovs *ovs = ovs_downcast(nl);
	int ret = -1;

	if (ovs_get_header(buf, &ovs->ovsh) < 0)
		goto err;

	if (inner && inner->parse)
		ret = inner->parse(nl, buf, inner->arg, inner + 1);
	else
		ret = 0;
err:
	return ret;
}

int ovs_put_header(struct buffer *buf, void *arg)
{
	struct ovs_header* ovsh = arg;
	char* dst;

	dst = buffer_reserve(buf, NLMSG_ALIGN(sizeof(*ovsh)));
	if (dst == 0)
		return -1;

	memcpy(dst, ovsh, sizeof(*ovsh));
	return 0;
}

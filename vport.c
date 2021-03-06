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
#include <sys/types.h>
#include "nla.h"
#include "nlmsg.h"
#include "genlmsg.h"
#include "vport.h"

struct vport* vport_init(struct vport* vport, struct nl_mmap_req *req)
{
	memset(vport, 0, offsetof(struct vport, ovs));

	if (!ovs_init(&vport->ovs, req))
		return 0;

	return vport;
}

void vport_free(struct vport *vport)
{
	ovs_free(&vport->ovs);
}

struct nl *vport_cast(struct vport *vport)
{
	return &vport->ovs.genl.nl;
}

struct vport* vport_downcast(struct nl *nl)
{
	struct genl *genl = container_of(nl, struct genl, nl);
	struct ovs *ovs = container_of(genl, struct ovs, genl);

	return container_of(ovs, struct vport, ovs);
}

static int parse_options(struct buffer *buf, struct ovs_vport_family *vport)
{
	struct nlattr *nla;

	while (!nla_parse(buf, &nla,1)) {
		if (vport->vport_type == OVS_VPORT_TYPE_GRE) {
			nla_discard(buf, nla);
		} else {
			nla_discard(buf, nla);
		}
        }
	return 0;
}

int vport_parse(struct nl *nl, struct buffer *buf, void *arg,
		struct nl_parser *inner)
{
	struct ovs_vport_family *vport = &ovs_downcast(nl)->family.vport;
	struct nlattr* nla;
	int ret = 0;

	memset(vport_downcast(nl), 0, offsetof(struct vport, ovs));

	while (!ret && !nla_parse(buf, &nla,0)) {
		switch (nla->nla_type) {
		case OVS_VPORT_ATTR_UNSPEC:
			ret = nla_discard(buf, nla);
			break;
		case OVS_VPORT_ATTR_PORT_NO:
			ret = nla_get_u32(buf, nla, &vport->vport_no);
			break;
		case OVS_VPORT_ATTR_TYPE:
			ret = nla_get_u32(buf, nla, &vport->vport_type);
			break;
		case OVS_VPORT_ATTR_NAME:
			ret = nla_get_str(buf, nla, &vport->vport_name);
			break;
		case OVS_VPORT_ATTR_OPTIONS: {
			struct buffer slice;

			nla_slice(buf, &slice, nla);
			ret = parse_options(&slice, vport);
			nla_slice_end(buf, &slice);
			break;
		}
		case OVS_VPORT_ATTR_UPCALL_PID:
			ret = nla_get_u32(buf, nla, &vport->vport_upcall_pid);
			break;
		case OVS_VPORT_ATTR_STATS:
			ret = nla_get_data(buf, nla,
					   (void**)&vport->vport_stats);
			break;
#ifdef OVS_VPORT_ATTR_IPV4_REASM
		case OVS_VPORT_ATTR_IPV4_REASM:
			ret = nla_discard(buf, nla);
			break;
#endif
#ifdef OVS_VPORT_ATTR_IPV4_PMTUD
		case OVS_VPORT_ATTR_IPV4_PMTUD:
			ret = nla_discard(buf, nla);
			break;
#endif
		default:
			ret = nla_discard(buf, nla);
		}
	}

	if (!ret && inner && inner->parse)
		ret = inner->parse(nl, buf, inner->arg, inner + 1);

	return ret;
}

int vport_build(struct vport *vport, struct buffer *buf, void *arg,
		vport_builder_t build)
{
	return nl_frame_build(buf->memory, build(vport, buf, arg));
}

int vport_exec(struct vport *vport, void *arg, vport_builder_t build)
{
	struct nl *nl = vport_cast(vport);
	struct buffer buf;
	struct memory mem;
	static struct nl_parser inner[4] = {
		{ .parse = genl_parse, },
		{ .parse = ovs_parse, },
		{ .parse = vport_parse, },
	};
	int ret = -1;

	nl_frame_init(&buf, &mem, &nl->tx_ring);
	vport_build(vport, &buf, arg, build);
	ret = nl_send(nl, &buf, inner);
	if (ret <= 0)
		goto err;
	ret = nl_dispatch(nl, inner);
err:
	buffer_release(&buf);
	assert(mem.refcnt == 0);
	return ret;
}

#define _VPORT_BUILDER(builder, flag, cmd)				\
int vport_##builder(struct vport *vport, struct buffer *buf, void *arg)	\
{									\
	return genl_builder(builder,					\
			    vport->ovs.ovs_vport_family, flag,		\
			    cmd, OVS_VPORT_VERSION,			\
			    &vport->ovs.genl, buf, arg);		\
}

static int cmd_new(struct buffer *buf, void *arg)
{
	struct vport_req *req = arg;
	struct ovs_header ovsh = {
		.dp_ifindex = req->dp_ifindex,
	};
	struct nlattr *nest[1];

	ovs_put_header(buf, &ovsh);
	nla_put_str(buf, req->vport_name, OVS_VPORT_ATTR_NAME);
	nla_put_u32(buf, req->vport_type, OVS_VPORT_ATTR_TYPE);
	if (req->vport_type == OVS_VPORT_TYPE_LISP ||
	    req->vport_type == OVS_VPORT_TYPE_VXLAN) {
		nla_nest_begin(buf, &nest[0], OVS_VPORT_ATTR_OPTIONS);
		nla_put_u16(buf, req->opt.tun.dst_port,
			    OVS_TUNNEL_ATTR_DST_PORT);
		nla_nest_end(buf, nest[0]);
	}
	nla_put_u32(buf, req->vport_upcall_pid, OVS_VPORT_ATTR_UPCALL_PID);
#ifdef OVS_VPORT_ATTR_IPV4_REASM
	nla_put_empty(buf, OVS_VPORT_ATTR_IPV4_REASM);
#endif
#ifdef OVS_VPORT_ATTR_IPV4_PMTUD
	if (req->vport_type == OVS_VPORT_TYPE_NETDEV ||
	    req->vport_type == OVS_VPORT_TYPE_INTERNAL)
		nla_put_empty(buf, OVS_VPORT_ATTR_IPV4_PMTUD);
#endif
	return 0;
}
_VPORT_BUILDER(cmd_new, NLM_F_ECHO, OVS_VPORT_CMD_NEW)

static int cmd_del(struct buffer *buf, void *arg)
{
	struct vport_req *req = arg;
	struct ovs_header ovsh = {
		.dp_ifindex = req->dp_ifindex,
	};

	ovs_put_header(buf, &ovsh);
	nla_put_str(buf, req->vport_name, OVS_VPORT_ATTR_NAME);
	return 0;

}
_VPORT_BUILDER(cmd_del, NLM_F_ACK, OVS_VPORT_CMD_DEL)

static int cmd_list(struct buffer *buf, void *arg)
{
	struct vport_req *req = arg;
	struct ovs_header ovsh = {
		.dp_ifindex = req->dp_ifindex,
	};

	return ovs_put_header(buf, &ovsh);
}
_VPORT_BUILDER(cmd_list, NLM_F_DUMP, OVS_VPORT_CMD_GET)

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
#ifndef _NLA_H
#define _NLA_H

#include <linux/netlink.h>
#include <linux/types.h>
#include "buffer.h"

struct nlattr *nla_begin(struct buffer *, int);
void nla_end(struct buffer *, struct nlattr *, int);
void nla_slice(struct buffer *, struct buffer *, struct nlattr *);
void nla_slice_end(struct buffer *, struct buffer *);
struct buffer* nla_nest_begin(struct buffer *, struct nlattr **, int);
void nla_nest_end(struct buffer *, struct nlattr*);
int nla_parse(struct buffer *, struct nlattr **, int);
int nla_discard(struct buffer *, struct nlattr*);
int nla_get_data(struct buffer *, struct nlattr *, void **);
int nla_put_data(struct buffer *, void *, int, int);
int nla_get_str(struct buffer *, struct nlattr *, char **);
int nla_put_str(struct buffer *, char *, int);
int nla_put_empty(struct buffer *, int);

#define _NLA_GET_DECL(type) \
int nla_get_##type(struct buffer *, struct nlattr *, __##type *);

_NLA_GET_DECL(u8)
_NLA_GET_DECL(u16)
_NLA_GET_DECL(u32)
_NLA_GET_DECL(u64)
_NLA_GET_DECL(s8)
_NLA_GET_DECL(s16)
_NLA_GET_DECL(s32)
_NLA_GET_DECL(s64)
_NLA_GET_DECL(be16)
_NLA_GET_DECL(be32)
_NLA_GET_DECL(be64)

#define _NLA_PUT_DECL(type) \
int nla_put_##type(struct buffer *, __##type, int);

_NLA_PUT_DECL(u8)
_NLA_PUT_DECL(u16)
_NLA_PUT_DECL(u32)
_NLA_PUT_DECL(u64)
_NLA_PUT_DECL(s8)
_NLA_PUT_DECL(s16)
_NLA_PUT_DECL(s32)
_NLA_PUT_DECL(s64)
_NLA_PUT_DECL(be32)
_NLA_PUT_DECL(be64)

#endif

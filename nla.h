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

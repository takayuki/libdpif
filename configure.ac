#
# Copyright 2013 Takayuki Usui
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
AC_PREREQ(2.59)
AC_INIT([libdpif], [0.1], [takayuki.usui@gmail.com], [libdpif])
AC_CONFIG_SRCDIR([buffer.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([tools])
AC_PROG_CC
AC_CHECK_TYPES([struct nl_mmap_req, struct nl_mmap_hdr, enum nl_mmap_status],
               [], [], [[#include <linux/netlink.h>]])
AC_PROG_INSTALL
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

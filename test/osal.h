/*
 * Copyright 2017 Leonid Yuriev <leo@yuriev.ru>
 * and other libmdbx authors: please see AUTHORS file.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#pragma once

#include "base.h"

void osal_setup(const std::vector<actor_config> &actors);
void osal_broadcast(unsigned id);
int osal_waitfor(unsigned id);

mdbx_pid_t osal_getpid(void);
int osal_actor_start(const actor_config &config, mdbx_pid_t &pid);
actor_status osal_actor_info(const mdbx_pid_t pid);
void osal_killall_actors(void);
int osal_actor_poll(mdbx_pid_t &pid, unsigned timeout);
void osal_wait4barrier(void);

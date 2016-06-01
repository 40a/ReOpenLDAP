/* Generic Regex */
/* $ReOpenLDAP$ */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _AC_REGEX_H_
#define _AC_REGEX_H_

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef HAVE_REGEX_H
/*	NO POSIX REGEX!!
 *  You'll need to install a POSIX compatible REGEX library.
 *  Either Henry Spencer's or GNU regex will do.
 */
#error "No POSIX REGEX available."

#elif HAVE_GNUREGEX_H
	/* system has GNU gnuregex.h */
#	include <gnuregex.h>
#else
	/* have regex.h, assume it's POSIX compliant */
#	include <regex.h>
#endif /* regex.h */

#endif /* _AC_REGEX_H_ */

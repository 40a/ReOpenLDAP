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
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 John C. Quillan.
 * Portions Copyright 2002 myinternet Limited.
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

#ifndef PERL_BACK_H
#define PERL_BACK_H 1

#include <EXTERN.h>
#include <perl.h>
#undef _	/* #defined by both Perl and ac/localize.h */

#include "reldap.h"

#include "slap.h"

LDAP_BEGIN_DECL

/*
 * From Apache mod_perl: test for Perl version.
 */

#if defined(pTHX_) || (PERL_REVISION > 5 || (PERL_REVISION == 5 && PERL_VERSION >= 6))
#define PERL_IS_5_6
#endif

#define EVAL_BUF_SIZE 500

extern ldap_pvt_thread_mutex_t  perl_interpreter_mutex;

#ifdef PERL_IS_5_6
/* We should be using the PL_errgv, I think */
/* All the old style variables are prefixed with PL_ now */
# define errgv	PL_errgv
# define na	PL_na
#else
# define call_method(m, f)	perl_call_method(m, f)
# define eval_pv(m, f)	perl_eval_pv(m, f)
# define ERRSV	GvSV(errgv)
#endif

#ifdef USE_ITHREADS
/* pTHX is needed often now */
# define PERL_INTERPRETER			my_perl
# define PERL_BACK_XS_INIT_PARAMS		pTHX
# define PERL_BACK_BOOT_DYNALOADER_PARAMS	pTHX, CV *cv
#else
# define PERL_INTERPRETER			perl_interpreter
# define PERL_BACK_XS_INIT_PARAMS		void
# define PERL_BACK_BOOT_DYNALOADER_PARAMS	CV *cv
# define PERL_SET_CONTEXT(i)
#endif

extern PerlInterpreter *PERL_INTERPRETER;


typedef struct perl_backend_instance {
	char *pb_module_name;
	BerVarray pb_module_path;
	BerVarray pb_module_config;
	SV	*pb_obj_ref;
	int	pb_filter_search_results;
} PerlBackend;

LDAP_END_DECL

#include "proto-perl.h"

#endif /* PERL_BACK_H */

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
/* Portions Copyright (c) 1994 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * This file controls defaults for OpenLDAP package.
 * You probably do not need to edit the defaults provided by this file.
 */

#ifndef _LDAP_DEFAULTS_H
#define _LDAP_DEFAULTS_H

#include <ldap_dirs.h>

#define LDAP_CONF_FILE	LDAP_SYSCONFDIR LDAP_DIRSEP "ldap.conf"
#define LDAP_USERRC_FILE "ldaprc"
#define LDAP_ENV_PREFIX	"LDAP"
#define SASL_CONFIGPATH	LDAP_SYSCONFDIR LDAP_DIRSEP "sasl2"

/* default ldapi:// socket */
#define LDAPI_SOCK LDAP_RUNDIR LDAP_DIRSEP "run" LDAP_DIRSEP "slapd" LDAP_DIRSEP "ldapi"

/*
 * SLAPD DEFINITIONS
 */
	/* location of the default slapd config file */
#define SLAPD_DEFAULT_CONFIGFILE	LDAP_SYSCONFDIR LDAP_DIRSEP "slapd.conf"
#define SLAPD_DEFAULT_CONFIGDIR		LDAP_SYSCONFDIR LDAP_DIRSEP "slapd.d"
#define SLAPD_DEFAULT_DB_DIR		LDAP_RUNDIR LDAP_DIRSEP "openldap-data"
#define SLAPD_DEFAULT_DB_MODE		0600
#define SLAPD_DEFAULT_UCDATA		LDAP_DATADIR LDAP_DIRSEP "ucdata"
	/* default max deref depth for aliases */
#define SLAPD_DEFAULT_MAXDEREFDEPTH	15
	/* default sizelimit on number of entries from a search */
#define SLAPD_DEFAULT_SIZELIMIT		500
	/* default timelimit to spend on a search */
#define SLAPD_DEFAULT_TIMELIMIT		3600

/* the following DNs must be normalized! */
	/* dn of the default subschema subentry */
#define SLAPD_SCHEMA_DN			"cn=Subschema"
	/* dn of the default "monitor" subentry */
#define SLAPD_MONITOR_DN		"cn=Monitor"


#ifndef SLAPD_BIGLOCK_TRACELATENCY
#	define SLAPD_BIGLOCK_TRACELATENCY 0
#endif

#endif /* _LDAP_DEFAULTS_H */

/* referral.c - MDB backend referral handler */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2016 The OpenLDAP Foundation.
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

#include "portable.h"
#include <stdio.h>
#include <ac/string.h>

#include "back-mdb.h"

int
mdb_referrals( Operation *op, SlapReply *rs )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	Entry *e = NULL;
	int rc = LDAP_SUCCESS;

	MDB_txn		*rtxn;
	mdb_op_info	opinfo = {0}, *moi = &opinfo;

	if( op->o_tag == LDAP_REQ_SEARCH ) {
		/* let search take care of itself */
		return rc;
	}

	if( get_manageDSAit( op ) ) {
		/* let op take care of DSA management */
		return rc;
	}

	rc = mdb_opinfo_get(op, mdb, 1, &moi);
	switch(rc) {
	case 0:
		break;
	default:
		return LDAP_OTHER;
	}

	rtxn = moi->moi_txn;

	/* get entry */
	rc = mdb_dn2entry( op, rtxn, &op->o_req_ndn, &e, 1 );

	switch(rc) {
	case MDB_NOTFOUND:
	case 0:
		break;
	case LDAP_BUSY:
		rs->sr_text = "ldap server busy";
		goto done;
	default:
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_referrals)
			": dn2entry failed: %s (%d)\n",
			mdb_strerror(rc), rc, 0 );
		rs->sr_text = "internal error";
		rc = LDAP_OTHER;
		goto done;
	}

	if ( rc == MDB_NOTFOUND ) {
		rc = LDAP_SUCCESS;
		rs->sr_matched = NULL;
		if ( e != NULL ) {
			Debug( LDAP_DEBUG_TRACE,
				LDAP_XSTRING(mdb_referrals)
				": tag=%lu target=\"%s\" matched=\"%s\"\n",
				(unsigned long)op->o_tag, op->o_req_dn.bv_val, e->e_name.bv_val );

			if( is_entry_referral( e ) ) {
				BerVarray ref = get_entry_referrals( op, e );
				rc = LDAP_OTHER;
				rs->sr_ref = referral_rewrite( ref, &e->e_name,
					&op->o_req_dn, LDAP_SCOPE_DEFAULT );
				ber_bvarray_free( ref );
				if ( rs->sr_ref ) {
					rs->sr_matched = ber_strdup_x( e->e_name.bv_val, op->o_tmpmemctx );
					rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
				}
			}

			mdb_entry_return( op, e );
			e = NULL;
		}

		if( rs->sr_ref != NULL ) {
			/* send referrals */
			rc = rs->sr_err = LDAP_REFERRAL;
			send_ldap_result( op, rs );
			rs_send_cleanup( rs );
		} else if ( rc != LDAP_SUCCESS ) {
			rs->sr_text = rs->sr_matched ? "bad referral object" : NULL;
		}
		goto done;
	}

	if ( is_entry_referral( e ) ) {
		/* entry is a referral */
		BerVarray refs = get_entry_referrals( op, e );
		rs->sr_ref = referral_rewrite(
			refs, &e->e_name, &op->o_req_dn, LDAP_SCOPE_DEFAULT );
		ber_bvarray_free( refs );

		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(mdb_referrals)
			": tag=%lu target=\"%s\" matched=\"%s\"\n",
			(unsigned long)op->o_tag, op->o_req_dn.bv_val, e->e_name.bv_val );

		if( rs->sr_ref != NULL ) {
			rc = rs->sr_err = LDAP_REFERRAL;
			rs->sr_matched = e->e_name.bv_val;
			rs->sr_flags = REP_REF_MUSTBEFREED;
			send_ldap_result( op, rs );
			rs_send_cleanup( rs );
		} else {
			rc = LDAP_OTHER;
			rs->sr_text = "bad referral object";
		}
	}

done:
	if ( moi == &opinfo || --moi->moi_ref < 1 ) {
		int rc2 = mdb_txn_reset( moi->moi_txn );
		assert(rc2 == MDB_SUCCESS);
		if ( moi->moi_oe.oe_key )
			LDAP_SLIST_REMOVE( &op->o_extra, &moi->moi_oe, OpExtra, oe_next );
		if ( moi->moi_flag & MOI_FREEIT )
			op->o_tmpfree( moi, op->o_tmpmemctx );
	}
	if ( e )
		mdb_entry_return( op, e );
	return rc;
}

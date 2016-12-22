/*
 * Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC.
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

#include "mdbx.h"

int mdb_runtime_flags = MDBX_DBG_PRINT
#if MDB_DEBUG
		| MDBX_DBG_ASSERT
#endif
#if MDB_DEBUG > 1
		| MDBX_DBG_TRACE
#endif
#if MDB_DEBUG > 2
		| MDBX_DBG_AUDIT
#endif
#if MDB_DEBUG > 3
		| MDBX_DBG_EXTRA
#endif
	;

static MDBX_debug_func *mdb_debug_logger;

int mdbx_setup_debug(int flags, MDBX_debug_func* logger, long edge_txn);

#include "mdb.c"

int __cold
mdbx_setup_debug(int flags, MDBX_debug_func* logger, long edge_txn) {
	unsigned ret = mdb_runtime_flags;
	if (flags != (int) MDBX_DBG_DNT)
		mdb_runtime_flags = flags;
	if (logger != (MDBX_debug_func*) MDBX_DBG_DNT)
		mdb_debug_logger = logger;
	if (edge_txn != (long) MDBX_DBG_DNT) {
#if MDB_DEBUG
		mdb_debug_edge = edge_txn;
#endif
	}
	return ret;
}

static txnid_t __cold
mdbx_oomkick(MDB_env *env, txnid_t oldest)
{
	int retry;
	txnid_t snap;
	mdb_debug("DB size maxed out");

	for(retry = 0; ; ++retry) {
		int reader;

		if (mdb_reader_check(env, NULL))
			break;

		snap = mdb_find_oldest(env, &reader);
		if (oldest < snap || reader < 0) {
			if (retry && env->me_oom_func) {
				/* LY: notify end of oom-loop */
				env->me_oom_func(env, 0, 0, oldest, snap - oldest, -retry);
			}
			return snap;
		}

		MDB_reader *r;
		pthread_t tid;
		pid_t pid;
		int rc;

		if (!env->me_oom_func)
			break;

		r = &env->me_txns->mti_readers[ reader ];
		pid = r->mr_pid;
		tid = r->mr_tid;
		if (r->mr_txnid != oldest || pid <= 0)
			continue;

		rc = env->me_oom_func(env, pid, (void*) tid, oldest,
			mdb_meta_head_w(env)->mm_txnid - oldest, retry);
		if (rc < 0)
			break;

		if (rc) {
			r->mr_txnid = ~(txnid_t)0;
			if (rc > 1) {
				r->mr_tid = 0;
				r->mr_pid = 0;
				mdbx_coherent_barrier();
			}
		}
	}

	if (retry && env->me_oom_func) {
		/* LY: notify end of oom-loop */
		env->me_oom_func(env, 0, 0, oldest, 0, -retry);
	}
	return mdb_find_oldest(env, NULL);
}

int __cold
mdbx_env_set_syncbytes(MDB_env *env, size_t bytes)
{
	if (unlikely(!env))
		return EINVAL;

	if(unlikely(env->me_signature != MDBX_ME_SIGNATURE))
		return MDB_VERSION_MISMATCH;

	env->me_sync_threshold = bytes;
	return env->me_map ? mdb_env_sync(env, 0) : 0;
}

void __cold
mdbx_env_set_oomfunc(MDB_env *env, MDBX_oom_func *oomfunc)
{
	if (likely(env && env->me_signature == MDBX_ME_SIGNATURE))
		env->me_oom_func = oomfunc;
}

MDBX_oom_func* __cold
mdbx_env_get_oomfunc(MDB_env *env)
{
	return likely(env && env->me_signature == MDBX_ME_SIGNATURE)
		? env->me_oom_func : NULL;
}

ATTRIBUTE_NO_SANITIZE_THREAD /* LY: avoid tsan-trap by me_txn, mm_last_pg and mt_next_pgno */
int mdbx_txn_straggler(MDB_txn *txn, int *percent)
{
	MDB_env	*env;
	MDB_meta *meta;
	txnid_t lag;

	if(unlikely(!txn))
		return -EINVAL;

	if(unlikely(txn->mt_signature != MDBX_MT_SIGNATURE))
		return MDB_VERSION_MISMATCH;

	if (unlikely(! txn->mt_u.reader))
		return -1;

	env = txn->mt_env;
	meta = mdb_meta_head_r(env);
	if (percent) {
		size_t maxpg = env->me_maxpg;
		size_t last = meta->mm_last_pg + 1;
		if (env->me_txn)
			last = env->me_txn0->mt_next_pgno;
		*percent = (last * 100ull + maxpg / 2) / maxpg;
	}
	lag = meta->mm_txnid - txn->mt_u.reader->mr_txnid;
	return (0 > (long) lag) ? ~0u >> 1: lag;
}

typedef struct mdb_walk_ctx {
	MDB_txn *mw_txn;
	void *mw_user;
	MDBX_pgvisitor_func *mw_visitor;
} mdb_walk_ctx_t;

/** Depth-first tree traversal. */
static int __cold
mdb_env_walk(mdb_walk_ctx_t *ctx, const char* dbi, pgno_t pg, int flags, int deep)
{
	MDB_page *mp;
	int rc, i, nkeys;
	unsigned header_size, unused_size, payload_size, align_bytes;
	const char* type;

	if (pg == P_INVALID)
		return MDB_SUCCESS; /* empty db */

	MDB_cursor mc;
	memset(&mc, 0, sizeof(mc));
	mc.mc_snum = 1;
	mc.mc_txn = ctx->mw_txn;

	rc = mdb_page_get(&mc, pg, &mp, NULL);
	if (rc)
		return rc;
	if (pg != mp->mp_p.p_pgno)
		return MDB_CORRUPTED;

	nkeys = NUMKEYS(mp);
	header_size = IS_LEAF2(mp) ? PAGEHDRSZ : PAGEBASE + mp->mp_lower;
	unused_size = SIZELEFT(mp);
	payload_size = 0;

	/* LY: Don't use mask here, e.g bitwise (P_BRANCH|P_LEAF|P_LEAF2|P_META|P_OVERFLOW|P_SUBP).
	 * Pages should not me marked dirty/loose or otherwise. */
	switch (mp->mp_flags) {
	case P_BRANCH:
		type = "branch";
		if (nkeys < 1)
			return MDB_CORRUPTED;
		break;
	case P_LEAF:
		type = "leaf";
		break;
	case P_LEAF|P_SUBP:
		type = "dupsort-subleaf";
		break;
	case P_LEAF|P_LEAF2:
		type = "dupfixed-leaf";
		break;
	case P_LEAF|P_LEAF2|P_SUBP:
		type = "dupsort-dupfixed-subleaf";
		break;
	case P_META:
	case P_OVERFLOW:
	default:
		return MDB_CORRUPTED;
	}

	for (align_bytes = i = 0; i < nkeys;
		align_bytes += ((payload_size + align_bytes) & 1), i++) {
		MDB_node *node;

		if (IS_LEAF2(mp)) {
			/* LEAF2 pages have no mp_ptrs[] or node headers */
			payload_size += mp->mp_leaf2_ksize;
			continue;
		}

		node = NODEPTR(mp, i);
		payload_size += NODESIZE + node->mn_ksize;

		if (IS_BRANCH(mp)) {
			rc = mdb_env_walk(ctx, dbi, NODEPGNO(node), flags, deep);
			if (rc)
				return rc;
			continue;
		}

		assert(IS_LEAF(mp));
		if (node->mn_ksize < 1)
			return MDB_CORRUPTED;
		if (node->mn_flags & F_BIGDATA) {
			MDB_page *omp;
			pgno_t *opg;
			size_t over_header, over_payload, over_unused;

			payload_size += sizeof(pgno_t);
			opg = NODEDATA(node);
			rc = mdb_page_get(&mc, *opg, &omp, NULL);
			if (rc)
				return rc;
			if (*opg != omp->mp_p.p_pgno)
				return MDB_CORRUPTED;
			/* LY: Don't use mask here, e.g bitwise (P_BRANCH|P_LEAF|P_LEAF2|P_META|P_OVERFLOW|P_SUBP).
			 * Pages should not me marked dirty/loose or otherwise. */
			if (P_OVERFLOW != omp->mp_flags)
				return MDB_CORRUPTED;

			over_header = PAGEHDRSZ;
			over_payload = NODEDSZ(node);
			over_unused = omp->mp_pages * ctx->mw_txn->mt_env->me_psize
					- over_payload - over_header;

			rc = ctx->mw_visitor(*opg, omp->mp_pages, ctx->mw_user, dbi,
					"overflow-data", 1, over_payload, over_header, over_unused);
			if (rc)
				return rc;
			continue;
		}

		payload_size += NODEDSZ(node);
		if (node->mn_flags & F_SUBDATA) {
			MDB_db *db = NODEDATA(node);
			char* name = NULL;

			if (NODEDSZ(node) < 1)
				return MDB_CORRUPTED;
			if (! (node->mn_flags & F_DUPDATA)) {
				name = NODEKEY(node);
				int namelen = (char*) db - name;
				name = memcpy(alloca(namelen + 1), name, namelen);
				name[namelen] = 0;
			}
			rc = mdb_env_walk(ctx, (name && name[0]) ? name : dbi,
					db->md_root, node->mn_flags & F_DUPDATA, deep + 1);
			if (rc)
				return rc;
		}
	}

	return ctx->mw_visitor(mp->mp_p.p_pgno, 1, ctx->mw_user, dbi, type,
		nkeys, payload_size, header_size, unused_size + align_bytes);
}

int __cold
mdbx_env_pgwalk(MDB_txn *txn, MDBX_pgvisitor_func* visitor, void* user)
{
	mdb_walk_ctx_t ctx;
	int rc;

	if (unlikely(!txn))
		return MDB_BAD_TXN;
	if (unlikely(txn->mt_signature != MDBX_MT_SIGNATURE))
		return MDB_VERSION_MISMATCH;

	ctx.mw_txn = txn;
	ctx.mw_user = user;
	ctx.mw_visitor = visitor;

	rc = visitor(0, 2, user, "lmdb", "meta", 2, sizeof(MDB_meta)*2, PAGEHDRSZ*2,
				 (txn->mt_env->me_psize - sizeof(MDB_meta) - PAGEHDRSZ) *2);
	if (! rc)
		rc = mdb_env_walk(&ctx, "free", txn->mt_dbs[FREE_DBI].md_root, 0, 0);
	if (! rc)
		rc = mdb_env_walk(&ctx, "main", txn->mt_dbs[MAIN_DBI].md_root, 0, 0);
	if (! rc)
		rc = visitor(P_INVALID, 0, user, NULL, NULL, 0, 0, 0, 0);
	return rc;
}

int mdbx_canary_put(MDB_txn *txn, const mdbx_canary* canary)
{
	if (unlikely(!txn))
		return EINVAL;

	if (unlikely(txn->mt_signature != MDBX_MT_SIGNATURE))
		return MDB_VERSION_MISMATCH;

	if (unlikely(F_ISSET(txn->mt_flags, MDB_TXN_RDONLY)))
		return EACCES;

	if (likely(canary)) {
		txn->mt_canary.x = canary->x;
		txn->mt_canary.y = canary->y;
		txn->mt_canary.z = canary->z;
	}
	txn->mt_canary.v = txn->mt_txnid;

	return MDB_SUCCESS;
}

size_t mdbx_canary_get(MDB_txn *txn, mdbx_canary* canary)
{
	if(unlikely(!txn || txn->mt_signature != MDBX_MT_SIGNATURE))
		return 0;

	if (likely(canary))
		*canary = txn->mt_canary;

	return txn->mt_txnid;
}

int mdbx_cursor_eof(MDB_cursor *mc)
{
	if (unlikely(mc == NULL))
		return EINVAL;

	if (unlikely(mc->mc_signature != MDBX_MC_SIGNATURE))
		return MDB_VERSION_MISMATCH;

	return (mc->mc_flags & C_INITIALIZED) ? 0 : 1;
}

static int mdbx_is_samedata(MDB_val* a, MDB_val* b) {
	return a->iov_len == b->iov_len
		&& memcmp(a->iov_base, b->iov_base, a->iov_len) == 0;
}

/* Позволяет обновить или удалить существующую запись с получением
 * в old_data предыдущего значения данных. При этом если new_data равен
 * нулю, то выполняется удаление, иначе обновление/вставка.
 *
 * Текущее значение может находиться в уже измененной (грязной) странице.
 * В этом случае страница будет перезаписана при обновлении, а само старое
 * значение утрачено. Поэтому исходно в old_data должен быть передан
 * дополнительный буфер для копирования старого значения.
 * Если переданный буфер слишком мал, то функция вернет -1, установив
 * old_data->iov_len в соответствующее значение.
 *
 * Для не-уникальных ключей также возможен второй сценарий использования,
 * когда посредством old_data из записей с одинаковым ключом для
 * удаления/обновления выбирается конкретная. Для выбора этого сценария
 * во flags следует одновременно указать MDB_CURRENT и MDB_NOOVERWRITE.
 *
 * Функция может быть замещена соответствующими операциями с курсорами
 * после двух доработок (TODO):
 *  - внешняя аллокация курсоров, в том числе на стеке (без malloc).
 *  - получения статуса страницы по адресу (знать о P_DIRTY).
 */
int mdbx_replace(MDB_txn *txn, MDB_dbi dbi,
	MDB_val *key, MDB_val *new_data, MDB_val *old_data, unsigned flags)
{
	MDB_cursor mc;
	MDB_xcursor mx;

	if (unlikely(!key || !old_data || !txn))
		return EINVAL;

	if (unlikely(txn->mt_signature != MDBX_MT_SIGNATURE))
		return MDB_VERSION_MISMATCH;

	if (unlikely(old_data->iov_base == NULL && old_data->iov_len))
		return EINVAL;

	if (unlikely(new_data == NULL && !(flags & MDB_CURRENT)))
		return EINVAL;

	if (unlikely(!TXN_DBI_EXIST(txn, dbi, DB_USRVALID)))
		return EINVAL;

	if (unlikely(flags & ~(MDB_NOOVERWRITE|MDB_NODUPDATA|MDB_RESERVE|MDB_APPEND|MDB_APPENDDUP|MDB_CURRENT)))
		return EINVAL;

	if (unlikely(txn->mt_flags & (MDB_TXN_RDONLY|MDB_TXN_BLOCKED)))
		return (txn->mt_flags & MDB_TXN_RDONLY) ? EACCES : MDB_BAD_TXN;

	mdb_cursor_init(&mc, txn, dbi, &mx);
	mc.mc_next = txn->mt_cursors[dbi];
	txn->mt_cursors[dbi] = &mc;

	int rc;
	MDB_val present_key = *key;
	if (F_ISSET(flags, MDB_CURRENT | MDB_NOOVERWRITE)
			&& (txn->mt_dbs[dbi].md_flags & MDB_DUPSORT)) {
		/* в old_data значение для выбора конкретного дубликата */
		rc = mdbx_cursor_get(&mc, &present_key, old_data, MDB_GET_BOTH);
		if (rc != MDB_SUCCESS)
			goto bailout;
		/* если данные совпадают, то ничего делать не надо */
		if (new_data && mdbx_is_samedata(old_data, new_data))
			goto bailout;
	} else {
		/* в old_data буфер получения предыдущего значения */
		MDB_val present_data;
		rc = mdbx_cursor_get(&mc, &present_key, &present_data, MDB_SET_KEY);
		if (unlikely(rc != MDB_SUCCESS)) {
			old_data->iov_base = NULL;
			old_data->iov_len = rc;
			if (rc != MDB_NOTFOUND || (flags & MDB_CURRENT))
				goto bailout;
		} else if (flags & MDB_NOOVERWRITE) {
			rc = MDB_KEYEXIST;
			*old_data = present_data;
			goto bailout;
		} else {
			MDB_page *page = mc.mc_pg[mc.mc_top];
			if (txn->mt_dbs[dbi].md_flags & MDB_DUPSORT) {
				if (flags & MDB_CURRENT) {
					/* для не-уникальных ключей позволяем update/delete только если ключ один */
					MDB_node *leaf = NODEPTR(page, mc.mc_ki[mc.mc_top]);
					if (F_ISSET(leaf->mn_flags, F_DUPDATA)) {
						mdb_tassert(txn, XCURSOR_INITED(&mc) && mc.mc_xcursor->mx_db.md_entries > 1);
						rc = MDB_KEYEXIST;
						goto bailout;
					}
					/* если данные совпадают, то ничего делать не надо */
					if (new_data && mdbx_is_samedata(&present_data, new_data))
						goto bailout;
				} else if ((flags & MDB_NODUPDATA) && mdbx_is_samedata(&present_data, new_data)) {
					/* если данные совпадают и установлен MDB_NODUPDATA */
					rc = MDB_KEYEXIST;
					goto bailout;
				}
			} else {
				flags |= MDB_CURRENT;
			}

			if (page->mp_flags & P_DIRTY) {
				if (unlikely(old_data->iov_len < present_data.iov_len)) {
					old_data->iov_base = NULL;
					old_data->iov_len = present_data.iov_len;
					rc = -1;
					goto bailout;
				}
				memcpy(old_data->iov_base, present_data.iov_base, present_data.iov_len);
				old_data->iov_len = present_data.iov_len;
			} else {
				*old_data = present_data;
			}
		}
	}

	if (likely(new_data))
		rc = mdbx_cursor_put(&mc, key, new_data, flags);
	else
		rc = mdbx_cursor_del(&mc, 0);

bailout:
	txn->mt_cursors[dbi] = mc.mc_next;
	return rc;
}

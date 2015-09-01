/*
	Copyright (c) 2015 Leonid Yuriev <leo@yuriev.ru>.
	Copyright (c) 2015 Peter-Service R&D LLC.

	This file is part of ReOpenLDAP.

	ReOpenLDAP is free software; you can redistribute it and/or modify it under
	the terms of the GNU Affero General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	ReOpenLDAP is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* mdb_chk.c - memory-mapped database check tool */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <malloc.h>
#include <time.h>

#include "lmdb.h"
#include "midl.h"

typedef struct flagbit {
	int bit;
	char *name;
} flagbit;

flagbit dbflags[] = {
	{ MDB_DUPSORT, "dupsort" },
	{ MDB_INTEGERKEY, "integerkey" },
	{ MDB_REVERSEKEY, "reversekey" },
	{ MDB_DUPFIXED, "dupfixed" },
	{ MDB_REVERSEDUP, "reversedup" },
	{ MDB_INTEGERDUP, "integerdup" },
	{ 0, NULL }
};

static volatile sig_atomic_t gotsignal;

static void signal_hanlder( int sig ) {
	gotsignal = 1;
}

#define MAX_DBI 32768

#define EXIT_INTERRUPTED			(EXIT_FAILURE+4)
#define EXIT_FAILURE_SYS			(EXIT_FAILURE+3)
#define EXIT_FAILURE_MDB			(EXIT_FAILURE+2)
#define EXIT_FAILURE_CHECK_MAJOR	(EXIT_FAILURE+1)
#define EXIT_FAILURE_CHECK_MINOR	EXIT_FAILURE

struct {
	const char* dbi_names[MAX_DBI];
	size_t dbi_pages[MAX_DBI];
	size_t dbi_payload_bytes[MAX_DBI];
	short *pagemap;
	size_t total_payload_bytes;
	size_t pgcount;
} walk = {
	.dbi_names = { "@gc" }
};

size_t total_unused_bytes;
int exclusive = 2;

MDB_env *env;
MDB_txn *txn, *locktxn;
MDB_envinfo info;
MDB_stat stat;
size_t maxkeysize, reclaimable_pages, freedb_pages, lastpgno;
size_t userdb_count, skipped_subdb;
unsigned verbose, quiet;
const char* only_subdb;

struct problem {
	struct problem* pr_next;
	size_t count;
	const char* caption;
};

struct problem* problems_list;
size_t total_problems;

static void __attribute__ ((format (printf, 1, 2)))
print(const char* msg, ...) {
	if (! quiet) {
		va_list args;

		fflush(stderr);
		va_start(args, msg);
		vfprintf(stdout, msg, args);
		va_end(args);
	}
}

static void __attribute__ ((format (printf, 1, 2)))
error(const char* msg, ...) {
	total_problems++;

	if (! quiet) {
		va_list args;

		fflush(stdout);
		va_start(args, msg);
		vfprintf(stderr, msg, args);
		va_end(args);
		fflush(NULL);
	}
}

static int pagemap_lookup_dbi(const char* dbi) {
	static int last;

	if (last > 0 && strcmp(walk.dbi_names[last], dbi) == 0)
		return last;

	for(last = 1; walk.dbi_names[last] && last < MAX_DBI; ++last)
		if (strcmp(walk.dbi_names[last], dbi) == 0)
			return last;

	if (last == MAX_DBI)
		return last = -1;

	walk.dbi_names[last] = strdup(dbi);
	return last;
}

static void problem_add(size_t entry_number, const char* msg, const char *extra, ...) {
	total_problems++;

	if (! quiet) {
		int need_fflush = 0;
		struct problem* p;

		for (p = problems_list; p; p = p->pr_next)
			if (p->caption == msg)
				break;

		if (! p) {
			p = calloc(1, sizeof(*p));
			p->caption = msg;
			p->pr_next = problems_list;
			problems_list = p;
			need_fflush = 1;
		}

		p->count++;
		if (verbose > 1) {
			print(" - entry #%zu: %s", entry_number, msg);
			if (extra) {
				va_list args;
				va_start(args, extra);
				vfprintf(stdout, extra, args);
				va_end(args);
			}
			printf("\n");
			if (need_fflush)
				fflush(NULL);
		}
	}
}

static struct problem* problems_push() {
	struct problem* p = problems_list;
	problems_list = NULL;
	return p;
}

static size_t problems_pop(struct problem* list) {
	size_t count = 0;

	if (problems_list) {
		int i;

		print(" - problems: ");
		for (i = 0; problems_list; ++i) {
			struct problem* p = problems_list->pr_next;
			count += problems_list->count;
			print("%s%s (%zu)", i ? ", " : "", problems_list->caption, problems_list->count);
			free(problems_list);
			problems_list = p;
		}
		print("\n");
		fflush(NULL);
	}

	problems_list = list;
	return count;
}

static int pgvisitor(size_t pgno, unsigned pgnumber, void* ctx, const char* dbi,
					 char type, int payload_bytes, int header_bytes)
{
	if (pgnumber) {
		int index = pagemap_lookup_dbi(dbi);
		if (index < 0)
			return ENOMEM;

		walk.pgcount += pgnumber;

		if (header_bytes < sizeof(long) || header_bytes >= stat.ms_psize - sizeof(long))
			problem_add(pgno, "wrong header-length", "(%zu < %i < %zu)",
				sizeof(long), header_bytes, header_bytes >= stat.ms_psize - sizeof(long));
		else if (payload_bytes < 1)
			problem_add(pgno, "empty page",  "(payload %zu bytes)", payload_bytes);
		else if (payload_bytes + header_bytes > pgnumber * stat.ms_psize)
			problem_add(pgno, "overflowed page",  "(%zu + %zu > %zu)",
						payload_bytes, header_bytes, pgnumber * stat.ms_psize);
		else {
			walk.dbi_payload_bytes[index] += payload_bytes + header_bytes;
			walk.total_payload_bytes += payload_bytes + header_bytes;
		}

		do {
			if (pgno >= lastpgno)
				problem_add(pgno, "wrong page-no", "(> %zi)", lastpgno);
			else if (walk.pagemap[pgno])
				problem_add(pgno, "page already used", "(in %s)", walk.dbi_names[walk.pagemap[pgno]]);
			else {
				walk.pagemap[pgno] = index;
				walk.dbi_pages[index] += 1;
			}
			++pgno;
		} while(--pgnumber);
	}

	return gotsignal ? EINTR : MDB_SUCCESS;
}

typedef int (visitor)(size_t record_number, MDB_val *key, MDB_val* data);
static int process_db(MDB_dbi dbi, char *name, visitor *handler, int silent);

static int handle_userdb(size_t record_number, MDB_val *key, MDB_val* data) {
	return MDB_SUCCESS;
}

static int handle_freedb(size_t record_number, MDB_val *key, MDB_val* data) {
	char *bad = "";
	size_t pg, prev;
	ssize_t i, number, span = 0;
	size_t *iptr = data->mv_data, txnid = *(size_t*)key->mv_data;

	if (key->mv_size != sizeof(txnid))
		problem_add(record_number, "wrong txn-id size", "(key-size %zi)", key->mv_size);
	else if (txnid < 1 || txnid > info.me_last_txnid)
		problem_add(record_number, "wrong txn-id", "(%zu)", txnid);

	if (data->mv_size < sizeof(size_t) || data->mv_size % sizeof(size_t))
		problem_add(record_number, "wrong idl size", "(%zu)", data->mv_size);
	else {
		number = *iptr++;
		if (number <= 0 || number >= MDB_IDL_UM_MAX)
			problem_add(record_number, "wrong idl length", "(%zi)", number);
		else if ((number + 1) * sizeof(size_t) != data->mv_size)
			problem_add(record_number, "mismatch idl length", "(%zi != %zu)",
						number * sizeof(size_t), data->mv_size);
		else {
			freedb_pages  += number;
			if (info.me_tail_txnid > txnid)
				reclaimable_pages += number;
			for (i = number, prev = 1; --i >= 0; ) {
				pg = iptr[i];
				if (pg < 2 /* META_PAGE */ || pg > info.me_last_pgno)
					problem_add(record_number, "wrong idl entry", "(2 < %zi < %zi)",
								pg, info.me_last_pgno);
				else if (pg <= prev) {
					bad = " [bad sequence]";
					problem_add(record_number, "bad sequence", "(%zi <= %zi)",
								pg, prev);
				}
				prev = pg;
				pg += span;
				for (; i >= span && iptr[i - span] == pg; span++, pg++) ;
			}
			if (verbose > 2)
				print(" - transaction %zu, %zd pages, maxspan %zd%s\n",
					*(size_t *)key->mv_data, number, span, bad);
			if (verbose > 3) {
				int j = number - 1;
				while (j >= 0) {
					pg = iptr[j];
					for (span = 1; --j >= 0 && iptr[j] == pg + span; span++) ;
					print((span > 1) ? "    %9zu[%zd]\n" : "    %9zu\n", pg, span);
				}
			}
		}
	}

	return MDB_SUCCESS;
}

static int handle_maindb(size_t record_number, MDB_val *key, MDB_val* data) {
	char *name;
	int i, rc;

	name = key->mv_data;
	for(i = 0; i < key->mv_size; ++i) {
		if (name[i] < ' ')
			return handle_userdb(record_number, key, data);
	}

	name = malloc(key->mv_size + 1);
	memcpy(name, key->mv_data, key->mv_size);
	name[key->mv_size] = '\0';
	userdb_count++;

	rc = process_db(-1, name, handle_userdb, 0);
	free(name);
	if (rc != MDB_INCOMPATIBLE)
		return rc;

	return handle_userdb(record_number, key, data);
}

static int process_db(MDB_dbi dbi, char *name, visitor *handler, int silent)
{
	MDB_cursor *mc;
	MDB_stat ms;
	MDB_val key, data;
	MDB_val prev_key, prev_data;
	unsigned flags;
	int rc, i;
	struct problem* saved_list;
	size_t problems_count;

	unsigned record_count = 0, dups = 0;
	size_t key_bytes = 0, data_bytes = 0;

	if (0 > (int) dbi) {
		rc = mdb_dbi_open(txn, name, 0, &dbi);
		if (rc) {
			if (!name || rc != MDB_INCOMPATIBLE) /* LY: mainDB's record is not a user's DB. */ {
				error(" - mdb_open '%s' failed, error %d %s\n",
					  name ? name : "main", rc, mdb_strerror(rc));
			}
			return rc;
		}
	}

	if (dbi >= 2 /* CORE_DBS */ && name && only_subdb && strcmp(only_subdb, name)) {
		if (verbose) {
			print("Skip processing %s'db...\n", name);
			fflush(NULL);
		}
		skipped_subdb++;
		mdb_dbi_close(env, dbi);
		return MDB_SUCCESS;
	}

	if (! silent && verbose) {
		print("Processing %s'db...\n", name ? name : "main");
		fflush(NULL);
	}

	rc = mdb_dbi_flags(txn, dbi, &flags);
	if (rc) {
		error(" - mdb_dbi_flags failed, error %d %s\n", rc, mdb_strerror(rc));
		mdb_dbi_close(env, dbi);
		return rc;
	}

	rc = mdb_stat(txn, dbi, &ms);
	if (rc) {
		error(" - mdb_stat failed, error %d %s\n", rc, mdb_strerror(rc));
		mdb_dbi_close(env, dbi);
		return rc;
	}

	if (! silent && verbose) {
		print(" - dbi-id %d, flags:", dbi);
		if (! flags)
			print(" none");
		else {
			for (i=0; dbflags[i].bit; i++)
				if (flags & dbflags[i].bit)
					print(" %s", dbflags[i].name);
		}
		print(" (0x%02X)\n", flags);
		if (verbose > 1) {
			print(" - page size %u, entries %zu\n", ms.ms_psize, ms.ms_entries);
			print(" - b-tree depth %u, pages: branch %zu, leaf %zu, overflow %zu\n",
				  ms.ms_depth, ms.ms_branch_pages, ms.ms_leaf_pages, ms.ms_overflow_pages);
		}
	}

	rc = mdb_cursor_open(txn, dbi, &mc);
	if (rc) {
		error(" - mdb_cursor_open failed, error %d %s\n", rc, mdb_strerror(rc));
		mdb_dbi_close(env, dbi);
		return rc;
	}

	saved_list = problems_push();
	prev_key.mv_data = NULL;
	prev_data.mv_size = 0;
	rc = mdb_cursor_get(mc, &key, &data, MDB_FIRST);
	while (rc == MDB_SUCCESS) {
		if (gotsignal) {
			print(" - interrupted by signal\n");
			fflush(NULL);
			rc = EINTR;
			goto bailout;
		}

		if (key.mv_size == 0) {
			problem_add(record_count, "key with zero length", NULL);
		} else if (key.mv_size > maxkeysize) {
			problem_add(record_count, "key length exceeds max-key-size",
						" (%zu > %zu)", key.mv_size, maxkeysize);
		} else if ((flags & MDB_INTEGERKEY)
			&& key.mv_size != sizeof(size_t) && key.mv_size != sizeof(int)) {
			problem_add(record_count, "wrong key length",
						" (%zu != %zu)", key.mv_size, sizeof(size_t));
		}

		if ((flags & MDB_INTEGERDUP)
			&& data.mv_size != sizeof(size_t) && data.mv_size != sizeof(int)) {
			problem_add(record_count, "wrong data length",
						" (%zu != %zu)", data.mv_size, sizeof(size_t));
		}

		if (prev_key.mv_data) {
			if ((flags & MDB_DUPFIXED) && prev_data.mv_size != data.mv_size) {
				problem_add(record_count, "different data length",
						" (%zu != %zu)", prev_data.mv_size, data.mv_size);
			}

			int cmp = mdb_cmp(txn, dbi, &prev_key, &key);
			if (cmp > 0) {
				problem_add(record_count, "broken ordering of entries", NULL);
			} else if (cmp == 0) {
				++dups;
				if (! (flags & MDB_DUPSORT))
					problem_add(record_count, "duplicated entries", NULL);
				else if (flags & MDB_INTEGERDUP) {
					cmp = mdb_dcmp(txn, dbi, &prev_data, &data);
					if (cmp > 0)
						problem_add(record_count, "broken ordering of multi-values", NULL);
				}
			}
		} else if (verbose) {
			if (flags & MDB_INTEGERKEY)
				print(" - fixed key-size %zu\n", key.mv_size );
			if (flags & (MDB_INTEGERDUP | MDB_DUPFIXED))
				print(" - fixed data-size %zu\n", data.mv_size );
		}

		if (handler) {
			rc = handler(record_count, &key, &data);
			if (rc)
				goto bailout;
		}

		record_count++;
		key_bytes += key.mv_size;
		data_bytes += data.mv_size;

		prev_key = key;
		prev_data = data;
		rc = mdb_cursor_get(mc, &key, &data, MDB_NEXT);
	}
	if (rc != MDB_NOTFOUND)
		error(" - mdb_cursor_get failed, error %d %s\n", rc, mdb_strerror(rc));
	else
		rc = 0;

	if (record_count != ms.ms_entries)
		problem_add(record_count, "differentent number of entries",
				" (%zu != %zu)", record_count, ms.ms_entries);
bailout:
	problems_count = problems_pop(saved_list);
	if (! silent && verbose) {
		print(" - summary: %u records, %u dups, %zu key's bytes, %zu data's bytes, %zu problems\n",
			  record_count, dups, key_bytes, data_bytes, problems_count);
		fflush(NULL);
	}

	mdb_cursor_close(mc);
	mdb_dbi_close(env, dbi);
	return rc || problems_count;
}

static void usage(char *prog)
{
	fprintf(stderr, "usage: %s dbpath [-V] [-v] [-n] [-q] [-w] [-c] [-d] [-s subdb]\n"
			"  -V\t\tshow version\n"
			"  -v\t\tmore verbose, could be used multiple times\n"
			"  -n\t\tNOSUBDIR mode for open\n"
			"  -q\t\tbe quiet\n"
			"  -w\t\tlock DB for writing while checking\n"
			"  -d\t\tdisable page-by-page traversal of b-tree\n"
			"  -s subdb\tprocess a specific subdatabase only\n"
			"  -c\t\tforce cooperative mode (don't try exclusive)\n", prog);
	exit(EXIT_INTERRUPTED);
}

const char* meta_synctype(size_t sign) {
	switch(sign) {
	case 0:
		return "no-sync/legacy";
	case 1:
		return "weak";
	default:
		return "steady";
	}
}

int meta_lt(size_t txn1, size_t sign1, size_t txn2, size_t sign2) {
	return ((sign1 > 1) == (sign2 > 1)) ? txn1 < txn2 : txn2 && sign2 > 1;
}

int main(int argc, char *argv[])
{
	int i, rc;
	char *prog = argv[0];
	char *envname;
	int envflags = MDB_RDONLY;
	int problems_maindb = 0, problems_freedb = 0, problems_meta = 0;
	int dont_traversal = 0;
	size_t n;
	struct timespec timestamp_start, timestamp_finish;
	double elapsed;

	if (clock_gettime(CLOCK_MONOTONIC, &timestamp_start)) {
		rc = errno;
		error("clock_gettime failed, error %d %s\n", rc, mdb_strerror(rc));
		return EXIT_FAILURE_SYS;
	}

	if (argc < 2) {
		usage(prog);
	}

	while ((i = getopt(argc, argv, "Vvqnwcds:")) != EOF) {
		switch(i) {
		case 'V':
			printf("%s\n", MDB_VERSION_STRING);
			exit(EXIT_SUCCESS);
			break;
		case 'v':
			verbose++;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'n':
			envflags |= MDB_NOSUBDIR;
			break;
		case 'w':
			envflags &= ~MDB_RDONLY;
			break;
		case 'c':
			exclusive = 0;
			break;
		case 'd':
			dont_traversal = 1;
			break;
		case 's':
			if (only_subdb && strcmp(only_subdb, optarg))
				usage(prog);
			only_subdb = optarg;
			break;
		default:
			usage(prog);
		}
	}

	if (optind != argc - 1)
		usage(prog);

#ifdef SIGPIPE
	signal(SIGPIPE, signal_hanlder);
#endif
#ifdef SIGHUP
	signal(SIGHUP, signal_hanlder);
#endif
	signal(SIGINT, signal_hanlder);
	signal(SIGTERM, signal_hanlder);

	envname = argv[optind];
	print("Running mdb_chk for '%s' in %s mode...\n",
		  envname, (envflags & MDB_RDONLY) ? "read-only" : "write-lock");
	fflush(NULL);

	rc = mdb_env_create(&env);
	if (rc) {
		error("mdb_env_create failed, error %d %s\n", rc, mdb_strerror(rc));
		return rc < 0 ? EXIT_FAILURE_MDB : EXIT_FAILURE_SYS;
	}

	rc = mdb_env_get_maxkeysize(env);
	if (rc < 0) {
		error("mdb_env_get_maxkeysize failed, error %d %s\n", rc, mdb_strerror(rc));
		goto bailout;
	}
	maxkeysize = rc;

	mdb_env_set_maxdbs(env, 3);

	rc = mdb_env_open_ex(env, envname, envflags, 0664, &exclusive);
	if (rc) {
		error("mdb_env_open failed, error %d %s\n", rc, mdb_strerror(rc));
		goto bailout;
	}
	if (verbose)
		print(" - %s mode\n", exclusive ? "monopolistic" : "cooperative");

	if (! (envflags & MDB_RDONLY)) {
		rc = mdb_txn_begin(env, NULL, 0, &locktxn);
		if (rc) {
			error("mdb_txn_begin(lock-write) failed, error %d %s\n", rc, mdb_strerror(rc));
			goto bailout;
		}
	}

	rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
	if (rc) {
		error("mdb_txn_begin(read-only) failed, error %d %s\n", rc, mdb_strerror(rc));
		goto bailout;
	}

	rc = mdb_env_info(env, &info);
	if (rc) {
		error("mdb_env_info failed, error %d %s\n", rc, mdb_strerror(rc));
		goto bailout;
	}

	rc = mdb_env_stat(env, &stat);
	if (rc) {
		error("mdb_env_stat failed, error %d %s\n", rc, mdb_strerror(rc));
		goto bailout;
	}

	lastpgno = info.me_last_pgno + 1;
	errno = 0;

	if (verbose) {
		double k = 1024.0;
		const char sf[] = "KMGTPEZY"; /* LY: Kilo, Mega, Giga, Tera, Peta, Exa, Zetta, Yotta! */
		for(i = 0; sf[i+1] && info.me_mapsize / k > 1000.0; ++i)
			k *= 1024;
		print(" - map size %zu (%.2f %cb)\n", info.me_mapsize,
			  info.me_mapsize / k, sf[i]);
		if (info.me_mapaddr)
			print(" - mapaddr %p\n", info.me_mapaddr);
		print(" - pagesize %u, max keysize %zu, max readers %u\n",
				  stat.ms_psize, maxkeysize, info.me_maxreaders);
		print(" - transactions: last %zu, bottom %zu, lag reading %zi\n", info.me_last_txnid,
			  info.me_tail_txnid, info.me_last_txnid - info.me_tail_txnid);

		print(" - meta-1: %s %zu, %s",
			   meta_synctype(info.me_meta1_sign), info.me_meta1_txnid,
			   meta_lt(info.me_meta1_txnid, info.me_meta1_sign,
					   info.me_meta2_txnid, info.me_meta2_sign) ? "tail" : "head");
		print(info.me_meta1_txnid > info.me_last_txnid
			  ? ", rolled-back %zu (%zu >>> %zu)\n" : "\n",
			  info.me_meta1_txnid - info.me_last_txnid,
			  info.me_meta1_txnid, info.me_last_txnid);

		print(" - meta-2: %s %zu, %s",
			   meta_synctype(info.me_meta2_sign), info.me_meta2_txnid,
			   meta_lt(info.me_meta2_txnid, info.me_meta2_sign,
					   info.me_meta1_txnid, info.me_meta1_sign) ? "tail" : "head");
		print(info.me_meta2_txnid > info.me_last_txnid
			  ? ", rolled-back %zu (%zu >>> %zu)\n" : "\n",
			  info.me_meta2_txnid - info.me_last_txnid,
			  info.me_meta2_txnid, info.me_last_txnid);
	}

	if (exclusive > 1) {
		if (verbose)
			print(" - perform full check last-txn-id with meta-pages\n");

		if (! meta_lt(info.me_meta1_txnid, info.me_meta1_sign,
				info.me_meta2_txnid, info.me_meta2_sign)
			&& info.me_meta1_txnid != info.me_last_txnid) {
			print(" - meta-1 txn-id mismatch last-txn-id (%zi != %zi)\n",
				  info.me_meta1_txnid, info.me_last_txnid);
			++problems_meta;
		}

		if (! meta_lt(info.me_meta2_txnid, info.me_meta2_sign,
				info.me_meta1_txnid, info.me_meta1_sign)
			&& info.me_meta2_txnid != info.me_last_txnid) {
			print(" - meta-2 txn-id mismatch last-txn-id (%zi != %zi)\n",
				  info.me_meta2_txnid, info.me_last_txnid);
			++problems_meta;
		}
	} else if (locktxn) {
		if (verbose)
			print(" - perform lite check last-txn-id with meta-pages (not a monopolistic mode)\n");
		size_t last = (info.me_meta2_txnid > info.me_meta1_txnid) ? info.me_meta2_txnid : info.me_meta1_txnid;
		if (last != info.me_last_txnid) {
			print(" - last-meta mismatch last-txn-id (%zi != %zi)\n",
				  last, info.me_last_txnid);
			++problems_meta;
		}
	} else if (verbose) {
		print(" - skip check last-txn-id with meta-pages (monopolistic or write-lock mode only)\n");
	}

	if (!dont_traversal) {
		print("Traversal b-tree...\n");
		fflush(NULL);
		walk.pagemap = calloc(lastpgno, sizeof(*walk.pagemap));
		if (! walk.pagemap) {
			rc = errno ? errno : ENOMEM;
			error("calloc failed, error %d %s\n", rc, mdb_strerror(rc));
			goto bailout;
		}

		rc = mdb_env_pgwalk(txn, pgvisitor, NULL);
		if (rc) {
			if (rc == EINTR && gotsignal) {
				print(" - interrupted by signal\n");
				fflush(NULL);
			} else {
				error("mdb_env_pgwalk failed, error %d %s\n", rc, mdb_strerror(rc));
			}
			goto bailout;
		}
		for( n = 0; n < lastpgno; ++n)
			if (! walk.pagemap[n])
				walk.dbi_pages[0] += 1;

		if (verbose) {
			size_t total_page_bytes = walk.pgcount * stat.ms_psize;
			print(" - dbi pages: %zu total", walk.pgcount);
			if (verbose > 1)
				for (i = 1; i < MAX_DBI && walk.dbi_names[i]; ++i)
					print(", %s %zu", walk.dbi_names[i], walk.dbi_pages[i]);
			print(", %s %zu\n", walk.dbi_names[0], walk.dbi_pages[0]);
			if (verbose > 1) {
				print(" - space info: total %zu bytes, payload %zu (%.1f%%), unused %zu (%.1f%%)\n",
					total_page_bytes, walk.total_payload_bytes,
					walk.total_payload_bytes * 100.0 / total_page_bytes,
					total_page_bytes - walk.total_payload_bytes,
					(total_page_bytes - walk.total_payload_bytes) * 100.0 / total_page_bytes);
				for (i = 1; i < MAX_DBI && walk.dbi_names[i]; ++i) {
					size_t dbi_bytes = walk.dbi_pages[i] * stat.ms_psize;
					print("     %s: subtotal %zu bytes (%.1f%%), payload %zu (%.1f%%), unused %zu (%.1f%%)\n",
						walk.dbi_names[i],
						dbi_bytes, dbi_bytes * 100.0 / total_page_bytes,
						walk.dbi_payload_bytes[i], walk.dbi_payload_bytes[i] * 100.0 / dbi_bytes,
						dbi_bytes - walk.dbi_payload_bytes[i],
						(dbi_bytes - walk.dbi_payload_bytes[i]) * 100.0 / dbi_bytes);
				}
			}
			print(" - summary: average fill %.1f%%, %zu problems\n",
				walk.total_payload_bytes * 100.0 / total_page_bytes, total_problems);
		}
	} else if (verbose) {
		print("Skipping b-tree walk...\n");
		fflush(NULL);
	}

	if (! verbose)
		print("Iterating DBIs...\n");
	problems_maindb = process_db(-1, /* MAIN_DBI */ NULL, NULL, 0);
	problems_freedb = process_db(0 /* FREE_DBI */, "free", handle_freedb, 0);

	if (verbose) {
		size_t value = info.me_mapsize / stat.ms_psize;
		double percent = value / 100.0;
		print(" - pages info: %zu total", value);
		print(", allocated %zu (%.1f%%)", lastpgno, lastpgno / percent);

		if (verbose > 1) {
			value = info.me_mapsize / stat.ms_psize - lastpgno;
			print(", remained %zu (%.1f%%)", value, value / percent);

			value = lastpgno - freedb_pages;
			print(", used %zu (%.1f%%)", value, value / percent);

			print(", gc %zu (%.1f%%)", freedb_pages, freedb_pages / percent);

			value = freedb_pages - reclaimable_pages;
			print(", reading %zu (%.1f%%)", value, value / percent);

			print(", reclaimable %zu (%.1f%%)", reclaimable_pages, reclaimable_pages / percent);
		}

		value = info.me_mapsize / stat.ms_psize - lastpgno + reclaimable_pages;
		print(", available %zu (%.1f%%)\n", value, value / percent);
	}

	if (problems_maindb == 0 && problems_freedb == 0) {
		if (!dont_traversal && (exclusive || locktxn)) {
			if (walk.pgcount != lastpgno - freedb_pages) {
				error("used pages mismatch (%zu != %zu)\n", walk.pgcount, lastpgno - freedb_pages);
			}
			if (walk.dbi_pages[0] != freedb_pages) {
				error("gc pages mismatch (%zu != %zu)\n", walk.dbi_pages[0], freedb_pages);
			}
		} else if (verbose) {
			print(" - skip check used and gc pages (btree-traversal with monopolistic or write-lock mode only)\n");
		}

		if (! process_db(-1, NULL, handle_maindb, 1)) {
			if (! userdb_count && verbose)
				print(" - does not contain multiple databases\n");
		}
	}

bailout:
	if (txn)
		mdb_txn_abort(txn);
	if (locktxn)
		mdb_txn_abort(locktxn);
	if (env)
		mdb_env_close(env);
	free(walk.pagemap);
	fflush(NULL);
	if (rc) {
		if (rc < 0)
			return gotsignal ? EXIT_INTERRUPTED : EXIT_FAILURE_SYS;
		return EXIT_FAILURE_MDB;
	}

	if (clock_gettime(CLOCK_MONOTONIC, &timestamp_finish)) {
		rc = errno;
		error("clock_gettime failed, error %d %s\n", rc, mdb_strerror(rc));
		return EXIT_FAILURE_SYS;
	}

	elapsed = timestamp_finish.tv_sec - timestamp_start.tv_sec
			+ (timestamp_finish.tv_nsec - timestamp_start.tv_nsec) * 1e-9;

	total_problems += problems_meta;
	if (total_problems || problems_maindb || problems_freedb) {
		print("Total %zu error(s) is detected, elapsed %.3f seconds.\n",
			  total_problems, elapsed);
		if (problems_meta || problems_maindb || problems_freedb)
			return EXIT_FAILURE_CHECK_MAJOR;
		return EXIT_FAILURE_CHECK_MINOR;
	}
	print("No error is detected, elapsed %.3f seconds\n", elapsed);
	return EXIT_SUCCESS;
}

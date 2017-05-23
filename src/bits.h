/*
 * Copyright 2015-2017 Leonid Yuriev <leo@yuriev.ru>
 * and other libmdbx authors: please see AUTHORS file.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>. */

#pragma once
/* *INDENT-OFF* */
/* clang-format off */

#ifndef _FILE_OFFSET_BITS
#	define _FILE_OFFSET_BITS 64
#endif

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#	define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4464) /* C4464: relative include path contains '..' */
#pragma warning(disable : 4710) /* C4710: 'xyz': function not inlined */
#pragma warning(disable : 4711) /* C4711: function 'xyz' selected for automatic inline expansion */
//#pragma warning(disable : 4061) /* C4061: enumerator 'abc' in switch of enum 'xyz' is not explicitly handled by a case label */
#pragma warning(disable : 4201) /* C4201: nonstandard extension used : nameless struct / union */
#pragma warning(disable : 4706) /* C4706: assignment within conditional expression */
#pragma warning(disable : 4127) /* C4127: conditional expression is constant */
#endif                          /* _MSC_VER (warnings) */

#include "../mdbx.h"
#include "./defs.h"

#if defined(USE_VALGRIND)
#	include <valgrind/memcheck.h>
#	ifndef VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE
/* LY: available since Valgrind 3.10 */
#		define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#		define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#	endif
#else
#	define VALGRIND_CREATE_MEMPOOL(h,r,z)
#	define VALGRIND_DESTROY_MEMPOOL(h)
#	define VALGRIND_MEMPOOL_TRIM(h,a,s)
#	define VALGRIND_MEMPOOL_ALLOC(h,a,s)
#	define VALGRIND_MEMPOOL_FREE(h,a)
#	define VALGRIND_MEMPOOL_CHANGE(h,a,b,s)
#	define VALGRIND_MAKE_MEM_NOACCESS(a,s)
#	define VALGRIND_MAKE_MEM_DEFINED(a,s)
#	define VALGRIND_MAKE_MEM_UNDEFINED(a,s)
#	define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#	define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#	define VALGRIND_CHECK_MEM_IS_ADDRESSABLE(a,s) (0)
#	define VALGRIND_CHECK_MEM_IS_DEFINED(a,s) (0)
#endif /* USE_VALGRIND */

#ifdef __SANITIZE_ADDRESS__
#	include <sanitizer/asan_interface.h>
#else
#	define ASAN_POISON_MEMORY_REGION(addr, size) \
		((void)(addr), (void)(size))
#	define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
		((void)(addr), (void)(size))
#endif /* __SANITIZE_ADDRESS__ */

#include "./osal.h"

#ifndef MDBX_DEBUG
#	define MDBX_DEBUG 0
#endif

#if MDBX_DEBUG
#	undef NDEBUG
#endif

#if defined(__GNUC__) && !__GNUC_PREREQ(4,2)
	/* Actualy libmdbx was not tested with compilers older than GCC from RHEL6.
	 * But you could remove this #error and try to continue at your own risk.
	 * In such case please don't rise up an issues related ONLY to old compilers.
	 */
#	warning "libmdbx required at least GCC 4.2 compatible C/C++ compiler."
#endif

#if defined(__GLIBC__) && !__GLIBC_PREREQ(2,12)
	/* Actualy libmdbx was not tested with something older than glibc 2.12 (from RHEL6).
	 * But you could remove this #error and try to continue at your own risk.
	 * In such case please don't rise up an issues related ONLY to old systems.
	 */
#	warning "libmdbx required at least GLIBC 2.12."
#endif

#if defined(__i386) || defined(__x86_64) || defined(_M_IX86)
#	define UNALIGNED_OK 1 /* TODO */
#endif
#ifndef UNALIGNED_OK
#	define UNALIGNED_OK 0
#endif /* UNALIGNED_OK */

#if (-6 & 5) || CHAR_BIT != 8 || UINT_MAX < 0xffffffff || ULONG_MAX % 0xFFFF
#	error "Sanity checking failed: Two's complement, reasonably sized integer types"
#endif

/*----------------------------------------------------------------------------*/

#ifndef ARRAY_LENGTH
#	ifdef __cplusplus
		template <typename T, size_t N>
		char (&__ArraySizeHelper(T (&array)[N]))[N];
#		define ARRAY_LENGTH(array) (sizeof(::__ArraySizeHelper(array)))
#	else
#		define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))
#	endif
#endif /* ARRAY_LENGTH */

#ifndef ARRAY_END
#	define ARRAY_END(array) (&array[ARRAY_LENGTH(array)])
#endif /* ARRAY_END */

#ifndef STRINGIFY
#	define STRINGIFY_HELPER(x) #x
#	define STRINGIFY(x) STRINGIFY_HELPER(x)
#endif /* STRINGIFY */

#ifndef offsetof
#	define offsetof(type, member)  __builtin_offsetof(type, member)
#endif /* offsetof */

#ifndef container_of
#	define container_of(ptr, type, member) \
		((type *)((char *)(ptr) - offsetof(type, member)))
#endif /* container_of */

/* *INDENT-ON* */
/* clang-format on */

#define FIXME "FIXME: " __FILE__ ", " STRINGIFY(__LINE__)

/*----------------------------------------------------------------------------*/

/* handle for the DB used to track free pages. */
#define FREE_DBI 0
/* handle for the default DB. */
#define MAIN_DBI 1
/* Number of DBs in metapage (free and main) - also hardcoded elsewhere */
#define CORE_DBS 2

/* Number of meta pages - also hardcoded elsewhere */
#define NUM_METAS 2

/* A page number in the database.
 *
 * MDBX uses 32 bit for page numbers. This limits database
 * size up to 2^44 bytes, in case of 4K pages. */
typedef uint32_t pgno_t;
#define PRIaPGNO PRIu32

/* A transaction ID. */
typedef uint64_t txnid_t;
#define PRIaTXN PRIi64

/* An IDL is an ID List, a sorted array of IDs. The first
 * element of the array is a counter for how many actual
 * IDs are in the list. In the original back-bdb code, IDLs are
 * sorted in ascending order. For libmdb IDLs are sorted in
 * descending order. */
typedef pgno_t *MDBX_IDL;

/* An ID2 is an ID/pointer pair. */
typedef struct MDBX_ID2 {
  pgno_t mid; /* The ID */
  void *mptr; /* The pointer */
} MDBX_ID2;

/* An ID2L is an ID2 List, a sorted array of ID2s.
 * The first element's mid member is a count of how many actual
 * elements are in the array. The mptr member of the first element is
 * unused. The array is sorted in ascending order by mid. */
typedef MDBX_ID2 *MDBX_ID2L;

/* Used for offsets within a single page.
 * Since memory pages are typically 4 or 8KB in size, 12-13 bits,
 * this is plenty. */
typedef uint16_t indx_t;

#pragma pack(push, 1)

/* The actual reader record, with cacheline padding. */
typedef struct MDBX_reader {
  /* Current Transaction ID when this transaction began, or (txnid_t)-1.
   * Multiple readers that start at the same time will probably have the
   * same ID here. Again, it's not important to exclude them from
   * anything; all we need to know is which version of the DB they
   * started from so we can avoid overwriting any data used in that
   * particular version. */
  volatile txnid_t mr_txnid;

  /* The information we store in a single slot of the reader table.
   * In addition to a transaction ID, we also record the process and
   * thread ID that owns a slot, so that we can detect stale information,
   * e.g. threads or processes that went away without cleaning up.
   *
   * NOTE: We currently don't check for stale records.
   * We simply re-init the table when we know that we're the only process
   * opening the lock file. */

  /* The process ID of the process owning this reader txn. */
  volatile mdbx_pid_t mr_pid;
  /* The thread ID of the thread owning this txn. */
  volatile mdbx_tid_t mr_tid;

  /* cache line alignment */
  uint8_t pad[~(MDBX_CACHELINE_SIZE - 1) &
              (sizeof(txnid_t) + sizeof(mdbx_pid_t) + sizeof(mdbx_tid_t) +
               MDBX_CACHELINE_SIZE - 1)];
} MDBX_reader;

/* Information about a single database in the environment. */
typedef struct MDBX_db {
  uint32_t md_xsize;        /* also ksize for LEAF2 pages */
  uint16_t md_flags;        /* see mdbx_dbi_open */
  uint16_t md_depth;        /* depth of this tree */
  uint64_t md_seq;          /* table sequence counter */
  pgno_t md_branch_pages;   /* number of internal pages */
  pgno_t md_leaf_pages;     /* number of leaf pages */
  pgno_t md_overflow_pages; /* number of overflow pages */
  pgno_t md_root;           /* the root page of this tree */
  uint64_t md_entries;      /* number of data items */
} MDBX_db;

/* Meta page content.
 * A meta page is the start point for accessing a database snapshot.
 * Pages 0-1 are meta pages. Transaction N writes meta page (N % 2). */
typedef struct MDBX_meta {
  /* Stamp identifying this as an MDBX file. It must be set
   * to MDBX_MAGIC. */
  uint32_t mm_magic;
  /* Version number of this file. Must be set to MDBX_DATA_VERSION. */
  uint32_t mm_version;
  size_t mm_mapsize;        /* size of mmap region */
  MDBX_db mm_dbs[CORE_DBS]; /* first is free space, 2nd is main db */
                            /* The size of pages used in this DB */
#define mm_psize mm_dbs[FREE_DBI].md_xsize
/* Any persistent environment flags, see mdbx_env */
#define mm_flags mm_dbs[FREE_DBI].md_flags
  /* Last used page in the datafile.
  * Actually the file may be shorter if the freeDB lists the final pages. */
  pgno_t mm_last_pg;
  volatile txnid_t mm_txnid; /* txnid that committed this page */
#define MDBX_DATASIGN_NONE 0u
#define MDBX_DATASIGN_WEAK 1u
  volatile uint64_t mm_datasync_sign;
#define SIGN_IS_WEAK(sign) ((sign) == MDBX_DATASIGN_WEAK)
#define SIGN_IS_STEADY(sign) ((sign) > MDBX_DATASIGN_WEAK)
#define META_IS_WEAK(meta) SIGN_IS_WEAK((meta)->mm_datasync_sign)
#define META_IS_STEADY(meta) SIGN_IS_STEADY((meta)->mm_datasync_sign)
  volatile mdbx_canary mm_canary;
} MDBX_meta;

/* Common header for all page types. The page type depends on mp_flags.
 *
 * P_BRANCH and P_LEAF pages have unsorted 'MDBX_node's at the end, with
 * sorted mp_ptrs[] entries referring to them. Exception: P_LEAF2 pages
 * omit mp_ptrs and pack sorted MDBX_DUPFIXED values after the page header.
 *
 * P_OVERFLOW records occupy one or more contiguous pages where only the
 * first has a page header. They hold the real data of F_BIGDATA nodes.
 *
 * P_SUBP sub-pages are small leaf "pages" with duplicate data.
 * A node with flag F_DUPDATA but not F_SUBDATA contains a sub-page.
 * (Duplicate data can also go in sub-databases, which use normal pages.)
 *
 * P_META pages contain MDBX_meta, the start point of an MDBX snapshot.
 *
 * Each non-metapage up to MDBX_meta.mm_last_pg is reachable exactly once
 * in the snapshot: Either used by a database or listed in a freeDB record. */
typedef struct MDBX_page {
  union {
    pgno_t mp_pgno;            /* page number */
    struct MDBX_page *mp_next; /* for in-memory list of freed pages */
  };
  uint16_t mp_leaf2_ksize; /* key size if this is a LEAF2 page */
#define P_BRANCH 0x01      /* branch page */
#define P_LEAF 0x02        /* leaf page */
#define P_OVERFLOW 0x04    /* overflow page */
#define P_META 0x08        /* meta page */
#define P_DIRTY 0x10       /* dirty page, also set for P_SUBP pages */
#define P_LEAF2 0x20       /* for MDBX_DUPFIXED records */
#define P_SUBP 0x40        /* for MDBX_DUPSORT sub-pages */
#define P_LOOSE 0x4000     /* page was dirtied then freed, can be reused */
#define P_KEEP 0x8000      /* leave this page alone during spill */
  uint16_t mp_flags;
  union {
    struct {
      indx_t mp_lower; /* lower bound of free space */
      indx_t mp_upper; /* upper bound of free space */
    };
    uint32_t mp_pages; /* number of overflow pages */
  };
  indx_t mp_ptrs[1]; /* dynamic size */
} MDBX_page;

/* Size of the page header, excluding dynamic data at the end */
#define PAGEHDRSZ ((unsigned)offsetof(MDBX_page, mp_ptrs))

/* Buffer for a stack-allocated meta page.
 * The members define size and alignment, and silence type
 * aliasing warnings.  They are not used directly; that could
 * mean incorrectly using several union members in parallel. */
typedef union MDBX_metabuf {
  MDBX_page mb_page;
  struct {
    char mm_pad[PAGEHDRSZ];
    MDBX_meta mm_meta;
  } mb_metabuf;
} MDBX_metabuf;

/* The header for the reader table (a memory-mapped lock file). */
typedef struct MDBX_lockinfo {
  /* Stamp identifying this as an MDBX file. It must be set to MDBX_MAGIC. */
  uint64_t mti_magic;
  /* Format of this lock file. Must be set to MDBX_LOCK_FORMAT. */
  uint64_t mti_format;
  /* Flags which environment was opened. */
  uint64_t mti_envmode;

#ifdef MDBX_OSAL_LOCK
  MDBX_OSAL_LOCK mti_wmutex;
#endif

  /* The number of slots that have been used in the reader table.
   * This always records the maximum count, it is not decremented
   * when readers release their slots. */
  __cache_aligned volatile unsigned mti_numreaders;
#ifdef MDBX_OSAL_LOCK
  /* Mutex protecting access to this table. */
  MDBX_OSAL_LOCK mti_rmutex;
#endif
  MDBX_reader mti_readers[1];
} MDBX_lockinfo;

#pragma pack(pop)

/* Auxiliary DB info.
 * The information here is mostly static/read-only. There is
 * only a single copy of this record in the environment. */
typedef struct MDBX_dbx {
  MDBX_val md_name;       /* name of the database */
  MDBX_cmp_func *md_cmp;  /* function for comparing keys */
  MDBX_cmp_func *md_dcmp; /* function for comparing data items */
} MDBX_dbx;

/* A database transaction.
 * Every operation requires a transaction handle. */
struct MDBX_txn {
#define MDBX_MT_SIGNATURE (0x93D53A31)
  unsigned mt_signature;
  MDBX_txn *mt_parent; /* parent of a nested txn */
  /* Nested txn under this txn, set together with flag MDBX_TXN_HAS_CHILD */
  MDBX_txn *mt_child;
  pgno_t mt_next_pgno; /* next unallocated page */
  /* The ID of this transaction. IDs are integers incrementing from 1.
   * Only committed write transactions increment the ID. If a transaction
   * aborts, the ID may be re-used by the next writer. */
  txnid_t mt_txnid;
  MDBX_env *mt_env; /* the DB environment */
                    /* The list of reclaimed txns from freeDB */
  MDBX_IDL mt_lifo_reclaimed;
  /* The list of pages that became unused during this transaction. */
  MDBX_IDL mt_free_pages;
  /* The list of loose pages that became unused and may be reused
   * in this transaction, linked through NEXT_LOOSE_PAGE(page). */
  MDBX_page *mt_loose_pages;
  /* Number of loose pages (mt_loose_pages) */
  unsigned mt_loose_count;
  /* The sorted list of dirty pages we temporarily wrote to disk
   * because the dirty list was full. page numbers in here are
   * shifted left by 1, deleted slots have the LSB set. */
  MDBX_IDL mt_spill_pages;
  union {
    /* For write txns: Modified pages. Sorted when not MDBX_WRITEMAP. */
    MDBX_ID2L mt_rw_dirtylist;
    /* For read txns: This thread/txn's reader table slot, or NULL. */
    MDBX_reader *mt_ro_reader;
  };
  /* Array of records for each DB known in the environment. */
  MDBX_dbx *mt_dbxs;
  /* Array of MDBX_db records for each known DB */
  MDBX_db *mt_dbs;
  /* Array of sequence numbers for each DB handle */
  unsigned *mt_dbiseqs;

/* Transaction DB Flags */
#define DB_DIRTY 0x01    /* DB was written in this txn */
#define DB_STALE 0x02    /* Named-DB record is older than txnID */
#define DB_NEW 0x04      /* Named-DB handle opened in this txn */
#define DB_VALID 0x08    /* DB handle is valid, see also MDBX_VALID */
#define DB_USRVALID 0x10 /* As DB_VALID, but not set for FREE_DBI */
#define DB_DUPDATA 0x20  /* DB is MDBX_DUPSORT data */
  /* In write txns, array of cursors for each DB */
  MDBX_cursor **mt_cursors;
  /* Array of flags for each DB */
  uint8_t *mt_dbflags;
  /* Number of DB records in use, or 0 when the txn is finished.
   * This number only ever increments until the txn finishes; we
   * don't decrement it when individual DB handles are closed. */
  MDBX_dbi mt_numdbs;

/* Transaction Flags */
/* mdbx_txn_begin() flags */
#define MDBX_TXN_BEGIN_FLAGS (MDBX_NOMETASYNC | MDBX_NOSYNC | MDBX_RDONLY)
#define MDBX_TXN_NOMETASYNC                                                    \
  MDBX_NOMETASYNC                   /* don't sync meta for this txn on commit */
#define MDBX_TXN_NOSYNC MDBX_NOSYNC /* don't sync this txn on commit */
#define MDBX_TXN_RDONLY MDBX_RDONLY /* read-only transaction */
                                    /* internal txn flags */
#define MDBX_TXN_WRITEMAP MDBX_WRITEMAP /* copy of MDBX_env flag in writers */
#define MDBX_TXN_FINISHED 0x01          /* txn is finished or never began */
#define MDBX_TXN_ERROR 0x02             /* txn is unusable after an error */
#define MDBX_TXN_DIRTY 0x04     /* must write, even if dirty list is empty */
#define MDBX_TXN_SPILLS 0x08    /* txn or a parent has spilled pages */
#define MDBX_TXN_HAS_CHILD 0x10 /* txn has an MDBX_txn.mt_child */
/* most operations on the txn are currently illegal */
#define MDBX_TXN_BLOCKED                                                       \
  (MDBX_TXN_FINISHED | MDBX_TXN_ERROR | MDBX_TXN_HAS_CHILD)
  unsigned mt_flags;
  /* dirtylist room: Array size - dirty pages visible to this txn.
   * Includes ancestor txns' dirty pages not hidden by other txns'
   * dirty/spilled pages. Thus commit(nested txn) has room to merge
   * dirtylist into mt_parent after freeing hidden mt_parent pages. */
  unsigned mt_dirtyroom;
  mdbx_canary mt_canary;
};

/* Enough space for 2^32 nodes with minimum of 2 keys per node. I.e., plenty.
 * At 4 keys per node, enough for 2^64 nodes, so there's probably no need to
 * raise this on a 64 bit machine. */
#define CURSOR_STACK 32

struct MDBX_xcursor;

/* Cursors are used for all DB operations.
 * A cursor holds a path of (page pointer, key index) from the DB
 * root to a position in the DB, plus other state. MDBX_DUPSORT
 * cursors include an xcursor to the current data item. Write txns
 * track their cursors and keep them up to date when data moves.
 * Exception: An xcursor's pointer to a P_SUBP page can be stale.
 * (A node with F_DUPDATA but no F_SUBDATA contains a subpage). */
struct MDBX_cursor {
#define MDBX_MC_SIGNATURE (0xFE05D5B1)
#define MDBX_MC_READY4CLOSE (0x2817A047)
#define MDBX_MC_WAIT4EOT (0x90E297A7)
  unsigned mc_signature;
  /* Next cursor on this DB in this txn */
  MDBX_cursor *mc_next;
  /* Backup of the original cursor if this cursor is a shadow */
  MDBX_cursor *mc_backup;
  /* Context used for databases with MDBX_DUPSORT, otherwise NULL */
  struct MDBX_xcursor *mc_xcursor;
  /* The transaction that owns this cursor */
  MDBX_txn *mc_txn;
  /* The database handle this cursor operates on */
  MDBX_dbi mc_dbi;
  /* The database record for this cursor */
  MDBX_db *mc_db;
  /* The database auxiliary record for this cursor */
  MDBX_dbx *mc_dbx;
  /* The mt_dbflag for this database */
  uint8_t *mc_dbflag;
  uint16_t mc_snum;               /* number of pushed pages */
  uint16_t mc_top;                /* index of top page, normally mc_snum-1 */
                                  /* Cursor state flags. */
#define C_INITIALIZED 0x01        /* cursor has been initialized and is valid */
#define C_EOF 0x02                /* No more data */
#define C_SUB 0x04                /* Cursor is a sub-cursor */
#define C_DEL 0x08                /* last op was a cursor_del */
#define C_UNTRACK 0x40            /* Un-track cursor when closing */
#define C_RECLAIMING 0x80         /* FreeDB lookup is prohibited */
  unsigned mc_flags;              /* see mdbx_cursor */
  MDBX_page *mc_pg[CURSOR_STACK]; /* stack of pushed pages */
  indx_t mc_ki[CURSOR_STACK];     /* stack of page indices */
};

/* Context for sorted-dup records.
 * We could have gone to a fully recursive design, with arbitrarily
 * deep nesting of sub-databases. But for now we only handle these
 * levels - main DB, optional sub-DB, sorted-duplicate DB. */
typedef struct MDBX_xcursor {
  /* A sub-cursor for traversing the Dup DB */
  MDBX_cursor mx_cursor;
  /* The database record for this Dup DB */
  MDBX_db mx_db;
  /* The auxiliary DB record for this Dup DB */
  MDBX_dbx mx_dbx;
  /* The mt_dbflag for this Dup DB */
  uint8_t mx_dbflag;
} MDBX_xcursor;

/* Check if there is an inited xcursor, so XCURSOR_REFRESH() is proper */
#define XCURSOR_INITED(mc)                                                     \
  ((mc)->mc_xcursor && ((mc)->mc_xcursor->mx_cursor.mc_flags & C_INITIALIZED))

/* Update sub-page pointer, if any, in mc->mc_xcursor.
 * Needed when the node which contains the sub-page may have moved.
 * Called with mp = mc->mc_pg[mc->mc_top], ki = mc->mc_ki[mc->mc_top]. */
#define XCURSOR_REFRESH(mc, mp, ki)                                            \
  do {                                                                         \
    MDBX_page *xr_pg = (mp);                                                   \
    MDBX_node *xr_node = NODEPTR(xr_pg, ki);                                   \
    if ((xr_node->mn_flags & (F_DUPDATA | F_SUBDATA)) == F_DUPDATA)            \
      (mc)->mc_xcursor->mx_cursor.mc_pg[0] = NODEDATA(xr_node);                \
  } while (0)

/* State of FreeDB old pages, stored in the MDBX_env */
typedef struct MDBX_pgstate {
  pgno_t *mf_pghead; /* Reclaimed freeDB pages, or NULL before use */
  txnid_t mf_pglast; /* ID of last used record, or 0 if !mf_pghead */
} MDBX_pgstate;

#define MDBX_LOCKINFO_WHOLE_SIZE                                               \
  ((sizeof(MDBX_lockinfo) + MDBX_CACHELINE_SIZE - 1) &                         \
   ~((size_t)MDBX_CACHELINE_SIZE - 1))

/* Lockfile format signature: version, features and field layout */
#define MDBX_LOCK_FORMAT                                                       \
  (((uint64_t)(MDBX_OSAL_LOCK_SIGN) << 32) +                                   \
   ((MDBX_LOCKINFO_WHOLE_SIZE + MDBX_CACHELINE_SIZE - 1) << 16) +              \
   (MDBX_LOCK_VERSION) /* Flags which describe functionality */)

/* The database environment. */
struct MDBX_env {
#define MDBX_ME_SIGNATURE (0x9A899641)
  unsigned me_signature;
  mdbx_filehandle_t me_fd;  /*  The main data file */
  mdbx_filehandle_t me_lfd; /*  The lock file */
/* Failed to update the meta page. Probably an I/O error. */
#define MDBX_FATAL_ERROR 0x80000000U
/* Some fields are initialized. */
#define MDBX_ENV_ACTIVE 0x20000000U
/* me_txkey is set */
#define MDBX_ENV_TXKEY 0x10000000U
  uint32_t me_flags;      /* see mdbx_env */
  unsigned me_psize;      /* DB page size, inited from me_os_psize */
  unsigned me_os_psize;   /* OS page size, from mdbx_syspagesize() */
  unsigned me_maxreaders; /* size of the reader table */
  /* Max MDBX_lockinfo.mti_numreaders of interest to mdbx_env_close() */
  unsigned me_close_readers;
  MDBX_dbi me_numdbs;         /* number of DBs opened */
  MDBX_dbi me_maxdbs;         /* size of the DB table */
  mdbx_pid_t me_pid;          /* process ID of this env */
  char *me_path;              /* path to the DB files */
  char *me_map;               /* the memory map of the data file */
  MDBX_lockinfo *me_lck;      /* the memory map of the lock file, never NULL */
  void *me_pbuf;              /* scratch area for DUPSORT put() */
  MDBX_txn *me_txn;           /* current write transaction */
  MDBX_txn *me_txn0;          /* prealloc'd write transaction */
  size_t me_mapsize;          /* size of the data memory map */
  pgno_t me_maxpg;            /* me_mapsize / me_psize */
  MDBX_dbx *me_dbxs;          /* array of static DB info */
  uint16_t *me_dbflags;       /* array of flags from MDBX_db.md_flags */
  unsigned *me_dbiseqs;       /* array of dbi sequence numbers */
  mdbx_thread_key_t me_txkey; /* thread-key for readers */
  txnid_t me_pgoldest;        /* ID of oldest reader last time we looked */
  MDBX_pgstate me_pgstate;    /* state of old pages from freeDB */
#define me_pglast me_pgstate.mf_pglast
#define me_pghead me_pgstate.mf_pghead
  MDBX_page *me_dpages; /* list of malloc'd blocks for re-use */
                        /* IDL of pages that became unused in a write txn */
  MDBX_IDL me_free_pgs;
  /* ID2L of pages written during a write txn. Length MDBX_IDL_UM_SIZE. */
  MDBX_ID2L me_dirtylist;
  /* Max number of freelist items that can fit in a single overflow page */
  unsigned me_maxfree_1pg;
  /* Max size of a node on a page */
  unsigned me_nodemax;
  unsigned me_maxkey_limit; /* max size of a key */
  int me_live_reader;       /* have liveness lock in reader table */
  void *me_userctx;         /* User-settable context */
#if MDBX_DEBUG
  MDBX_assert_func *me_assert_func; /*  Callback for assertion failures */
#endif
  uint64_t me_sync_pending;   /* Total dirty/non-sync'ed bytes
                               * since the last mdbx_env_sync() */
  uint64_t me_sync_threshold; /* Treshold of above to force synchronous flush */
  MDBX_oom_func *me_oom_func; /* Callback for kicking laggard readers */
#ifdef USE_VALGRIND
  int me_valgrind_handle;
#endif
};

/* Nested transaction */
typedef struct MDBX_ntxn {
  MDBX_txn mnt_txn;         /* the transaction */
  MDBX_pgstate mnt_pgstate; /* parent transaction's saved freestate */
} MDBX_ntxn;

/*----------------------------------------------------------------------------*/

extern int mdbx_runtime_flags;
extern MDBX_debug_func *mdbx_debug_logger;
extern txnid_t mdbx_debug_edge;

void mdbx_debug_log(int type, const char *function, int line, const char *fmt,
                    ...)
#if defined(__GNUC__) || __has_attribute(format)
    __attribute__((format(printf, 4, 5)))
#endif
    ;

void mdbx_panic(const char *fmt, ...)
#if defined(__GNUC__) || __has_attribute(format)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

#if MDBX_DEBUG

#define mdbx_assert_enabled() unlikely(mdbx_runtime_flags &MDBX_DBG_ASSERT)

#define mdbx_audit_enabled() unlikely(mdbx_runtime_flags &MDBX_DBG_AUDIT)

#define mdbx_debug_enabled(type)                                               \
  unlikely(mdbx_runtime_flags &(type & (MDBX_DBG_TRACE | MDBX_DBG_EXTRA)))

#else
#define mdbx_debug_enabled(type) (0)
#define mdbx_audit_enabled() (0)
#ifndef NDEBUG
#define mdbx_assert_enabled() (1)
#else
#define mdbx_assert_enabled() (0)
#endif /* NDEBUG */
#endif /* MDBX_DEBUG */

#define mdbx_print(fmt, ...)                                                   \
  mdbx_debug_log(MDBX_DBG_PRINT, NULL, 0, fmt, ##__VA_ARGS__)

/*----------------------------------------------------------------------------*/

#define mdbx_trace(fmt, ...)                                                   \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE))                                    \
      mdbx_debug_log(MDBX_DBG_TRACE, __FUNCTION__, __LINE__, fmt "\n",         \
                     ##__VA_ARGS__);                                           \
  } while (0)

#define mdbx_verbose(fmt, ...)                                                 \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE /* FIXME */))                        \
      mdbx_debug_log(MDBX_DBG_TRACE /* FIXME */, __FUNCTION__, __LINE__,       \
                     fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define mdbx_info(fmt, ...)                                                    \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE /* FIXME */))                        \
      mdbx_debug_log(MDBX_DBG_TRACE /* FIXME */, __FUNCTION__, __LINE__,       \
                     fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define mdbx_notice(fmt, ...)                                                  \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE /* FIXME */))                        \
      mdbx_debug_log(MDBX_DBG_TRACE /* FIXME */, __FUNCTION__, __LINE__,       \
                     fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define mdbx_warning(fmt, ...)                                                 \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE /* FIXME */))                        \
      mdbx_debug_log(MDBX_DBG_TRACE /* FIXME */, __FUNCTION__, __LINE__,       \
                     fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define mdbx_error(fmt, ...)                                                   \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE /* FIXME */))                        \
      mdbx_debug_log(MDBX_DBG_TRACE /* FIXME */, __FUNCTION__, __LINE__,       \
                     fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define mdbx_fatal(fmt, ...)                                                   \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE /* FIXME */))                        \
      mdbx_debug_log(MDBX_DBG_TRACE /* FIXME */, __FUNCTION__, __LINE__,       \
                     fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

/*----------------------------------------------------------------------------*/

#define mdbx_debug(fmt, ...)                                                   \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE))                                    \
      mdbx_debug_log(MDBX_DBG_TRACE, __FUNCTION__, __LINE__, fmt "\n",         \
                     ##__VA_ARGS__);                                           \
  } while (0)

#define mdbx_debug_print(fmt, ...)                                             \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_TRACE))                                    \
      mdbx_debug_log(MDBX_DBG_TRACE, NULL, 0, fmt, ##__VA_ARGS__);             \
  } while (0)

#define mdbx_debug_extra(fmt, ...)                                             \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_EXTRA))                                    \
      mdbx_debug_log(MDBX_DBG_EXTRA, __FUNCTION__, __LINE__, fmt,              \
                     ##__VA_ARGS__);                                           \
  } while (0)

#define mdbx_debug_extra_print(fmt, ...)                                       \
  do {                                                                         \
    if (mdbx_debug_enabled(MDBX_DBG_EXTRA))                                    \
      mdbx_debug_log(MDBX_DBG_EXTRA, NULL, 0, fmt, ##__VA_ARGS__);             \
  } while (0)

#define mdbx_ensure_msg(env, expr, msg)                                        \
  do {                                                                         \
    if (unlikely(!(expr)))                                                     \
      mdbx_assert_fail(env, msg, __FUNCTION__, __LINE__);                      \
  } while (0)

#define mdbx_ensure(env, expr) mdbx_ensure_msg(env, expr, #expr)

/* assert(3) variant in environment context */
#define mdbx_assert(env, expr)                                                 \
  do {                                                                         \
    if (mdbx_assert_enabled())                                                 \
      mdbx_ensure(env, expr);                                                  \
  } while (0)

/* assert(3) variant in cursor context */
#define mdbx_cassert(mc, expr) mdbx_assert((mc)->mc_txn->mt_env, expr)

/* assert(3) variant in transaction context */
#define mdbx_tassert(txn, expr) mdbx_assert((txn)->mt_env, expr)

/*----------------------------------------------------------------------------*/

static __inline void mdbx_jitter4testing(bool tiny) {
#ifndef NDEBUG
  mdbx_osal_jitter(tiny);
#else
  (void)tiny;
#endif
}

int mdbx_reader_check0(MDBX_env *env, int rlocked, int *dead);

#define METAPAGE_1(env) (&((MDBX_metabuf *)(env)->me_map)->mb_metabuf.mm_meta)

#define METAPAGE_2(env)                                                        \
  (&((MDBX_metabuf *)((env)->me_map + env->me_psize))->mb_metabuf.mm_meta)

static __inline MDBX_meta *mdbx_meta_head(MDBX_env *env) {
  mdbx_jitter4testing(true);
  MDBX_meta *a = METAPAGE_1(env);
  mdbx_jitter4testing(true);
  MDBX_meta *b = METAPAGE_2(env);
  mdbx_jitter4testing(true);

  return (a->mm_txnid > b->mm_txnid) ? a : b;
}

void mdbx_rthc_dtor(void *rthc);
void mdbx_rthc_lock(void);
void mdbx_rthc_unlock(void);
int mdbx_rthc_alloc(mdbx_thread_key_t *key, MDBX_reader *begin,
                    MDBX_reader *end);
void mdbx_rthc_remove(mdbx_thread_key_t key);
void mdbx_rthc_cleanup(void);

static __inline bool is_power2(size_t x) { return (x & (x - 1)) == 0; }

static __inline size_t roundup2(size_t value, size_t granularity) {
  assert(is_power2(granularity));
  return (value + granularity - 1) & ~(granularity - 1);
}

#define MDBX_IS_ERROR(rc)                                                      \
  ((rc) != MDBX_RESULT_TRUE && (rc) != MDBX_RESULT_FALSE)

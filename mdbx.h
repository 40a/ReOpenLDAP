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
 * <http://www.OpenLDAP.org/license.html>.
 *
 * ---
 *
 * This code is derived from "LMDB engine" written by
 * Howard Chu (Symas Corporation), which itself derived from btree.c
 * written by Martin Hedenfalk.
 *
 * ---
 *
 * Portions Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 *
 * ---
 *
 * Portions Copyright (c) 2009, 2010 Martin Hedenfalk <martin@bzero.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once
/* *INDENT-OFF* */
/* clang-format off */

#ifndef _MDBX_H_
#define _MDBX_H_
#define MDBX_MODE_ENABLED 1

#ifndef __has_attribute
#	define __has_attribute(x) (0)
#endif

#ifndef __dll_export
#	if defined(_WIN32) || defined(__CYGWIN__)
#		if defined(__GNUC__) || __has_attribute(dllexport)
#			define __dll_export __attribute__((dllexport))
#		elif defined(_MSC_VER)
#			define __dll_export __declspec(dllexport)
#		else
#			define __dll_export
#		endif
#	elif defined(__GNUC__) || __has_attribute(visibility)
#		define __dll_export __attribute__((visibility("default")))
#	else
#		define __dll_export
#	endif
#endif /* __dll_export */

#ifndef __dll_import
#	if defined(_WIN32) || defined(__CYGWIN__)
#		if defined(__GNUC__) || __has_attribute(dllimport)
#			define __dll_import __attribute__((dllimport))
#		elif defined(_MSC_VER)
#			define __dll_import __declspec(dllimport)
#		else
#			define __dll_import
#		endif
#	else
#		define __dll_import
#	endif
#endif /* __dll_import */

#if defined(LIBMDBX_EXPORTS)
#	define LIBMDBX_API __dll_export
#elif defined(LIBMDBX_IMPORTS)
#	define LIBMDBX_API __dll_import
#else
#	define LIBMDBX_API
#endif /* LIBMDBX_API */

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4514) /* 'xyz': unreferenced inline function  \
                                   has been removed */
#pragma warning(disable : 4710) /* 'xyz': function not inlined */
#pragma warning(disable : 4711) /* function 'xyz' selected for          \
                                   automatic inline expansion */
#pragma warning(disable : 4061) /* enumerator 'abc' in switch of enum   \
                                   'xyz' is not explicitly handled by a case   \
                                   label */
#pragma warning(disable : 4201) /* nonstandard extension used :         \
                                   nameless struct / union */
#pragma warning(disable : 4127) /* conditional expression is constant   \
                                   */

#pragma warning(push, 1)
#pragma warning(disable : 4530) /* C++ exception handler used, but      \
                                    unwind semantics are not enabled. Specify  \
                                    /EHsc */
#pragma warning(disable : 4577) /* 'noexcept' used with no exception    \
                                    handling mode specified; termination on    \
                                    exception is not guaranteed. Specify /EHsc \
                                    */
#endif                          /* _MSC_VER (warnings) */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#	include <windows.h>
#	include <winnt.h>
	typedef unsigned mode_t;
	typedef HANDLE mdbx_filehandle_t;
	typedef DWORD mdbx_pid_t;
	typedef DWORD mdbx_tid_t;
#else
#	include <pthread.h> /* for pthread_t */
#	include <sys/uio.h> /* for truct iovec */
#	include <sys/types.h> /* for pid_t */
#	define HAVE_STRUCT_IOVEC 1
	typedef int mdbx_filehandle_t;
	typedef pid_t mdbx_pid_t;
	typedef pthread_t mdbx_tid_t;
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/* *INDENT-ON* */
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** Library major version */
#define MDBX_VERSION_MAJOR 0
/** Library minor version */
#define MDBX_VERSION_MINOR 2
/** Library patch version */
#define MDBX_VERSION_PATCH 0

/** Combine args a,b,c into a single integer for easy version comparisons */
#define MDB_VERINT(a, b, c) (((a) << 24) | ((b) << 16) | (c))

/** The full library version as a single integer */
#define MDBX_VERSION_FULL                                                      \
  MDB_VERINT(MDBX_VERSION_MAJOR, MDBX_VERSION_MINOR, MDBX_VERSION_PATCH)

/* The release date of this library version */
#define MDBX_VERSION_DATE "DEVEL"

/* A stringifier for the version info */
#define MDBX_VERSTR(a, b, c, d)                                                \
  "MDBX " #a "." #b "." #c ": (" d ", https://github.com/ReOpen/libmdbx)"

/* A helper for the stringifier macro */
#define MDBX_VERFOO(a, b, c, d) MDBX_VERSTR(a, b, c, d)

/* The full library version as a C string */
#define MDBX_VERSION_STRING                                                    \
  MDBX_VERFOO(MDBX_VERSION_MAJOR, MDBX_VERSION_MINOR, MDBX_VERSION_PATCH,      \
              MDBX_VERSION_DATE)

/* Opaque structure for a database environment.
 *
 * A DB environment supports multiple databases, all residing in the same
 * shared-memory map.
 */
typedef struct MDB_env MDB_env;

/* Opaque structure for a transaction handle.
 *
 * All database operations require a transaction handle. Transactions may be
 * read-only or read-write.
 */
typedef struct MDB_txn MDB_txn;

/* A handle for an individual database in the DB environment. */
typedef unsigned MDB_dbi;

/* Opaque structure for navigating through a database */
typedef struct MDB_cursor MDB_cursor;

/* Generic structure used for passing keys and data in and out
 * of the database.
 *
 * Values returned from the database are valid only until a subsequent
 * update operation, or the end of the transaction. Do not modify or
 * free them, they commonly point into the database itself.
 *
 * Key sizes must be between 1 and mdbx_env_get_maxkeysize() inclusive.
 * The same applies to data sizes in databases with the MDB_DUPSORT flag.
 * Other data items can in theory be from 0 to 0xffffffff bytes long.
 */
#ifndef HAVE_STRUCT_IOVEC
struct iovec {
  void *iov_base;
  size_t iov_len;
};
#define HAVE_STRUCT_IOVEC
#endif /* HAVE_STRUCT_IOVEC */

typedef struct iovec MDB_val;
#define mv_size iov_len
#define mv_data iov_base

/* The maximum size of a data item.
 * MDBX only store a 32 bit value for node sizes. */
#define MDBX_MAXDATASIZE INT32_MAX

/* A callback function used to compare two keys in a database */
typedef int(MDB_cmp_func)(const MDB_val *a, const MDB_val *b);

/* Environment Flags */
/* no environment directory */
#define MDB_NOSUBDIR 0x4000u
/* don't fsync after commit */
#define MDB_NOSYNC 0x10000u
/* read only */
#define MDB_RDONLY 0x20000u
/* don't fsync metapage after commit */
#define MDB_NOMETASYNC 0x40000u
/* use writable mmap */
#define MDB_WRITEMAP 0x80000u
/* use asynchronous msync when MDB_WRITEMAP is used */
#define MDB_MAPASYNC 0x100000u
/* tie reader locktable slots to MDB_txn objects instead of to threads */
#define MDB_NOTLS 0x200000u
/* don't do any locking, caller must manage their own locks
 * WARNING: libmdbx don't support this mode. */
#define MDB_NOLOCK__UNSUPPORTED 0x400000u
/* don't do readahead */
#define MDB_NORDAHEAD 0x800000u
/* don't initialize malloc'd memory before writing to datafile */
#define MDB_NOMEMINIT 0x1000000u
/* aim to coalesce FreeDB records */
#define MDBX_COALESCE 0x2000000u
/* LIFO policy for reclaiming FreeDB records */
#define MDBX_LIFORECLAIM 0x4000000u
/* make a steady-sync only on close and explicit env-sync */
#define MDBX_UTTERLY_NOSYNC (MDB_NOSYNC | MDB_MAPASYNC)
/* debuging option, fill/perturb released pages */
#define MDBX_PAGEPERTURB 0x8000000u

/* Database Flags */
/* use reverse string keys */
#define MDB_REVERSEKEY 0x02u
/* use sorted duplicates */
#define MDB_DUPSORT 0x04u
/* numeric keys in native byte order, either unsigned int or mdbx_size_t.
 *	(lmdb expects 32-bit int <= size_t <= 32/64-bit mdbx_size_t.)
 *  The keys must all be of the same size. */
#define MDB_INTEGERKEY 0x08u
/* with MDB_DUPSORT, sorted dup items have fixed size */
#define MDB_DUPFIXED 0x10u
/* with MDB_DUPSORT, dups are MDB_INTEGERKEY-style integers */
#define MDB_INTEGERDUP 0x20u
/* with MDB_DUPSORT, use reverse string dups */
#define MDB_REVERSEDUP 0x40u
/* create DB if not already existing */
#define MDB_CREATE 0x40000u

/* Write Flags */
/* For put: Don't write if the key already exists. */
#define MDB_NOOVERWRITE 0x10u
/* Only for MDB_DUPSORT
 * For put: don't write if the key and data pair already exist.
 * For mdbx_cursor_del: remove all duplicate data items.
 */
#define MDB_NODUPDATA 0x20u
/* For mdbx_cursor_put: overwrite the current key/data pair
 *  MDBX allows this flag for mdbx_put() for explicit overwrite/update without
 * insertion. */
#define MDB_CURRENT 0x40u
/* For put: Just reserve space for data, don't copy it. Return a
 * pointer to the reserved space.
 */
#define MDB_RESERVE 0x10000u
/* Data is being appended, don't split full pages. */
#define MDB_APPEND 0x20000u
/* Duplicate data is being appended, don't split full pages. */
#define MDB_APPENDDUP 0x40000u
/* Store multiple data items in one call. Only for MDB_DUPFIXED. */
#define MDB_MULTIPLE 0x80000u

/* Copy Flags */
/* Compacting copy: Omit free space from copy, and renumber all
 * pages sequentially. */
#define MDB_CP_COMPACT 1u

/* Cursor Get operations.
 *
 *	This is the set of all operations for retrieving data
 *	using a cursor.
 */
typedef enum MDB_cursor_op {
  MDB_FIRST,          /* Position at first key/data item */
  MDB_FIRST_DUP,      /* Position at first data item of current key.
                                              Only for MDB_DUPSORT */
  MDB_GET_BOTH,       /* Position at key/data pair. Only for MDB_DUPSORT */
  MDB_GET_BOTH_RANGE, /* position at key, nearest data. Only for
                         MDB_DUPSORT */
  MDB_GET_CURRENT,    /* Return key/data at current cursor position */
  MDB_GET_MULTIPLE,   /* Return key and up to a page of duplicate data items
                                              from current cursor position. Move
                         cursor to prepare
                                              for MDB_NEXT_MULTIPLE. Only for
                         MDB_DUPFIXED */
  MDB_LAST,           /* Position at last key/data item */
  MDB_LAST_DUP,       /* Position at last data item of current key.
                                              Only for MDB_DUPSORT */
  MDB_NEXT,           /* Position at next data item */
  MDB_NEXT_DUP,       /* Position at next data item of current key.
                                              Only for MDB_DUPSORT */
  MDB_NEXT_MULTIPLE,  /* Return key and up to a page of duplicate data items
                                              from next cursor position. Move
                         cursor to prepare
                                              for MDB_NEXT_MULTIPLE. Only for
                         MDB_DUPFIXED */
  MDB_NEXT_NODUP,     /* Position at first data item of next key */
  MDB_PREV,           /* Position at previous data item */
  MDB_PREV_DUP,       /* Position at previous data item of current key.
                                              Only for MDB_DUPSORT */
  MDB_PREV_NODUP,     /* Position at last data item of previous key */
  MDB_SET,            /* Position at specified key */
  MDB_SET_KEY,        /* Position at specified key, return key + data */
  MDB_SET_RANGE,    /* Position at first key greater than or equal to specified
                       key. */
  MDB_PREV_MULTIPLE /* Position at previous page and return key and up to
                                            a page of duplicate data items.
                       Only for MDB_DUPFIXED */
} MDB_cursor_op;

/* Return Codes
 * BerkeleyDB uses -30800 to -30999, we'll go under them */

/* Successful result */
#define MDB_SUCCESS 0
#define MDBX_RESULT_FALSE MDB_SUCCESS
#define MDBX_RESULT_TRUE (-1)

/* key/data pair already exists */
#define MDB_KEYEXIST (-30799)
/* key/data pair not found (EOF) */
#define MDB_NOTFOUND (-30798)
/* Requested page not found - this usually indicates corruption */
#define MDB_PAGE_NOTFOUND (-30797)
/* Located page was wrong type */
#define MDB_CORRUPTED (-30796)
/* Update of meta page failed or environment had fatal error */
#define MDB_PANIC (-30795)
/* DB file version mismatch with libmdbx */
#define MDB_VERSION_MISMATCH (-30794)
/* File is not a valid LMDB file */
#define MDB_INVALID (-30793)
/* Environment mapsize reached */
#define MDB_MAP_FULL (-30792)
/* Environment maxdbs reached */
#define MDB_DBS_FULL (-30791)
/* Environment maxreaders reached */
#define MDB_READERS_FULL (-30790)
/* Txn has too many dirty pages */
#define MDB_TXN_FULL (-30788)
/* Cursor stack too deep - internal error */
#define MDB_CURSOR_FULL (-30787)
/* Page has not enough space - internal error */
#define MDB_PAGE_FULL (-30786)
/* Database contents grew beyond environment mapsize */
#define MDB_MAP_RESIZED (-30785)
/* Operation and DB incompatible, or DB type changed. This can mean:
 *	 - The operation expects an MDB_DUPSORT / MDB_DUPFIXED database.
 *	 - Opening a named DB when the unnamed DB has
 *MDB_DUPSORT/MDB_INTEGERKEY.
 *	 - Accessing a data record as a database, or vice versa.
 *	 - The database was dropped and recreated with different flags.
 */
#define MDB_INCOMPATIBLE (-30784)
/* Invalid reuse of reader locktable slot */
#define MDB_BAD_RSLOT (-30783)
/* Transaction must abort, has a child, or is invalid */
#define MDB_BAD_TXN (-30782)
/* Unsupported size of key/DB name/data, or wrong DUPFIXED size */
#define MDB_BAD_VALSIZE (-30781)
/* The specified DBI was changed unexpectedly */
#define MDB_BAD_DBI (-30780)
/* Unexpected problem - txn should abort */
#define MDB_PROBLEM (-30779)
/* The last defined error code */
#define MDB_LAST_ERRCODE MDB_PROBLEM

/* The mdbx_put() or mdbx_replace() was called for key,
    that has more that one associated value. */
#define MDBX_EMULTIVAL (-30421)

/* Bad signature of a runtime object(s), this can mean:
 *  - memory corruption or double-free;
 *  - ABI version mismatch (rare case); */
#define MDBX_EBADSIGN (-30420)

/* Statistics for a database in the environment */
typedef struct MDBX_stat {
  unsigned ms_psize;        /* Size of a database page.
                               This is currently the
                               same for all databases. */
  unsigned ms_depth;        /* Depth (height) of the B-tree */
  size_t ms_branch_pages;   /* Number of internal (non-leaf) pages */
  size_t ms_leaf_pages;     /* Number of leaf pages */
  size_t ms_overflow_pages; /* Number of overflow pages */
  size_t ms_entries;        /* Number of data items */
} MDBX_stat;

/* Information about the environment */
typedef struct MDBX_envinfo {
  void *me_mapaddr;       /* Address of map, if fixed */
  size_t me_mapsize;      /* Size of the data memory map */
  size_t me_last_pgno;    /* ID of the last used page */
  uint64_t me_last_txnid; /* ID of the last committed transaction */
  unsigned me_maxreaders; /* max reader slots in the environment */
  unsigned me_numreaders; /* max reader slots used in the environment */
  uint64_t me_tail_txnid; /* ID of the last reader transaction */
  uint64_t me_meta1_txnid, me_meta1_sign;
  uint64_t me_meta2_txnid, me_meta2_sign;
} MDBX_envinfo;

/* Return the LMDB library version information.
 *
 * [out] major if non-NULL, the library major version number is copied
 * here
 * [out] minor if non-NULL, the library minor version number is copied
 * here
 * [out] patch if non-NULL, the library patch version number is copied
 * here
 * Returns "version string" The library version as a string
 */
LIBMDBX_API const char *mdbx_version(int *major, int *minor, int *patch);

/* Return a string describing a given error code.
 *
 * This function is a superset of the ANSI C X3.159-1989 (ANSI C) strerror(3)
 * function. If the error code is greater than or equal to 0, then the string
 * returned by the system function strerror(3) is returned. If the error code
 * is less than 0, an error string corresponding to the LMDB library error is
 * returned. See errors for a list of LMDB-specific error codes.
 * [in] err The error code
 * Returns "error message" The description of the error
 */
LIBMDBX_API const char *mdbx_strerror(int errnum);
LIBMDBX_API const char *mdbx_strerror_r(int errnum, char *buf, size_t buflen);

/* Create an LMDB environment handle.
 *
 * This function allocates memory for a MDB_env structure. To release
 * the allocated memory and discard the handle, call mdbx_env_close().
 * Before the handle may be used, it must be opened using mdbx_env_open().
 * Various other options may also need to be set before opening the handle,
 * e.g. mdbx_env_set_mapsize(), mdbx_env_set_maxreaders(),
 * mdbx_env_set_maxdbs(),
 * depending on usage requirements.
 * [out] env The address where the new handle will be stored
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_create(MDB_env **env);

/* Open an environment handle.
 *
 * If this function fails, mdbx_env_close() must be called to discard the
 *MDB_env handle.
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] path The directory in which the database files reside. This
 * directory must already exist and be writable.
 * [in] flags Special options for this environment. This parameter
 * must be set to 0 or by bitwise OR'ing together one or more of the
 * values described here.
 * Flags set by mdbx_env_set_flags() are also used.
 *	 - MDB_NOSUBDIR
 *		By default, LMDB creates its environment in a directory whose
 *		pathname is given in \b path, and creates its data and lock
 *files
 *		under that directory. With this option, \b path is used as-is
 *for
 *		the database main data file. The database lock file is the \b
 *path
 *		with "-lock" appended.
 *	 - MDB_RDONLY
 *		Open the environment in read-only mode. No write operations will
 *be
 *		allowed. LMDB will still modify the lock file - except on
 *read-only
 *		filesystems, where LMDB does not use locks.
 *	 - MDB_WRITEMAP
 *		Use a writeable memory map unless MDB_RDONLY is set. This uses
 *		fewer mallocs but loses protection from application bugs
 *		like wild pointer writes and other bad updates into the
 *database.
 *		This may be slightly faster for DBs that fit entirely in RAM,
 *but
 *		is slower for DBs larger than RAM.
 *		Incompatible with nested transactions.
 *		Do not mix processes with and without MDB_WRITEMAP on the same
 *		environment.  This can defeat durability (mdbx_env_sync etc).
 *	 - MDB_NOMETASYNC
 *		Flush system buffers to disk only once per transaction, omit
 *the
 *		metadata flush. Defer that until the system flushes files to
 *disk,
 *		or next non-MDB_RDONLY commit or mdbx_env_sync(). This
 *optimization
 *		maintains database integrity, but a system crash may undo the
 *last
 *		committed transaction. I.e. it preserves the ACI (atomicity,
 *		consistency, isolation) but not D (durability) database
 *property.
 *		This flag may be changed at any time using
 *mdbx_env_set_flags().
 *	 - MDB_NOSYNC
 *		Don't flush system buffers to disk when committing a
 *transaction.
 *		This optimization means a system crash can corrupt the database
 *or
 *		lose the last transactions if buffers are not yet flushed to
 *disk.
 *		The risk is governed by how often the system flushes dirty
 *buffers
 *		to disk and how often mdbx_env_sync() is called.  However, if
 *the
 *		filesystem preserves write order and the MDB_WRITEMAP flag is
 *not
 *		used, transactions exhibit ACI (atomicity, consistency,
 *isolation)
 *		properties and only lose D (durability).  I.e. database
 *integrity
 *		is maintained, but a system crash may undo the final
 *transactions.
 *		Note that (MDB_NOSYNC | MDB_WRITEMAP) leaves the system with
 *no
 *		hint for when to write transactions to disk, unless
 *mdbx_env_sync()
 *		is called. (MDB_MAPASYNC | MDB_WRITEMAP) may be preferable.
 *		This flag may be changed at any time using
 *mdbx_env_set_flags().
 *	 - MDB_MAPASYNC
 *		When using MDB_WRITEMAP, use asynchronous flushes to disk.
 *		As with MDB_NOSYNC, a system crash can then corrupt the
 *		database or lose the last transactions. Calling
 *mdbx_env_sync()
 *		ensures on-disk database integrity until next commit.
 *		This flag may be changed at any time using
 *mdbx_env_set_flags().
 *	 - MDB_NOTLS
 *		Don't use Thread-Local Storage. Tie reader locktable slots to
 *		MDB_txn objects instead of to threads. I.e. mdbx_txn_reset()
 *keeps
 *		the slot reseved for the MDB_txn object. A thread may use
 *parallel
 *		read-only transactions. A read-only transaction may span threads
 *if
 *		the user synchronizes its use. Applications that multiplex
 *many
 *		user threads over individual OS threads need this option. Such
 *an
 *		application must also serialize the write transactions in an
 *OS
 *		thread, since LMDB's write locking is unaware of the user
 *threads.
 *	 - MDB_NOLOCK
 *		Don't do any locking. If concurrent access is anticipated, the
 *		caller must manage all concurrency itself. For proper
 *operation
 *		the caller must enforce single-writer semantics, and must
 *ensure
 *		that no readers are using old transactions while a writer is
 *		active. The simplest approach is to use an exclusive lock so
 *that
 *		no readers may be active at all when a writer begins.
 *	 - MDB_NORDAHEAD
 *		Turn off readahead. Most operating systems perform readahead
 *on
 *		read requests by default. This option turns it off if the OS
 *		supports it. Turning it off may help random read performance
 *		when the DB is larger than RAM and system RAM is full.
 *	 - MDB_NOMEMINIT
 *		Don't initialize malloc'd memory before writing to unused
 *spaces
 *		in the data file. By default, memory for pages written to the
 *data
 *		file is obtained using malloc. While these pages may be reused
 *in
 *		subsequent transactions, freshly malloc'd pages will be
 *initialized
 *		to zeroes before use. This avoids persisting leftover data from
 *other
 *		code (that used the heap and subsequently freed the memory) into
 *the
 *		data file. Note that many other system libraries may allocate
 *		and free memory from the heap for arbitrary uses. E.g., stdio
 *may
 *		use the heap for file I/O buffers. This initialization step has
 *a
 *		modest performance cost so some applications may want to
 *disable
 *		it using this flag. This option can be a problem for
 *applications
 *		which handle sensitive data like passwords, and it makes
 *memory
 *		checkers like Valgrind noisy. This flag is not needed with
 *MDB_WRITEMAP,
 *		which writes directly to the mmap instead of using malloc for
 *pages. The
 *		initialization is also skipped if MDB_RESERVE is used; the
 *		caller is expected to overwrite all of the memory that was
 *		reserved in that case.
 *		This flag may be changed at any time using
 *mdbx_env_set_flags().
 *   - #MDBX_COALESCE
 *		Aim to coalesce records while reclaiming FreeDB.
 *		This flag may be changed at any time using
 *mdbx_env_set_flags().
 *   - #MDBX_LIFORECLAIM
 *		LIFO policy for reclaiming FreeDB records. This significantly
 *reduce
 *		write IPOS in case MDB_NOSYNC with periodically checkpoints.
 * [in] mode The UNIX permissions to set on created files and
 *semaphores.
 * This parameter is ignored on Windows.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_VERSION_MISMATCH - the version of the LMDB library doesn't
 *match the
 *	version that created the database environment.
 *	 - MDB_INVALID - the environment file headers are corrupted.
 *	 - ENOENT - the directory specified by the path parameter doesn't
 *exist.
 *	 - EACCES - the user didn't have permission to access the environment
 *files.
 *	 - EAGAIN - the environment was locked by another process.
 */
LIBMDBX_API int mdbx_env_open(MDB_env *env, const char *path, unsigned flags,
                              mode_t mode);
LIBMDBX_API int mdbx_env_open_ex(MDB_env *env, const char *path, unsigned flags,
                                 mode_t mode, int *exclusive);

/* Copy an LMDB environment to the specified path.
 *
 * This function may be used to make a backup of an existing environment.
 * No lockfile is created, since it gets recreated at need.
 * Note: This call can trigger significant file size growth if run in
 * parallel with write transactions, because it employs a read-only
 * transaction. See long-lived transactions under caveats_sec.
 * [in] env An environment handle returned by mdbx_env_create(). It
 * must have already been opened successfully.
 * [in] path The directory in which the copy will reside. This
 * directory must already exist and be writable but must otherwise be
 * empty.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_copy(MDB_env *env, const char *path);

/* Copy an LMDB environment to the specified file descriptor.
 *
 * This function may be used to make a backup of an existing environment.
 * No lockfile is created, since it gets recreated at need.
 * Note: This call can trigger significant file size growth if run in
 * parallel with write transactions, because it employs a read-only
 * transaction. See long-lived transactions under caveats_sec.
 * [in] env An environment handle returned by mdbx_env_create(). It
 * must have already been opened successfully.
 * [in] fd The filedescriptor to write the copy to. It must
 * have already been opened for Write access.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_copyfd(MDB_env *env, mdbx_filehandle_t fd);

/* Copy an LMDB environment to the specified path, with options.
 *
 * This function may be used to make a backup of an existing environment.
 * No lockfile is created, since it gets recreated at need.
 * Note: This call can trigger significant file size growth if run in
 * parallel with write transactions, because it employs a read-only
 * transaction. See long-lived transactions under caveats_sec.
 * [in] env An environment handle returned by mdbx_env_create(). It
 * must have already been opened successfully.
 * [in] path The directory in which the copy will reside. This
 * directory must already exist and be writable but must otherwise be
 * empty.
 * [in] flags Special options for this operation. This parameter
 * must be set to 0 or by bitwise OR'ing together one or more of the
 * values described here.
 *	 - MDB_CP_COMPACT - Perform compaction while copying: omit free
 *		pages and sequentially renumber all pages in output. This
 *option
 *		consumes more CPU and runs more slowly than the default.
 *		Currently it fails if the environment has suffered a page
 *leak.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_copy2(MDB_env *env, const char *path, unsigned flags);

/* Copy an LMDB environment to the specified file descriptor,
 *	with options.
 *
 * This function may be used to make a backup of an existing environment.
 * No lockfile is created, since it gets recreated at need. See
 * mdbx_env_copy2() for further details.
 * Note: This call can trigger significant file size growth if run in
 * parallel with write transactions, because it employs a read-only
 * transaction. See long-lived transactions under caveats_sec.
 * [in] env An environment handle returned by mdbx_env_create(). It
 * must have already been opened successfully.
 * [in] fd The filedescriptor to write the copy to. It must
 * have already been opened for Write access.
 * [in] flags Special options for this operation.
 * See mdbx_env_copy2() for options.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_copyfd2(MDB_env *env, mdbx_filehandle_t fd,
                                 unsigned flags);

/* Return statistics about the LMDB environment.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [out] stat The address of an MDB_stat structure
 * 	where the statistics will be copied
 */
LIBMDBX_API int mdbx_env_stat(MDB_env *env, MDBX_stat *stat, size_t bytes);

/* Return information about the LMDB environment.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [out] stat The address of an MDB_envinfo structure
 * 	where the information will be copied
 */
LIBMDBX_API int mdbx_env_info(MDB_env *env, MDBX_envinfo *info, size_t bytes);

/* Flush the data buffers to disk.
 *
 * Data is always written to disk when mdbx_txn_commit() is called,
 * but the operating system may keep it buffered. LMDB always flushes
 * the OS buffers upon commit as well, unless the environment was
 * opened with MDB_NOSYNC or in part MDB_NOMETASYNC. This call is
 * not valid if the environment was opened with MDB_RDONLY.
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] force If non-zero, force a synchronous flush.  Otherwise
 *  if the environment has the MDB_NOSYNC flag set the flushes
 *	will be omitted, and with MDB_MAPASYNC they will be asynchronous.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EACCES - the environment is read-only.
 *	 - EINVAL - an invalid parameter was specified.
 *	 - EIO - an error occurred during synchronization.
 */
LIBMDBX_API int mdbx_env_sync(MDB_env *env, int force);

/* Close the environment and release the memory map.
 *
 * Only a single thread may call this function. All transactions, databases,
 * and cursors must already be closed before calling this function. Attempts
 * to
 * use any such handles after calling this function will cause a SIGSEGV.
 * The environment handle will be freed and must not be used again after this
 * call.
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] dont_sync A dont'sync flag, if non-zero the last checkpoint
 *  (meta-page update) will be kept "as is" and may be still "weak"
 *  in NOSYNC/MAPASYNC modes. Such "weak" checkpoint will be ignored
 *  on opening next time, and transactions since the last non-weak
 *  checkpoint (meta-page update) will rolledback for consistency guarantee.
 */
LIBMDBX_API void mdbx_env_close(MDB_env *env);

/* Set environment flags.
 *
 * This may be used to set some flags in addition to those from
 * mdbx_env_open(), or to unset these flags.  If several threads
 * change the flags at the same time, the result is undefined.
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] flags The flags to change, bitwise OR'ed together
 * [in] onoff A non-zero value sets the flags, zero clears them.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_env_set_flags(MDB_env *env, unsigned flags, int onoff);

/* Get environment flags.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [out] flags The address of an integer to store the flags
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_env_get_flags(MDB_env *env, unsigned *flags);

/* Return the path that was used in mdbx_env_open().
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [out] path Address of a string pointer to contain the path. This
 * is the actual string in the environment, not a copy. It should not be
 * altered in any way.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_env_get_path(MDB_env *env, const char **path);

/* Return the filedescriptor for the given environment.
 *
 * This function may be called after fork(), so the descriptor can be
 * closed before exec*().  Other LMDB file descriptors have FD_CLOEXEC.
 * (Until LMDB 0.9.18, only the lockfile had that.)
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [out] fd Address of a int to contain the descriptor.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_env_get_fd(MDB_env *env, mdbx_filehandle_t *fd);

/* Set the size of the memory map to use for this environment.
 *
 * The size should be a multiple of the OS page size. The default is
 * 10485760 bytes. The size of the memory map is also the maximum size
 * of the database. The value should be chosen as large as possible,
 * to accommodate future growth of the database.
 * This function should be called after mdbx_env_create() and before
 *mdbx_env_open().
 * It may be called at later times if no transactions are active in
 * this process. Note that the library does not check for this condition,
 * the caller must ensure it explicitly.
 *
 * The new size takes effect immediately for the current process but
 * will not be persisted to any others until a write transaction has been
 * committed by the current process. Also, only mapsize increases are
 * persisted into the environment.
 *
 * If the mapsize is increased by another process, and data has grown
 * beyond the range of the current mapsize, mdbx_txn_begin() will
 * return MDB_MAP_RESIZED. This function may be called with a size
 * of zero to adopt the new size.
 *
 * Any attempt to set a size smaller than the space already consumed
 * by the environment will be silently changed to the current size of the used
 *space.
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] size The size in bytes
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified, or the environment
 *has
 *   	an active write transaction.
 */
LIBMDBX_API int mdbx_env_set_mapsize(MDB_env *env, size_t size);

/* Set the maximum number of threads/reader slots for the environment.
 *
 * This defines the number of slots in the lock table that is used to track
 *readers in the
 * the environment. The default is 126.
 * Starting a read-only transaction normally ties a lock table slot to the
 * current thread until the environment closes or the thread exits. If
 * MDB_NOTLS is in use, mdbx_txn_begin() instead ties the slot to the
 * MDB_txn object until it or the MDB_env object is destroyed.
 * This function may only be called after mdbx_env_create() and before
 *mdbx_env_open().
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] readers The maximum number of reader lock table slots
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified, or the environment is
 *already open.
 */
LIBMDBX_API int mdbx_env_set_maxreaders(MDB_env *env, unsigned readers);

/* Get the maximum number of threads/reader slots for the environment.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [out] readers Address of an integer to store the number of readers
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_env_get_maxreaders(MDB_env *env, unsigned *readers);

/* Set the maximum number of named databases for the environment.
 *
 * This function is only needed if multiple databases will be used in the
 * environment. Simpler applications that use the environment as a single
 * unnamed database can ignore this option.
 * This function may only be called after mdbx_env_create() and before
 *mdbx_env_open().
 *
 * Currently a moderate number of slots are cheap but a huge number gets
 * expensive: 7-120 words per transaction, and every mdbx_dbi_open()
 * does a linear search of the opened slots.
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] dbs The maximum number of databases
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified, or the environment is
 *already open.
 */
LIBMDBX_API int mdbx_env_set_maxdbs(MDB_env *env, MDB_dbi dbs);

/* Get the maximum size of keys and MDB_DUPSORT data we can write.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * Returns The maximum size of a key we can write
 */
LIBMDBX_API int mdbx_env_get_maxkeysize(MDB_env *env);
LIBMDBX_API int mdbx_get_maxkeysize(size_t pagesize);

/* Set application information associated with the MDB_env.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] ctx An arbitrary pointer for whatever the application needs.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_set_userctx(MDB_env *env, void *ctx);

/* Get the application information associated with the MDB_env.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * Returns The pointer set by mdbx_env_set_userctx().
 */
LIBMDBX_API void *mdbx_env_get_userctx(MDB_env *env);

/* A callback function for most LMDB assert() failures,
 * called before printing the message and aborting.
 *
 * [in] env An environment handle returned by mdbx_env_create().
 * [in] msg The assertion message, not including newline.
 */
typedef void MDB_assert_func(MDB_env *env, const char *msg,
                             const char *function, unsigned line);

/* Set or reset the assert() callback of the environment.
 * Disabled if liblmdb is buillt with MDB_DEBUG=0.
 * Note: This hack should become obsolete as lmdb's error handling matures.
 * [in] env An environment handle returned by mdbx_env_create().
 * [in] func An MDB_assert_func function, or 0.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_set_assert(MDB_env *env, MDB_assert_func *func);

/* Create a transaction for use with the environment.
 *
 * The transaction handle may be discarded using mdbx_txn_abort() or
 *mdbx_txn_commit().
 * Note: A transaction and its cursors must only be used by a single
 * thread, and a thread may only have a single transaction at a time.
 * If MDB_NOTLS is in use, this does not apply to read-only transactions.
 * Note: Cursors may not span transactions.
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] parent If this parameter is non-NULL, the new transaction
 * will be a nested transaction, with the transaction indicated by \b parent
 * as its parent. Transactions may be nested to any level. A parent
 * transaction and its cursors may not issue any other operations than
 * mdbx_txn_commit and mdbx_txn_abort while it has active child transactions.
 * [in] flags Special options for this transaction. This parameter
 * must be set to 0 or by bitwise OR'ing together one or more of the
 * values described here.
 *	 - MDB_RDONLY
 *		This transaction will not perform any write operations.
 * [out] txn Address where the new MDB_txn handle will be stored
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_PANIC - a fatal error occurred earlier and the environment
 *		must be shut down.
 *	 - MDB_MAP_RESIZED - another process wrote data beyond this
 *MDB_env's
 *		mapsize and this environment's map must be resized as well.
 *		See mdbx_env_set_mapsize().
 *	 - MDB_READERS_FULL - a read-only transaction was requested and
 *		the reader lock table is full. See mdbx_env_set_maxreaders().
 *	 - ENOMEM - out of memory.
 */
LIBMDBX_API int mdbx_txn_begin(MDB_env *env, MDB_txn *parent, unsigned flags,
                               MDB_txn **txn);

/* Returns the transaction's MDB_env
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 */
LIBMDBX_API MDB_env *mdbx_txn_env(MDB_txn *txn);

/* Return the transaction's ID.
 *
 * This returns the identifier associated with this transaction. For a
 * read-only transaction, this corresponds to the snapshot being read;
 * concurrent readers will frequently have the same transaction ID.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * Returns A transaction ID, valid if input is an active transaction.
 */
LIBMDBX_API size_t mdbx_txn_id(MDB_txn *txn);

/* Commit all the operations of a transaction into the database.
 *
 * The transaction handle is freed. It and its cursors must not be used
 * again after this call, except with mdbx_cursor_renew().
 *
 * Note: MDBX-mode:
 * A cursor must be closed explicitly always, before
 * or after its transaction ends. It can be reused with
 * mdbx_cursor_renew() before finally closing it.
 *
 * Note: LMDB-compatible mode:
 * Earlier documentation incorrectly said all cursors would be freed.
 * Only write-transactions free cursors.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 *	 - ENOSPC - no more disk space.
 *	 - EIO - a low-level I/O error occurred while writing.
 *	 - ENOMEM - out of memory.
 */
LIBMDBX_API int mdbx_txn_commit(MDB_txn *txn);

/* Abandon all the operations of the transaction instead of saving
 * them.
 *
 * The transaction handle is freed. It and its cursors must not be used
 * again after this call, except with mdbx_cursor_renew().
 *
 * Note: MDBX-mode:
 * A cursor must be closed explicitly always, before
 * or after its transaction ends. It can be reused with
 * mdbx_cursor_renew() before finally closing it.
 *
 * Note: LMDB-compatible mode:
 * Earlier documentation incorrectly said all cursors would be freed.
 * Only write-transactions free cursors.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 */
LIBMDBX_API int mdbx_txn_abort(MDB_txn *txn);

/* Reset a read-only transaction.
 *
 * Abort the transaction like mdbx_txn_abort(), but keep the transaction
 * handle. mdbx_txn_renew() may reuse the handle. This saves allocation
 * overhead if the process will start a new read-only transaction soon,
 * and also locking overhead if MDB_NOTLS is in use. The reader table
 * lock is released, but the table slot stays tied to its thread or
 * MDB_txn. Use mdbx_txn_abort() to discard a reset handle, and to free
 * its lock table slot if MDB_NOTLS is in use.
 * Cursors opened within the transaction must not be used
 * again after this call, except with mdbx_cursor_renew().
 * Reader locks generally don't interfere with writers, but they keep old
 * versions of database pages allocated. Thus they prevent the old pages
 * from being reused when writers commit new data, and so under heavy load
 * the database size may grow much more rapidly than otherwise.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 */
LIBMDBX_API int mdbx_txn_reset(MDB_txn *txn);

/* Renew a read-only transaction.
 *
 * This acquires a new reader lock for a transaction handle that had been
 * released by mdbx_txn_reset(). It must be called before a reset transaction
 * may be used again.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_PANIC - a fatal error occurred earlier and the environment
 *		must be shut down.
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_txn_renew(MDB_txn *txn);

/* Open a database in the environment.
 * A database handle denotes the name and parameters of a database,
 * independently of whether such a database exists.
 * The database handle may be discarded by calling mdbx_dbi_close().
 * The old database handle is returned if the database was already open.
 * The handle may only be closed once.
 *
 * The database handle will be private to the current transaction until
 * the transaction is successfully committed. If the transaction is
 * aborted the handle will be closed automatically.
 * After a successful commit the handle will reside in the shared
 * environment, and may be used by other transactions.
 *
 * This function must not be called from multiple concurrent
 * transactions in the same process. A transaction that uses
 * this function must finish (either commit or abort) before
 * any other transaction in the process may use this function.
 *
 * To use named databases (with name != NULL), mdbx_env_set_maxdbs()
 * must be called before opening the environment.  Database names are
 * keys in the unnamed database, and may be read but not written.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] name The name of the database to open. If only a single
 * 	database is needed in the environment, this value may be NULL.
 * [in] flags Special options for this database. This parameter
 * must be set to 0 or by bitwise OR'ing together one or more of the
 * values described here.
 *	 - MDB_REVERSEKEY
 *		Keys are strings to be compared in reverse order, from the end
 *		of the strings to the beginning. By default, Keys are treated as
 *strings and
 *		compared from beginning to end.
 *	 - MDB_DUPSORT
 *		Duplicate keys may be used in the database. (Or, from another
 *perspective,
 *		keys may have multiple data items, stored in sorted order.) By
 *default
 *		keys must be unique and may have only a single data item.
 *	 - MDB_INTEGERKEY
 *		Keys are binary integers in native byte order, either unsigned
 *int
 *		or mdbx_size_t, and will be sorted as such.
 *		(lmdb expects 32-bit int <= size_t <= 32/64-bit mdbx_size_t.)
 *		The keys must all be of the same size.
 *	 - MDB_DUPFIXED
 *		This flag may only be used in combination with MDB_DUPSORT.
 *This option
 *		tells the library that the data items for this database are all
 *the same
 *		size, which allows further optimizations in storage and
 *retrieval. When
 *		all data items are the same size, the MDB_GET_MULTIPLE,
 *MDB_NEXT_MULTIPLE
 *		and MDB_PREV_MULTIPLE cursor operations may be used to retrieve
 *multiple
 *		items at once.
 *	 - MDB_INTEGERDUP
 *		This option specifies that duplicate data items are binary
 *integers,
 *		similar to MDB_INTEGERKEY keys.
 *	 - MDB_REVERSEDUP
 *		This option specifies that duplicate data items should be
 *compared as
 *		strings in reverse order.
 *	 - MDB_CREATE
 *		Create the named database if it doesn't exist. This option is
 *not
 *		allowed in a read-only transaction or a read-only environment.
 * [out] dbi Address where the new MDB_dbi handle will be stored
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_NOTFOUND - the specified database doesn't exist in the
 *environment
 *		and MDB_CREATE was not specified.
 *	 - MDB_DBS_FULL - too many databases have been opened. See
 *mdbx_env_set_maxdbs().
 */
LIBMDBX_API int mdbx_dbi_open(MDB_txn *txn, const char *name, unsigned flags,
                              MDB_dbi *dbi);

/* Retrieve statistics for a database.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [out] stat The address of an MDB_stat structure
 * 	where the statistics will be copied
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_dbi_stat(MDB_txn *txn, MDB_dbi dbi, MDBX_stat *stat,
                              size_t bytes);

/* Retrieve the DB flags for a database handle.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [out] flags Address where the flags will be returned.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_dbi_flags(MDB_txn *txn, MDB_dbi dbi, unsigned *flags);

/* Close a database handle. Normally unnecessary. Use with care:
 *
 * This call is not mutex protected. Handles should only be closed by
 * a single thread, and only if no other threads are going to reference
 * the database handle or one of its cursors any further. Do not close
 * a handle if an existing transaction has modified its database.
 * Doing so can cause misbehavior from database corruption to errors
 * like MDB_BAD_VALSIZE (since the DB name is gone).
 *
 * Closing a database handle is not necessary, but lets mdbx_dbi_open()
 * reuse the handle value.  Usually it's better to set a bigger
 * mdbx_env_set_maxdbs(), unless that value would be large.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 */
LIBMDBX_API void mdbx_dbi_close(MDB_env *env, MDB_dbi dbi);

/* Empty or delete+close a database.
 *
 * See mdbx_dbi_close() for restrictions about closing the DB handle.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] del 0 to empty the DB, 1 to delete it from the
 * environment and close the DB handle.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_drop(MDB_txn *txn, MDB_dbi dbi, int del);

/* Set a custom key comparison function for a database.
 *
 * The comparison function is called whenever it is necessary to compare a
 * key specified by the application with a key currently stored in the
 *database.
 * If no comparison function is specified, and no special key flags were
 *specified
 * with mdbx_dbi_open(), the keys are compared lexically, with shorter keys
 *collating
 * before longer keys.
 * Warning: This function must be called before any data access functions are
 *used,
 * otherwise data corruption may occur. The same comparison function must be
 *used by every
 * program accessing the database, every time the database is used.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] cmp A MDB_cmp_func function
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_set_compare(MDB_txn *txn, MDB_dbi dbi, MDB_cmp_func *cmp);

/* Set a custom data comparison function for a MDB_DUPSORT database.
 *
 * This comparison function is called whenever it is necessary to compare a
 *data
 * item specified by the application with a data item currently stored in the
 *database.
 * This function only takes effect if the database was opened with the
 *MDB_DUPSORT
 * flag.
 * If no comparison function is specified, and no special key flags were
 *specified
 * with mdbx_dbi_open(), the data items are compared lexically, with shorter
 *items collating
 * before longer items.
 * Warning: This function must be called before any data access functions are
 *used,
 * otherwise data corruption may occur. The same comparison function must be
 *used by every
 * program accessing the database, every time the database is used.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] cmp A MDB_cmp_func function
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_set_dupsort(MDB_txn *txn, MDB_dbi dbi, MDB_cmp_func *cmp);

/* Get items from a database.
 *
 * This function retrieves key/data pairs from the database. The address
 * and length of the data associated with the specified \b key are returned
 * in the structure to which \b data refers.
 * If the database supports duplicate keys (MDB_DUPSORT) then the
 * first data item for the key will be returned. Retrieval of other
 * items requires the use of mdbx_cursor_get().
 *
 * Note: The memory pointed to by the returned values is owned by the
 * database. The caller need not dispose of the memory, and may not
 * modify it in any way. For values returned in a read-only transaction
 * any modification attempts will cause a SIGSEGV.
 * Note: Values returned from the database are valid only until a
 * subsequent update operation, or the end of the transaction.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] key The key to search for in the database
 * [out] data The data corresponding to the key
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_NOTFOUND - the key was not in the database.
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_get(MDB_txn *txn, MDB_dbi dbi, MDB_val *key,
                         MDB_val *data);

/* Store items into a database.
 *
 * This function stores key/data pairs in the database. The default behavior
 * is to enter the new key/data pair, replacing any previously existing key
 * if duplicates are disallowed, or adding a duplicate data item if
 * duplicates are allowed (MDB_DUPSORT).
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] key The key to store in the database
 * [in,out] data The data to store
 * [in] flags Special options for this operation. This parameter
 * must be set to 0 or by bitwise OR'ing together one or more of the
 * values described here.
 *	 - MDB_NODUPDATA - enter the new key/data pair only if it does not
 *		already appear in the database. This flag may only be
 *specified
 *		if the database was opened with MDB_DUPSORT. The function
 *will
 *		return MDB_KEYEXIST if the key/data pair already appears in
 *the
 *		database.
 *	 - MDB_NOOVERWRITE - enter the new key/data pair only if the key
 *		does not already appear in the database. The function will
 *return
 *		MDB_KEYEXIST if the key already appears in the database, even
 *if
 *		the database supports duplicates (MDB_DUPSORT). The \b data
 *		parameter will be set to point to the existing item.
 *	 - MDB_RESERVE - reserve space for data of the given size, but
 *		don't copy the given data. Instead, return a pointer to the
 *		reserved space, which the caller can fill in later - before
 *		the next update operation or the transaction ends. This saves
 *		an extra memcpy if the data is being generated later.
 *		LMDB does nothing else with this memory, the caller is
 *expected
 *		to modify all of the space requested. This flag must not be
 *		specified if the database was opened with MDB_DUPSORT.
 *	 - MDB_APPEND - append the given key/data pair to the end of the
 *		database. This option allows fast bulk loading when keys are
 *		already known to be in the correct order. Loading unsorted
 *keys
 *		with this flag will cause a MDB_KEYEXIST error.
 *	 - MDB_APPENDDUP - as above, but for sorted dup data.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_MAP_FULL - the database is full, see mdbx_env_set_mapsize().
 *	 - MDB_TXN_FULL - the transaction has too many dirty pages.
 *	 - EACCES - an attempt was made to write in a read-only transaction.
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_put(MDB_txn *txn, MDB_dbi dbi, MDB_val *key, MDB_val *data,
                         unsigned flags);

/* Delete items from a database.
 *
 * This function removes key/data pairs from the database.
 *
 * MDBX-mode:
 * The data parameter is NOT ignored regardless the database does
 * support sorted duplicate data items or not. If the data parameter
 * is non-NULL only the matching data item will be deleted.
 *
 * LMDB-compatible mode:
 * If the database does not support sorted duplicate data items
 * (MDB_DUPSORT) the data parameter is ignored.
 * If the database supports sorted duplicates and the data parameter
 * is NULL, all of the duplicate data items for the key will be
 * deleted. Otherwise, if the data parameter is non-NULL
 * only the matching data item will be deleted.
 *
 * This function will return MDB_NOTFOUND if the specified key/data
 * pair is not in the database.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] key The key to delete from the database
 * [in] data The data to delete
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EACCES - an attempt was made to write in a read-only transaction.
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_del(MDB_txn *txn, MDB_dbi dbi, MDB_val *key,
                         MDB_val *data);

/* Create a cursor handle.
 *
 * A cursor is associated with a specific transaction and database.
 * A cursor cannot be used when its database handle is closed.  Nor
 * when its transaction has ended, except with mdbx_cursor_renew().
 * It can be discarded with mdbx_cursor_close().
 *
 * MDBX-mode:
 * A cursor must be closed explicitly always, before
 * or after its transaction ends. It can be reused with
 * mdbx_cursor_renew() before finally closing it.
 *
 * LMDB-compatible mode:
 * A cursor in a write-transaction can be closed before its transaction
 * ends, and will otherwise be closed when its transaction ends.
 * A cursor in a read-only transaction must be closed explicitly, before
 * or after its transaction ends. It can be reused with
 * mdbx_cursor_renew() before finally closing it.
 * Note: Earlier documentation said that cursors in every transaction
 * were closed when the transaction committed or aborted.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [out] cursor Address where the new MDB_cursor handle will be stored
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_cursor_open(MDB_txn *txn, MDB_dbi dbi,
                                 MDB_cursor **cursor);

/* Close a cursor handle.
 *
 * The cursor handle will be freed and must not be used again after this call.
 * Its transaction must still be live if it is a write-transaction.
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 */
LIBMDBX_API void mdbx_cursor_close(MDB_cursor *cursor);

/* Renew a cursor handle.
 *
 * A cursor is associated with a specific transaction and database.
 * Cursors that are only used in read-only
 * transactions may be re-used, to avoid unnecessary malloc/free overhead.
 * The cursor may be associated with a new read-only transaction, and
 * referencing the same database handle as it was created with.
 * This may be done whether the previous transaction is live or dead.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_cursor_renew(MDB_txn *txn, MDB_cursor *cursor);

/* Return the cursor's transaction handle.
 *
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 */
LIBMDBX_API MDB_txn *mdbx_cursor_txn(MDB_cursor *cursor);

/* Return the cursor's database handle.
 *
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 */
LIBMDBX_API MDB_dbi mdbx_cursor_dbi(MDB_cursor *cursor);

/* Retrieve by cursor.
 *
 * This function retrieves key/data pairs from the database. The address and
 *length
 * of the key are returned in the object to which \b key refers (except for
 *the
 * case of the MDB_SET option, in which the \b key object is unchanged), and
 * the address and length of the data are returned in the object to which \b
 *data
 * refers.
 * See mdbx_get() for restrictions on using the output values.
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 * [in,out] key The key for a retrieved item
 * [in,out] data The data of a retrieved item
 * [in] op A cursor operation MDB_cursor_op
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_NOTFOUND - no matching key found.
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_cursor_get(MDB_cursor *cursor, MDB_val *key, MDB_val *data,
                                MDB_cursor_op op);

/* Store by cursor.
 *
 * This function stores key/data pairs into the database.
 * The cursor is positioned at the new item, or on failure usually near it.
 * Note: Earlier documentation incorrectly said errors would leave the
 * state of the cursor unchanged.
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 * [in] key The key operated on.
 * [in] data The data operated on.
 * [in] flags Options for this operation. This parameter
 * must be set to 0 or one of the values described here.
 *	 - MDB_CURRENT - replace the item at the current cursor position.
 *		The \b key parameter must still be provided, and must match
 *it.
 *		If using sorted duplicates (MDB_DUPSORT) the data item must
 *still
 *		sort into the same place. This is intended to be used when the
 *		new data is the same size as the old. Otherwise it will simply
 *		perform a delete of the old record followed by an insert.
 *	 - MDB_NODUPDATA - enter the new key/data pair only if it does not
 *		already appear in the database. This flag may only be
 *specified
 *		if the database was opened with MDB_DUPSORT. The function
 *will
 *		return MDB_KEYEXIST if the key/data pair already appears in
 *the
 *		database.
 *	 - MDB_NOOVERWRITE - enter the new key/data pair only if the key
 *		does not already appear in the database. The function will
 *return
 *		MDB_KEYEXIST if the key already appears in the database, even
 *if
 *		the database supports duplicates (MDB_DUPSORT).
 *	 - MDB_RESERVE - reserve space for data of the given size, but
 *		don't copy the given data. Instead, return a pointer to the
 *		reserved space, which the caller can fill in later - before
 *		the next update operation or the transaction ends. This saves
 *		an extra memcpy if the data is being generated later. This
 *flag
 *		must not be specified if the database was opened with
 *MDB_DUPSORT.
 *	 - MDB_APPEND - append the given key/data pair to the end of the
 *		database. No key comparisons are performed. This option allows
 *		fast bulk loading when keys are already known to be in the
 *		correct order. Loading unsorted keys with this flag will cause
 *		a MDB_KEYEXIST error.
 *	 - MDB_APPENDDUP - as above, but for sorted dup data.
 *	 - MDB_MULTIPLE - store multiple contiguous data elements in a
 *		single request. This flag may only be specified if the
 *database
 *		was opened with MDB_DUPFIXED. The \b data argument must be an
 *		array of two MDB_vals. The mv_size of the first MDB_val must
 *be
 *		the size of a single data element. The mv_data of the first
 *MDB_val
 *		must point to the beginning of the array of contiguous data
 *elements.
 *		The mv_size of the second MDB_val must be the count of the
 *number
 *		of data elements to store. On return this field will be set to
 *		the count of the number of elements actually written. The
 *mv_data
 *		of the second MDB_val is unused.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - MDB_MAP_FULL - the database is full, see mdbx_env_set_mapsize().
 *	 - MDB_TXN_FULL - the transaction has too many dirty pages.
 *	 - EACCES - an attempt was made to write in a read-only transaction.
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_cursor_put(MDB_cursor *cursor, MDB_val *key, MDB_val *data,
                                unsigned flags);

/* Delete current key/data pair
 *
 * This function deletes the key/data pair to which the cursor refers.
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 * [in] flags Options for this operation. This parameter
 * must be set to 0 or one of the values described here.
 *	 - MDB_NODUPDATA - delete all of the data items for the current key.
 *		This flag may only be specified if the database was opened with
 *MDB_DUPSORT.
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EACCES - an attempt was made to write in a read-only transaction.
 *	 - EINVAL - an invalid parameter was specified.
 */
LIBMDBX_API int mdbx_cursor_del(MDB_cursor *cursor, unsigned flags);

/* Return count of duplicates for current key.
 *
 * This call is only valid on databases that support sorted duplicate
 * data items MDB_DUPSORT.
 * [in] cursor A cursor handle returned by mdbx_cursor_open()
 * [out] countp Address where the count will be stored
 * Returns A non-zero error value on failure and 0 on success. Some possible
 * errors are:
 *	 - EINVAL - cursor is not initialized, or an invalid parameter was
 *specified.
 */
LIBMDBX_API int mdbx_cursor_count(MDB_cursor *cursor, size_t *countp);

/* Compare two data items according to a particular database.
 *
 * This returns a comparison as if the two data items were keys in the
 * specified database.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] a The first item to compare
 * [in] b The second item to compare
 * Returns < 0 if a < b, 0 if a == b, > 0 if a > b
 */
LIBMDBX_API int mdbx_cmp(MDB_txn *txn, MDB_dbi dbi, const MDB_val *a,
                         const MDB_val *b);

/* Compare two data items according to a particular database.
 *
 * This returns a comparison as if the two items were data items of
 * the specified database. The database must have the MDB_DUPSORT flag.
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [in] dbi A database handle returned by mdbx_dbi_open()
 * [in] a The first item to compare
 * [in] b The second item to compare
 * Returns < 0 if a < b, 0 if a == b, > 0 if a > b
 */
LIBMDBX_API int mdbx_dcmp(MDB_txn *txn, MDB_dbi dbi, const MDB_val *a,
                          const MDB_val *b);

/* A callback function used to print a message from the library.
 *
 * [in] msg The string to be printed.
 * [in] ctx An arbitrary context pointer for the callback.
 * Returns < 0 on failure, >= 0 on success.
 */
typedef int(MDB_msg_func)(const char *msg, void *ctx);

/* Dump the entries in the reader lock table.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] func A MDB_msg_func function
 * [in] ctx Anything the message function needs
 * Returns < 0 on failure, >= 0 on success.
 */
LIBMDBX_API int mdbx_reader_list(MDB_env *env, MDB_msg_func *func, void *ctx);

/* Check for stale entries in the reader lock table.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [out] dead Number of stale slots that were cleared
 * Returns 0 on success, non-zero on failure.
 */
LIBMDBX_API int mdbx_reader_check(MDB_env *env, int *dead);

LIBMDBX_API char *mdbx_dkey(MDB_val *key, char *buf, const size_t bufsize);

LIBMDBX_API int mdbx_env_close_ex(MDB_env *env, int dont_sync);

/* Set threshold to force flush the data buffers to disk,
 * even of MDB_NOSYNC, MDB_NOMETASYNC and MDB_MAPASYNC flags
 * in the environment.
 *
 * Data is always written to disk when mdbx_txn_commit() is called,
 * but the operating system may keep it buffered. LMDB always flushes
 * the OS buffers upon commit as well, unless the environment was
 * opened with MDB_NOSYNC or in part MDB_NOMETASYNC.
 *
 * The default is 0, than mean no any threshold checked,
 * and no additional flush will be made.
 *
 * [in] env An environment handle returned by mdbx_env_create()
 * [in] bytes The size in bytes of summary changes
 * when a synchronous flush would be made.
 * Returns A non-zero error value on failure and 0 on success.
 */
LIBMDBX_API int mdbx_env_set_syncbytes(MDB_env *env, size_t bytes);

/* Returns a lag of the reading.
 *
 * Returns an information for estimate how much given read-only
 * transaction is lagging relative the to actual head.
 *
 * [in] txn A transaction handle returned by mdbx_txn_begin()
 * [out] percent Percentage of page allocation in the database.
 * Returns Number of transactions committed after the given was started for
 * read, or -1 on failure.
 */
LIBMDBX_API int mdbx_txn_straggler(MDB_txn *txn, int *percent);

/* A callback function for killing a laggard readers,
 * but also could waiting ones. Called in case of MDB_MAP_FULL error.
 *
 * [in] env An environment handle returned by mdbx_env_create().
 * [in] pid pid of the reader process.
 * [in] thread_id thread_id of the reader thread.
 * [in] txn Transaction number on which stalled.
 * [in] gap a lag from the last commited txn.
 * [in] retry a retry number, less that zero for notify end of OOM-loop.
 * Returns -1 on failure (reader is not killed),
 * 	0 on a race condition (no such reader),
 * 	1 on success (reader was killed),
 * 	>1 on success (reader was SURE killed).
 */
typedef int(MDBX_oom_func)(MDB_env *env, int pid, mdbx_tid_t thread_id,
                           size_t txn, unsigned gap, int retry);

/* Set the OOM callback.
 *
 * Callback will be called only on out-of-pages case for killing
 * a laggard readers to allowing reclaiming of freeDB.
 *
 * [in] env An environment handle returned by mdbx_env_create().
 * [in] oomfunc A #MDBX_oom_func function or NULL to disable.
 */
LIBMDBX_API void mdbx_env_set_oomfunc(MDB_env *env, MDBX_oom_func *oom_func);

/* Get the current oom_func callback.
 *
 * Callback will be called only on out-of-pages case for killing
 * a laggard readers to allowing reclaiming of freeDB.
 *
 * [in] env An environment handle returned by mdbx_env_create().
 * Returns A #MDBX_oom_func function or NULL if disabled.
 */
LIBMDBX_API MDBX_oom_func *mdbx_env_get_oomfunc(MDB_env *env);

#define MDBX_DBG_ASSERT 1
#define MDBX_DBG_PRINT 2
#define MDBX_DBG_TRACE 4
#define MDBX_DBG_EXTRA 8
#define MDBX_DBG_AUDIT 16
#define MDBX_DBG_EDGE 32

/* LY: a "don't touch" value */
#define MDBX_DBG_DNT (-1L)

typedef void MDBX_debug_func(int type, const char *function, int line,
                             const char *msg, va_list args);

LIBMDBX_API int mdbx_setup_debug(int flags, MDBX_debug_func *logger,
                                 long edge_txn);

typedef int MDBX_pgvisitor_func(size_t pgno, unsigned pgnumber, void *ctx,
                                const char *dbi, const char *type, int nentries,
                                int payload_bytes, int header_bytes,
                                int unused_bytes);
LIBMDBX_API int mdbx_env_pgwalk(MDB_txn *txn, MDBX_pgvisitor_func *visitor,
                                void *ctx);

typedef struct mdbx_canary { uint64_t x, y, z, v; } mdbx_canary;

LIBMDBX_API int mdbx_canary_put(MDB_txn *txn, const mdbx_canary *canary);
LIBMDBX_API int mdbx_canary_get(MDB_txn *txn, mdbx_canary *canary);

/* Returns:
 *	- MDBX_RESULT_TRUE	when no more data available
 *				or cursor not positioned;
 *	- MDBX_RESULT_FALSE	when data available;
 *	- Otherwise the error code. */
LIBMDBX_API int mdbx_cursor_eof(MDB_cursor *mc);

/* Returns: MDBX_RESULT_TRUE, MDBX_RESULT_FALSE or Error code. */
LIBMDBX_API int mdbx_cursor_on_first(MDB_cursor *mc);

/* Returns: MDBX_RESULT_TRUE, MDBX_RESULT_FALSE or Error code. */
LIBMDBX_API int mdbx_cursor_on_last(MDB_cursor *mc);

LIBMDBX_API int mdbx_replace(MDB_txn *txn, MDB_dbi dbi, MDB_val *key,
                             MDB_val *new_data, MDB_val *old_data,
                             unsigned flags);
/* Same as mdbx_get(), but:
 * 1) if values_count is not NULL, then returns the count
 *    of multi-values/duplicates for a given key.
 * 2) updates the key for pointing to the actual key's data inside DB. */
LIBMDBX_API int mdbx_get_ex(MDB_txn *txn, MDB_dbi dbi, MDB_val *key,
                            MDB_val *data, int *values_count);

LIBMDBX_API int mdbx_is_dirty(const MDB_txn *txn, const void *ptr);

LIBMDBX_API int mdbx_dbi_open_ex(MDB_txn *txn, const char *name, unsigned flags,
                                 MDB_dbi *dbi, MDB_cmp_func *keycmp,
                                 MDB_cmp_func *datacmp);

LIBMDBX_API int mdbx_dbi_sequence(MDB_txn *txn, MDB_dbi dbi, uint64_t *result,
                                  uint64_t increment);

#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* _MDBX_H_ */

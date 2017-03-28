/* mtest4.c - memory-mapped database tester/toy */

/*
 * Copyright 2015-2017 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright 2011-2017 Howard Chu, Symas Corp.
 * Copyright 2015,2016 Peter-Service R&D LLC.
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

/* Tests for sorted duplicate DBs with fixed-size keys */
#include "../mdbx.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define E(expr) CHECK((rc = (expr)) == MDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg)                                                       \
  ((test) ? (void)0 : ((void)fprintf(stderr, "%s:%d: %s: %s\n", __FILE__,      \
                                     __LINE__, msg, mdbx_strerror(rc)),        \
                       abort()))

#ifndef DBPATH
#define DBPATH "./tmp.db"
#endif

int main(int argc, char *argv[]) {
  int i = 0, j = 0, rc;
  MDB_env *env;
  MDB_dbi dbi;
  MDB_val key, data;
  MDB_txn *txn;
  MDBX_stat mst;
  MDB_cursor *cursor;
  int count;
  int *values;
  char sval[8];
  char kval[sizeof(int)];
  int env_oflags;
  struct stat db_stat, exe_stat;

  (void)argc;
  (void)argv;
  memset(sval, 0, sizeof(sval));

  count = 510;
  values = (int *)malloc(count * sizeof(int));

  for (i = 0; i < count; i++) {
    values[i] = i * 5;
  }

  E(mdbx_env_create(&env));
  E(mdbx_env_set_mapsize(env, 10485760));
  E(mdbx_env_set_maxdbs(env, 4));

  E(stat("/proc/self/exe", &exe_stat) ? errno : 0);
  E(stat(DBPATH "/.", &db_stat) ? errno : 0);
  env_oflags = MDB_NOSYNC;
  if (major(db_stat.st_dev) != major(exe_stat.st_dev)) {
    /* LY: Assume running inside a CI-environment:
     *  1) don't use FIXEDMAP to avoid EBUSY in case collision,
     *     which could be inspired by address space randomisation feature.
     *  2) drop MDB_NOSYNC expecting that DBPATH is at a tmpfs or some
     * dedicated storage.
     */
    env_oflags = 0;
  }
  E(mdbx_env_open(env, DBPATH, env_oflags, 0664));

  E(mdbx_txn_begin(env, NULL, 0, &txn));
  if (mdbx_dbi_open(txn, "id4", MDB_CREATE, &dbi) == MDB_SUCCESS)
    E(mdbx_drop(txn, dbi, 1));
  E(mdbx_dbi_open(txn, "id4", MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, &dbi));

  key.mv_size = sizeof(int);
  key.mv_data = kval;
  data.mv_size = sizeof(sval);
  data.mv_data = sval;

  printf("Adding %d values\n", count);
  strcpy(kval, "001");
  for (i = 0; i < count; i++) {
    sprintf(sval, "%07x", values[i]);
    if (RES(MDB_KEYEXIST, mdbx_put(txn, dbi, &key, &data, MDB_NODUPDATA)))
      j++;
  }
  if (j)
    printf("%d duplicates skipped\n", j);
  E(mdbx_txn_commit(txn));
  E(mdbx_env_stat(env, &mst, sizeof(mst)));

  /* there should be one full page of dups now.
   */
  E(mdbx_txn_begin(env, NULL, MDB_RDONLY, &txn));
  E(mdbx_cursor_open(txn, dbi, &cursor));
  while ((rc = mdbx_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
    printf("key: %p %.*s, data: %p %.*s\n", key.mv_data, (int)key.mv_size,
           (char *)key.mv_data, data.mv_data, (int)data.mv_size,
           (char *)data.mv_data);
  }
  CHECK(rc == MDB_NOTFOUND, "mdbx_cursor_get");
  mdbx_cursor_close(cursor);
  mdbx_txn_abort(txn);

  /* test all 3 branches of split code:
   * 1: new key in lower half
   * 2: new key at split point
   * 3: new key in upper half
   */

  key.mv_size = sizeof(int);
  key.mv_data = kval;
  data.mv_size = sizeof(sval);
  data.mv_data = sval;

  sprintf(sval, "%07x", values[3] + 1);
  E(mdbx_txn_begin(env, NULL, 0, &txn));
  (void)RES(MDB_KEYEXIST, mdbx_put(txn, dbi, &key, &data, MDB_NODUPDATA));
  mdbx_txn_abort(txn);

  sprintf(sval, "%07x", values[255] + 1);
  E(mdbx_txn_begin(env, NULL, 0, &txn));
  (void)RES(MDB_KEYEXIST, mdbx_put(txn, dbi, &key, &data, MDB_NODUPDATA));
  mdbx_txn_abort(txn);

  sprintf(sval, "%07x", values[500] + 1);
  E(mdbx_txn_begin(env, NULL, 0, &txn));
  (void)RES(MDB_KEYEXIST, mdbx_put(txn, dbi, &key, &data, MDB_NODUPDATA));
  E(mdbx_txn_commit(txn));

  /* Try MDB_NEXT_MULTIPLE */
  E(mdbx_txn_begin(env, NULL, 0, &txn));
  E(mdbx_cursor_open(txn, dbi, &cursor));
  while ((rc = mdbx_cursor_get(cursor, &key, &data, MDB_NEXT_MULTIPLE)) == 0) {
    printf("key: %.*s, data: %.*s\n", (int)key.mv_size, (char *)key.mv_data,
           (int)data.mv_size, (char *)data.mv_data);
  }
  CHECK(rc == MDB_NOTFOUND, "mdbx_cursor_get");
  mdbx_cursor_close(cursor);
  mdbx_txn_abort(txn);
  j = 0;

  for (i = count - 1; i > -1; i -= (rand() % 3)) {
    j++;
    txn = NULL;
    E(mdbx_txn_begin(env, NULL, 0, &txn));
    sprintf(sval, "%07x", values[i]);
    key.mv_size = sizeof(int);
    key.mv_data = kval;
    data.mv_size = sizeof(sval);
    data.mv_data = sval;
    if (RES(MDB_NOTFOUND, mdbx_del(txn, dbi, &key, &data))) {
      j--;
      mdbx_txn_abort(txn);
    } else {
      E(mdbx_txn_commit(txn));
    }
  }
  free(values);
  printf("Deleted %d values\n", j);

  E(mdbx_env_stat(env, &mst, sizeof(mst)));
  E(mdbx_txn_begin(env, NULL, MDB_RDONLY, &txn));
  E(mdbx_cursor_open(txn, dbi, &cursor));
  printf("Cursor next\n");
  while ((rc = mdbx_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
    printf("key: %.*s, data: %.*s\n", (int)key.mv_size, (char *)key.mv_data,
           (int)data.mv_size, (char *)data.mv_data);
  }
  CHECK(rc == MDB_NOTFOUND, "mdbx_cursor_get");
  printf("Cursor prev\n");
  while ((rc = mdbx_cursor_get(cursor, &key, &data, MDB_PREV)) == 0) {
    printf("key: %.*s, data: %.*s\n", (int)key.mv_size, (char *)key.mv_data,
           (int)data.mv_size, (char *)data.mv_data);
  }
  CHECK(rc == MDB_NOTFOUND, "mdbx_cursor_get");
  mdbx_cursor_close(cursor);
  mdbx_txn_abort(txn);

  mdbx_dbi_close(env, dbi);
  mdbx_env_close(env);
  return 0;
}

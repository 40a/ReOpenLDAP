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

#include "test.h"

#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct shared_t {
  pthread_barrier_t barrier;
  pthread_mutex_t mutex;
  pthread_cond_t conds[0];
};

static shared_t *shared;
static std::unordered_map<unsigned, pthread_cond_t *> events;

void osal_wait4barrier(void) {
  assert(shared != nullptr && shared != MAP_FAILED);
  int rc = pthread_barrier_wait(&shared->barrier);
  if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    failure_perror("pthread_barrier_wait(shared)", rc);
  }
}

void osal_setup(const std::vector<actor_config> &actors) {
  assert(shared == nullptr);

  pthread_mutexattr_t mutexattr;
  int rc = pthread_mutexattr_init(&mutexattr);
  if (rc)
    failure_perror("pthread_mutexattr_init()", rc);
  rc = pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
  if (rc)
    failure_perror("pthread_mutexattr_setpshared()", rc);

  pthread_barrierattr_t barrierattr;
  rc = pthread_barrierattr_init(&barrierattr);
  if (rc)
    failure_perror("pthread_barrierattr_init()", rc);
  rc = pthread_barrierattr_setpshared(&barrierattr, PTHREAD_PROCESS_SHARED);
  if (rc)
    failure_perror("pthread_barrierattr_setpshared()", rc);

  pthread_condattr_t condattr;
  rc = pthread_condattr_init(&condattr);
  if (rc)
    failure_perror("pthread_condattr_init()", rc);
  rc = pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
  if (rc)
    failure_perror("pthread_condattr_setpshared()", rc);

  size_t n = 0;
  for (const auto &a : actors)
    if (a.wanna_event4signalling())
      ++n;

  shared = (shared_t *)mmap(
      nullptr, sizeof(shared_t) + n * sizeof(pthread_cond_t),
      PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (MAP_FAILED == (void *)shared)
    failure_perror("mmap(shared_conds)", errno);

  rc = pthread_mutex_init(&shared->mutex, &mutexattr);
  if (rc)
    failure_perror("pthread_mutex_init(shared)", rc);

  rc = pthread_barrier_init(&shared->barrier, &barrierattr, actors.size() + 1);
  if (rc)
    failure_perror("pthread_barrier_init(shared)", rc);

  auto a = actors.begin();
  for (size_t i = 0; i < n; ++i) {
    pthread_cond_t *event = &shared->conds[i];
    rc = pthread_cond_init(event, &condattr);
    if (rc)
      failure_perror("pthread_cond_init(shared)", rc);

    unsigned id = 0;
    while (a != actors.end()) {
      if (a->wanna_event4signalling()) {
        id = a->id;
        break;
      }
      ++a;
    }
    assert(id != 0);
    events[id] = event;
  }

  pthread_barrierattr_destroy(&barrierattr);
  pthread_condattr_destroy(&condattr);
  pthread_mutexattr_destroy(&mutexattr);
}

void osal_broadcast(unsigned id) {
  assert(shared != nullptr && shared != MAP_FAILED);
  int rc = pthread_cond_broadcast(events.at(id));
  if (rc)
    failure_perror("sem_post(shared)", rc);
}

int osal_waitfor(unsigned id) {
  assert(shared != nullptr && shared != MAP_FAILED);

  int rc = pthread_mutex_lock(&shared->mutex);
  if (rc != 0)
    failure_perror("pthread_mutex_lock(shared)", rc);

  rc = pthread_cond_wait(events.at(id), &shared->mutex);
  if (rc && rc != EINTR)
    failure_perror("pthread_cond_wait(shared)", rc);

  rc = pthread_mutex_unlock(&shared->mutex);
  if (rc != 0)
    failure_perror("pthread_mutex_unlock(shared)", rc);

  return (rc == 0) ? true : false;
}

//-----------------------------------------------------------------------------

const std::string
actor_config::osal_serialize(simple_checksum &checksum) const {
  (void)checksum;
  /* not used in workload, but just for testing */
  return "unix.fork";
}

bool actor_config::osal_deserialize(const char *str, const char *end,
                                    simple_checksum &checksum) {
  (void)checksum;
  /* not used in workload, but just for testing */
  return strncmp(str, "unix.fork", 9) == 0 && str + 9 == end;
}

//-----------------------------------------------------------------------------

static std::unordered_map<pid_t, actor_status> childs;

static void handler_SIGCHLD(int unused) { (void)unused; }

mdbx_pid_t osal_getpid(void) { return getpid(); }

int osal_actor_start(const actor_config &config, mdbx_pid_t &pid) {
  if (childs.empty())
    signal(SIGCHLD, handler_SIGCHLD);

  pid = fork();

  if (pid == 0) {
    const bool result = test_execute(config);
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
  }

  if (pid < 0)
    return errno;

  childs[pid] = as_running;
  return 0;
}

actor_status osal_actor_info(const mdbx_pid_t pid) { return childs.at(pid); }

void osal_killall_actors(void) {
  for (auto &pair : childs) {
    kill(pair.first, SIGKILL);
    pair.second = as_killed;
  }
}

int osal_actor_poll(mdbx_pid_t &pid, unsigned timeout) {
retry:
  int status, options = WNOHANG;
#ifdef WUNTRACED
  options |= WUNTRACED;
#endif
#ifdef WCONTINUED
  options |= WCONTINUED;
#endif
  pid = waitpid(0, &status, options);

  if (pid > 0) {
    if (WIFEXITED(status))
      childs[pid] =
          (WEXITSTATUS(status) == EXIT_SUCCESS) ? as_successful : as_failed;
    else if (WIFSIGNALED(status) || WCOREDUMP(status))
      childs[pid] = as_killed;
    else if (WIFSTOPPED(status))
      childs[pid] = as_debuging;
    else if (WIFCONTINUED(status))
      childs[pid] = as_running;
    else {
      assert(false);
    }
    return 0;
  }

  if (pid == 0) {
    if (timeout && sleep(timeout))
      goto retry;
    return 0;
  }

  switch (errno) {
  case EINTR:
    pid = 0;
    return 0;

  case ECHILD:
  default:
    pid = 0;
    return errno;
  }
}

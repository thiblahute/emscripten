/*
 * Copyright 2026 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static int fds[2];

static void *writer(void *arg) {
  write(fds[1], "x", 1);
  return NULL;
}

int main(void) {
  pthread_t t;
  char buf;
  struct pollfd pfd = {.events = POLLIN};

  pipe(fds);
  pfd.fd = fds[0];

  // poll should timeout on an empty pipe
  assert(poll(&pfd, 1, 100) == 0);

  // poll should return immediately when data is already available
  write(fds[1], "a", 1);
  assert(poll(&pfd, 1, 1000) == 1);
  assert(pfd.revents & POLLIN);
  assert(read(fds[0], &buf, 1) == 1 && buf == 'a');

  // poll should wake up from a cross-thread write
  pfd.revents = 0;
  pthread_create(&t, NULL, writer, NULL);
  assert(poll(&pfd, 1, 5000) == 1);
  assert(pfd.revents & POLLIN);
  assert(read(fds[0], &buf, 1) == 1 && buf == 'x');
  pthread_join(t, NULL);

  // ppoll should also timeout on an empty pipe
  struct timespec ts = {0, 200 * 1000000L};
  struct timespec begin, end;
  pfd.revents = 0;
  clock_gettime(CLOCK_MONOTONIC, &begin);
  assert(ppoll(&pfd, 1, &ts, NULL) == 0);
  clock_gettime(CLOCK_MONOTONIC, &end);
  long elapsed_ms = (end.tv_sec - begin.tv_sec) * 1000 +
                    (end.tv_nsec - begin.tv_nsec) / 1000000;
  assert(elapsed_ms >= 195);

  printf("done\n");
}

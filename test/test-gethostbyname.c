/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "../uv.h"
#include "task.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* strlen */

ares_channel channel;
struct ares_options options;
int optmask;

struct in_addr testsrv;

int ares_bynamecallbacks;
int bynamecallbacksig;
int ares_byaddrcallbacks;
int byaddrcallbacksig;

static uv_buf_t alloc_cb(uv_handle_t* handle, size_t size) {
  uv_buf_t buf;
  buf.base = (char*)malloc(size);
  buf.len = size;
  return buf;
}

static void aresbynamecallback( void *arg,
                          int status,
                          int timeouts,
                          struct hostent *hostent) {
    int * iargs;
    ASSERT(arg != NULL);
    iargs = (int*)arg;
    ASSERT(*iargs == bynamecallbacksig);
    ASSERT(timeouts == 0);

    ares_bynamecallbacks++;
}

static void aresbyaddrcallback( void *arg,
                          int status,
                          int timeouts,
                          struct hostent *hostent) {
    int * iargs;
    ASSERT(arg != NULL);
    iargs = (int*)arg;
    ASSERT(*iargs == byaddrcallbacksig);
    ASSERT(timeouts == 0);

    ares_byaddrcallbacks++;
}

static void prep_tcploopback()
{
  int rc = 0;
  /* for test, use echo server - TCP port TEST_PORT on loopback */
  testsrv.S_un.S_un_b.s_b1 = 127;
  testsrv.S_un.S_un_b.s_b2 = 0;
  testsrv.S_un.S_un_b.s_b3 = 0;
  testsrv.S_un.S_un_b.s_b4 = 1;

  optmask = ARES_OPT_SERVERS | ARES_OPT_TCP_PORT | ARES_OPT_FLAGS;
  options.servers = &testsrv;
  options.nservers = 1;
  options.tcp_port = htons(TEST_PORT);
  options.flags = ARES_FLAG_USEVC;

  rc = uv_ares_init_options(&channel, &options, optmask);

  ASSERT(rc == ARES_SUCCESS);
}


TEST_IMPL(gethostbyname) {

  int rc = 0;
  char addr[4];

  rc = ares_library_init(ARES_LIB_INIT_ALL);
  if (rc != 0) {
    printf("ares library init fails %d\n", rc);
    return 1;
  }

  uv_init(alloc_cb);

  printf("Start basic gethostbyname test\n");
  prep_tcploopback();

  ares_bynamecallbacks = 0;
  bynamecallbacksig = 7;

  ares_gethostbyname(channel,
                    "microsoft.com",
                    AF_INET,
                    &aresbynamecallback,
                    &bynamecallbacksig);
  uv_run();

  ASSERT(ares_bynamecallbacks == 1);

  uv_ares_destroy(channel);
  printf("Done basic gethostbyname test\n");


  /* two sequential call on new channel */

  printf("Start gethostbyname and gethostbyaddr sequential test\n");
  prep_tcploopback();

  ares_bynamecallbacks = 0;
  bynamecallbacksig = 7;

  ares_gethostbyname(channel,
                    "microsoft.com",
                    AF_INET,
                    &aresbynamecallback,
                    &bynamecallbacksig);
  uv_run();

  ASSERT(ares_bynamecallbacks == 1);

  ares_byaddrcallbacks = 0;
  byaddrcallbacksig = 8;
  addr[0] = 10;
  addr[1] = 0;
  addr[2] = 1;
  addr[3] = 99;

  ares_gethostbyaddr(channel,
                    addr,
                    4,
                    AF_INET,
                    &aresbyaddrcallback,
                    &byaddrcallbacksig);

  uv_run();

  ASSERT(ares_byaddrcallbacks == 1);

  uv_ares_destroy(channel);
  printf("Done gethostbyname and gethostbyaddr sequential test\n");


  /* two simultaneous calls on new channel */

  printf("Start gethostbyname and gethostbyaddr concurrent test\n");
  prep_tcploopback();

  ares_bynamecallbacks = 0;
  bynamecallbacksig = 7;

  ares_gethostbyname(channel,
                    "microsoft.com",
                    AF_INET,
                    &aresbynamecallback,
                    &bynamecallbacksig);

  ares_byaddrcallbacks = 0;
  byaddrcallbacksig = 8;
  addr[0] = 10;
  addr[1] = 0;
  addr[2] = 1;
  addr[3] = 99;

  ares_gethostbyaddr(channel,
                    addr,
                    4,
                    AF_INET,
                    &aresbyaddrcallback,
                    &byaddrcallbacksig);

  uv_run();

  ASSERT(ares_bynamecallbacks == 1);
  ASSERT(ares_byaddrcallbacks == 1);


  uv_ares_destroy(channel);
  printf("Done gethostbyname and gethostbyaddr concurrent test\n");

  return 0;
}

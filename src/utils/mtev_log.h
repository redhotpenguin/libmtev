/*
 * Copyright (c) 2005-2009, OmniTI Computer Consulting, Inc.
 * All rights reserved.
 * Copyright (c) 2013-2017, Circonus, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name OmniTI Computer Consulting, Inc. nor the names
 *      of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _UTILS_MTEV_LOG_H
#define _UTILS_MTEV_LOG_H

#include "mtev_defines.h"
#include <pthread.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <sys/time.h>
#include "mtev_hash.h"
#include "mtev_hooks.h"
#include "mtev_time.h"
#include "mtev_json.h"
#include "mtev_zipkin.h"

#ifdef mtev_log_impl
typedef struct _mtev_log_stream mtev_log_stream_t;
#define mtev_log_stream_t mtev_log_stream_t *
#else
typedef void * mtev_log_stream_public_t;
#define mtev_log_stream_t mtev_log_stream_public_t
#endif

#define MTEV_LOG_DEFAULT_DEDUP_S 5
#define MTEV_LOG_SPECULATE_ROLLBACK ((mtev_log_stream_t)NULL)

struct _mtev_log_stream_outlet_list {
  mtev_log_stream_t outlet;
  struct _mtev_log_stream_outlet_list *next;
};

typedef struct {
  mtev_boolean supports_async;
  int (*openop)(mtev_log_stream_t);
  int (*reopenop)(mtev_log_stream_t);
  int (*writeop)(mtev_log_stream_t, const struct timeval *whence, const void *, size_t);
  int (*writevop)(mtev_log_stream_t, const struct timeval *whence, const struct iovec *iov, int iovcnt);
  int (*closeop)(mtev_log_stream_t);
  size_t (*sizeop)(mtev_log_stream_t);
  int (*renameop)(mtev_log_stream_t, const char *);
  int (*cullop)(mtev_log_stream_t, int age, ssize_t bytes);
} logops_t;

#define	MTEV_LOG_STREAM_ENABLED		0x01
#define	MTEV_LOG_STREAM_DEBUG		0x02
#define	MTEV_LOG_STREAM_TIMESTAMPS	0x04
#define MTEV_LOG_STREAM_FACILITY	0x08
#define MTEV_LOG_STREAM_RECALCULATE     0x10
#define MTEV_LOG_STREAM_FEATURES        (MTEV_LOG_STREAM_DEBUG|MTEV_LOG_STREAM_TIMESTAMPS|MTEV_LOG_STREAM_FACILITY)

extern mtev_log_stream_t mtev_stderr;
extern mtev_log_stream_t mtev_debug;
extern mtev_log_stream_t mtev_error;
extern mtev_log_stream_t mtev_notice;

#define N_L_S_ON(ls) ((ls != NULL) && (*((unsigned *)ls) & MTEV_LOG_STREAM_ENABLED))

API_EXPORT(void) mtev_log_enter_sighandler(void);
API_EXPORT(void) mtev_log_leave_sighandler(void);
API_EXPORT(int) mtev_log_global_enabled(void);
API_EXPORT(void) mtev_log_init(int debug_on);
API_EXPORT(mtev_boolean) mtev_log_final_resolve(void);
API_EXPORT(int) mtev_log_go_asynch(void);
API_EXPORT(int) mtev_log_go_synch(void);
API_EXPORT(int) mtev_log_reopen_all(void);
API_EXPORT(int) mtev_log_reopen_type(const char *type);
API_EXPORT(void) mtev_register_logops(const char *name, logops_t *ops);
API_EXPORT(void *) mtev_log_stream_get_ctx(mtev_log_stream_t);
API_EXPORT(void) mtev_log_stream_set_ctx(mtev_log_stream_t, void *);
API_EXPORT(int) mtev_log_stream_get_dedup_s(mtev_log_stream_t);
API_EXPORT(int) mtev_log_stream_set_dedup_s(mtev_log_stream_t, int);
API_EXPORT(int) mtev_log_stream_get_flags(mtev_log_stream_t);
API_EXPORT(int) mtev_log_stream_set_flags(mtev_log_stream_t, int);
API_EXPORT(const char *) mtev_log_stream_get_type(mtev_log_stream_t);
API_EXPORT(const char *) mtev_log_stream_get_name(mtev_log_stream_t);
API_EXPORT(const char *) mtev_log_stream_get_path(mtev_log_stream_t);

API_EXPORT(mtev_log_stream_t)
  mtev_log_stream_new(const char *, const char *, const char *,
                      void *, mtev_hash_table *);
API_EXPORT(mtev_log_stream_t)
  mtev_log_stream_new_on_fd(const char *, int, mtev_hash_table *);
API_EXPORT(mtev_log_stream_t)
  mtev_log_stream_new_on_file(const char *, mtev_hash_table *);
API_EXPORT(mtev_log_stream_t) mtev_log_speculate(int nlogs, int nbytes);
API_EXPORT(void)
  mtev_log_speculate_finish(mtev_log_stream_t ls, mtev_log_stream_t speculation);

API_EXPORT(mtev_boolean) mtev_log_stream_exists(const char *);
API_EXPORT(mtev_log_stream_t) mtev_log_stream_find(const char *);
API_EXPORT(void) mtev_log_stream_remove(const char *name);
API_EXPORT(void) mtev_log_stream_add_stream(mtev_log_stream_t ls,
                                            mtev_log_stream_t outlet);
API_EXPORT(void) mtev_log_stream_removeall_streams(mtev_log_stream_t ls);
API_EXPORT(mtev_log_stream_t)
  mtev_log_stream_remove_stream(mtev_log_stream_t ls, const char *name);
API_EXPORT(void) mtev_log_stream_reopen(mtev_log_stream_t ls);
API_EXPORT(int) mtev_log_stream_cull(mtev_log_stream_t ls,
                                     int age, ssize_t bytes);
API_EXPORT(void) mtev_log_dedup_flush(const struct timeval *now);
API_EXPORT(void) mtev_log_dedup_init(void);

#define MTEV_LOG_RENAME_AUTOTIME ((const char *)-1)

API_EXPORT(int) mtev_log_stream_rename(mtev_log_stream_t ls, const char *);
API_EXPORT(void) mtev_log_stream_close(mtev_log_stream_t ls);
API_EXPORT(size_t) mtev_log_stream_size(mtev_log_stream_t ls);
API_EXPORT(size_t) mtev_log_stream_written(mtev_log_stream_t ls);
API_EXPORT(const char *) mtev_log_stream_get_property(mtev_log_stream_t ls,
                                                      const char *);
API_EXPORT(void) mtev_log_stream_set_property(mtev_log_stream_t ls,
                                              const char *, const char *);
API_EXPORT(void) mtev_log_stream_free(mtev_log_stream_t ls);
API_EXPORT(int) mtev_vlog(mtev_log_stream_t ls, const struct timeval *,
                          const char *file, int line,
                          const char *format, va_list arg);
API_EXPORT(int) mtev_log(mtev_log_stream_t ls, const struct timeval *,
                         const char *file, int line,
                         const char *format, ...)
#ifdef __GNUC__
  __attribute__ ((format (printf, 5, 6)))
#endif
  ;

/* fills logger with up to nsize loggers.
 * If there are more loggers thatn nsize, -total is returned.
 * Otherwise, the number loggers is returned.
 */
API_EXPORT(int) mtev_log_list(mtev_log_stream_t *loggers, int nsize);

API_EXPORT(mtev_json_object *)
  mtev_log_stream_to_json(mtev_log_stream_t ls);

/* finds log_lines most recent log lines and calls f with their
 * sequence number and content.  If f returns non-zero, the iteration
 * is aborted early.
 */
API_EXPORT(int)
  mtev_log_memory_lines(mtev_log_stream_t ls, int log_lines,
                        int (*f)(uint64_t, const struct timeval *,
                                 const char *, size_t, void *),
                        void *closure);

API_EXPORT(int)
  mtev_log_memory_lines_since(mtev_log_stream_t ls, uint64_t afterwhich,
                              int (*f)(uint64_t, const struct timeval *,
                                      const char *, size_t, void *),
                              void *closure);
API_EXPORT(void)
  mtev_log_init_globals(void);

#define mtevLT(ls, t, args...) do { \
  if((ls)) { \
    bool mtevLT_doit = mtev_log_global_enabled() || N_L_S_ON((ls)); \
    if(!mtevLT_doit) { \
      Zipkin_Span *mtevLT_span = mtev_zipkin_active_span(NULL); \
      mtevLT_doit = mtev_zipkin_span_logs_attached(mtevLT_span); \
    } \
    if(mtevLT_doit) { \
      mtev_log((ls), t, __FILE__, __LINE__, args); \
    } \
  } \
} while(0)
#define mtevL(ls, args...) do { \
  if((ls)) { \
    bool mtevLT_doit = mtev_log_global_enabled() || N_L_S_ON((ls)); \
    if(!mtevLT_doit) { \
      Zipkin_Span *mtevLT_span = mtev_zipkin_active_span(NULL); \
      mtevLT_doit = mtev_zipkin_span_logs_attached(mtevLT_span); \
    } \
    if(mtevLT_doit) { \
      mtev_log((ls), NULL, __FILE__, __LINE__, args); \
    } \
  } \
} while(0)
#define mtevFatal(ls,args...) do {\
  mtev_log_go_synch(); \
  mtevL((ls), "[FATAL] " args); \
  abort(); \
} while(0)

/* inline prototype here so we don't have circular includes */
#define mtevTerminate(ls,args...) do {\
  mtev_log_go_synch(); \
  mtevL((ls), "[TERMINATE] " args); \
  exit(2); \
} while(0)

extern uint32_t mtev_watchdog_number_of_starts(void);
#define mtevStartupTerminate(ls,args...) do {\
  if(mtev_watchdog_number_of_starts() > 0) { \
    mtevFatal(ls,args); \
  } \
  mtevTerminate(ls,args); \
} while(0)

#ifdef NDEBUG
#error "need to audit mtevAssert usage"
#define mtevAssert(condition) do {} while(0)
#define mtevEvalAssert(condition) do { if (!(condition)) ; } while(0)
#else
#define mtevAssert(condition) do {\
  if(!(condition)) { \
    mtevFatal(mtev_error, "assertion (%s) at %s:%d failed\n", #condition, __FILE__, __LINE__);\
  }\
} while(0)
#define mtevEvalAssert(condition) mtevAssert(condition)
#endif

#define SETUP_LOG(a, b) do { if(!a##_log) a##_log = mtev_log_stream_find(#a); \
                             if(!a##_log) { b; } } while(0)

MTEV_HOOK_PROTO(mtev_log_line,
                (mtev_log_stream_t ls, const struct timeval *whence,
                 const char *timebuf, int timebuflen,
                 const char *debugbuf, int debugbuflen,
                 const char *buffer, size_t len),
                void *, closure,
                (void *closure, mtev_log_stream_t ls, const struct timeval *whence,
                 const char *timebuf, int timebuflen,
                 const char *debugbuf, int debugbuflen,
                 const char *buffer, size_t len))

#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

/* ========================================================================== */
/*                      ASYNC REACTOR CORE IMPLEMENTATION                     */
/* ========================================================================== */

#define REACTOR_MAX_EVENTS 1024

typedef enum {
    REACTOR_READ  = (1 << 0),
    REACTOR_WRITE = (1 << 1),
    REACTOR_ERROR = (1 << 2)
} reactor_event_t;

typedef enum {
    REACTOR_STATE_STOPPED,
    REACTOR_STATE_RUNNING,
    REACTOR_STATE_STOPPING
} reactor_state_t;

struct reactor;
typedef void (*reactor_callback_t)(struct reactor *r, int fd, short events, void *arg);

typedef struct {
    int fd;
    short events;
    reactor_callback_t callback;
    void *arg;
    bool marked_delete;
} reactor_entry_t;

typedef struct reactor {
    reactor_entry_t entries[REACTOR_MAX_EVENTS];
    struct pollfd poll_fds[REACTOR_MAX_EVENTS];
    size_t count;
    reactor_state_t state;
    bool in_dispatch;
    int wakeup_pipe[2];
    pthread_mutex_t lock;
} reactor_t;

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void reactor_wakeup_handler(reactor_t *r, int fd, short events, void *arg) {
    (void)r; (void)events; (void)arg;
    char buf[128];
    while (read(fd, buf, sizeof(buf)) > 0) {}
}

reactor_t* reactor_create(void) {
    reactor_t *r = (reactor_t*)calloc(1, sizeof(reactor_t));
    if (!r) return NULL;

    if (pipe(r->wakeup_pipe) != 0) {
        free(r);
        return NULL;
    }
    set_nonblocking(r->wakeup_pipe[0]);
    set_nonblocking(r->wakeup_pipe[1]);

    pthread_mutex_init(&r->lock, NULL);
    r->state = REACTOR_STATE_STOPPED;
    r->in_dispatch = false;
    r->count = 0;

    r->entries[0].fd = r->wakeup_pipe[0];
    r->entries[0].events = REACTOR_READ;
    r->entries[0].callback = reactor_wakeup_handler;
    r->entries[0].arg = NULL;
    r->entries[0].marked_delete = false;

    r->poll_fds[0].fd = r
#ifndef REACTOR_H
#define REACTOR_H

#define _GNU_SOURCE
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define REACTOR_MAX_EVENTS 1024
#define RING_BUFFER_SIZE 8192
#define CONNECTION_POOL_SIZE 4096
#define WORKER_THREAD_COUNT 4

typedef struct connection connection_t;
typedef struct reactor reactor_t;

typedef enum {
    EVENT_READ  = EPOLLIN,
    EVENT_WRITE = EPOLLOUT,
    EVENT_RDWR  = (EPOLLIN | EPOLLOUT)
} reactor_event_t;

typedef void (*reactor_callback_t)(reactor_t *reactor, connection_t *conn, uint32_t events, void *arg);

typedef struct io_buf {
    char data[RING_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t size;
} io_buf_t;

struct connection {
    int fd;
    uint32_t events;
    bool is_listener;
    bool in_pool;
    
    io_buf_t read_buf;
    io_buf_t write_buf;
    
    reactor_callback_t read_cb;
    reactor_callback_t write_cb;
    void *user_data;
    
    pthread_mutex_t lock;
    connection_t *next;
};

struct reactor {
    int epoll_fd;
    bool running;
    pthread_t thread_id;
    
    connection_t *conn_pool;
    connection_t *free_list;
    pthread_mutex_t pool_lock;
};

reactor_t *reactor_create(void);
void reactor_destroy(reactor_t *reactor);
int reactor_add(reactor_t *reactor, int fd, uint32_t events, reactor_callback_t cb, void *arg);
int reactor_mod(reactor_t *reactor, int fd, uint32_t events);
int reactor_del(reactor_t *reactor, int fd);
void reactor_run(reactor_t *reactor);
void reactor_stop(reactor_t *reactor);

connection_t *connection_acquire(reactor_t *reactor, int fd);
void connection_release(reactor_t *reactor, connection_t *conn);

int io_buf_init(io_buf_t *buf);
size_t io_buf_writable_space(const io_buf_t *buf);
size_t io_buf_readable_space(const io_buf_t *buf);
ssize_t io_buf_read_fd(io_buf_t *buf, int fd);
ssize_t io_buf_write_fd(io_buf_t *buf, int fd);

int set_nonblocking(int fd);

#endif /* REACTOR_H */
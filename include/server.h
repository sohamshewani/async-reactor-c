#ifndef INCLUDE_SERVER_H
#define INCLUDE_SERVER_H

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_EVENTS 1024
#define BACKLOG 4096
#define RING_BUFFER_SIZE 8192

typedef enum {
    CONN_STATE_FREE = 0,
    CONN_STATE_LISTENING,
    CONN_STATE_CONNECTED,
    CONN_STATE_CLOSING,
    CONN_STATE_CLOSED
} connection_state_t;

typedef struct io_buf {
    char data[RING_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t capacity;
} io_buf_t;

typedef struct connection {
    int fd;
    connection_state_t state;
    uint32_t events;
    io_buf_t read_buf;
    io_buf_t write_buf;
    pthread_mutex_t lock;
    struct connection *next;
} connection_t;

typedef struct connection_pool {
    connection_t *pool;
    connection_t *free_list;
    size_t capacity;
    pthread_mutex_t lock;
} connection_pool_t;

typedef struct reactor {
    int epoll_fd;
    volatile bool running;
} reactor_t;

typedef struct task {
    connection_t *conn;
    uint32_t events;
    struct task *next;
} task_t;

typedef struct task_queue {
    task_t *head;
    task_t *tail;
    size_t size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} task_queue_t;

typedef struct worker_pool {
    pthread_t *threads;
    size_t num_threads;
    task_queue_t queue;
    volatile bool running;
} worker_pool_t;

typedef struct server {
    int listen_fd;
    int port;
    reactor_t reactor;
    connection_pool_t conn_pool;
    worker_pool_t worker_pool;
} server_t;

int server_init(server_t *server, int port, size_t max_connections, size_t num_workers);
int server_start(server_t *server);
void server_stop(server_t *server);
void server_destroy(server_t *server);

connection_pool_t *pool_create(size_t capacity);
connection_t *pool_acquire(connection_pool_t *pool);
void pool_release(connection_pool_t *pool, connection_t *conn);
void pool_destroy(connection_pool_t *pool);

void io_buf_init(io_buf_t *buf);
size_t io_buf_available_read(const io_buf_t *buf);
size_t io_buf_available_write(const io_buf_t *buf);
ssize_t io_buf_read_fd(io_buf_t *buf, int fd);
ssize_t io_buf_write_fd(io_buf_t *buf, int fd);

int reactor_init(reactor_t *reactor);
int reactor_add(reactor_t *reactor, int fd, uint32_t events, void *user_data);
int reactor_mod(reactor_t *reactor, int fd, uint32_t events, void *user_data);
int reactor_del(reactor_t *reactor, int fd);
void reactor_run(reactor_t *reactor, server_t *server);
void reactor_destroy(reactor_t *reactor);

int worker_pool_init(worker_pool_t *pool, size_t num_threads);
void worker_pool_push(worker_pool_t *pool, connection_t *conn, uint32_t events);
void worker_pool_stop(worker_pool_t *pool);
void worker_pool_destroy(worker_pool_t *pool);

#endif /* INCLUDE_SERVER_H */
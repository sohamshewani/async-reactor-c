#ifndef ASYNC_REACTOR_C_BUFFER_H
#define ASYNC_REACTOR_C_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * Bounded ring buffer (`io_buf_t`) designed for non-blocking stream I/O pipelines.
 * Optimized for zero-copy operations, complete socket draining via EPOLLET,
 * and memory safety under high-concurrency conditions.
 */
typedef struct io_buf {
    uint8_t *data;       /* Pointer to the underlying memory allocation */
    size_t capacity;     /* Total allocated capacity in bytes (must be a power of 2 for optimal masking, or strictly bounded) */
    size_t read_idx;     /* Current read head offset */
    size_t write_idx;    /* Current write tail offset */
    size_t data_len;     /* Total number of valid unread bytes currently in the buffer */
} io_buf_t;

/*
 * Initializes an io_buf_t instance with a pre-allocated or provided memory region of `capacity` bytes.
 * Returns 0 on success, or -1 on failure (e.g., invalid parameters or NULL pointers).
 */
int io_buf_init(io_buf_t *buf, size_t capacity);

/*
 * Destroys and cleans up the io_buf_t instance, freeing internal memory if dynamically allocated.
 */
void io_buf_destroy(io_buf_t *buf);

/*
 * Resets the buffer pointers and length to zero without releasing underlying memory.
 */
void io_buf_reset(io_buf_t *buf);

/*
 * Returns the number of readable (valid) bytes currently stored in the buffer.
 */
size_t io_buf_readable_bytes(const io_buf_t *buf);

/*
 * Returns the number of writable bytes remaining in the buffer.
 */
size_t io_buf_writable_bytes(const io_buf_t *buf);

/*
 * Reads data directly from a file descriptor (e.g., non-blocking socket) into the buffer.
 * Designed to loop and drain the kernel socket buffer until EAGAIN/EWOULDBLOCK or EOF.
 * 
 * Parameters:
 *   - buf: Pointer to the target io_buf_t.
 *   - fd: The file descriptor to read from.
 *   - bytes_read: Output parameter tracking total bytes read during this call.
 *   - saved_errno: Output parameter to preserve errno if an error or EAGAIN occurs.
 *
 * Returns:
 *   -  1 on successful read continuation.
 *   -  0 on EOF (peer closed connection cleanly).
 *   - -1 on error (check saved_errno; EAGAIN/EWOULDBLOCK is handled internally or indicated appropriately).
 */
ssize_t io_buf_read_fd(io_buf_t *buf, int fd, int *saved_errno);

/*
 * Writes data from the buffer directly to a file descriptor (e.g., non-blocking socket).
 * Drains the buffer as much as the kernel socket send buffer will accept without blocking.
 * 
 * Parameters:
 *   - buf: Pointer to the source io_buf_t.
 *   - fd: The file descriptor to write to.
 *   - bytes_written: Output parameter tracking total bytes written during this call.
 *   - saved_errno: Output parameter to preserve errno if an error or EAGAIN occurs.
 *
 * Returns:
 *   -  1 on successful write progress.
 *   -  0 if the buffer is empty nothing to write.
 *   - -1 on error (check saved_errno).
 */
ssize_t io_buf_write_fd(io_buf_t *buf, int fd, int *saved_errno);

/*
 * Peeks at readable data in the buffer without advancing the read pointer.
 * Copies up to `len` bytes into `dest`.
 * Returns the number of bytes actually copied.
 */
size_t io_buf_peek(const io_buf_t *buf, uint8_t *dest, size_t len);

/*
 * Consumes/advances the read pointer by `len` bytes, discarding them from the buffer.
 * Returns 0 on success, or -1 if `len` exceeds readable bytes.
 */
int io_buf_consume(io_buf_t *buf, size_t len);

/*
 * Appends raw data from `src` of length `len` directly into the buffer.
 * Returns the number of bytes successfully written into the buffer.
 */
size_t io_buf_write_data(io_buf_t *buf, const uint8_t *src, size_t len);

#endif /* ASYNC_REACTOR_C_BUFFER_H */
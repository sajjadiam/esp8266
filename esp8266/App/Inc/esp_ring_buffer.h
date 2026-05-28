/**
 * @file esp_ring_buffer.h
 * @brief Single-producer/single-consumer ring buffer for embedded systems.
 *
 * This module implements a byte-oriented circular FIFO buffer using external
 * storage supplied by the caller. It is intended for SPSC usage:
 *
 *   - Producer context: ISR or driver receive callback
 *   - Consumer context: main loop, parser, or task-level code
 *
 * No locking is required only if the ownership rules are respected:
 *
 *   - esp_ring_buffer_push() / esp_ring_buffer_write() are producer-side APIs.
 *   - esp_ring_buffer_pop() / esp_ring_buffer_read() / esp_ring_buffer_drop()
 *     are consumer-side APIs.
 *   - esp_ring_buffer_clear() must only be called when the producer is stopped
 *     or externally protected.
 *   - esp_ring_buffer_push_overwrite() is not recommended for protocol streams
 *     such as ESP-AT responses because it may corrupt unread frames.
 *
 * The implementation uses one wasted slot to distinguish full and empty states.
 * Therefore, usable capacity is always capacity - 1 bytes.
 *
 * Capacity must be a power of two and at least ESP_RING_BUFFER_MIN_CAPACITY.
 */

 #ifndef ESP_RING_BUFFER_H
 #define ESP_RING_BUFFER_H
 
 #include <stdbool.h>
 #include <stdint.h>

 #ifndef NULL
 #define NULL ((void *)0)
 #endif
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /** Minimum accepted backing storage size in bytes. */
 #define ESP_RING_BUFFER_MIN_CAPACITY    (64u)
 
 /**
  * @brief Ring buffer status codes.
  */
 typedef enum {
     ESP_RING_BUFFER_OK = 0,       /**< Operation completed successfully. */
     ESP_RING_BUFFER_ERR_NULL,     /**< NULL pointer or uninitialized buffer. */
     ESP_RING_BUFFER_ERR_SIZE,     /**< Invalid capacity; must be power-of-two and large enough. */
     ESP_RING_BUFFER_ERR_FULL,     /**< Buffer is full; write operation rejected. */
     ESP_RING_BUFFER_ERR_EMPTY     /**< Buffer is empty; read operation rejected. */
 } esp_ring_buffer_status_t;
 
 /**
  * @brief Ring buffer state object.
  *
  * The memory referenced by buf is owned by the caller and must remain valid for
  * the lifetime of this ring buffer instance.
  */
 typedef struct {
     uint8_t           *buf;      /**< External byte storage. */
     uint16_t           cap;      /**< Total storage size; must be a power of two. */
     uint16_t           mask;     /**< Wrap mask, equal to cap - 1. */
     volatile uint16_t  head;     /**< Producer write index. */
     volatile uint16_t  tail;     /**< Consumer read index. */
     volatile uint32_t  dropped;  /**< Count of bytes rejected or overwritten. */
 } esp_ring_buffer_t;
 
 /**
  * @brief Initialize a ring buffer instance.
  *
  * @param rb Ring buffer object to initialize.
  * @param mem_pool External byte array used as backing storage.
  * @param capacity Size of mem_pool in bytes. Must be power-of-two and at least
  *                 ESP_RING_BUFFER_MIN_CAPACITY.
  * @return ESP_RING_BUFFER_OK on success, otherwise an error code.
  */
 esp_ring_buffer_status_t esp_ring_buffer_init(esp_ring_buffer_t *rb,
                                               uint8_t *mem_pool,
                                               uint16_t capacity);
 
 /**
  * @brief Clear logical buffer state.
  *
  * This resets head, tail, and dropped count. It does not zero the backing memory.
  * Do not call while the producer ISR/callback is active unless externally
  * protected.
  *
  * @param rb Ring buffer object.
  */
 void esp_ring_buffer_clear(esp_ring_buffer_t *rb);
 
 /**
  * @brief Return whether the buffer contains no unread data.
  *
  * @param rb Ring buffer object.
  * @return true if empty or invalid; false otherwise.
  */
 bool esp_ring_buffer_is_empty(const esp_ring_buffer_t *rb);
 
 /**
  * @brief Return whether the buffer cannot accept another byte.
  *
  * @param rb Ring buffer object.
  * @return true if full; false if not full or invalid.
  */
 bool esp_ring_buffer_is_full(const esp_ring_buffer_t *rb);
 
 /**
  * @brief Return number of readable bytes.
  *
  * @param rb Ring buffer object.
  * @return Number of unread bytes; zero if invalid.
  */
 uint16_t esp_ring_buffer_size(const esp_ring_buffer_t *rb);
 
 /**
  * @brief Return number of bytes that can still be written.
  *
  * Because one slot is intentionally wasted, maximum free space is cap - 1.
  *
  * @param rb Ring buffer object.
  * @return Number of writable bytes; zero if invalid.
  */
 uint16_t esp_ring_buffer_free_space(const esp_ring_buffer_t *rb);
 
 /**
  * @brief Return cumulative dropped-byte counter.
  *
  * @param rb Ring buffer object.
  * @return Dropped-byte counter; zero if invalid.
  */
 uint32_t esp_ring_buffer_dropped_count(const esp_ring_buffer_t *rb);
 
 /**
  * @brief Push one byte without overwriting unread data.
  *
  * If full, the new byte is rejected and dropped is incremented.
  *
  * @param rb Ring buffer object.
  * @param data Byte to store.
  * @return ESP_RING_BUFFER_OK, ESP_RING_BUFFER_ERR_FULL, or ESP_RING_BUFFER_ERR_NULL.
  */
 esp_ring_buffer_status_t esp_ring_buffer_push(esp_ring_buffer_t *rb,
                                               uint8_t data);
 
 /**
  * @brief Push one byte, overwriting the oldest unread byte if full.
  *
  * @warning Do not use this for ESP-AT UART RX streams unless you explicitly
  *          accept protocol/frame corruption on overflow. Prefer
  *          esp_ring_buffer_push() for command-response protocols.
  *
  * @param rb Ring buffer object.
  * @param data Byte to store.
  * @return ESP_RING_BUFFER_OK or ESP_RING_BUFFER_ERR_NULL.
  */
 esp_ring_buffer_status_t esp_ring_buffer_push_overwrite(esp_ring_buffer_t *rb,
                                                         uint8_t data);
 
 /**
  * @brief Pop one byte from the buffer.
  *
  * @param rb Ring buffer object.
  * @param[out] data Destination for the byte.
  * @return ESP_RING_BUFFER_OK, ESP_RING_BUFFER_ERR_EMPTY, or ESP_RING_BUFFER_ERR_NULL.
  */
 esp_ring_buffer_status_t esp_ring_buffer_pop(esp_ring_buffer_t *rb,
                                              uint8_t *data);
 
 /**
  * @brief Read and consume up to len bytes.
  *
  * @param rb Ring buffer object.
  * @param[out] data Destination buffer.
  * @param len Maximum bytes to read.
  * @return Actual number of bytes read.
  */
 uint16_t esp_ring_buffer_read(esp_ring_buffer_t *rb,
                               uint8_t *data,
                               uint16_t len);
 
 /**
  * @brief Copy up to len bytes without consuming them.
  *
  * @param rb Ring buffer object.
  * @param[out] data Destination buffer.
  * @param len Maximum bytes to copy.
  * @return Actual number of bytes copied.
  */
 uint16_t esp_ring_buffer_peek(const esp_ring_buffer_t *rb,
                               uint8_t *data,
                               uint16_t len);
 
 /**
  * @brief Write up to len bytes without overwriting unread data.
  *
  * If free space is smaller than len, only the bytes that fit are written and
  * dropped is incremented by the number of rejected bytes.
  *
  * @param rb Ring buffer object.
  * @param[in] data Source data.
  * @param len Number of bytes requested for writing.
  * @return Actual number of bytes written.
  */
 uint16_t esp_ring_buffer_write(esp_ring_buffer_t *rb,
                                const uint8_t *data,
                                uint16_t len);
 
 /**
  * @brief Discard up to n unread bytes.
  *
  * @param rb Ring buffer object.
  * @param n Number of bytes to discard.
  */
 void esp_ring_buffer_drop(esp_ring_buffer_t *rb,
                           uint16_t n);
 
 /**
  * @brief Find the first occurrence of target in unread data.
  *
  * This function does not consume data.
  *
  * @param rb Ring buffer object.
  * @param target Byte to search for.
  * @return Zero-based offset from tail, or -1 if not found or invalid.
  */
 int16_t esp_ring_buffer_find(const esp_ring_buffer_t *rb,
                              uint8_t target);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* ESP_RING_BUFFER_H */
 
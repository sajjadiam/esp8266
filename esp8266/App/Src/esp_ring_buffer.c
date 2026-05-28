#include "esp_ring_buffer.h"

#include <string.h>

static bool esp_ring_buffer_is_power_of_two(uint16_t value) {
    return (value != 0u) && ((value & (uint16_t)(value - 1u)) == 0u);
}

static bool esp_ring_buffer_is_valid(const esp_ring_buffer_t *rb) {
    return (rb != NULL) &&
           (rb->buf != NULL) &&
           (rb->cap >= ESP_RING_BUFFER_MIN_CAPACITY) &&
           (rb->mask == (uint16_t)(rb->cap - 1u));
}

static uint16_t min_u16(uint16_t a, uint16_t b) {
    return (a < b) ? a : b;
}

esp_ring_buffer_status_t esp_ring_buffer_init(esp_ring_buffer_t *rb, uint8_t *mem_pool, uint16_t capacity) {
    if ((rb == NULL) || (mem_pool == NULL)) {
        return ESP_RING_BUFFER_ERR_NULL;
    }

    if ((capacity < ESP_RING_BUFFER_MIN_CAPACITY) ||
        !esp_ring_buffer_is_power_of_two(capacity)) {
        rb->buf = NULL;
        rb->cap = 0u;
        rb->mask = 0u;
        rb->head = 0u;
        rb->tail = 0u;
        rb->dropped = 0u;
        return ESP_RING_BUFFER_ERR_SIZE;
    }

    rb->buf = mem_pool;
    rb->cap = capacity;
    rb->mask = (uint16_t)(capacity - 1u);
    rb->head = 0u;
    rb->tail = 0u;
    rb->dropped = 0u;

    return ESP_RING_BUFFER_OK;
}

void esp_ring_buffer_clear(esp_ring_buffer_t *rb) {
    if (rb == NULL) {
        return;
    }

    rb->head = 0u;
    rb->tail = 0u;
    rb->dropped = 0u;
}

bool esp_ring_buffer_is_empty(const esp_ring_buffer_t *rb) {
    if (!esp_ring_buffer_is_valid(rb)) {
        return true;
    }

    return rb->head == rb->tail;
}

bool esp_ring_buffer_is_full(const esp_ring_buffer_t *rb) {
    if (!esp_ring_buffer_is_valid(rb)) {
        return false;
    }

    return ((uint16_t)(rb->head + 1u) & rb->mask) == rb->tail;
}

uint16_t esp_ring_buffer_size(const esp_ring_buffer_t *rb) {
    if (!esp_ring_buffer_is_valid(rb)) {
        return 0u;
    }

    return (uint16_t)((rb->head - rb->tail) & rb->mask);
}

uint16_t esp_ring_buffer_free_space(const esp_ring_buffer_t *rb) {
    if (!esp_ring_buffer_is_valid(rb)) {
        return 0u;
    }

    return (uint16_t)((rb->cap - 1u) - esp_ring_buffer_size(rb));
}

uint32_t esp_ring_buffer_dropped_count(const esp_ring_buffer_t *rb) {
    if (rb == NULL) {
        return 0u;
    }

    return rb->dropped;
}

esp_ring_buffer_status_t esp_ring_buffer_push(esp_ring_buffer_t *rb, uint8_t data) {
    if (!esp_ring_buffer_is_valid(rb)) {
        return ESP_RING_BUFFER_ERR_NULL;
    }

    if (esp_ring_buffer_is_full(rb)) {
        rb->dropped++;
        return ESP_RING_BUFFER_ERR_FULL;
    }

    rb->buf[rb->head] = data;
    rb->head = (uint16_t)((rb->head + 1u) & rb->mask);

    return ESP_RING_BUFFER_OK;
}

esp_ring_buffer_status_t esp_ring_buffer_push_overwrite(esp_ring_buffer_t *rb, uint8_t data) {
    if (!esp_ring_buffer_is_valid(rb)) {
        return ESP_RING_BUFFER_ERR_NULL;
    }

    if (esp_ring_buffer_is_full(rb)) {
        rb->tail = (uint16_t)((rb->tail + 1u) & rb->mask);
        rb->dropped++;
    }

    rb->buf[rb->head] = data;
    rb->head = (uint16_t)((rb->head + 1u) & rb->mask);

    return ESP_RING_BUFFER_OK;
}

esp_ring_buffer_status_t esp_ring_buffer_pop(esp_ring_buffer_t *rb, uint8_t *data) {
    if (!esp_ring_buffer_is_valid(rb) || (data == NULL)) {
        return ESP_RING_BUFFER_ERR_NULL;
    }

    if (esp_ring_buffer_is_empty(rb)) {
        return ESP_RING_BUFFER_ERR_EMPTY;
    }

    *data = rb->buf[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1u) & rb->mask);

    return ESP_RING_BUFFER_OK;
}

uint16_t esp_ring_buffer_read(esp_ring_buffer_t *rb, uint8_t *data, uint16_t len) {
    if (!esp_ring_buffer_is_valid(rb) || (data == NULL) || (len == 0u)) {
        return 0u;
    }

    uint16_t available = esp_ring_buffer_size(rb);
    if (available == 0u) {
        return 0u;
    }

    len = min_u16(len, available);

    uint16_t first = min_u16(len, (uint16_t)(rb->cap - rb->tail));
    uint16_t second = (uint16_t)(len - first);

    memcpy(data, &rb->buf[rb->tail], first);
    if (second > 0u) {
        memcpy(&data[first], &rb->buf[0], second);
    }

    rb->tail = (uint16_t)((rb->tail + len) & rb->mask);

    return len;
}

uint16_t esp_ring_buffer_peek(const esp_ring_buffer_t *rb, uint8_t *data, uint16_t len) {
    if (!esp_ring_buffer_is_valid(rb) || (data == NULL) || (len == 0u)) {
        return 0u;
    }

    uint16_t available = esp_ring_buffer_size(rb);
    if (available == 0u) {
        return 0u;
    }

    len = min_u16(len, available);

    uint16_t first = min_u16(len, (uint16_t)(rb->cap - rb->tail));
    uint16_t second = (uint16_t)(len - first);

    memcpy(data, &rb->buf[rb->tail], first);
    if (second > 0u) {
        memcpy(&data[first], &rb->buf[0], second);
    }

    return len;
}

uint16_t esp_ring_buffer_write(esp_ring_buffer_t *rb, const uint8_t *data, uint16_t len) {
    if (!esp_ring_buffer_is_valid(rb) || (data == NULL) || (len == 0u)) {
        return 0u;
    }

    uint16_t free_space = esp_ring_buffer_free_space(rb);
    if (free_space == 0u) {
        rb->dropped += len;
        return 0u;
    }

    if (len > free_space) {
        rb->dropped += (uint16_t)(len - free_space);
        len = free_space;
    }

    uint16_t first = min_u16(len, (uint16_t)(rb->cap - rb->head));
    uint16_t second = (uint16_t)(len - first);

    memcpy(&rb->buf[rb->head], data, first);
    if (second > 0u) {
        memcpy(&rb->buf[0], &data[first], second);
    }

    rb->head = (uint16_t)((rb->head + len) & rb->mask);

    return len;
}

void esp_ring_buffer_drop(esp_ring_buffer_t *rb, uint16_t n) {
    if (!esp_ring_buffer_is_valid(rb) || (n == 0u)) {
        return;
    }

    uint16_t available = esp_ring_buffer_size(rb);
    if (n >= available) {
        rb->tail = rb->head;
    } else {
        rb->tail = (uint16_t)((rb->tail + n) & rb->mask);
    }
}

int16_t esp_ring_buffer_find(const esp_ring_buffer_t *rb, uint8_t target) {
    if (!esp_ring_buffer_is_valid(rb)) {
        return -1;
    }

    uint16_t available = esp_ring_buffer_size(rb);
    for (uint16_t i = 0u; i < available; i++) {
        uint16_t idx = (uint16_t)((rb->tail + i) & rb->mask);
        if (rb->buf[idx] == target) {
            return (int16_t)i;
        }
    }

    return -1;
}

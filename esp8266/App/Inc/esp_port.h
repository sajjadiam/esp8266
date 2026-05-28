#ifndef ESP_PORT_H
#define ESP_PORT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus // اگر سی ++ باشد
extern "C" { // شروع سی ++
#endif// پایان سی ++

typedef struct {
    uint16_t (*write)(const void *data, uint16_t size, uint32_t timeout_ms); // نوشتن داده به پورت , write باید تعداد بایت ارسال شده را گزارش کند.
    uint16_t (*read)(void *data, uint16_t size); // خواندن داده از پورت ,read باید تعداد بایت خوانده‌شده را گزارش کند.
    uint16_t (*available)(void); // available یعنی تعداد بایت‌هایی که بدون block شدن قابل خواندن هستند.
    uint32_t (*millis)(void); // زمان فعلی به میلی ثانیه
    void (*delay_ms)(uint32_t ms); // تاخیر به مدت مشخص
    void (*set_reset)(bool active); // active = true  یعنی ESP در reset نگه داشته شود وactive = false یعنی ESP از reset خارج شود
    void (*set_enable)(bool enable); // enable = true یعنی ESP فعال شود و enable = false یعنی ESP غیر فعال شود
} esp_port_t;

#ifdef __cplusplus // اگر سی ++ باشد 
} // پایان سی ++
#endif // پایان سی ++
#endif // ESP_PORT_H // پایان فایل
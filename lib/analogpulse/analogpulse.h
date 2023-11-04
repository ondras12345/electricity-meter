#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /*
     * CONFIGURATION
     */
    /// minimum difference between min and max for a valid signal
    uint16_t min_amplitude;
    /**
     * Min / max will increase / decrease by (decay_speed/256)
     * during each call to analogpulse_sample.
     */
    uint32_t decay_speed;

    /*
     * STATE
     */
    // min & max are stored << 8 to increase resolution
    uint32_t min;
    uint32_t max;

    // mostly just for ease of debugging
    uint16_t sample;

    bool digital;
} analogpulse_t;


void analogpulse_init(analogpulse_t * ctx);
void analogpulse_sample(analogpulse_t * ctx, uint16_t sample);
bool analogpulse_valid(analogpulse_t * ctx);
bool analogpulse_digital(analogpulse_t * ctx);
uint16_t analogpulse_threshold(analogpulse_t * ctx);

#ifdef __cplusplus
}
#endif

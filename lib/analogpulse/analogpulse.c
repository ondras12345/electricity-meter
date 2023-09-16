#include "analogpulse.h"

void analogpulse_init(analogpulse_t * ctx)
{
    // start in an invalid state
    ctx->min = -1;
    ctx->max = 0;
    ctx->sample = 0;
    ctx->digital = false;
}


/**
 * Processes a new analog sample.
 * This function should be called with a regular time period.
 */
void analogpulse_sample(analogpulse_t * ctx, uint16_t sample)
{
    uint32_t tmp;
    tmp = ctx->min + ctx->decay_speed;
    if (tmp > ctx->min) ctx->min += tmp;
    else ctx->min = -1;
    tmp = ctx->max - ctx->decay_speed;
    if (tmp < ctx->max) ctx->max = tmp;
    else ctx->max = 0;

    tmp = sample<<8;
    if (tmp > ctx->max) ctx->max = tmp;
    if (tmp < ctx->min) ctx->min = tmp;

    ctx->sample = sample;
    ctx->digital = sample > analogpulse_threshold(ctx);
}


/**
 * @return true if currently outputting valid digital values
 */
bool analogpulse_valid(analogpulse_t * ctx)
{
    uint32_t min_plus = ctx->min + ((uint32_t)(ctx->min_amplitude)<<8);
    if (min_plus < ctx->min) min_plus = ctx->min;

    return min_plus < ctx->max;
}


/**
 * Return digital value corresponding to last sample.
 * @see analogpulse_valid to check whether the value is meaningless.
 */
bool analogpulse_digital(analogpulse_t * ctx)
{
    return ctx->digital;
}


/**
 * Return threshold value.
 */
uint16_t analogpulse_threshold(analogpulse_t * ctx)
{
    uint16_t threshold = (((uint64_t)ctx->max + (uint64_t)ctx->min)/2)>>8;
    return threshold;
}

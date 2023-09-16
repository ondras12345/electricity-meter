#include "pulse_counter.h"
#include "hardware.h"
#include <analogpulse.h>

#define F_SAMPLING 1000  // Hz
#define DEBOUNCE_TIME 8  // ms
#define DEBOUNCE_MAX (DEBOUNCE_TIME * F_SAMPLING / 1000)

static analogpulse_t ap;
static HardwareTimer timer_sampling(TIM1);

static volatile bool valid = false;
static volatile bool digital = false;
static volatile bool digital_filtered = false;
static volatile uint32_t pulses = 0;
static volatile analogpulse_t ap_copy;


static void ISR_timer_sampling()
{
    uint16_t sample = analogRead(PIN_PULSE_COUNTER);  // TODO analogread is probably slow
    analogpulse_sample(&ap, sample);
    digital = analogpulse_digital(&ap);
    valid = analogpulse_valid(&ap);

    // copy the structure (needs to be done manually because of volatile
    ap_copy.min_amplitude = ap.min_amplitude;
    ap_copy.decay_speed = ap.decay_speed;
    ap_copy.min = ap.min;
    ap_copy.max = ap.max;
    ap_copy.sample = ap.sample;
    ap_copy.digital = ap.digital;

    if (!valid) return;

    static bool prev_digital_filtered = false;
    static uint_fast16_t integrator = 0;
    if (digital)
    {
        if (integrator < DEBOUNCE_MAX)
            integrator++;
    }
    else
    {
        if (integrator > 0)
            integrator --;
    }

    if (integrator == 0)
    {
        digital_filtered = false;
    }
    if (integrator >= DEBOUNCE_MAX)
    {
        digital_filtered = true;
        integrator = DEBOUNCE_MAX;
    }

    if (digital_filtered != prev_digital_filtered)
    {
        prev_digital_filtered = digital_filtered;
        if (prev_digital_filtered) pulses++;
    }
}


void pulse_counter_init()
{
    ap.min_amplitude = 16;
    ap.decay_speed = 16;
    analogpulse_init(&ap);

    timer_sampling.setOverflow(F_SAMPLING, HERTZ_FORMAT);
    timer_sampling.attachInterrupt(ISR_timer_sampling);
    timer_sampling.resume();
}


void pulse_counter_loop()
{
    // reading these volatile variables is atomic (bools)
    LED_STATUS(valid);
    LED_PULSE(digital);
    LED_FILTERED(digital_filtered);
}


uint32_t pulse_counter_get_count()
{
    uint32_t ret;
    noInterrupts();
    ret = pulses;
    interrupts();
    return ret;
}


/// This function should only be used for debugging.
analogpulse_t pulse_counter_get_analogpulse()
{
    analogpulse_t ret;
    noInterrupts();
    ret.min_amplitude = ap_copy.min_amplitude;
    ret.decay_speed = ap_copy.decay_speed;
    ret.min = ap_copy.min;
    ret.max = ap_copy.max;
    ret.sample = ap_copy.sample;
    ret.digital = ap_copy.digital;
    interrupts();
    return ret;
}

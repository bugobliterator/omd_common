/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common/timing.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

static uint32_t systick_period_ns;
static uint32_t counts_per_us;
static uint32_t counts_per_ms;
static volatile uint32_t system_millis;

void sys_tick_handler(void);

void timing_init(void)
{
    systick_period_ns =1000000000UL/rcc_ahb_frequency;
    counts_per_ms = rcc_ahb_frequency/1000UL;
    counts_per_us = rcc_ahb_frequency/1000000UL;
    systick_set_reload(counts_per_ms-1); // 1 ms
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

uint32_t millis(void) {
    return system_millis;
}

uint32_t micros(void) {
    uint32_t ms;
    uint32_t counter;

    do {
        ms = system_millis;
        counter = systick_get_value();
        if (systick_get_countflag()) {
            system_millis++;
        }
    } while(system_millis != ms);

    return ms*1000UL + (counts_per_ms-counter)/counts_per_us;
}

void usleep(uint32_t delay) {
    uint32_t tbegin = micros();
    while (micros()-tbegin < delay);
}

void sys_tick_handler(void)
{
    if (systick_get_countflag()) {
        system_millis++;
    }
}

void nsleep(uint32_t delay_ns) {
    uint32_t starting_value_systicks;
    starting_value_systicks = systick_get_value();
    while (timing_timeSinceMark_ns(starting_value_systicks) < delay_ns);
}

/* For measuring durations less than 1mS */
uint32_t timing_timeSinceMark_ns(uint32_t starting_value_systicks) {
    uint32_t final_value_systicks;
    uint32_t duration_systicks;
    uint32_t duration_ns;

    final_value_systicks = systick_get_value();
    if (final_value_systicks > starting_value_systicks){
        duration_systicks = (starting_value_systicks + counts_per_ms - final_value_systicks);
    } else {
        duration_systicks = (starting_value_systicks - final_value_systicks);
    }
    duration_ns = duration_systicks * systick_period_ns;
    return duration_ns;
}

//static uint64 timing_base_time_us = 0;
uint64_t timing_get_time_us(void) {
    return (uint64_t)millis() * (1000UL);
}

uint64_t timing_time_mark_in_us(time_mark_t *time_mark) {
    return *time_mark;
}

void timing_set_time_mark(time_mark_t *time_mark) {
    *time_mark = timing_get_time_us();
}

uint64_t timing_time_since_mark_us(time_mark_t *time_mark) {
    return timing_get_time_us() - timing_time_mark_in_us(time_mark);
}


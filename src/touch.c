
#include "ch.h"
#include "hal.h"
#include "touch.h"
#include "terminal.h"
#include "lcd.h"

#include <stdbool.h>


#define XP 5
#define XN 6
#define YP 4
#define YN 7

#define NUM_SAMPLES 32


static void setup_axis(u16 rp, u16 rn, u16 bp, u16 bn);
static adcsample_t adc_avg(adcsample_t* samples, uint16_t num_samples);
static msg_t touch_thread(void* arg);


static const ADCConversionGroup y_conv_grp = {
  .circular = FALSE,
  .num_channels = 1,
  .end_cb = NULL,
  .error_cb = NULL,

  /* HW dependent part.*/
  .cr1   = ADC_CR1_RES_0, // 10-bit resolution
  .cr2   = ADC_CR2_SWSTART, // software triggered
  .smpr1 = 0,
  .smpr2 = ADC_SMPR2_SMP_AN4(ADC_SAMPLE_480),
  .sqr1  = ADC_SQR1_NUM_CH(1),
  .sqr2  = 0,
  .sqr3  = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN4),
};

static const ADCConversionGroup x_conv_grp = {
  .circular = FALSE,
  .num_channels = 1,
  .end_cb = NULL,
  .error_cb = NULL,

  /* HW dependent part.*/
  .cr1   = ADC_CR1_RES_0, // 10-bit resolution
  .cr2   = ADC_CR2_SWSTART, // software triggered
  .smpr1 = 0,
  .smpr2 = ADC_SMPR2_SMP_AN5(ADC_SAMPLE_480),
  .sqr1  = ADC_SQR1_NUM_CH(1),
  .sqr2  = 0,
  .sqr3  = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN5),
};

static uint8_t wa_touch_thread[1024];
static adcsample_t samples[NUM_SAMPLES];

void
touch_init()
{
  adcStart(&ADCD1, NULL);

  chThdCreateStatic(wa_touch_thread, sizeof(wa_touch_thread), NORMALPRIO, touch_thread, NULL);
}

static void
setup_axis(u16 rp, u16 rn, u16 bp, u16 bn)
{
  palSetPadMode(GPIOA, rp, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, rn, PAL_MODE_INPUT);
  palSetPadMode(GPIOA, bp, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, bn, PAL_MODE_OUTPUT_PUSHPULL);

  palSetPad(GPIOA, bp);
  palClearPad(GPIOA, bn);
}

void
dump_readings(void)
{
  int j;

  for (j = 0; j < NUM_SAMPLES; ++j) {
    terminal_write_int(samples[j]);
    terminal_write(" ");
  }
  adcsample_t avg_val = adc_avg(samples+2, NUM_SAMPLES-2);
  terminal_write("\navg: ");
  terminal_write_int(avg_val);
  terminal_write("\n");
}

static msg_t
touch_thread(void* arg)
{
  (void)arg;

  while (1) {
    terminal_clear();

    terminal_write("\ny samples:\n");
    setup_axis(YP, YN, XP, XN);
    adcConvert(&ADCD1, &y_conv_grp, samples, NUM_SAMPLES);
    dump_readings();

    terminal_write("\nx samples:\n");
    setup_axis(XP, XN, YP, YN);
    adcConvert(&ADCD1, &x_conv_grp, samples, NUM_SAMPLES);
    dump_readings();

    chThdSleepMilliseconds(500);
  }

  return 0;
}

adcsample_t
adc_avg(adcsample_t* samples, uint16_t num_samples)
{
  uint32_t avg_sample = 0;
  int i;

  for (i = 0; i < num_samples; ++i) {
    avg_sample += samples[i];
  }
  avg_sample /= num_samples;

  return avg_sample;
}

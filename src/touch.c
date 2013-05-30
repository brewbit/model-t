
#include "ch.h"
#include "hal.h"
#include "touch.h"
#include "terminal.h"
#include "lcd.h"
#include "touch_calib.h"
#include "gui.h"

#include <stdbool.h>


#define XP 5
#define XN 6
#define YP 4
#define YN 7

#define NUM_SAMPLES 8
#define DISCARDED_SAMPLES 1

#define TOUCH_THRESHOLD 20

typedef struct {
  uint16_t read_pos_pad;
  uint16_t read_neg_pad;
  uint16_t pull_pos_pad;
  uint16_t pull_neg_pad;
  const ADCConversionGroup* conv_grp;
} axis_cfg_t;


static uint16_t read_axis(const axis_cfg_t* axis_cfg);
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

static const axis_cfg_t x_axis = {
    .read_pos_pad = XP,
    .read_neg_pad = XN,
    .pull_pos_pad = YP,
    .pull_neg_pad = YN,
    .conv_grp = &x_conv_grp,
};

static const axis_cfg_t y_axis = {
    .read_pos_pad = YP,
    .read_neg_pad = YN,
    .pull_pos_pad = XP,
    .pull_neg_pad = XN,
    .conv_grp = &y_conv_grp,
};

static uint8_t wa_touch_thread[1024];
static matrix_t calib_matrix;

void
touch_init()
{
  point_t perfectScreenSample[3] = {
    { 100, 100 },
    { 900, 500 },
    { 500, 900 }
  } ;
  setCalibrationMatrix(
      perfectScreenSample,
      perfectScreenSample,
      &calib_matrix);

  adcStart(&ADCD1, NULL);

  chThdCreateStatic(wa_touch_thread, sizeof(wa_touch_thread), NORMALPRIO, touch_thread, NULL);
}

static uint16_t
read_axis(const axis_cfg_t* axis_cfg)
{
  adcsample_t samples[NUM_SAMPLES];

  /* setup the pad modes */
  palSetPadMode(GPIOA, axis_cfg->read_pos_pad, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, axis_cfg->read_neg_pad, PAL_MODE_INPUT);
  palSetPadMode(GPIOA, axis_cfg->pull_pos_pad, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, axis_cfg->pull_neg_pad, PAL_MODE_OUTPUT_PUSHPULL);

  /* set the 'pull' pins to the correct state */
  palSetPad(GPIOA, axis_cfg->pull_pos_pad);
  palClearPad(GPIOA, axis_cfg->pull_neg_pad);

  /* capture a number of samples from the read pin */
  adcConvert(&ADCD1, axis_cfg->conv_grp, samples, NUM_SAMPLES);

  /* average and return the samples */
  return adc_avg(samples+DISCARDED_SAMPLES,
      NUM_SAMPLES-DISCARDED_SAMPLES);
}

static msg_t
touch_thread(void* arg)
{
  (void)arg;

  while (1) {
    point_t raw;
    raw.x = read_axis(&x_axis);
    raw.y = read_axis(&y_axis);

    if ((raw.x > TOUCH_THRESHOLD) &&
        (raw.y > TOUCH_THRESHOLD)) {
      point_t calib;
      getDisplayPoint(&calib, &raw, &calib_matrix);

      gui_send_touch(&raw, &calib);
    }

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

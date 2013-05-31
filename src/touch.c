
#include "ch.h"
#include "hal.h"
#include "touch.h"
#include "terminal.h"
#include "lcd.h"
#include "touch_calib.h"
#include "gui.h"

#include <stdbool.h>


#define XP 4
#define XN 6
#define YP 5
#define YN 7

#define NUM_SAMPLES 8
#define DISCARDED_SAMPLES 1

#define TOUCH_THRESHOLD 40
#define DEBOUNCE_TIME MS2ST(100)

// ADC sample resolution
#define Q 1024

#define MAX_TOUCH_VELOCITY 10

typedef struct {
  uint16_t drive_pos_pad;
  uint16_t drive_neg_pad;
  uint16_t sample_pos_pad;
  uint16_t sample_neg_pad;
  const ADCConversionGroup* conv_grp;
} axis_cfg_t;


static uint16_t read_axis(const axis_cfg_t* axis_cfg);
static adcsample_t adc_avg(adcsample_t* samples, uint16_t num_samples);
static msg_t touch_thread(void* arg);


static const ADCConversionGroup xp_conv_grp = {
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

static const ADCConversionGroup yp_conv_grp = {
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

static const ADCConversionGroup yn_conv_grp = {
  .circular = FALSE,
  .num_channels = 1,
  .end_cb = NULL,
  .error_cb = NULL,

  /* HW dependent part.*/
  .cr1   = ADC_CR1_RES_0, // 10-bit resolution
  .cr2   = ADC_CR2_SWSTART, // software triggered
  .smpr1 = 0,
  .smpr2 = ADC_SMPR2_SMP_AN7(ADC_SAMPLE_480),
  .sqr1  = ADC_SQR1_NUM_CH(1),
  .sqr2  = 0,
  .sqr3  = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN7),
};

static const axis_cfg_t z1_axis = {
    .drive_pos_pad = YP,
    .drive_neg_pad = XN,
    .sample_pos_pad = XP,
    .sample_neg_pad = YN,
    .conv_grp = &xp_conv_grp,
};

static const axis_cfg_t z2_axis = {
    .drive_pos_pad = YP,
    .drive_neg_pad = XN,
    .sample_pos_pad = XP,
    .sample_neg_pad = YN,
    .conv_grp = &yn_conv_grp,
};

static const axis_cfg_t x_axis = {
    .drive_pos_pad = XP,
    .drive_neg_pad = XN,
    .sample_pos_pad = YP,
    .sample_neg_pad = YN,
    .conv_grp = &yp_conv_grp,
};

static const axis_cfg_t y_axis = {
    .drive_pos_pad = YP,
    .drive_neg_pad = YN,
    .sample_pos_pad = XP,
    .sample_neg_pad = XN,
    .conv_grp = &xp_conv_grp,
};

static uint8_t wa_touch_thread[1024];
static uint8_t touch_down;

void
touch_init()
{
  adcStart(&ADCD1, NULL);

  chThdCreateStatic(wa_touch_thread, sizeof(wa_touch_thread), NORMALPRIO, touch_thread, NULL);
}

static uint16_t
read_axis(const axis_cfg_t* axis_cfg)
{
  adcsample_t samples[NUM_SAMPLES];

  /* setup the pad modes */
  palSetPadMode(GPIOA, axis_cfg->sample_pos_pad, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, axis_cfg->sample_neg_pad, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, axis_cfg->drive_pos_pad, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, axis_cfg->drive_neg_pad, PAL_MODE_OUTPUT_PUSHPULL);

  /* set the 'drive' pins to the correct state */
  palSetPad(GPIOA, axis_cfg->drive_pos_pad);
  palClearPad(GPIOA, axis_cfg->drive_neg_pad);

  /* Let the pins settle */
  chThdSleepMilliseconds(20);

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
    uint32_t z1 = read_axis(&z1_axis);
    uint32_t z2 = read_axis(&z2_axis);
    uint32_t x = read_axis(&x_axis);
    uint32_t y = read_axis(&y_axis);

    /* Calculate pressure of touch based on equations from TI Application Note SBAA155A */
    /* Prevent divide by zero */
    z1 = MAX(1, z1);
    /* Modified form of equation 8 with rx = 1 */
    uint32_t rz = (((x * z2) / z1) - x) / Q;
    /* Modified form of equation 9 with a = 1024, b = 1 */
    uint32_t p = Q - rz;

//    terminal_clear();
//    terminal_write("z: ");
//    terminal_write_int(z1);
//    terminal_write(", ");
//    terminal_write_int(z2);
//    terminal_write("\nrz: ");
//    terminal_write_int(rz);
//    terminal_write("\np: ");
//    terminal_write_int(p);

    // swapped since the screen is rotated...
    point_t raw = {y, x};

    if (p > 900) {
//      terminal_write("\npos: (");
//      terminal_write_int(x);
//      terminal_write(", ");
//      terminal_write_int(y);
//      terminal_write(")");
      gui_touch_down(&raw, &raw);
      touch_down = 1;
    }
    else {
      if (touch_down) {
        gui_touch_up(&raw, &raw);
        touch_down = 0;
      }
    }

    chThdSleepMilliseconds(100);
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

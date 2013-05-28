
#include "ch.h"
#include "hal.h"
#include "touch.h"
#include "terminal.h"
#include "lcd.h"

#include <stdbool.h>

static void adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n);
static void setup_axis(u16 rp, u16 rn, u16 bp, u16 bn);
static void setup_y_axis(void);
static void setup_x_axis(void);
static adcsample_t adc_avg(adcsample_t* samples, uint16_t num_samples);
static msg_t touch_thread(void* arg);


// X+ = PA4 = ADC1-4
#define XP 5
// X- = PA6
#define XN 6
// Y+ = PA5 = ADC1-5
#define YP 4
// Y- = PA7
#define YN 7



/* Total number of channels to be sampled by a single ADC operation.*/
#define ADC_GRP1_NUM_CHANNELS   2

/* Depth of the conversion buffer, channels are sampled four times each.*/
#define ADC_GRP1_BUF_DEPTH      32

/*
 * ADC samples buffer.
 */
static adcsample_t samples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 4 samples of 2 channels, SW triggered.
 * Channels:    IN11   (48 cycles sample time)
 *              Sensor (192 cycles sample time)
 */
static const ADCConversionGroup adcgrpcfg = {
  FALSE,
  ADC_GRP1_NUM_CHANNELS,
  adccb,
  NULL,
  /* HW dependent part.*/
  ADC_CR1_RES_0,
  ADC_CR2_SWSTART,
  0,
  ADC_SMPR2_SMP_AN4(ADC_SAMPLE_480) | ADC_SMPR2_SMP_AN5(ADC_SAMPLE_480),
  ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),
  0,
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN4) | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN5)
};

static adcsample_t avg_y, avg_x;
static volatile uint8_t reading_ready;
static uint8_t wa_touch_thread[1024];


static void
adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n) {

  (void) buffer; (void) n;
  /* Note, only in the ADC_COMPLETE state because the ADC driver fires an
     intermediate callback when the buffer is half full.*/
  if (adcp->state != ADC_COMPLETE) {
    setup_x_axis();
  }
  else {
    /* Calculates the average values from the ADC samples.*/
    avg_y = (samples[0] + samples[2] + samples[4] + samples[6]) / 4;
    avg_x = (samples[1] + samples[3] + samples[5] + samples[7]) / 4;

    reading_ready = 1;
  }
}

void
touch_init()
{
  chThdCreateStatic(wa_touch_thread, sizeof(wa_touch_thread), NORMALPRIO, touch_thread, NULL);

  adcStart(&ADCD1, NULL);


  /* Starts an asynchronous ADC conversion operation, the conversion
     will be executed in parallel to the current PWM cycle and will
     terminate before the next PWM cycle.*/
  chSysLockFromIsr();
  setup_y_axis();
  adcStartConversionI(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
  chSysUnlockFromIsr();
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

static void
setup_y_axis()
{
  setup_axis(YP, YN, XP, XN);
}

static void
setup_x_axis()
{
  setup_axis(XP, XN, YP, YN);
}

static msg_t
touch_thread(void* arg)
{
  (void)arg;

  while (1) {
    if (reading_ready) {
      reading_ready = 0;
      terminal_clear();
      terminal_write("reading ready\n");
      int i,j;
      for (i = 0; i < ADC_GRP1_NUM_CHANNELS; ++i) {
        terminal_write("channel:\n");
        for (j = 0; j < ADC_GRP1_BUF_DEPTH; ++j) {
          terminal_write_int(samples[i*ADC_GRP1_BUF_DEPTH + j]);
          terminal_write(" ");
        }
        adcsample_t avg_val = adc_avg(samples + i*ADC_GRP1_BUF_DEPTH + 2, ADC_GRP1_BUF_DEPTH-2);
        terminal_write("avg: ");
        terminal_write_int(avg_val);
        terminal_write("\n");
      }


      chThdSleepMilliseconds(500);

      chSysLockFromIsr();
      setup_y_axis();
      adcStartConversionI(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
      chSysUnlockFromIsr();
    }
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

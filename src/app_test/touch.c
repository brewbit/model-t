
#include "ch.h"
#include "hal.h"
#include "touch.h"
#include "lcd.h"

#include <stdbool.h>


#define XP 4
#define XN 6
#define YP 5
#define YN 7

#ifndef               OK
  #define       OK     1
  #define     NOT_OK   0
#endif

#define NUM_SAMPLES 8
#define DISCARDED_SAMPLES 1

#define TOUCH_THRESHOLD 950
#define DEBOUNCE_TIME MS2ST(20)

// Number of samples to collect before starting to dispatch them to the system.
// This is mainly used because the last few samples collected on a touch up
// event are bad. So we use the latest sample to determine if the touch has
// been lifted, but we report an slightly old (but valid) location that was
// previously collected.
#define SAMPLE_DELAY 4

// ADC sample resolution
#define Q 1024

matrix_t default_calib = {
    .An      = 76320,
    .Bn      = 3080,
    .Cn      = -9475080,
    .Dn      = -560,
    .En      = 60340,
    .Fn      = -4360660,
    .Divider = 205664
};

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
static void touch_dispatch(void);

static int getDisplayPoint(
  point_t* displayPtr,
  const point_t* screenPtr,
  const matrix_t* matrixPtr);


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

static uint8_t touch_down;
static systime_t last_touch_time;
static uint8_t down_samples;
static uint8_t sample_idx;
static point_t touch_coord_raw[SAMPLE_DELAY];
static point_t touch_coord_calib[SAMPLE_DELAY];

void
touch_init()
{
  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, touch_thread, NULL);
}

static void
touch_dispatch()
{

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
  chThdSleepMicroseconds(100);

  /* capture a number of samples from the read pin */
  adcAcquireBus(&ADCD1);
  adcConvert(&ADCD1, axis_cfg->conv_grp, samples, NUM_SAMPLES);
  adcReleaseBus(&ADCD1);

  /* average and return the samples */
  return adc_avg(samples+DISCARDED_SAMPLES,
      NUM_SAMPLES-DISCARDED_SAMPLES);
}

/**********************************************************************
 *
 *     Function: getDisplayPoint()
 *
 *  Description: Given a valid set of calibration factors and a point
 *                value reported by the touch screen, this function
 *                calculates and returns the true (or closest to true)
 *                display point below the spot where the touch screen
 *                was touched.
 *
 *
 *
 *  Argument(s): displayPtr (output) - Pointer to the calculated
 *                                      (true) display point.
 *               screenPtr (input) - Pointer to the reported touch
 *                                    screen point.
 *               matrixPtr (input) - Pointer to calibration factors
 *                                    matrix previously calculated
 *                                    from a call to
 *                                    setCalibrationMatrix()
 *
 *
 *  The function simply solves for Xd and Yd by implementing the
 *   computations required by the translation matrix.
 *
 *                                              /-     -\
 *              /-    -\     /-            -\   |       |
 *              |      |     |              |   |   Xs  |
 *              |  Xd  |     | A    B    C  |   |       |
 *              |      |  =  |              | * |   Ys  |
 *              |  Yd  |     | D    E    F  |   |       |
 *              |      |     |              |   |   1   |
 *              \-    -/     \-            -/   |       |
 *                                              \-     -/
 *
 *  It must be kept brief to avoid consuming CPU cycles.
 *
 *
 *       Return: OK - the display point was correctly calculated
 *                     and its value is in the output argument.
 *               NOT_OK - an error was detected and the function
 *                         failed to return a valid point.
 *
 *
 *
 *                 NOTE!    NOTE!    NOTE!
 *
 *  setCalibrationMatrix() and getDisplayPoint() will do fine
 *  for you as they are, provided that your digitizer
 *  resolution does not exceed 10 bits (1024 values).  Higher
 *  resolutions may cause the integer operations to overflow
 *  and return incorrect values.  If you wish to use these
 *  functions with digitizer resolutions of 12 bits (4096
 *  values) you will either have to a) use 64-bit signed
 *  integer variables and math, or b) judiciously modify the
 *  operations to scale results by a factor of 2 or even 4.
 *
 *
 */
static int
getDisplayPoint(
    point_t* displayPtr,
    const point_t* screenPtr,
    const matrix_t* matrixPtr)
{
  int  retValue = OK;

  if (matrixPtr->Divider != 0) {
    /* Operation order is important since we are doing integer */
    /*  math. Make sure you add all terms together before      */
    /*  dividing, so that the remainder is not rounded off     */
    /*  prematurely.                                           */

    displayPtr->x =
        ((matrixPtr->An * screenPtr->x) +
         (matrixPtr->Bn * screenPtr->y) +
         (matrixPtr->Cn)) / matrixPtr->Divider;

    displayPtr->y =
        ((matrixPtr->Dn * screenPtr->x) +
         (matrixPtr->En * screenPtr->y) +
         (matrixPtr->Fn)) / matrixPtr->Divider;
  }
  else {
      retValue = NOT_OK;
  }

  return retValue;
}

static msg_t
touch_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("touch");

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

    if (p > TOUCH_THRESHOLD) {
#if (DISP_ORIENT == LANDSCAPE)
      /* swap the coordinates since the screen is rotated */
      touch_coord_raw[sample_idx].x = y;
      touch_coord_raw[sample_idx].y = x;
#else
      touch_coord_raw[sample_idx].x = x;
      touch_coord_raw[sample_idx].y = y;
#endif

      /* calibrate the raw touch coordinate */
      getDisplayPoint(
          &touch_coord_calib[sample_idx],
          &touch_coord_raw[sample_idx],
          &default_calib);

      touch_down = 1;
      last_touch_time = chTimeNow();
      sample_idx = (sample_idx + 1) % SAMPLE_DELAY;

      if (down_samples < SAMPLE_DELAY)
        down_samples++;
      else
        touch_dispatch();
    }
    else {
      if (touch_down &&
          !chTimeIsWithin(last_touch_time, last_touch_time + DEBOUNCE_TIME)) {
        touch_down = 0;
        down_samples = 0;
        touch_dispatch();
      }
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

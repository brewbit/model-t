/*
 *
 *   Copyright (c) 2001, Carlos E. Vidales. All rights reserved.
 *
 *   This sample program was written and put in the public domain
 *    by Carlos E. Vidales.  The program is provided "as is"
 *    without warranty of any kind, either expressed or implied.
 *   If you choose to use the program within your own products
 *    you do so at your own risk, and assume the responsibility
 *    for servicing, repairing or correcting the program should
 *    it prove defective in any manner.
 *   You may copy and distribute the program's source code in any
 *    medium, provided that you also include in each copy an
 *    appropriate copyright notice and disclaimer of warranty.
 *   You may also modify this program and distribute copies of
 *    it provided that you include prominent notices stating
 *    that you changed the file(s) and the date of any change,
 *    and that you do not charge any royalties or licenses for
 *    its use.
 *
 *
 *   File Name:  calibrate.h
 *
 *
 *   Definition of constants and structures, and declaration of functions
 *    in Calibrate.c
 *
 */

#ifndef CALIBRATE_H
#define CALIBRATE_H

#include "common.h"

#include <math.h>
#include <stdint.h>



#ifndef               OK
  #define       OK     1
  #define     NOT_OK   0
#endif

#define MAX_SAMPLES     3


/* This arrangement of values facilitates
 *  calculations within getDisplayPoint()
 */
typedef struct {
  int32_t An;     /* A = An/Divider */
  int32_t Bn;     /* B = Bn/Divider */
  int32_t Cn;     /* C = Cn/Divider */
  int32_t Dn;     /* D = Dn/Divider */
  int32_t En;     /* E = En/Divider */
  int32_t Fn;     /* F = Fn/Divider */
  int32_t Divider;
} matrix_t;


int
setCalibrationMatrix(
    const point_t* display,
    const point_t* screen,
    matrix_t* matrix);


int
getDisplayPoint(
    point_t* display,
    const point_t* screen,
    const matrix_t* matrix);


#endif


#include <math.h>

#include <complex.h>

#include "cmag.h"

double cmag(double complex z) {

  double x, y;

  x = creal(z);
  y = cimag(z);

  return sqrt(x * x + y * y);
  
}

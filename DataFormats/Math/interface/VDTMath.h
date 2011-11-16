#ifndef vdt_math_h
#define vdt_math_h

/*******************************************************************************
 * 
 * VDT math library: collection of double precision vectorisable trascendental 
 * functions.
 * 
 * The basic idea is to exploit pade polinomials.
 * A lot of ideas were inspired by the cephes math library.
 * http://www.netlib.org/cephes/
 * 
 * Authors Danilo Piparo, Vincenzo Innocente and Thomas Hauth
 * 
 ******************************************************************************/


#include <iostream>
#include <cmath>
#include <limits>

#define CMS_VECTORIZE
//#define CMS_VECTORIZE_VERBOSE __attribute__ ((optimize("-ftree-vectorizer-verbose=7")))
#define CMS_VECTORIZE_VERBOSE



namespace vdt {

// paramters
static const double LOG2E = 1.4426950408889634073599; // 1/log(2)
static const double SQRTH =0.70710678118654752440;

// Pade' polinomials coefficients

// exp
static const double EXP_LIMIT = 708.;
static const double PX1exp = 1.26177193074810590878E-4;
static const double PX2exp = 3.02994407707441961300E-2;
static const double PX3exp = 9.99999999999999999910E-1;
static const double QX1exp = 3.00198505138664455042E-6;
static const double QX2exp = 2.52448340349684104192E-3;
static const double QX3exp = 2.27265548208155028766E-1;
static const double QX4exp = 2.00000000000000000009E0;

// logarithm
static const double LOG_UPPER_LIMIT = 5e307;
static const double LOG_LOWER_LIMIT = 5e-307;
static const double PX1log = 1.01875663804580931796E-4;
static const double PX2log = 4.97494994976747001425E-1;
static const double PX3log = 4.70579119878881725854E0;
static const double PX4log = 1.44989225341610930846E1;
static const double PX5log = 1.79368678507819816313E1;
static const double PX6log = 7.70838733755885391666E0;

static const double QX1log = 1.12873587189167450590E1;
static const double QX2log = 4.52279145837532221105E1;
static const double QX3log = 8.29875266912776603211E1;
static const double QX4log = 7.11544750618563894466E1;
static const double QX5log = 2.31251620126765340583E1;


typedef union {
	double d;
	int i[2];
	long long ll;
	unsigned short s[4];
} ieee754;

inline double ll2d(unsigned long long x) {
   union { double f; unsigned long long i; } tmp;
   tmp.i=x;
   return tmp.f;
 }


inline unsigned long long d2ll(double x) {
   union { double f; unsigned long long i; } tmp;
   tmp.f=x;
   return tmp.i;
 }


// Exp -------------------------------------------------------------------------
// Vectorises in a loop without any change in 4.7
inline double fast_exp(double x){

    double initial_x = x;
    int n;
    double xx, px, qx;
    ieee754 u;

    px =int(LOG2E * x + 0.5);// std::floor(LOG2E * x + 0.5);

    n = px;

    x -= px * 6.93145751953125E-1;
    x -= px * 1.42860682030941723212E-6;

    xx = x * x;

    // px = x * P(x**2).
    px = PX1exp;
    px *= xx;
    px += PX2exp;
    px *= xx;
    px += PX3exp;
    px *= x;

    // Evaluate Q(x**2).
    qx = QX1exp;
    qx *= xx;
    qx += QX2exp;
    qx *= xx;
    qx += QX3exp;
    qx *= xx;
    qx += QX4exp;

    // e**x = 1 + 2x P(x**2)/( Q(x**2) - P(x**2) )
    x = px / (qx - px);
    x = 1.0 + 2.0 * x;

    // Build 2^n in double.
    u.d = 0;
    n += 1023;
    u.ll = (long long) (n) << 52;
        
    double res = x * u.d;
    if (initial_x > EXP_LIMIT)
            res = std::numeric_limits<double>::infinity();
    if (initial_x < -EXP_LIMIT)
            res = 0.;   
    
    return res;
}


//------------------------------------------------------------------------------
// Vectorises pulling out some operations from the loop. Works in 4.6
void fast_exp_vect(double* input,
                   double* output,
                   const unsigned int arr_size);// CMS_VECTORIZE_VERBOSE;

//------------------------------------------------------------------------------                     
// Vectorises without any operation before the loop, works only in 4.7
void __future_fast_exp_vect(double* input, 
                            double* output, 
                            const unsigned int arr_size);// CMS_VECTORIZE_VERBOSE;

//------------------------------------------------------------------------------
// For comparison, a function with a loop that won't vectorise
// unless intel or amd math lib is linked
void std_exp_vect(double* arg_inp, double* arg_out, const unsigned int arg_arr_size);

//------------------------------------------------------------------------------

// Log -------------------------------------------------------------------------

double fast_log(double x){

    double input_x=x;
    double y, z;
    double px,qx;

    /* separate mantissa from exponent
    */
    
    unsigned long long n = d2ll(x);

    unsigned long long le = ((n >> 52) & 0x7ffL);
    double fe = (le-1023);
    n &=0xfffffffffffffLL;
    const unsigned long long p05 = d2ll(0.5);
    n |= p05;
    x = ll2d(n);

    /* logarithm using log(x) = z + z**3 P(z)/Q(z),
    * where z = 2(x-1)/x+1)
    */
    /* logarithm using log(1+x) = x - .5x**2 + x**3 P(x)/Q(x) */

    // blending   
    if( x > SQRTH ) fe+=1.;
    if( x < SQRTH )   x += x;
    x =   x - 1.0;
    
    
    /* rational form */

    z = x*x;
    px =  PX1log;
    px *= x;    
    px += PX2log;
    px *= x;    
    px += PX3log;
    px *= x; 
    px += PX4log;
    px *= x; 
    px += PX5log;
    px *= x;
    px += PX6log;
    //
    //for the final formula
    px *= x; 
    px *= z;


    qx = x;
    qx += QX1log;
    qx *=x;
    qx += QX2log;
    qx *=x;    
    qx += QX3log;
    qx *=x;    
    qx += QX4log;
    qx *=x;    
    qx += QX5log;

    
    y = px / qx ;
   
    y -= fe * 2.121944400546905827679e-4; 
    y -= 0.5 * z  ;
    
    z = x + y;
    z += fe * 0.693359375;

    if (input_x > LOG_UPPER_LIMIT)
      z = std::numeric_limits<double>::infinity();
    if (input_x < LOG_LOWER_LIMIT)
      z =  - std::numeric_limits<double>::infinity();       

//     std::cout << input_x << " " << std::log(input_x) << " " << z << std::endl;

    return( z );  
  
  }

//------------------------------------------------------------------------------

void fast_log_vect(double* input, 
                   double* outupt, 
                   const unsigned int arr_size);

//------------------------------------------------------------------------------
// For comparison, a function with a loop that won't vectorise
// unless intel or amd math lib is linked
void std_log_vect(double* input, 
                  double* outupt, 
                  const unsigned int arr_size);

//------------------------------------------------------------------------------
// Will vectorise only with gcc 4.7
void __future_fast_log_vect(double* input, 
                            double* outupt, 
                            const unsigned int arr_size) CMS_VECTORIZE_VERBOSE;

// Service----------------------------------------------------------------------

void print_instructions_info();

}

#endif

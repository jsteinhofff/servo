
#define FIXED32_Q (16)
#define FIXED32_FMASK (((Fixed32)1 << FIXED32_Q) - 1)

typedef int32_t Fixed32;
typedef int64_t Fixed64;

Fixed32 Fixed32_FromInt(int32_t n) {
	return (Fixed32)((Fixed32)n << FIXED32_Q);
}

int32_t Fixed32_Frac(Fixed32 a){
	return a & FIXED32_FMASK;
}

// Optional
Fixed32 Fixed32_FromFloat(float f) {
	return (Fixed32)((f) * (((Fixed64)1 << FIXED32_Q) + ((f) >= 0 ? 0.5 : -0.5)));
}

// Optional
Fixed32 Fixed32_ToFloat(float T) {
	return (float)((T)*((float)(1)/(float)(1 << FIXED32_Q)));
}

Fixed32 Fixed32_Mul(Fixed32 a, Fixed32 b) {
	return (Fixed32)(((Fixed64)a * (Fixed64)b) >> FIXED32_Q);	
}

Fixed32 Fixed32_Div(Fixed32 a, Fixed32 b) {
	return (Fixed32)(((Fixed64)a << FIXED32_Q) / (Fixed64)b);
}


#include <stdfix.h>

#define REAL accum
#define REAL_CONST( x ) x##k

REAL  a, b, c = REAL_CONST( 100.001 );
accum d = REAL_CONST( 85.08765 ); 


#define N 5

void test_fixpoint() {

    int32_t flt_ticks = 0;
    int32_t fix_ticks = 0;
    int32_t conv_ticks = 0;

    float flt_as[N] = {1.234, 100.1, 1000.45454, 0.0, -234324.4};
    float flt_bs[N] = {6.789, 1000.2, -765.65443, 44.44, 34.12};
    
    for (int i = 0 ; i < N; i++) {
        float flt_a = flt_as[i];
        float flt_b = flt_bs[i];
        
        uint32_t start_flt = get_clock_ticks();
        float flt_mul = flt_a * flt_b;
        float flt_div = flt_a / flt_b;
        uint32_t end_flt = get_clock_ticks();

        flt_ticks += end_flt - start_flt;

        uint32_t start_conv = get_clock_ticks();
        Fixed32 fix_a = Fixed32_FromFloat(flt_a);
        Fixed32 fix_b = Fixed32_FromFloat(flt_b);
        uint32_t end_conv = get_clock_ticks();
        conv_ticks += end_conv - start_conv;

        uint32_t start_fix = get_clock_ticks();
        Fixed32 fix_mul = Fixed32_Mul(fix_a, fix_b);
        Fixed32 fix_div = Fixed32_Div(fix_a, fix_b);
        uint32_t end_fix = get_clock_ticks();
        fix_ticks += end_fix - start_fix;

        start_conv = get_clock_ticks();
        float fix_flt_mul = Fixed32_ToFloat(fix_mul);
        float fix_flt_div = Fixed32_ToFloat(fix_div);
        end_conv = get_clock_ticks();
        conv_ticks += end_conv - start_conv;

        if (fabs(fix_flt_mul - flt_mul) > 0.001) {
            Serial.printf("float vs. Fixed32 results differ for %f * %f = %f (float) vs. %f (fixed)\n", flt_a, flt_b, flt_mul, fix_mul);
        }

        if (fabs(fix_flt_div - flt_div) > 0.001) {
            Serial.printf("float vs. Fixed32 results differ for %f / %f = %f (float) vs. %f (fixed)\n", flt_a, flt_b, flt_div, fix_div);
        }
    }

    Serial.printf("==========================================\n");
    Serial.printf("Fixed-Point test results\n");
    Serial.printf("==========================================\n");
    Serial.printf("Ticks for float calculations: %d\n", flt_ticks);
    Serial.printf("Ticks for conversion to fixpoint: %d\n", conv_ticks);
    Serial.printf("Ticks for fixpoint calculation: %d\n", fix_ticks);
    Serial.printf("==========================================");
}


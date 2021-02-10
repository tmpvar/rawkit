
#include <intrin.h>
#include <xmmintrin.h>


void callmemaybe(__m128 v);


void setup() {
    __m128 a = _mm_set_ps(0.25f, 0.5f, 0.75f, 1.0f);
    __m128 b = _mm_set_ps(0.1f, 0.2f, 0.3f, 0.4f);
    callmemaybe(_mm_add_ps(a, b));
}

void loop() {}
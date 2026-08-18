#ifndef _IMMINTRIN_H
#define _IMMINTRIN_H

/*
 * Compatibility placeholder for projects that probe for the umbrella x86
 * intrinsic header but do not use intrinsic types or functions in CPC builds.
 * If a header enabled optional intrinsic paths only because this placeholder
 * exists, disable those paths again.
 */
#undef MA_SUPPORT_AVX
#undef MA_SUPPORT_AVX2

#endif

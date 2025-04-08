// The MIT License
// Copyright © 2014 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


/*
Comparison of three bicubic filter kernels. B=Spline, Catmull-Rom and "recommended", as described
in this article: http://http.developer.nvidia.com/GPUGems/gpugems_ch24.html

Done the naive way with 16 samples rather than the smart way of performing bilinear filters. For
the fast way to do it, see Dave Hoskins' shader: https://www.shadertoy.com/view/4df3Dn


// Mitchell Netravali Reconstruction Filter
// B = 1,   C = 0   - cubic B-spline
// B = 1/3, C = 1/3 - recommended
// B = 0,   C = 1/2 - Catmull-Rom spline
//
// ca = {  12 - 9*B - 6*C,  -18 + 12*B + 6*C, 0, 6 - 2*B  } / 6;
// cb = {  -B - 6*C, 6*B + 30*C, -12*B - 48*C, 8*B + 24*C } / 6;
*/

//------------------------------------------------------------------------------

#define BS_A float4( 3.0,  -6.0,   0.0,  4.0) /  6.0
#define BS_B float4(-1.0,   6.0, -12.0,  8.0) /  6.0
#define RE_A float4(21.0, -36.0,   0.0, 16.0) / 18.0
#define RE_B float4(-7.0,  36.0, -60.0, 32.0) / 18.0
#define CR_A float4( 3.0,  -5.0,   0.0,  2.0) /  2.0
#define CR_B float4(-1.0,   5.0,  -8.0,  4.0) /  2.0
        
#define ca BS_A
#define cb BS_B

#define SAM(tex, res, i, a, b) tex.Sample(smp_base, (i + float2(a, b) + 0.5) / res)


float4 powers( float x ) {
  return float4(x*x*x, x*x, x, 1.0);
}

float4 spline(float x, float4 c0, float4 c1, float4 c2, float4 c3)
{
    // We could expand the powers and build a matrix instead (twice as many coefficients
    // would need to be stored, but it could be faster.
    return c0 * dot(cb, powers(x + 1.0)) + 
           c1 * dot(ca, powers(x      )) +
           c2 * dot(ca, powers(1.0 - x)) +
           c3 * dot(cb, powers(2.0 - x));
}

float4 bicubic(Texture2D tex, float2 res, float2 t)
{
    float2 p = res * t - 0.5;
    float2 f = frac(p);
    float2 i = floor(p);

    return spline(
        f.y,
        spline(
            f.x,
            SAM(tex, res, i, -1, -1),
            SAM(tex, res, i, 0, -1),
            SAM(tex, res, i, 1, -1),
            SAM(tex, res, i, 2, -1)
        ),
        spline(
            f.x,
            SAM(tex, res, i, -1, 0),
            SAM(tex, res, i, 0, 0),
            SAM(tex, res, i, 1, 0),
            SAM(tex, res, i, 2, 0)
        ),
        spline(
            f.x,
            SAM(tex, res, i, -1, 1),
            SAM(tex, res, i, 0, 1),
            SAM(tex, res, i, 1, 1),
            SAM(tex, res, i, 2, 1)
        ),
        spline(
            f.x,
            SAM(tex, res, i, -1, 2),
            SAM(tex, res, i, 0, 2),
            SAM(tex, res, i, 1, 2),
            SAM(tex, res, i, 2, 2)
        )
    );
}

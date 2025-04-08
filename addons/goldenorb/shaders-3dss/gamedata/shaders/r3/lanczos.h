#define LANCZOS2_WINDOW_SINC 0.5
#define LANCZOS2_SINC 0.5
#define LANCZOS2_AR_STRENGTH 0.0
// END PARAMETERS

//// A=0.5, B=0.825 is the best jinc approximation for x<2.5. if B=1.0, it's a lanczos filter.
// Increase A to get more blur. Decrease it to get a sharper picture. 
// B = 0.825 to get rid of dithering. Increase B to get a fine sharpness, though dithering returns.

#define halfpi  1.5707963267948966192313216916398
#define pi    3.1415926535897932384626433832795
#define wa    (LANCZOS2_WINDOW_SINC*pi)
#define wb    (LANCZOS2_SINC*pi)

SamplerState PointSampler {
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Calculates the distance between two points
float d(float2 pt1, float2 pt2)
{
    float2 v = pt2 - pt1;
    return sqrt(dot(v,v));
}

float3 min4(float3 a, float3 b, float3 c, float3 d)
{
    return min(a, min(b, min(c, d)));
}

float3 max4(float3 a, float3 b, float3 c, float3 d)
{
    return max(a, max(b, max(c, d)));
}

float4 lanczos(float4 x)
{
    float4 res;

    res = (x==float4(0.0, 0.0, 0.0, 0.0))
        ? float4(wa*wb,wa*wb,wa*wb,wa*wb)
        : sin(x*wa)*sin(x*wb)/(x*x);

    return res;
}
     
float4 lanczos2_sharp(
    Texture2D tex,
    float2 texture_size,
    float2 texCoord
)
{
    float3 color;
    float4x4 weights;

    float2 dx = float2(1.0, 0.0);
    float2 dy = float2(0.0, 1.0);

    float2 pc = texCoord*texture_size;

    float2 tc = (floor(pc-float2(0.5,0.5))+float2(0.5,0.5));

    weights[0] = lanczos(float4(d(pc, tc    -dx    -dy), d(pc, tc	     -dy), d(pc, tc    +dx    -dy), d(pc, tc+2.0*dx    -dy)));
    weights[1] = lanczos(float4(d(pc, tc    -dx	 ), d(pc, tc		  ), d(pc, tc    +dx	 ), d(pc, tc+2.0*dx	 )));
    weights[2] = lanczos(float4(d(pc, tc    -dx    +dy), d(pc, tc	     +dy), d(pc, tc    +dx    +dy), d(pc, tc+2.0*dx    +dy)));
    weights[3] = lanczos(float4(d(pc, tc    -dx+2.0*dy), d(pc, tc	 +2.0*dy), d(pc, tc    +dx+2.0*dy), d(pc, tc+2.0*dx+2.0*dy)));

    dx = dx/texture_size;
    dy = dy/texture_size;
    tc = tc/texture_size;

    // reading the texels

    float3 c00 = tex.Sample(PointSampler, tc    -dx    -dy).xyz;
    float3 c10 = tex.Sample(PointSampler, tc	     -dy).xyz;
    float3 c20 = tex.Sample(PointSampler, tc    +dx    -dy).xyz;
    float3 c30 = tex.Sample(PointSampler, tc+2.0*dx    -dy).xyz;
    float3 c01 = tex.Sample(PointSampler, tc    -dx	 ).xyz;
    float3 c11 = tex.Sample(PointSampler, tc		  ).xyz;
    float3 c21 = tex.Sample(PointSampler, tc    +dx	 ).xyz;
    float3 c31 = tex.Sample(PointSampler, tc+2.0*dx	 ).xyz;
    float3 c02 = tex.Sample(PointSampler, tc    -dx    +dy).xyz;
    float3 c12 = tex.Sample(PointSampler, tc	     +dy).xyz;
    float3 c22 = tex.Sample(PointSampler, tc    +dx    +dy).xyz;
    float3 c32 = tex.Sample(PointSampler, tc+2.0*dx    +dy).xyz;
    float3 c03 = tex.Sample(PointSampler, tc    -dx+2.0*dy).xyz;
    float3 c13 = tex.Sample(PointSampler, tc	 +2.0*dy).xyz;
    float3 c23 = tex.Sample(PointSampler, tc    +dx+2.0*dy).xyz;
    float3 c33 = tex.Sample(PointSampler, tc+2.0*dx+2.0*dy).xyz;

    //  Get min/max samples
    float3 min_sample = min4(c11, c21, c12, c22);
    float3 max_sample = max4(c11, c21, c12, c22);

    color = mul(weights[0], float4x3(c00, c10, c20, c30));
    color+= mul(weights[1], float4x3(c01, c11, c21, c31));
    color+= mul(weights[2], float4x3(c02, c12, c22, c32));
    color+= mul(weights[3], float4x3(c03, c13, c23, c33));
    color = color/(dot(mul(weights, float4(1,1,1,1)), 1));

    // Anti-ringing
    float3 aux = color;
    color = clamp(color, min_sample, max_sample);

    color = lerp(aux, color, LANCZOS2_AR_STRENGTH);

    // final sum and weight normalization
    return float4(color, 1);
}

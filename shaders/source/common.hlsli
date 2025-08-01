#define PI ( 3.14159 )
#define EPSILON ( 0.0001 )
#define INV_EPSILON ( 1.0 - EPSILON )

#ifndef EXCLUDE_COMMON
#define EXCLUDE_COMMON

#define TEX_DECLARE1D(type, name, slot)                \
    Texture1D<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARE1DARRAY(type, name, slot)                \
    Texture1DArray<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARE2D(type, name, slot)                \
    Texture2D<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARE2DMS(type, name, slot)                \
    Texture2DMS<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARE2DARRAY(type, name, slot)                \
    Texture2DArray<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARE3D(type, name, slot)                \
    Texture3D<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARECUBE(type, name, slot)                \
    TextureCube<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARECUBEARRAY(type, name, slot)                \
    TextureCubeArray<##type> name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)

struct BasicAppData /* size 36 */ {
    float3 Position : POSITION; // size 12, offset 0
    float2 UV       : TEXCOORD; // size 8, offset 12
    float4 Color    : COLOR;    // size 16, offset 20
};

struct BasicMaterialAppData /* size 48 */ {
    float3 Position : POSITION; // size 12, offset 0
    float2 UV       : TEXCOORD; // size 8, offset 12
    float4 Color    : COLOR;    // size 16, offset 20
    float3 Normal   : NORMAL;   // size 12, offset 36
};

struct MaterialAppData /* size 60 */ {
    float3 Position  : POSITION;    // size 12, offset 0
    float2 UV        : TEXCOORD;    // size 8, offset 12
    float4 Color     : COLOR;       // size 16, offset 20
    float3 Normal    : NORMAL;      // size 12, offset 36
    float3 Tangent   : TANGENT;     // size 12, offset 48
};

struct InstancedBasicAppData /* size 44 */ {
    float3 Position : POSITION;         // size 12, offset 0
    float2 UV       : TEXCOORD0;        // size 8, offset 12
    float4 Color    : COLOR;            // size 16, offset 20
    uint InstanceID : SV_InstanceID;    // size 4, offset 36
    uint MaterialID : MATERIALID;       // size 4, offset 40
};

struct InstancedBasicMaterialAppData /* size 56 */ {
    float3 Position : POSITION;         // size 12, offset 0
    float2 UV       : TEXCOORD0;        // size 8, offset 12
    float4 Color    : COLOR;            // size 16, offset 20
    float3 Normal   : NORMAL;           // size 12, offset 36
    uint InstanceID : SV_InstanceID;    // size 4, offset 48
    uint MaterialID : MATERIALID;       // size 4, offset 52
};

struct InstancedMaterialAppData /* size 68 */ {
    float3 Position : POSITION;         // size 12, offset 0
    float2 UV       : TEXCOORD0;        // size 8,  offset 12
    float4 Color    : COLOR;            // size 16, offset 20
    float3 Normal   : NORMAL;           // size 12, offset 36
    float3 Tangent  : TANGENT;          // size 12, offset 48
    uint InstanceID : SV_InstanceID;    // size 4,  offset 60
    uint MaterialID : MATERIALID;       // size 4,  offset 64
};

struct BasicVertexOutput /* size 64 */ {
    centroid float4 Position : SV_POSITION;    // size 16, offset 0
    float2 UV                : TEXCOORD0;      // size 12, offset 16
    float4 Color             : COLOR;          // size 16, offset 28
    float4 WorldPosition     : WORLD_POSITION; // size 16, offset 44
    uint MaterialID          : MATERIALID;     // size 4,  offset 60
};

struct BasicMaterialVertexOutput /* size 76 */ {
    centroid float4 Position : SV_POSITION;    // size 16, offset 0
    float2 UV                : TEXCOORD0;      // size 12, offset 16
    float4 Color             : COLOR;          // size 16, offset 28
    float3 Normal            : NORMAL;         // size 12, offset 44
    float4 WorldPosition     : WORLD_POSITION; // size 16, offset 56
    uint MaterialID          : MATERIALID;     // size 4,  offset 72
};

struct MaterialVertexOutput /* size 100 */ {
    centroid float4 Position : SV_POSITION;    // size 16, offset 0
    float2 UV                : TEXCOORD0;      // size 12, offset 16
    float4 Color             : COLOR;          // size 16, offset 28
    float3 Tangent           : TANGENT;        // size 12, offset 44
    float3 Bitangent         : BINORMAL;       // size 12, offset 56
    float3 Normal            : NORMAL;         // size 12, offset 68
    float4 WorldPosition     : WORLD_POSITION; // size 16, offset 80
    uint MaterialID          : MATERIALID;     // size 4,  offset 96
};

#endif

/*
// GLSLES has limited number of vertex shader registers so we have to use less bones
#if defined(GLSLES) && !defined(GL3)
#define MAX_BONE_COUNT 32
#else
#define MAX_BONE_COUNT 72
#endif

// PowerVR saturate() is compiled to min/max pair
// These are cross-platform specialized saturates that are free on PC and only cost 1 cycle on PowerVR
#ifdef GLSLES
float saturate0(float v) { return max(v, 0); }
float saturate1(float v) { return min(v, 1); }
#define WANG_SUBSET_SCALE 2
#else
float saturate0(float v) { return saturate(v); }
float saturate1(float v) { return saturate(v); }
#define WANG_SUBSET_SCALE 1
#endif

#define GBUFFER_MAX_DEPTH 500.0f


#ifndef DX11
#define TEX_DECLARE2D(name, reg) sampler2D name: register(s##reg)
#define TEX_DECLARE3D(name, reg) sampler3D name: register(s##reg)
#define TEX_DECLARECUBE(name, reg) samplerCUBE name: register(s##reg)

#define TEXTURE(name) name
#define TEXTURE_IN_2D(name) sampler2D name
#define TEXTURE_IN_3D(name) sampler3D name
#define TEXTURE_IN_CUBE(name) samplerCUBE name

#define WORLD_MATRIX(name) uniform float4x4 name;
#define WORLD_MATRIX_ARRAY(name, count) uniform float4 name [ count ];

#ifdef GLSL
#define ATTR_INT4 float4
#define ATTR_INT3 float3
#define ATTR_INT2 float2
#define ATTR_INT float
#else
#define ATTR_INT4 int4
#define ATTR_INT3 int3
#define ATTR_INT2 int2
#define ATTR_INT int
#endif
#else

#define tex2D(tex, uv) tex##Texture.Sample(tex##Sampler, uv)
#define tex3D(tex, uv) tex##Texture.Sample(tex##Sampler, uv)
#define texCUBE(tex, uv) tex##Texture.Sample(tex##Sampler, uv)
#define tex2Dgrad(tex, uv, DDX, DDY) tex##Texture.SampleGrad(tex##Sampler, uv, DDX, DDY)
#define tex2Dbias(tex, uv) tex##Texture.SampleBias(tex##Sampler, uv.xy, uv.w)
#define texCUBEbias(tex, uv) tex##Texture.SampleBias(tex##Sampler, uv.xyz, uv.w)

#define TEXTURE(name) name##Sampler, name##Texture
#define TEXTURE_IN_2D(name) SamplerState name##Sampler, Texture2D name##Texture
#define TEXTURE_IN_3D(name) SamplerState name##Sampler, Texture3D name##Texture
#define TEXTURE_IN_CUBE(name) SamplerState name##Sampler, TextureCube name##Texture

#define WORLD_MATRIX(name) cbuffer WorldMatrixCB : register( b1 ) { float4x4 name; }
#define WORLD_MATRIX_ARRAY(name, count) cbuffer WorldMatrixCB : register( b1 ) { float4 name[ count ]; }

#define ATTR_INT4 int4
#define ATTR_INT3 int3
#define ATTR_INT2 int2
#define ATTR_INT int
#endif

#if defined(GLSLES) || defined(PIN_WANG_FALLBACK)
#define TEXTURE_WANG(name) 0
void getWang(float unused, float2 uv, float tiling, out float2 wangUv, out float4 wangUVDerivatives)
{
    wangUv = uv * WANG_SUBSET_SCALE;
    wangUVDerivatives = float4(0, 0, 0, 0);    // not used in this mode
}
float4 sampleWang(TEXTURE_IN_2D(s), float2 uv, float4 wangUVDerivatives)
{
    return tex2D(s, uv);
}
#else
#define TEXTURE_WANG(name) TEXTURE(name)
void getWang(TEXTURE_IN_2D(s), float2 uv, float tiling, out float2 wangUv, out float4 wangUVDerivatives)
{
#ifndef WIN_MOBILE
    float idxTexSize = 128;
#else
    float idxTexSize = 32;
#endif

    float2 wangBase = uv * tiling * 4;

#if defined(DX11) && !defined(WIN_MOBILE)
    // compensate the precision problem of Point Sampling on some cards. (We do it just at DX11 for performance reasons)
    float2 wangUV = (floor(wangBase) + 0.5) / idxTexSize;
#else
    float2 wangUV = wangBase / idxTexSize;
#endif

#if defined(DX11) || defined(GL3)
    float2 wang = tex2D(s, wangUV).rg;
#else
    float2 wang = tex2D(s, wangUV).ba;
#endif

    wangUVDerivatives = float4(ddx(wangBase * 0.25), ddy(wangBase * 0.25));

    wang *= 255.0 / 256.0;
    wangUv = wang + frac(wangBase) * 0.25;
}
float4 sampleWang(TEXTURE_IN_2D(s), float2 uv, float4 derivates)
{
    return tex2Dgrad(s, uv, derivates.xy, derivates.zw);
}
#endif

#ifdef GLSLES
float3 nmapUnpack(float4 value)
{
    return value.rgb * 2 - 1;
}
#else
float3 nmapUnpack(float4 value)
{
    float2 xy = value.ag * 2 - 1;

    return float3(xy, sqrt(saturate(1 + dot(-xy, xy))));
}
#endif

float3 terrainNormal(float4 tnp0, float4 tnp1, float4 tnp2, float3 w, float3 normal, float3 tsel)
{
    // Inspired by "Voxel-Based Terrain for Real-Time Virtual Simulations" [Lengyel2010] 5.5.2
    float3 tangentTop = float3(normal.y, -normal.x, 0);
    float3 tangentSide = float3(normal.z, 0, -normal.x);

    float3 bitangentTop = float3(0, -normal.z, normal.y);
    float3 bitangentSide = float3(0, -1, 0);

    // Blend pre-unpack to save cycles
    float3 tn = nmapUnpack(tnp0 * w.x + tnp1 * w.y + tnp2 * w.z);

    // We blend all tangent frames together as a faster approximation to the correct world normal blend
    float tselw = dot(tsel, w);

    float3 tangent = lerp(tangentSide, tangentTop, tselw);
    float3 bitangent = lerp(bitangentSide, bitangentTop, tselw);

    return normalize(tangent * tn.x + bitangent * tn.y + normal * tn.z);
}*/
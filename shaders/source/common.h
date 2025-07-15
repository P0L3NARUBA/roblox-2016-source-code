#define EPSILON 0.0001
#define PI 3.14159

#define TEX_DECLARE2D(name, slot)                \
    Texture2D name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARE3D(name, slot)                \
    Texture3D name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)
#define TEX_DECLARECUBE(name, slot)                \
    TextureCube name##Texture : register(t##slot); \
    SamplerState name##Sampler : register(s##slot)

#ifndef EXCLUDE_STRUCTS
#define EXCLUDE_STRUCTS

struct BasicAppData {
    float3 Position : POSITION;
    float2 UV       : TEXCOORD;
    float4 Color    : COLOR;
};

struct BasicMaterialAppData {
    float3 Position : POSITION;
    float2 UV       : TEXCOORD;
    float4 Color    : COLOR;
    float3 Normal   : NORMAL;
};

struct MaterialAppData {
    float3 Position  : POSITION;
    float2 UV        : TEXCOORD;
    float4 Color     : COLOR;
    float3 Tangent   : TANGENT;
    float3 Normal    : NORMAL;
};

struct InstancedBasicAppData {
    float3 Position : POSITION;
    float2 UV       : TEXCOORD;
    float4 Color    : COLOR;
    uint InstanceID : SV_InstanceID;
    uint MaterialID;
};

struct InstancedBasicMaterialAppData {
    float3 Position : POSITION;
    float2 UV       : TEXCOORD;
    float4 Color    : COLOR;
    float3 Normal   : NORMAL;
    uint InstanceID : SV_InstanceID;
    uint MaterialID;
};

struct InstancedMaterialAppData {
    float3 Position  : POSITION;
    float2 UV        : TEXCOORD;
    float4 Color     : COLOR;
    float3 Normal    : NORMAL;
    float3 Tangent   : TANGENT;
    uint InstanceID  : SV_InstanceID;
    uint MaterialID;
};

struct BasicVertexOutput {
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD;
    float4 Color    : COLOR;
    uint MaterialID;
};

struct BasicMaterialVertexOutput {
    centroid float4 Position : SV_POSITION;
    float2 UV                : TEXCOORD;
    float4 Color             : COLOR;
    float3 Normal            : NORMAL;
    uint MaterialID;
};

struct MaterialVertexOutput {
    centroid float4 Position : SV_POSITION;
    float2 UV                : TEXCOORD;
    float4 Color             : COLOR;
    float3 Tangent           : TANGENT;
    float3 Bitangent         : BINORMAL;
    float3 Normal            : NORMAL;
    uint MaterialID;
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
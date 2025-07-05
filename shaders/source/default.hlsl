#include "common.h"

#define LQMAT_FADE_FACTOR (1.0f/300.0f)

struct Appdata
{
    float4 Position     : POSITION;
    float3 Normal       : NORMAL;
    float2 Uv           : TEXCOORD0;
    float2 UvStuds      : TEXCOORD1;

    float4 Color : COLOR0;
    ATTR_INT4 Extra : COLOR1;

#ifdef PIN_SURFACE
    float3 Tangent       : TEXCOORD2;
#endif
    float4 EdgeDistances : TEXCOORD3;
};

struct VertexOutput
{
    float4 Uv_EdgeDistance1 : TEXCOORD0;
	float4 UvStuds_EdgeDistance2 : TEXCOORD1;

    float4 Color             : COLOR0;
    float4 LightPosition_Fog : TEXCOORD2;

    float4 View_Depth : TEXCOORD3;
	#if defined(PIN_HQ) || defined(PIN_REFLECTION)
        float4 Normal_SpecPower  : TEXCOORD4;
    #endif
  
    #ifdef PIN_SURFACE
        float3 Tangent  : TEXCOORD5;
    #else
        float4 Diffuse_Specular  : COLOR1;
    #endif

    float4 PosLightSpace_Reflectance: TEXCOORD6;
    float4 WorldPosition : TEXCOORD7;
};

#ifdef PIN_SKINNED
    WORLD_MATRIX_ARRAY(WorldMatrixArray, MAX_BONE_COUNT * 3);
#endif

#ifdef PIN_DEBUG
uniform float4 DebugColor;
#endif

VertexOutput DefaultVS(Appdata IN, out float4 HPosition: POSITION)
{
    VertexOutput OUT = (VertexOutput)0;

    // Transform position and normal to world space
#ifdef PIN_SKINNED
    int boneIndex = IN.Extra.r;

    float4 worldRow0 = WorldMatrixArray[boneIndex * 3 + 0];
    float4 worldRow1 = WorldMatrixArray[boneIndex * 3 + 1];
    float4 worldRow2 = WorldMatrixArray[boneIndex * 3 + 2];
		
	float3 posWorld = float3(dot(worldRow0, IN.Position), dot(worldRow1, IN.Position), dot(worldRow2, IN.Position));
    float3 normalWorld = float3(dot(worldRow0.xyz, IN.Normal), dot(worldRow1.xyz, IN.Normal), dot(worldRow2.xyz, IN.Normal));
#else
	float3 posWorld = IN.Position.xyz;
    float3 normalWorld = IN.Normal;
#endif

    // Decode diffuse/specular parameters; encoding depends on the skinned flag due to vertex declaration differences
#if defined(PIN_DEBUG)
    float4 color = DebugColor;
#else
    float4 color = IN.Color;
#endif

    float specularIntensity = IN.Extra.g / 255.f;
    float specularPower = IN.Extra.b;

    float ndotl = dot(normalWorld, -Lamp0Dir);

#ifdef PIN_HQ
    // We'll calculate specular in pixel shader
    float2 lt = float2(saturate(ndotl), (ndotl > 0));
#else
    // Using lit here improves performance on software vertex shader implementations
    float2 lt = lit(ndotl, dot(normalize(-Lamp0Dir + normalize(CameraPosition.xyz - posWorld.xyz)), normalWorld), specularPower).yz;
#endif

	HPosition = mul(ViewProjection, float4(posWorld, 1));

	OUT.Uv_EdgeDistance1.xy = IN.Uv;
	OUT.UvStuds_EdgeDistance2.xy = IN.UvStuds;
	
    OUT.Color = color;
    OUT.LightPosition_Fog = float4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (FogParams.z - HPosition.w) * FogParams.w);

	OUT.View_Depth = float4(CameraPosition.xyz - posWorld, HPosition.w);

    OUT.WorldPosition = float4(posWorld, 1.0);

    #if defined(PIN_HQ) || defined(PIN_REFLECTION)
	    float4 edgeDistances = IN.EdgeDistances*FadeDistance_GlowFactor.z + 0.5 * OUT.View_Depth.w  * FadeDistance_GlowFactor.y;
		
	    OUT.Uv_EdgeDistance1.zw = edgeDistances.xy;
	    OUT.UvStuds_EdgeDistance2.zw = edgeDistances.zw;
        OUT.Normal_SpecPower = float4(normalWorld, specularPower);
        OUT.PosLightSpace_Reflectance.w = IN.Extra.a / 255.f;
    #endif

    #ifdef PIN_SURFACE
        #ifdef PIN_SKINNED
            float3 tangent = float3(dot(worldRow0.xyz, IN.Tangent), dot(worldRow1.xyz, IN.Tangent), dot(worldRow2.xyz, IN.Tangent));
        #else
            float3 tangent = IN.Tangent;
        #endif

        OUT.Tangent = tangent;
    #else
        float3 diffuse = lt.x * Lamp0Color + max(-ndotl, 0) * Lamp1Color;

        OUT.Diffuse_Specular = float4(diffuse, lt.y * specularIntensity);
    #endif

    OUT.PosLightSpace_Reflectance.xyz = shadowPrepareSample(posWorld);

	return OUT;
}

#ifdef PIN_SURFACE
struct SurfaceInput
{
    float4 Color;
    float2 Uv;
    float2 UvStuds;

#ifdef PIN_REFLECTION
    float Reflectance;
#endif
};

struct Surface
{
    float3 albedo;
    float3 normal;
    float specular;
    float gloss;
    float reflectance;
};

Surface surfaceShader(SurfaceInput IN, float2 fade);

Surface surfaceShaderExec(VertexOutput IN)
{
    SurfaceInput SIN;
    SIN.Color = IN.Color;
    SIN.Uv = IN.Uv_EdgeDistance1.xy;
    SIN.UvStuds = IN.UvStuds_EdgeDistance2.xy;

    #ifdef PIN_REFLECTION
        SIN.Reflectance = IN.PosLightSpace_Reflectance.w;
    #endif

    float2 fade;
	fade.x = saturate0(1 - IN.View_Depth.w * LQMAT_FADE_FACTOR );
	fade.y = saturate0(1 - IN.View_Depth.w * FadeDistance_GlowFactor.y );

    return surfaceShader(SIN, fade);
}
#endif

TEX_DECLARE2D(StudsMap, 0);

LGRID_SAMPLER(LightMap, 1);
TEX_DECLARE2D(LightMapLookup, 2);

TEX_DECLARE2D(ShadowMap, 3);

TEX_DECLARECUBE(EnvironmentMap, 4);

TEX_DECLARE2D(DiffuseMap, 5);
TEX_DECLARE2D(NormalMap, 6);
TEX_DECLARE2D(SpecularMap, 7);

#ifndef GLSLES
TEX_DECLARE2D(NormalDetailMap, 8);
#endif

uniform float4 LqmatFarTilingFactor; // material tiling factor for low-quality shader, must be the same as CFG_FAR_TILING


float4 sampleFar1(TEXTURE_IN_2D(s), float2 uv, float fade, float cutoff)
{
#ifdef GLSLES
    return tex2D(s, uv);
#else
    if (cutoff == 0)
        return tex2D(s, uv);
    else
    {
        float cscale = 1 / (1 - cutoff);

        return lerp(tex2D(s, uv * (LqmatFarTilingFactor.xy) ), tex2D(s, uv ), saturate0(fade * cscale - cutoff * cscale));
    }
#endif
}

#if defined(PIN_WANG) || defined(PIN_WANG_FALLBACK)
float4 sampleWangSimple(TEXTURE_IN_2D(s), float2 uv)
{
    float2 wangUv;
    float4 wangUVDerivatives;
    getWang(TEXTURE_WANG(NormalDetailMap), uv, 1, wangUv, wangUVDerivatives);
    return sampleWang(TEXTURE(s), wangUv, wangUVDerivatives);
}
#endif

void DefaultPS(VertexOutput IN,
#ifdef PIN_GBUFFER
    out float4 oColor1: COLOR1,
#endif
    out float4 oColor0: COLOR0)
{
    // Compute albedo term
#ifdef PIN_SURFACE
    Surface surface = surfaceShaderExec(IN);

    float4 albedo = float4(surface.albedo, IN.Color.a);

    float3 bitangent = cross(IN.Normal_SpecPower.xyz, IN.Tangent.xyz);
    float3 normal = normalize(surface.normal.x * IN.Tangent.xyz + surface.normal.y * bitangent + surface.normal.z * IN.Normal_SpecPower.xyz);

    float ndotl = dot(normal, -Lamp0Dir);

    float3 diffuseIntensity = saturate0(ndotl) * Lamp0Color + max(-ndotl, 0) * Lamp1Color;
    float specularIntensity = step(0, ndotl) * surface.specular;
    float specularPower = surface.gloss;

    float reflectance = surface.reflectance;
#elif PIN_LOWQMAT

#ifndef CFG_FAR_DIFFUSE_CUTOFF
#define CFG_FAR_DIFFUSE_CUTOFF (0.6f)
#endif

#if defined(PIN_WANG) || defined(PIN_WANG_FALLBACK)
	float4 albedo = sampleWangSimple(TEXTURE(DiffuseMap), IN.Uv_EdgeDistance1.xy);
#else
    float fade = saturate0(1 - IN.View_Depth.w * LQMAT_FADE_FACTOR);
    float4 albedo = sampleFar1(TEXTURE(DiffuseMap), IN.Uv_EdgeDistance1.xy, fade, CFG_FAR_DIFFUSE_CUTOFF);
#endif

	albedo.rgb = lerp(float3(1, 1, 1), IN.Color.rgb, albedo.a ) * albedo.rgb;
	albedo.a   = IN.Color.a;

	float3 diffuseIntensity = IN.Diffuse_Specular.xyz;
    float specularIntensity = IN.Diffuse_Specular.w;
	float reflectance = 0;

#else
    #ifdef PIN_PLASTIC
        float4 studs = tex2D(StudsMap, IN.UvStuds_EdgeDistance2.xy);
        float4 albedo = float4(IN.Color.rgb * (studs.r * 2), IN.Color.a);
	#else
        float4 albedo = tex2D(DiffuseMap, IN.Uv_EdgeDistance1.xy) * IN.Color;
	#endif

    #ifdef PIN_HQ
        float3 normal = normalize(IN.Normal_SpecPower.xyz);
        float specularPower = IN.Normal_SpecPower.w;
    #elif defined(PIN_REFLECTION)
        float3 normal = IN.Normal_SpecPower.xyz;
    #endif

    float3 diffuseIntensity = IN.Diffuse_Specular.xyz;
    float specularIntensity = IN.Diffuse_Specular.w;

    #ifdef PIN_REFLECTION
        float reflectance = IN.PosLightSpace_Reflectance.w;
    #endif

#endif

    float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
	float shadow = 1.0; //shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace_Reflectance.xyz, light.a);

    // Compute reflection term
#if defined(PIN_SURFACE) || defined(PIN_REFLECTION)
    float3 reflection = texCUBE(EnvironmentMap, reflect(-IN.View_Depth.xyz, normal)).rgb;

    albedo.rgb = lerp(albedo.rgb, reflection.rgb, reflectance);
#endif
    
    // Compute diffuse term
    float3 diffuse = AmbientColor * albedo.rgb;

    // Compute specular term
#ifdef PIN_HQ
    float3 viewDirection = normalize(IN.View_Depth.xyz);
    float uNDV = dot(normal, viewDirection);
    float NDV = (clamp(uNDV, -1.0, 1.0) + 1.0) / 2.0;

    float3 specular = Lamp0Color * Lighting(1.0, 1.0, normal, -Lamp0Dir, viewDirection, NDV, 1.0, albedo.rgb, specularPower, 1.46); //(specularIntensity * shadow * (float)(half)pow(saturate(dot(normal, normalize(-Lamp0Dir + viewDirection))), specularPower));

    uint numberOfLights, stride;
    LightList.GetDimensions(numberOfLights, stride);

    for (int i = 0; i < numberOfLights; ++i) {
        GPULight Light = LightList[i];
        int LightActive = int(Light.ShadowsColored_Type_Active.a);

        if (LightActive != 1)
            break;

        float3 LightPosition = Light.Position_Range.xyz;
        float3 LightColour = Light.Color_Attenuation.xyz;
        float3 LightDirection = normalize(LightPosition - IN.WorldPosition.xyz);
        float Range = Light.Position_Range.w;
        float2 InnerOuterAngleCos = Light.InnerOuterAngle_DiffSpecFac.xy;
        float2 DiffSpecScale = Light.InnerOuterAngle_DiffSpecFac.zw;
        
        float AngleFalloff = 1.0;

        if (InnerOuterAngleCos.y < 1.0)
        {
            float theta = dot(LightDirection, normalize(-Light.Direction_SubSurfaceFac.xyz));
            float Epsilon = max(InnerOuterAngleCos.x - 0.001, InnerOuterAngleCos.y) + 0.001;

            AngleFalloff = smoothstep(InnerOuterAngleCos.y, Epsilon, theta);
        }

        float Distance = length(LightPosition - IN.WorldPosition.xyz);
        float LinearFalloff = saturate(1.0 - Distance / Range) * AngleFalloff;
        float Attenuation = 1.0 / (1.0 + Light.Color_Attenuation.w * Distance);

        if (LinearFalloff > 0.0) {
            specular += max(Lighting(DiffSpecScale.x, DiffSpecScale.y, normal, LightDirection, viewDirection, NDV, Attenuation, albedo.rgb, specularPower, 1.46) * shadow * LightColour * LinearFalloff, 0.0);
        }
    }
#else
    float3 specular = Lamp0Color * (specularIntensity * shadow);
#endif

    // Combine
    oColor0.rgb = diffuse.rgb + specular.rgb;
    oColor0.a   = albedo.a;

#ifdef PIN_HQ
	float ViewDepthMul = IN.View_Depth.w * FadeDistance_GlowFactor.y;
	float outlineFade = saturate1( ViewDepthMul * OutlineBrightness_ShadowInfo.x + OutlineBrightness_ShadowInfo.y);
	float2 minIntermediate = min(IN.Uv_EdgeDistance1.wz, IN.UvStuds_EdgeDistance2.wz);
	float minEdgesPlus = min(minIntermediate.x, minIntermediate.y) / ViewDepthMul;
	oColor0.rgb *= saturate1(outlineFade *(1.5 - minEdgesPlus) + minEdgesPlus);
#endif

    float fogAlpha = saturate(IN.LightPosition_Fog.w);

#ifdef PIN_NEON
    oColor0.rgb = pow(IN.Color.rgb, 2.2) * 4.0;
    oColor0.a = IN.Color.a;
    diffuse.rgb = 0;
    specular.rgb = 0;
#endif

    oColor0.rgb = lerp(FogColor, oColor0.rgb, fogAlpha);

#ifdef PIN_GBUFFER
    oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif

}

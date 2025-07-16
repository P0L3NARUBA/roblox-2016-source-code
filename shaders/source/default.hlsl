#define GLOBALS
#define INSTANCED

#include "buffers.hlsli"
#include "common.hlsli"

/* Vertex Shaders */
BasicMaterialVertexOutput DepthOnlyVS(InstancedBasicMaterialAppData IN) {
    BasicMaterialVertexOutput OUT;

    OUT.Position = mul(float4(IN.Position.xyz, 1.0), ModelMatrixes[IN.InstanceID].Model);
    OUT.Position = mul(OUT.Position, ViewProjection);

    return OUT;
}

BasicMaterialVertexOutput BasicMaterialVS(InstancedBasicMaterialAppData IN) {
    BasicMaterialVertexOutput OUT;

    float4x4 ModelMatrix = ModelMatrixes[IN.InstanceID].Model;

    OUT.Position = mul(float4(IN.Position.xyz, 1.0), ModelMatrix);
    OUT.Position = mul(OUT.Position, ViewProjection);

    OUT.UV = IN.UV;
    OUT.Color = IN.Color;
    OUT.Normal = normalize(mul((float3x3)ModelMatrix, IN.Normal));
    OUT.MaterialID = IN.MaterialID;

    return OUT;
}

MaterialVertexOutput MaterialVS(InstancedMaterialAppData IN) {
    MaterialVertexOutput OUT;

    float4x4 ModelMatrix = ModelMatrixes[IN.InstanceID].Model;

    OUT.Position = mul(float4(IN.Position.xyz, 1.0), ModelMatrix);
    OUT.Position = mul(OUT.Position, ViewProjection);

    OUT.UV = IN.UV;
    OUT.Color = IN.Color;

    float3 Normal = normalize(mul((float3x3)ModelMatrix, IN.Normal));
    float3 Tangent = normalize(mul((float3x3)ModelMatrix, IN.Tangent));
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);

    OUT.Tangent = Tangent;
    OUT.Bitangent = cross(Normal, Tangent);
    OUT.Normal = Normal;
    OUT.MaterialID = IN.MaterialID;

    return OUT;
}

/* Pixel Shader */
void BlankPS(BasicMaterialVertexOutput IN) : SV_TARGET {
}

float4 DefaultPS(BasicMaterialVertexOutput IN) : SV_TARGET {
    return float4(1.0, 0.0, 1.0, 1.0);
}

/*void DefaultMaterialPS(VertexOutput IN, out float4 OUT : SV_TARGET) {
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

    albedo.rgb = lerp(float3(1, 1, 1), IN.Color.rgb, albedo.a) * albedo.rgb;
    albedo.a = IN.Color.a;

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
    float shadow = 1.0; // shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace_Reflectance.xyz, light.a);

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

    for (int i = 0; i < 1024; ++i)
    {
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

        float3 DistanceVector = LightPosition - IN.WorldPosition.xyz;
        float Distance = dot(DistanceVector, DistanceVector);
        float LinearFalloff = saturate(1.0 - Distance / Range) * AngleFalloff;
        float Attenuation = 1.0 / (1.0 + Light.Color_Attenuation.w * Distance);

        if (LinearFalloff > 0.0)
        {
            specular += max(Lighting(DiffSpecScale.x, DiffSpecScale.y, normal, LightDirection, viewDirection, NDV, Attenuation, albedo.rgb, specularPower, 1.46) * shadow * LightColour * LinearFalloff, 0.0);
        }
    }
#else
    float3 specular = float3(0.0, 0.0, 0.0); // Lamp0Color * (specularIntensity * shadow);
#endif

    // Combine
    oColor0.rgb = diffuse.rgb + specular.rgb;
    oColor0.a = albedo.a;

#ifdef PIN_HQ
    float ViewDepthMul = IN.View_Depth.w * FadeDistance_GlowFactor.y;
    float outlineFade = saturate1(ViewDepthMul * OutlineBrightness_ShadowInfo.x + OutlineBrightness_ShadowInfo.y);
    float2 minIntermediate = min(IN.Uv_EdgeDistance1.wz, IN.UvStuds_EdgeDistance2.wz);
    float minEdgesPlus = min(minIntermediate.x, minIntermediate.y) / ViewDepthMul;
    oColor0.rgb *= saturate1(outlineFade * (1.5 - minEdgesPlus) + minEdgesPlus);
#endif

    float fogAlpha = saturate(IN.LightPosition_Fog.w);

#ifdef PIN_NEON
    oColor0.rgb = pow(IN.Color.rgb, 2.2) * 16.0;
    oColor0.a = IN.Color.a;
    diffuse.rgb = 0;
    specular.rgb = 0;
#endif

    oColor0.rgb = lerp(FogColor, oColor0.rgb, fogAlpha);

#ifdef PIN_GBUFFER
    oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif
}*/
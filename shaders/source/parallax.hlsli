#define MIN_LAYERS 8.0f
#define MAX_LAYERS 32.0f

float2 ParallaxOcclusionMapping(Texture2DArray<float> Texture, SamplerState Sampler, float3 UVW, float3 ViewDirection, float HeightScale, float HeightOffset) {
    float NumLayers = lerp(MAX_LAYERS, MIN_LAYERS, abs(ViewDirection.z));
    
    float2 DeltaTexCoords = (ViewDirection.xy / ViewDirection.z * HeightScale) / NumLayers;
    
    float3 CurrentTexCoords = UVW;
    float CurrentDepthMapValue = 1.0f - Texture.Sample(Sampler, CurrentTexCoords);
    float CurrentLayerDepth = 0.0f;
    float LayerDepth = rcp(NumLayers);
    
    [loop]
    while (CurrentLayerDepth < CurrentDepthMapValue) {
        CurrentTexCoords.xy -= DeltaTexCoords;
        CurrentDepthMapValue = 1.0f - Texture.Sample(Sampler, CurrentTexCoords);  
        CurrentLayerDepth += LayerDepth;
    }

    float2 PrevTexCoords = CurrentTexCoords.xy + DeltaTexCoords;

    float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    float BeforeDepth = (1.0f - Texture.Sample(Sampler, float3(PrevTexCoords, UVW.z))) - CurrentLayerDepth + LayerDepth;
 
    float Weight = AfterDepth / (AfterDepth - BeforeDepth);

    return mad(PrevTexCoords, Weight, CurrentTexCoords.xy * (1.0f - Weight));
}

float3 ParallaxCorrection(float3 Position, float3 Reflect, float3 CubemapPosition, float3x3 CubemapMatrix) {
    float3 Ray = mul(Reflect, CubemapMatrix);
    float3 LocalPosition = mul(Position, CubemapMatrix);

    float3 Unitary = float3(1.0f, 1.0f, 1.0f);
    float3 FirstPlaneIntersect  = (Unitary - LocalPosition) / Ray;
    float3 SecondPlaneIntersect = (-Unitary - LocalPosition) / Ray;
    float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
    float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));

    return (Position + Reflect * Distance) - CubemapPosition;
}
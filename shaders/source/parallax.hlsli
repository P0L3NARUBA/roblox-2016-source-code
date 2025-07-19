#define MAX_LAYERS 32.0
#define MIN_LAYERS 8.0

float2 ParallaxOcclusionMapping(Texture2DArray<float> Texture, SamplerState Sampler, float3 UVW, float3 ViewDirection, float HeightScale, float HeightOffset) {
    float NumLayers = lerp(MAX_LAYERS, MIN_LAYERS, abs(ViewDirection.z));
    float LayerDepth = 1.0 / NumLayers;
    
    float2 P = ViewDirection.xy / ViewDirection.z * HeightScale; 
    float2 DeltaTexCoords = P / NumLayers;
    
    float3 CurrentTexCoords = UVW;
    float CurrentDepthMapValue = 1.0 - Texture.Sample(Sampler, CurrentTexCoords);
    float CurrentLayerDepth = 0.0;

    while (CurrentLayerDepth < CurrentDepthMapValue) {
        CurrentTexCoords.xy -= DeltaTexCoords;
        CurrentDepthMapValue = 1.0 - Texture.Sample(Sampler, CurrentTexCoords);  
        CurrentLayerDepth += LayerDepth;  
    }

    float2 PrevTexCoords = CurrentTexCoords.xy + DeltaTexCoords;

    float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    float BeforeDepth = (1.0 - Texture.Sample(Sampler, float3(PrevTexCoords, UVW.z))) - CurrentLayerDepth + LayerDepth;
 
    float Weight = AfterDepth / (AfterDepth - BeforeDepth);

    return PrevTexCoords * Weight + CurrentTexCoords.xy * (1.0 - Weight);
}

float3 ParallaxCorrectionAABB(float3 Position, float3 Reflect, float3 CubemapPosition, float3 CubemapMin, float3 CubemapMax) {
    float3 FirstPlaneIntersect = (CubemapMax - Position) / Reflect;
    float3 SecondPlaneIntersect = (CubemapMin - Position) / Reflect;
    float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
    float Distance = min(min(FurthestPlane.x, FurthestPlane.y), FurthestPlane.z);

    return (Position + Reflect * Distance) - CubemapPosition;
}

float3 ParallaxCorrectionOBB(float3 Position, float3 Reflect, float3 CubemapPosition, float3x3 CubemapMatrix) {
    float3 Ray = mul(CubemapMatrix, Reflect);
    float3 LocalPosition = mul(CubemapMatrix, Position);

    float3 Unitary = float3(1.0, 1.0, 1.0);
    float3 FirstPlaneIntersect  = (Unitary - LocalPosition) / Ray;
    float3 SecondPlaneIntersect = (-Unitary - LocalPosition) / Ray;
    float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
    float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));

    return (Position + Reflect * Distance) - CubemapPosition;
}
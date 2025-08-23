#define GLOBALS

#include "buffers.hlsli"
#include "common.hlsli"

#ifdef SINGLE_FACE
TEX_DECLARE2D(float3, SkyboxFace, 0);

float4 SkyFacePS(BasicVertexOutput IN) : SV_TARGET {
    float3 Skybox = pow(SkyboxFaceTexture.Sample(LinearClamp, IN.UV), 2.2f);
    Skybox = Skybox / (1.0f - Skybox * 0.9875f); // Inverse Reinhard Operator, by pure coincidence.

    return float4(Skybox, 1.0f);
}

#else
struct SkyVertexOutput {
    centroid float4 Position : SV_POSITION;
             float3 UVW      : TEXCOORD;
};

SkyVertexOutput SkyVS(BasicAppData IN) {
    SkyVertexOutput OUT;

    OUT.Position = mul(float4(IN.Position + CameraPosition.xyz, 1.0f), ViewProjection[0]);
    OUT.Position.z = 0.0f;

    OUT.UVW = IN.Position;
    OUT.UVW.x = -OUT.UVW.x;

    return OUT;
}

#ifdef EQUIRECTANGULAR
TEX_DECLARE2D(float3, SkyboxHDRI, 0);

float2 SampleSphericalMap(float3 V) {
    float2 UV = float2(atan2(V.z, V.x), asin(V.y));
    UV *= float2(0.1591f, -0.3183f); // Inverse arc tangent
    UV += 0.5f;

    return UV;
}

float4 SkyFacePS(SkyVertexOutput IN) : SV_TARGET {
    return float4(SkyboxHDRITexture.Sample(LinearClamp, SampleSphericalMap(normalize(IN.UVW))), 1.0f);
}

#elif IRRADIANCE
TEX_DECLARECUBE(float3, Skybox, 0);

float4 SkyIrradiancePS(SkyVertexOutput IN) : SV_TARGET {
    float3 Normal = normalize(IN.UVW);
    float3 Irradiance = float3(0.0f, 0.0f, 0.0f);

    float3 Up = float3(0.0f, 1.0f, 0.0f);
    float3 Right = normalize(cross(Up, Normal));
    Up = normalize(cross(Normal, Right));

    float SampleDelta = 0.025f;
    float Samples = 0.0f; 

    [loop]
    for (float Phi = 0.0f; Phi < 2.0f * PI; Phi += SampleDelta) {
        [loop]
        for (float Theta = 0.0f; Theta < 0.5f * PI; Theta += SampleDelta) {
            float3 TangentSample = float3(sin(Theta) * cos(Phi),  sin(Theta) * sin(Phi), cos(Theta));
            float3 SampleVec = TangentSample.x * Right + TangentSample.y * Up + TangentSample.z * Normal; 

            Irradiance += SkyboxTexture.SampleLevel(LinearWrap, SampleVec, 0.0f) * cos(Theta) * sin(Theta);
            Samples++;
        }
    }

    Irradiance = PI * Irradiance * (1.0f / float(Samples));

    return float4(Irradiance, 1.0f);
}

#elif CONVOLUTION
TEX_DECLARECUBE(float3, Skybox, 0);

float DistributionGGX(float NDH, float roughness2) {
    float NdotH2 = NDH * NDH;
	
    float num = roughness2;
    float denom = (NdotH2 * (roughness2 - 1.0f) + 1.0f);
	
    return num / (PI * denom * denom);
}

float RadicalInverse_VdC(uint bits) {
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N) {
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness2) {
	float phi = 2.0f * PI * Xi.x;
	float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (roughness2 * roughness2 - 1.0f) * Xi.y));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	float3 Up = abs(N.z) < INV_EPSILON ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 Tangent = normalize(cross(Up, N));
	float3 Bitangent = cross(N, Tangent);

	return normalize(Tangent * H.x + Bitangent * H.y + N * H.z);
}

#define SAMPLE_COUNT 128

float4 SkyConvolutionPS(SkyVertexOutput IN) : SV_TARGET {
    float3 N = normalize(IN.UVW);
    float3 R = N;
    float3 V = R;

    float Roughness = ViewportSize_ViewportScale.y;

    float Roughness2 = Roughness * Roughness;
    
    float3 Convolution = float3(0.0f, 0.0f, 0.0f);
    float TotalWeight = 0.0f;
    
    [loop]
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, Roughness2);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NDL = max(dot(N, L), 0.0f);

        if (NDL > 0.0f) {
            float NDH = max(dot(N, H), 0.0f);
            float HDV = max(dot(H, V), 0.0f);

            float D = DistributionGGX(NDH, Roughness2);
            float PDF = D * NDH / (4.0f * HDV) + EPSILON; 

            float Resolution = ViewportSize_ViewportScale.x;
            float SaTexel  = 4.0f * PI / (6.0f * Resolution * Resolution);
            float SaSample = 1.0f / (float(SAMPLE_COUNT) * PDF + EPSILON);

            float MipLevel = (Roughness == 0.0f) ? 0.0f : 0.5f * log2(SaSample / SaTexel); 
            
            Convolution += SkyboxTexture.SampleLevel(LinearWrap, L, MipLevel) * NDL;
            TotalWeight += NDL;
        }
    }

    return float4(Convolution / TotalWeight, 1.0f);
}

#else
TEX_DECLARECUBE(float3, Skybox, 0);

float4 SkyPS(SkyVertexOutput IN) : SV_TARGET {
    return float4(IN.UVW * 0.5 + 0.5, 1.0f);
    //return float4(SkyboxTexture.SampleLevel(LinearClamp, IN.UVW, 0.0), 1.0);
}

#endif
#endif
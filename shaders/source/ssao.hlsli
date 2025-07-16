#define PROCESSING

#include "common.hlsli"
#include "buffers.hlsli"

TEX_DECLARE2D(Depth, 0);

float4 ReconstructNormalPS(BasicVertexOutput IN) : SV_TARGET {
    float2 stc = IN.UV;
    float depth = DepthTexture.Sample(DepthSampler, stc).x;

    float2 ScreenSize = TextureSize_ViewportScale.zw;
    float2 u = float2(ScreenSize.x, 0.0);
    float2 v = float2(0.0, ScreenSize.y);

    float4 H;
    H.x = DepthTexture.Sample(DepthSampler, stc - u).x;
    H.y = DepthTexture.Sample(DepthSampler, stc + u).x;
    H.z = DepthTexture.Sample(DepthSampler, stc - u * 2.0).x;
    H.w = DepthTexture.Sample(DepthSampler, stc + u * 2.0).x;
    float2 he = abs(H.xy * H.zw * rcp(2.0 * H.zw - H.xy) - depth);
    float3 hDeriv;
    if (he.x > he.y)
        hDeriv = 0.0; //Calculate horizontal derivative of world position from taps | * | y |
    else
        hDeriv = 0.0; //Calculate horizontal derivative of world position from taps | x | * |

    float4 V;
    V.x = DepthTexture.Sample(DepthSampler, stc - v).x;
    V.y = DepthTexture.Sample(DepthSampler, stc + v).x;
    V.z = DepthTexture.Sample(DepthSampler, stc - v * 2.0).x;
    V.w = DepthTexture.Sample(DepthSampler, stc + v * 2.0).x;
    float2 ve = abs(V.xy * V.zw * rcp(2 * V.zw - V.xy) - depth);
    float3 vDeriv;
    if (ve.x > ve.y)
        vDeriv = 0.0; //Calculate vertical derivative of world position from taps | * | y |
    else
        vDeriv = 0.0; //Calculate vertical derivative of world position from taps | x | * |

    return float4(normalize(cross(hDeriv, vDeriv)), 1.0);
}
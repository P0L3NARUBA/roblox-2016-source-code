#include "common.h"

struct Appdata
{
    float4 Position	    : POSITION;
    float2 Uv	        : TEXCOORD0;
    float4 Color        : COLOR0;
};

struct VertexOutput
{
    float4 HPosition    : POSITION;
    float PSize         : PSIZE;

    float2 Uv           : TEXCOORD0;
    float4 Color        : COLOR0;
};

WORLD_MATRIX(WorldMatrix);

uniform float4 Color;
uniform float4 Color2;

VertexOutput SkyVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;

    float4 wpos = mul(WorldMatrix, IN.Position);

    OUT.HPosition = mul(ViewProjection, wpos);

    // Snap to furthest depth to prevent scene-sky intersections
    OUT.HPosition.z = 0.0;

    OUT.PSize = 2.0; // star size

    OUT.Uv = IN.Uv;
    OUT.Color = IN.Color * lerp(Color2, Color, wpos.y / 1700);
    //OUT.Color = IN.Color * Color;

    return OUT;
}

TEX_DECLARE2D(DiffuseMap, 0);

float4 SkyPS(VertexOutput IN): COLOR0
{
    float4 skybox = tex2D(DiffuseMap, IN.Uv) * IN.Color;

    float3 Brightness = skybox.rgb;
	Brightness = ((Brightness / (1.0 - Brightness * 0.9875)) / 8.0);

    return float4(skybox.rgb + Brightness, skybox.a);
}

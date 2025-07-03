#if defined(PIN_HQ)
#define PIN_SURFACE
#include "default.hlsl"

#define CFG_TEXTURE_TILING              1

#define CFG_BUMP_INTENSITY              0.5

#define CFG_SPECULAR					0.4
#define CFG_GLOSS						0.4

#define CFG_NORMAL_SHADOW_SCALE         0.1

Surface surfaceShader(SurfaceInput IN, float2 fade2)
{
    Surface surface = (Surface)0;
    surface.albedo = IN.Color.rgb;
    surface.normal = float3(0.0, 0.0, 1.0);;
    surface.specular = (CFG_SPECULAR);
    surface.gloss = (CFG_GLOSS);

#ifdef PIN_REFLECTION
    surface.reflectance = IN.Reflectance;
#endif

	return surface;
}
#else
#define PIN_PLASTIC
#include "default.hlsl"
#endif

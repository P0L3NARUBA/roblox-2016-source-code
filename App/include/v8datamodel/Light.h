#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/Effect.h"

namespace RBX
{
	extern const char* const sLight;
	class Light : public DescribedNonCreatable<Light, Instance, sLight>
					, public Effect

	{
	public:
		explicit Light(const char* name);
		virtual ~Light();
		
		bool getEnabled() const { return enabled; }
		void setEnabled(bool enabled);

		Color3 getColor() const { return color; }
		void setColor(Color3 color);

		float getBrightness() const { return brightness; }
        void setBrightness(float brightness);

		float getRange() const { return range; }
		void setRange(float range);

		float getAttenuation() const { return attenuation; }
		void setAttenuation(float attenuation);

		float getDiffuseFactor() const { return diffuseFactor; }
		void setDiffuseFactor(float diffuseFactor);

		float getSpecularFactor() const { return specularFactor; }
		void setSpecularFactor(float specularFactor);

		bool getShadows() const { return shadows; }
		void setShadows(bool shadows);

		bool getColored() const { return colored; }
		void setColored(bool colored);

		static Reflection::PropDescriptor<Light, bool> prop_Enabled;
		static Reflection::PropDescriptor<Light, Color3> prop_Color;
		static Reflection::PropDescriptor<Light, float> prop_Brightness;
		static Reflection::PropDescriptor<Light, float> prop_Range;
		static Reflection::PropDescriptor<Light, float> prop_Attenuation;
		static Reflection::PropDescriptor<Light, float> prop_DiffuseFactor;
		static Reflection::PropDescriptor<Light, float> prop_SpecularFactor;
		static Reflection::PropDescriptor<Light, bool> prop_Shadows;
		static Reflection::PropDescriptor<Light, bool> prop_Colored;

		static Reflection::PropDescriptor<Light, bool> prop_Shadows_deprecated;

	protected:
		bool askSetParent(const Instance* parent) const;
		bool askAddChild(const Instance* instance) const;
	
	private:
		bool enabled;
		Color3 color;
        float brightness;
		float range;
		float attenuation;
		float diffuseFactor;
		float specularFactor;
		bool shadows;
		bool colored;
	};
	
	extern const char* const sPointLight;
	class PointLight : public DescribedCreatable<PointLight, Light, sPointLight>
	{
	public:
		PointLight();
		virtual ~PointLight();

	};
	
	extern const char* const sSpotLight;
	class SpotLight : public DescribedCreatable<SpotLight, Light, sSpotLight>
	{
	public:
		SpotLight();
		virtual ~SpotLight();
        
        float getOuterAngle() const { return outerAngle; }
        void setOuterAngle(float outerAngle);

		float getInnerAngle() const { return innerAngle; }
		void setInnerAngle(float innerAngle);
        
		NormalId getFace() const { return face; }
		void setFace(RBX::NormalId value);

		static Reflection::PropDescriptor<SpotLight, float> prop_OuterAngle;
		static Reflection::PropDescriptor<SpotLight, float> prop_InnerAngle;
		static Reflection::EnumPropDescriptor<SpotLight, NormalId> prop_Face;

	private:
        float outerAngle;
		float innerAngle;
        NormalId face;
	};

	extern const char* const sAreaLight;
	class AreaLight : public DescribedCreatable<AreaLight, Light, sAreaLight>
	{
	public:
		AreaLight();
		virtual ~AreaLight();

		float getOuterAngle() const { return outerAngle; }
		void setOuterAngle(float outerAngle);

		float getInnerAngle() const { return innerAngle; }
		void setInnerAngle(float innerAngle);
        
		NormalId getFace() const { return face; }
		void setFace(RBX::NormalId value);

		static Reflection::PropDescriptor<AreaLight, float> prop_OuterAngle;
		static Reflection::PropDescriptor<AreaLight, float> prop_InnerAngle;
		static Reflection::EnumPropDescriptor<AreaLight, NormalId> prop_Face;

	private:
		float outerAngle;
		float innerAngle;
        NormalId face;
	};

}	// namespace RBX

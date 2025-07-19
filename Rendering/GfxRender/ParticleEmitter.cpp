#include "stdafx.h"
#include "ParticleEmitter.h"

#include "v8datamodel/Smoke.h"
#include "v8datamodel/Sparkles.h"
#include "v8datamodel/Fire.h"
#include "v8datamodel/ForceField.h"
#include "v8datamodel/Explosion.h"

#include "SceneUpdater.h"
#include "Util.h"

#include "Emitter.h"
#include "TextureManager.h"

#include "VisualEngine.h"

DYNAMIC_FASTFLAGVARIABLE(SphericalSparklesEmission, false)

namespace RBX
{
	namespace Graphics
	{

		ParticleEmitter::ParticleEmitter(VisualEngine* visualEngine)
			: Super(visualEngine, CullMode_SpatialHash)
			, dirty(false)
			, enabled(false)
			, localBox(Vector3::zero(), Vector3::zero())
		{
		}

		ParticleEmitter::~ParticleEmitter()
		{
			unbind();

			// notify scene updater about destruction so that the pointer to ParticleEmitter is no longer stored
			getVisualEngine()->getSceneUpdater()->notifyDestroyed(this);
		}

		void ParticleEmitter::onSleepingChangedEx(bool sleeping)
		{
			if (sleeping)
			{
				getVisualEngine()->getSceneUpdater()->notifySleeping(this);
			}
			else
			{
				getVisualEngine()->getSceneUpdater()->notifyAwake(this);
			}
		}

		void ParticleEmitter::updateCoordinateFrame(bool recalcLocalBounds)
		{
			CoordinateFrame frame = part ? part->calcRenderingCoordinateFrame() : CoordinateFrame();

			if (RBX::Explosion* expl = Instance::fastDynamicCast<RBX::Explosion>(effect.get()))
			{
				frame.rotation = Matrix3::identity();
				frame.translation = expl->getPosition();
			}

			if (!dirty && transform.fuzzyEq(frame))
			{
				// Nothing to update
				return;
			}

			transform = frame;

			// Update particle transforms
			if (emitter)
			{
				emitter->transform() = transform * emitterOffset;
			}

			if (secondary)
			{
				secondary->transform() = transform * secondaryOffset;
			}

			if (tertiary)
			{
				tertiary->transform() = transform * tertiaryOffset;
			}

			// Update world-space extents
			updateWorldBounds(localBox.toWorldSpace(transform));
		}

		void ParticleEmitter::onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data)
		{
			switch (type)
			{
			case Instance::PROPERTY_CHANGED:
				onPropertyChangedEx(boost::polymorphic_downcast<const Instance::PropertyChangedSignalData*>(data)->propertyDescriptor);
				break;
			case Instance::ANCESTRY_CHANGED:
				onAncestorChangedEx();
				break;
			default:
				break;
			}
		}

		void ParticleEmitter::onPropertyChangedEx(const RBX::Reflection::PropertyDescriptor* descriptor)
		{
			invalidateEntity();
		}

		void ParticleEmitter::onAncestorChangedEx()
		{
			shared_ptr<Instance> effectCopy = effect;

			// Remove me from the scene if I am being removed from the Workspace
			if (!isInWorkspace(effectCopy.get()))
			{
				// will cause a delete on next updateEntity()
				zombify();
			}
			else
			{
				unbind();

				RBX::PartInstance* parent = RBX::Instance::fastDynamicCast<RBX::PartInstance>(effectCopy->getParent());
				shared_ptr<RBX::PartInstance> part = shared_from(parent);

				bind(part, effectCopy);
			}
		}

		void ParticleEmitter::bind(const shared_ptr<RBX::PartInstance>& part, const shared_ptr<RBX::Instance>& instance)
		{
			RBXASSERT(!this->part && !this->effect);
			RBXASSERT(instance);

			this->part = part;
			this->effect = instance;

			connections.push_back(instance->combinedSignal.connect(boost::bind(&ParticleEmitter::onCombinedSignalEx, this, _1, _2)));

			if (part)
			{
				connections.push_back(part->onDemandWrite()->sleepingChangedSignal.connect(boost::bind(&ParticleEmitter::onSleepingChangedEx, this, _1)));

				// we just connected, so sync up the state.
				onSleepingChangedEx(part->getSleeping());
			}

			invalidateEntity();
		}

		void ParticleEmitter::unbind()
		{
			Super::unbind();

			part.reset();
			effect.reset();
		}

		void ParticleEmitter::invalidateEntity()
		{
			if (!dirty)
			{
				dirty = true;

				getVisualEngine()->getSceneUpdater()->queueInvalidatePart(this);
			}
		}

		void ParticleEmitter::updateEntity(bool assetsUpdated)
		{
			if (connections.empty()) // zombified.
			{
				getVisualEngine()->getSceneUpdater()->destroyAttachment(this);
				return;
			}

			applySettings();
			updateCoordinateFrame(true);

			dirty = false;
		}

		void ParticleEmitter::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
		{
			if (!enabled && emitter && !emitter->pcount())
				emitter.reset();

			if (!enabled && secondary && !secondary->pcount())
				secondary.reset();

			if (!enabled && tertiary && !tertiary->pcount())
				secondary.reset();

			if (emitter)
				emitter->draw(queue);

			if (secondary)
				secondary->draw(queue);

			if (tertiary)
				tertiary->draw(queue);

			// Render bounding box
			if (getVisualEngine()->getSettings()->getDebugShowBoundingBoxes() && (emitter || secondary || tertiary))
				debugRenderBoundingBox();
		}

		void ParticleEmitter::applySettings()
		{
			if (!effect) return;

			static const Vector4 zzz(0, 0, 0, 0);
			static const Vector3 one(1, 1, 1);
			emitterOffset = CoordinateFrame();
			secondaryOffset = CoordinateFrame();
			tertiaryOffset = CoordinateFrame();

			if (RBX::Sparkles* spk = effect->fastDynamicCast<RBX::Sparkles>())
			{
				enabled = spk->getEnabled();
				Vector4 c = Color4(spk->getColor());
				c.w = 0.9f; //spk->getAlpha();

				if (enabled && !emitter)
					emitter.reset(new Emitter(getVisualEngine()));

				if (emitter)
				{
					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/common_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/sparkles_color.dds";
					a.mainTexture = "rbxasset://textures/particles/sparkles_main.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_CrazySparkles;
					a.blendCode = Emitter::Blend_PremultipliedAlpha;

					emitter->setAppearance(a);
					emitter->setDampening(0.2f);
					emitter->setGlobalForce(Vector3(0.0f, 0.0f, 0.0f));
					emitter->setGrowth(Vector2(0.5f, 0.5f));
					emitter->setLocalForce(Vector3(0.0f, 1.0f, 0.0f));
					emitter->setRotation(Vector2(-90.0f / 57.3f, 90.0f / 57.3f));
					emitter->setSizeX(Vector2(0.37f, 0.37f));
					emitter->setSizeY(Vector2(0.37f, 0.37f));
					emitter->setSpeed(Vector2(5.0f, 5.0f));
					emitter->setSpin(Vector2(40.0f / 57.3f, 100.0f / 57.3f));

					if (!DFFlag::SphericalSparklesEmission)
					{
						emitter->setSpread(Vector2(100.0f / 57.3f, 100.0f / 57.3f));
					}
					else
					{
						emitter->setSpread(Math::pif() * Vector2(1.0f, 1.0f));
					}

					emitter->setEmissionRate(30.0f * enabled);
					emitter->setEmitterShape(0, Box(Vector3(-0.2f, -0.2f, -0.2f), Vector3(0.2f, 0.2f, 0.2f)));
					emitter->setLife(Vector2(1.3f, 1.3f));

					emitter->setModulateColor(c);
					emitter->setBlendRatio(0.6f);
				}

				if (enabled && !secondary)
					secondary.reset(new Emitter(getVisualEngine()));

				if (secondary)
				{
					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/common_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/sparkles_color.dds";
					a.mainTexture = "rbxasset://textures/particles/sparkles_main.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_CrazySparkles;
					a.blendCode = Emitter::Blend_PremultipliedAlpha;

					secondary->setAppearance(a);
					secondary->setDampening(2.0f);
					secondary->setGlobalForce(Vector3(0.0f, 0.0f, 0.0f));
					secondary->setGrowth(Vector2(0.2f, 0.2f));
					secondary->setLocalForce(Vector3(0.0f, 0.0f, 0.0f));
					secondary->setRotation(Vector2(-90.0f / 57.3f, 90.0f / 57.3f));
					secondary->setSizeX(Vector2(0.1f, 0.1f));
					secondary->setSizeY(Vector2(0.1f, 0.1f));
					secondary->setSpeed(Vector2(8.0f, 8.0f));
					secondary->setSpin(Vector2(-500.0f / 57.3f, 500.0f / 57.3f));

					if (!DFFlag::SphericalSparklesEmission)
					{
						secondary->setSpread(Vector2(150.0f / 57.3f, 150.0f / 57.3f));
						secondaryOffset = CoordinateFrame(Vector3(0.0f, 4.0f, 0.0f));
					}
					else
					{
						secondary->setSpread(Math::pif() * Vector2(1.0f, 1.0f));
						secondaryOffset = CoordinateFrame(Vector3(0.0f, 0.0f, 0.0f));
					}

					secondary->setEmissionRate(5.0f * enabled);
					secondary->setEmitterShape(0.0f, Box(Vector3(-0.2f, -0.2f, -0.2f), Vector3(0.2f, 0.2f, 0.2f)));
					secondary->setLife(Vector2(1.7f, 1.7f));
					secondary->setModulateColor(c);
					secondary->setBlendRatio(0.6f);
				}

				tertiary.reset();
			}
			else if (RBX::Smoke* smk = effect->fastDynamicCast<RBX::Smoke>())
			{
				enabled = smk->getEnabled();

				if (enabled && !emitter)
					emitter.reset(new Emitter(getVisualEngine()));

				if (emitter)
				{
					float size = smk->getClampedSize();
					float maxSize = 10.0f + size;
					float rise = smk->getClampedRiseVelocity();
					float opacity = smk->getClampedOpacity();
					float life = 5.0f;
					Vector4 c0(Color4(smk->getColor(), opacity));
					Vector4 c1(Color4(smk->getColor(), 0.0f));

					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/common_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/smoke_color.dds";
					a.mainTexture = "rbxasset://textures/particles/smoke_main.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Modulate;
					a.blendCode = Emitter::Blend_AlphaBlend;

					emitter->setAppearance(a);
					emitter->setDampening(0.1f);
					emitter->setGlobalForce(Vector3(0.0f, 0.0f, 0.4f));
					emitter->setGrowth((maxSize - size) / life * Vector2(1.0f, 1.0f));
					emitter->setLocalForce(Vector3(0.0f, 0.0f, 0.0f));
					emitter->setRotation(Vector2(-90.0f / 57.3f, 90.0f / 57.3f));
					emitter->setSizeX(size * Vector2(1.0f, 1.0f));
					emitter->setSizeY(size * Vector2(1.0f, 1.0f));
					emitter->setSpeed(rise * Vector2(0.9f, 1.0f));
					emitter->setSpin(Vector2(-20.0f / 57.3f, 20.0f / 57.3f));
					emitter->setSpread(Vector2(30.0f / 57.3f, 30.0f / 57.3f));

					emitter->setEmissionRate(7.0f * enabled);
					emitter->setEmitterShape(0, Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f)));
					emitter->setLife(life * Vector2(1.0f, 1.0f));

					emitter->setModulateColor(c0);
				}

				tertiary.reset();
			}
			else if (RBX::Fire* fire = effect->fastDynamicCast<RBX::Fire>())
			{
				float size = fire->getClampedSize() / 3.5f;
				float boxsize = size / 8.0f;
				float heat = fire->getClampedHeat();
				Vector4 c0(Color4(fire->getColor(), 1.0f));
				Vector4 c1(Color4(fire->getSecondaryColor(), 1.0f));
				enabled = fire->getEnabled();

				Box aabb(-boxsize * one, boxsize * one);
				Box aabb2(-boxsize * 2.0f * one, boxsize * 2.0f * one);

				if (enabled && !emitter)
					emitter.reset(new Emitter(getVisualEngine()));

				if (emitter)
				{
					Emitter::Appearance a = {};
					a.mainTexture = "rbxasset://textures/particles/fire_main.dds";
					a.colorStripTexture = "rbxasset://textures/particles/fire_color.dds";
					a.alphaStripTexture = "rbxasset://textures/particles/fire_alpha.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Crazy;
					a.blendCode = Emitter::Blend_PremultipliedAlpha;

					emitter->setAppearance(a);
					emitter->setDampening(0.4f);
					emitter->setGlobalForce(Vector3(0.0f, 0.0f, 0.0f));
					emitter->setGrowth(0.8f * size * Vector2(-1.0f, -1.0f));
					emitter->setLocalForce(Vector3(0.0f, 0.5f * (1.0f * size * size / 4.0f + 0.7f * heat), 0.0f));
					emitter->setRotation(Vector2(-90.0f / 57.3f, 90.0f / 57.3f));
					emitter->setSizeX(size * Vector2(1.1f, 1.1f));
					emitter->setSizeY(size * Vector2(1.1f, 1.1f));

					emitter->setSpeed(0.4f * (0.2f * size * size + 0.2f * heat) * Vector2(1.0f, 1.0f));
					emitter->setSpin(Vector2(100.0f / 57.3f, 100.0f / 57.3f));
					emitter->setSpread(Vector2(10.0f / 57.3f, 10.0f / 57.3f));

					emitter->setEmissionRate(65.0f * enabled);
					emitter->setEmitterShape(0, aabb);
					emitter->setLife(Vector2(1.0f, 2.0f));

					//float heatFactor = G3D::clamp( heat/25, 0.0f, 1.0f );
					//emitter->setModulateColor( G3D::lerp(3.0f, 7.0f,heatFactor) * c0 );
					emitter->setModulateColor(4.0f * c0);
					emitter->setBrightenOnThrottle(0.0f);
					emitter->setBlendRatio(0.40f);
				}

				if (enabled && !secondary)
					secondary.reset(new Emitter(getVisualEngine()));

				if (secondary)
				{
					float size2 = 0.2f * size;
					float life2 = 2.0f;
					float growth2 = -size2 / life2;

					Emitter::Appearance a = {};
					a.mainTexture = "rbxasset://textures/particles/fire_sparks_main.dds";
					a.colorStripTexture = "rbxasset://textures/particles/fire_sparks_color.dds";
					a.alphaStripTexture = "rbxasset://textures/particles/common_alpha.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Add;
					a.blendCode = Emitter::Blend_Additive;

					secondary->setAppearance(a);
					secondary->setDampening(0.4f);
					secondary->setGlobalForce(Vector3(0.0f, 0.0f, 0.0f));
					secondary->setGrowth(growth2 * Vector2(1.0f, 1.0f));
					secondary->setLocalForce(Vector3(0.0f, 0.5f * (1.0f * size * size / 4.0f + 0.7f * heat), 0.0f));
					secondary->setRotation(Vector2(-90.0f / 57.3f, 90.0f / 57.3f));
					secondary->setSizeX(size2 * Vector2(1.0f, 1.0f));
					secondary->setSizeY(size2 * Vector2(1.0f, 1.0f));
					secondary->setSpeed(0.4f * (0.1f * size * size + 0.2f * heat) * Vector2(1.0f, 1.0f));
					secondary->setSpin(Vector2(100.0f / 57.3f, 100.0f / 57.3f));
					secondary->setSpread(Vector2(10.0f / 57.3f, 10.0f / 57.3f));

					secondary->setEmissionRate(65.0f * enabled);
					secondary->setEmitterShape(0, aabb2);
					secondary->setLife(Vector2(life2 / 2.0f, life2));
					secondaryOffset = CoordinateFrame(Vector3(0.0f, 3.0f * (0.3f * size + 0.05f * heat), 0.0f));

					secondary->setModulateColor(c1);
					secondary->setBrightenOnThrottle(1.0f);
				}

				tertiary.reset();
			}
			else if (effect->isA<RBX::ForceField>())
			{
				enabled = true;
				if (enabled && !emitter)
					emitter.reset(new Emitter(getVisualEngine()));

				if (emitter) // vortex
				{
					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/common_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/forcefield_vortex_color.dds";
					a.mainTexture = "rbxasset://textures/particles/forcefield_vortex_main.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Add;
					a.blendCode = Emitter::Blend_AlphaOne;

					emitter->setAppearance(a);
					emitter->setDampening(0.0f);
					emitter->setGlobalForce(Vector3(0.0f, -2.0f, 0.0f));
					emitter->setGrowth(1.6f * Vector2(1.0f, 1.0f));
					emitter->setLocalForce(Vector3(0.0f, 5.0f, 0.0f));
					emitter->setRotation(Vector2(400.0f / 57.3f, 400.0f / 57.3f));
					emitter->setSizeX(Vector2(2.0f, 2.0f));
					emitter->setSizeY(Vector2(0.2f, 0.2f));

					emitter->setSpeed(2.0f * Vector2(1.0f, 1.0f));
					emitter->setSpin(Vector2(200.0f / 57.3f, 200.0f / 57.3f));
					emitter->setSpread(Vector2(10.0f / 57.3f, 10.0f / 57.3f));

					emitter->setEmissionRate(40.0f * enabled);
					emitter->setEmitterShape(0, Box(-0.5f * one, 0.5f * one));
					emitter->setLife(Vector2(1.0f, 1.0f));
					emitter->setModulateColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
					emitterOffset = Vector3(0.0f, -1.9f, 0.0f);
					emitter->setInheritMotion(1.0f);
					emitter->setLockedToLocalSpace(true);
				}

				if (enabled && !secondary)
					secondary.reset(new Emitter(getVisualEngine()));

				if (secondary) // stars
				{
					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/common_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/forcefield_vortex_color.dds";
					a.mainTexture = "rbxasset://textures/sparkle.png";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Add;
					a.blendCode = Emitter::Blend_AlphaOne;

					secondary->setAppearance(a);
					secondary->setDampening(0.0f);
					secondary->setGlobalForce(Vector3(0.0f, 0.0f, 0.0f));
					secondary->setGrowth(0.14f * Vector2(1.0f, 1.0f));
					secondary->setLocalForce(Vector3(0.0f, 2.0f, 0.0f));
					secondary->setRotation(Vector2(0.0f, 0.0f));
					secondary->setSizeX(0.04f * Vector2(1.0f, 1.0f));
					secondary->setSizeY(0.04f * Vector2(1.0f, 1.0f));

					secondary->setSpeed(3.0f * Vector2(1.0f, 1.0f));
					secondary->setSpin(Vector2(-500.0f / 57.3f, 500.0f / 57.3f));
					secondary->setSpread(Vector2(30.0f / 57.3f, 30.0f / 57.3f));

					secondary->setEmissionRate(12.0f * enabled);
					secondary->setEmitterShape(0, Box(-0.8f * one, 0.8f * one));
					secondary->setLife(Vector2(1.3f, 1.3f));
					secondary->setModulateColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
					secondary->setLockedToLocalSpace(true);
					secondaryOffset = Vector3(0.0f, 0.0f, 0.0f);
				}

				if (enabled && !tertiary)
					tertiary.reset(new Emitter(getVisualEngine()));

				if (tertiary) // glow
				{
					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/forcefield_glow_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/forcefield_glow_color.dds";
					a.mainTexture = "rbxasset://textures/particles/forcefield_glow_main.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Crazy;
					a.blendCode = Emitter::Blend_PremultipliedAlpha;

					tertiary->setAppearance(a);
					tertiary->setDampening(0.0f);
					tertiary->setGlobalForce(Vector3(0.0f, 0.0f, 0.0f));
					tertiary->setGrowth(0.5f * Vector2(1.0f, 1.0f));
					tertiary->setLocalForce(Vector3(0.0f, 0.0f, 0.0f));
					tertiary->setRotation(Vector2(0.0f, 0.0f));
					tertiary->setSizeX(4.4f * Vector2(1.0f, 1.0f));
					tertiary->setSizeY(4.4f * Vector2(1.0f, 1.0f));

					tertiary->setSpeed(0.0f * Vector2(1.0f, 1.0f));
					tertiary->setSpin(Vector2(200.0f / 57.3f, 200.0f / 57.3f));
					tertiary->setSpread(Vector2(2.0f / 57.3f, 2.0f / 57.3f));

					tertiary->setEmissionRate(15.0f * enabled);
					tertiary->setEmitterShape(0, Box(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f)));
					tertiary->setLife(Vector2(2.0f, 2.0f));
					tertiary->setModulateColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
					tertiaryOffset = Vector3(0.0f, -0.5f, 0.0f);
					tertiary->setZOffset(2.5f);
					tertiary->setInheritMotion(1.0f);
					tertiary->setBrightenOnThrottle(1.0f);
					tertiary->setBlendRatio(0.15f);
					tertiary->setLockedToLocalSpace(true);

				}
			}
			else if (RBX::Explosion* expl = effect->fastDynamicCast<RBX::Explosion>())
			{
				enabled = true;

				if (enabled && !emitter)
					emitter.reset(new Emitter(getVisualEngine()));

				if (emitter)
				{
					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/explosion_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/explosion_color.dds";
					a.mainTexture = "rbxasset://textures/particles/sparkles_main.dds";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Modulate;
					a.blendCode = Emitter::Blend_AlphaOne;

					emitter->setAppearance(a);
					emitter->setEmissionRate(0.0f);
					emitter->setGlobalForce(Vector3(0.0f, -40.0f, 0.0f));
					emitter->setDampening(0.2f);
					emitter->setSpeed(Vector2(0.0f, 30.0f));
					emitter->setSpread(Vector2(75.0f / 57.3f, 75.0f / 57.3f));
					emitter->setSizeX(0.7f * Vector2(1.0f, 1.0f));
					emitter->setSizeY(0.7f * Vector2(1.0f, 1.0f));

					emitter->setLife(Vector2(1.8f, 2.0f));
					emitter->setEmissionRate(0.0f);
					emitter->setEmitterShape(0, Box(0.0f * Vector3(-0.5f, -0.5f, -0.5f), 0.0f * Vector3(0.5f, 0.5f, 0.5f)));

					emitter->transform() = CoordinateFrame(expl->getPosition());
					emitter->emit(30);
				}


				if (!secondary)
					secondary.reset(new Emitter(getVisualEngine()));

				if (secondary)
				{
					Emitter::Appearance a = {};
					a.alphaStripTexture = "rbxasset://textures/particles/explosion_alpha.dds";
					a.colorStripTexture = "rbxasset://textures/particles/explosion_color.dds";
					a.mainTexture = "rbxasset://textures/explosion.png";
					a.colorStripBaseline = -1;
					a.shader = Emitter::Shader_Modulate;
					a.blendCode = Emitter::Blend_AlphaBlend;

					float fireSize = 4;

					secondary->setAppearance(a);
					secondary->setEmitterShape(0, Box(0.0f * Vector3(-0.5f, -0.5f, -0.5f), 0.0f * Vector3(0.5f, 0.5f, 0.5f)));
					secondary->setEmissionRate(0.0f);
					secondary->setGlobalForce(Vector3(0.0f, 0.0f, 0.0f));
					secondary->setDampening(0.5f);
					secondary->setSpeed(Vector2(15.0f, 30.0f));
					secondary->setSpread(Vector2(65.0f / 57.3f, 65.0f / 57.3f));
					secondary->setSpin(90.0f / 57.3f * Vector2(-1.0f, 1.0f));
					secondary->setLife(Vector2(1.5f, 2.5f));
					secondary->setSizeX(fireSize * Vector2(1.0f, 1.0f));
					secondary->setSizeY(fireSize * Vector2(1.0f, 1.0f));
					secondary->setRotation(45.0f / 57.3f * Vector2(-1.0f, 1.0f));
					secondary->setGrowth(Vector2(1.0f, 1.0f));
					secondary->setEmissionRate(0.0f);
					secondary->setModulateColor(Vector4(0.8f, 0.5f, 0.5f, 0.5f));

					secondary->transform() = CoordinateFrame(expl->getPosition());

					secondary->emit(30);
				}

				tertiary.reset();
			}

			localBox = Extents();

			if (emitter)
				localBox.expandToContain(emitter->computeBBox());

			if (secondary)
				localBox.expandToContain(secondary->computeBBox());
		}

	}
}

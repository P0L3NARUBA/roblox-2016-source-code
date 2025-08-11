#include "stdafx.h"
#include "Material.h"

#include "GfxCore/Device.h"

namespace RBX {
	namespace Graphics {

		Technique::Technique(const shared_ptr<ShaderProgram>& program, uint32_t lodIndex, RenderQueue::Pass pass)
			: lodIndex(lodIndex)
			, pass(pass)
			, rasterizerState(RasterizerState::Cull_Back)
			, depthState(DepthState::Function_GreaterEqual, true)
			, blendState(BlendState::Mode_None)
			, program(program)
		{
			RBXASSERT(program);
		}

		void Technique::setRasterizerState(const RasterizerState& state) {
			rasterizerState = state;
		}

		void Technique::setDepthState(const DepthState& state) {
			depthState = state;
		}

		void Technique::setBlendState(const BlendState& state) {
			blendState = state;
		}

		void Technique::setTexture(uint32_t stage, const TextureRef& texture, const SamplerState& state) {
			if (stage >= textures.size()) {
				TextureUnit dummy = { TextureRef(), SamplerState::Filter_Point };

				textures.resize(stage + 1u, dummy);
			}

			TextureUnit tu = { texture, state };

			textures[stage] = tu;
		}

		void Technique::apply(DeviceContext* context) const {
			context->setRasterizerState(rasterizerState);
			context->setDepthState(depthState);
			context->setBlendState(blendState);

			context->bindProgram(program.get());

			for (size_t i = 0u; i < textures.size(); ++i) {
				const TextureUnit& tu = textures[i];
				const shared_ptr<Texture>& tex = tu.texture.getTexture();

				if (tex)
					context->bindTexture(i, tex.get(), tu.state);
			}
		}

		const TextureRef& Technique::getTexture(uint32_t stage) const {
			if (stage < textures.size())
				return textures[stage].texture;
			else {
				static const TextureRef dummy;
				return dummy;
			}
		}

		Material::Material()
		{
		}

		void Material::addTechnique(const Technique& technique) {
			RBXASSERT(techniques.empty() || (techniques.back().getPass() == technique.getPass() && techniques.back().getLodIndex() < technique.getLodIndex()) || techniques.back().getPass() < technique.getPass());

			techniques.push_back(technique);
		}

		const Technique* Material::getBestTechnique(uint32_t lodIndex, RenderQueue::Pass pass) const {
			const Technique* best = nullptr;

			for (size_t i = 0u; i < techniques.size(); ++i) {
				const Technique& t = techniques[i];

				if (t.getPass() == pass) {
					if (t.getLodIndex() >= lodIndex)
						return &t;

					best = &t;
				}
			}

			// All techniques are better than LOD we need, return last matching one
			return best;
		}

	}
}

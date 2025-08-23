#include "stdafx.h"
#include "ScreenSpaceEffect.h"

#include "GfxCore/Device.h"
#include "GfxCore/Framebuffer.h"
#include "GfxCore/States.h"
#include "VisualEngine.h"
#include "ShaderManager.h"
#include "SceneManager.h"
#include "rbx/Profiler.h"

namespace RBX {
	namespace Graphics {
		ShaderProgram* ScreenSpaceEffect::renderFullscreenBegin(DeviceContext* context, VisualEngine* visualEngine, const char* vsName, const char* fsName, const BlendState& blendState, uint32_t fbWidth, uint32_t fbHeight) {
			ShaderProgram* program = visualEngine->getShaderManager()->getProgram(vsName, fsName).get();

			if (program) {
				context->setRasterizerState(RasterizerState::Cull_Back);
				context->setDepthState(DepthState(DepthState::Function_Always, false));
				context->setBlendState(blendState);

				context->bindProgram(program);
			}

			return program;
		}

		void ScreenSpaceEffect::renderFullscreenEnd(DeviceContext* context, VisualEngine* visualEngine) {
			context->draw(visualEngine->getSceneManager()->getFullscreenTriangle());
		}
	}
}


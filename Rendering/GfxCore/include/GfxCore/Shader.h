#pragma once

#include "Resource.h"

#include <vector>
#include <string>

namespace RBX {
	namespace Graphics {

		class VertexShader : public Resource {
		public:
			~VertexShader();

			virtual void reloadBytecode(const std::vector<char>& bytecode) = 0;

		protected:
			VertexShader(Device* device);
		};

		class FragmentShader : public Resource {
		public:
			~FragmentShader();

			virtual void reloadBytecode(const std::vector<char>& bytecode) = 0;

		protected:
			FragmentShader(Device* device);
		};

		class ComputeShader : public Resource {
		public:
			~ComputeShader();

			virtual void reloadBytecode(const std::vector<char>& bytecode) = 0;

		protected:
			ComputeShader(Device* device);
		};

		class GeometryShader : public Resource {
		public:
			~GeometryShader();

			virtual void reloadBytecode(const std::vector<char>& bytecode) = 0;

		protected:
			GeometryShader(Device* device);
		};

		class ShaderProgram : public Resource {
		public:
			~ShaderProgram();

			//virtual int getConstantHandle(const char* name) const = 0;

			virtual uint32_t getMaxWorldTransforms() const = 0;
			virtual uint32_t getSamplerMask() const = 0;

			static void dumpToFLog(const std::string& text, int channel);

		protected:
			ShaderProgram(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<GeometryShader>& geometryShader, const shared_ptr<FragmentShader>& fragmentShader);
			ShaderProgram(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);
			ShaderProgram(Device* device, const shared_ptr<ComputeShader>& computeShader);

			shared_ptr<VertexShader> vertexShader;
			shared_ptr<FragmentShader> fragmentShader;
			shared_ptr<ComputeShader> computeShader;
			shared_ptr<GeometryShader> geometryShader;
		};

		struct ShaderGlobalConstant {
			const char* name;
			uint32_t offset;
			uint32_t size;

			ShaderGlobalConstant(const char* name, uint32_t offset, uint32_t size);
		};

	}
}

#include "GfxCore/Shader.h"

#include <boost/algorithm/string.hpp>

LOGGROUP(Graphics)

namespace RBX {
	namespace Graphics {

		VertexShader::VertexShader(Device* device)
			: Resource(device)
		{
		}

		VertexShader::~VertexShader()
		{
		}

		FragmentShader::FragmentShader(Device* device)
			: Resource(device)
		{
		}

		FragmentShader::~FragmentShader()
		{
		}

		ComputeShader::ComputeShader(Device* device)
			: Resource(device)
		{
		}

		ComputeShader::~ComputeShader()
		{
		}

		GeometryShader::GeometryShader(Device* device)
			: Resource(device)
		{
		}

		GeometryShader::~GeometryShader()
		{
		}

		ShaderProgram::ShaderProgram(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<GeometryShader>& geometryShader, const shared_ptr<FragmentShader>& fragmentShader)
			: Resource(device)
			, vertexShader(vertexShader)
			, geometryShader(geometryShader)
			, fragmentShader(fragmentShader)
		{
		}

		ShaderProgram::ShaderProgram(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
			: Resource(device)
			, vertexShader(vertexShader)
			, fragmentShader(fragmentShader)
		{
		}

		ShaderProgram::ShaderProgram(Device* device, const shared_ptr<ComputeShader>& computeShader)
			: Resource(device)
			, computeShader(computeShader)
		{
		}

		ShaderProgram::~ShaderProgram()
		{
		}

		void ShaderProgram::dumpToFLog(const std::string& text, int32_t channel)
		{
			std::vector<std::string> messages;
			boost::split(messages, text, boost::is_from_range('\n', '\n'));
			
			while (!messages.empty() && messages.back().empty())
				messages.pop_back();

			for (size_t i = 0u; i < messages.size(); ++i)
				FASTLOGS(channel, "%s", messages[i]);
		}

		ShaderGlobalConstant::ShaderGlobalConstant(const char* name, uint32_t offset, uint32_t size)
			: name(name)
			, offset(offset)
			, size(size)
		{
		}

	}
}
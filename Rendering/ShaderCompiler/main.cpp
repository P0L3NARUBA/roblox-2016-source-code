// Force asserts in this file
#ifdef NDEBUG
#undef NDEBUG
#endif

#include "md5/md5.h"

#ifdef _WIN32
#include <d3d11.h>
#include <D3Dcompiler.h>
#endif

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

#include <assert.h>

#include "rapidjson/document.h"

#define error(...) (fprintf(stderr, __VA_ARGS__), 1)

static std::string readFile(const std::string& path) {
	std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
	if (!in)
		throw std::runtime_error("Can't read file: " + path);

	std::string result;

	while (in) {
		char buf[4096];

		in.read(buf, sizeof(buf));
		result.insert(result.end(), buf, buf + in.gcount());
	}

	return result;
}

static std::vector<std::string> split(const std::string& str) {
	std::vector<std::string> result;

	std::istringstream iss(str);
	std::string item;
	while (iss >> item)
		result.push_back(item);

	return result;
}

class IShaderCompiler {
public:
	virtual ~IShaderCompiler() {}

	virtual std::string preprocess(const std::string& folder, const std::string& path, const std::string& defines) = 0;
	virtual std::vector<char> compile(const std::string& source, const std::string& target, const std::string& entry, bool optimize) = 0;
};

class ShaderCompilerD3D : public IShaderCompiler {
public:

	enum ShaderProfile {
		shaderProfile_DX_11,
		shaderProfile_DX_11_DURANGO,
		shaderProfile_Count
	};

	static ShaderProfile getD3DProfile(const std::string& target) {
		if (target == "d3d11")
			return shaderProfile_DX_11;
		if (target == "d3d11_durango")
			return shaderProfile_DX_11_DURANGO;

		return shaderProfile_Count;
	}


	static bool isX86Profile(ShaderProfile profile) {
		static const ShaderProfile kX86Profiles[] = { shaderProfile_DX_11 };

		for (int i = 0; i < ARRAYSIZE(kX86Profiles); ++i)
			if (profile == kX86Profiles[i])
				return true;

		return false;
	}

	static bool isX64Profile(ShaderProfile profile) {
		if (profile == shaderProfile_DX_11_DURANGO)
			return true;

		return false;
	}

	ShaderCompilerD3D(ShaderProfile shaderProfile)
		: shaderProfile(shaderProfile)
	{
		HMODULE d3dCompiler = nullptr;

		if (shaderProfile == shaderProfile_DX_11_DURANGO)
			d3dCompiler = loadShaderCompilerDurangoDLL();
		else
			d3dCompiler = loadShaderCompilerDLL();

		compileShader = (TypeD3DCompile)GetProcAddress(d3dCompiler, "D3DCompile");
		assert(compileShader);

		preprocessShader = (TypeD3DPreprocess)GetProcAddress(d3dCompiler, "D3DPreprocess");
		assert(preprocessShader);
	}


	virtual std::string preprocess(const std::string& folder, const std::string& path, const std::string& defines) {
		// create d3dx defines
		std::vector<std::string> defineStrings = split(defines);
		std::vector<D3D_SHADER_MACRO> macros;

		for (size_t i = 0u; i < defineStrings.size(); ++i) {
			std::string& def = defineStrings[i];
			std::string::size_type pos = def.find('=');

			if (pos == std::string::npos) {
				D3D_SHADER_MACRO macro = { def.c_str(), "1" };
				macros.push_back(macro);
			}
			else {
				// split string into name and value
				def[pos] = 0;

				D3D_SHADER_MACRO macro = { def.c_str(), def.c_str() + pos + 1 };
				macros.push_back(macro);
			}
		}

		D3D_SHADER_MACRO macroEnd = {};
		macros.push_back(macroEnd);

		// let preprocessor know about the original filename
		std::string source = "#include \"" + path + "\"\n";

		// preprocess shader
		IncludeCallback callback(folder);

		ID3DBlob* text = nullptr;
		ID3DBlob* messages = nullptr;
		HRESULT hr = preprocessShader(source.c_str(), source.size(), path.c_str(), &macros[0], &callback, &text, &messages);

		std::vector<char> resultBuffer = consumeData<char>(hr, text, messages);
		std::string result = &resultBuffer[0];

		// preprocessor output includes a #line 1 "<full-path>\memory"; remove it!
		if (result.size() > 10 && result.compare(0, 9, "#line 1 \"") == 0 && result[10] == ':') {
			std::string::size_type pos = result.find_first_of('\n');
			assert(pos != std::string::npos);

			result.erase(result.begin(), result.begin() + pos);
		}

		return result;
	}

	void translateShaderProfile(const std::string& originalTarget, std::string& targetOut) {
		switch (shaderProfile) {
		case ShaderCompilerD3D::shaderProfile_DX_11_DURANGO:
		case ShaderCompilerD3D::shaderProfile_DX_11: {
			targetOut = originalTarget;
			return;
		}
		default:
			assert(false); // new enum?
			break;
		}
	}

	virtual std::vector<char> compile(const std::string& source, const std::string& target, const std::string& entry, bool optimize) {
		unsigned int flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

		if (!optimize)
			flags |= D3DCOMPILE_SKIP_OPTIMIZATION;

		std::string realTarget;
		translateShaderProfile(target, realTarget);

		ID3DBlob* bytecode = nullptr;
		ID3DBlob* messages = nullptr;
		//HRESULT hr = compileShader(source.c_str(), source.length(), NULL, NULL, entry.c_str(), target.c_str(), flags, &bytecode, &messages, NULL);
		HRESULT hr = compileShader(source.c_str(), source.length(), entry.c_str(), nullptr, nullptr, entry.c_str(), realTarget.c_str(), flags, 0, &bytecode, &messages);

		return consumeData<char>(hr, bytecode, messages);
	}

private:
	struct IncludeCallback : ID3DInclude {
		std::string folder;
		std::map<const void*, std::string> paths;

		IncludeCallback(const std::string& folder)
			: folder(folder)
		{
		}

		virtual HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) {
			std::string path;

			if (pParentData) {
				assert(paths.count(pParentData));

				path = paths[pParentData];

				std::string::size_type slash = path.find_last_of("\\/");
				path.erase(path.begin() + (slash == std::string::npos ? 0 : slash + 1), path.end());
			}

			path += pFileName;

			try {
				std::string source = readFile(folder + "/" + path);

				char* result = new char[source.length()];
				memcpy(result, source.c_str(), source.length());

				paths[result] = path;

				*ppData = result;
				*pBytes = source.length();

				return S_OK;
			}
			catch (...) {
				return ERROR_FILE_NOT_FOUND; //  D3DERR_FILENOTFOUND;
			}
		}

		virtual HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) {
			assert(paths.count(pData));
			paths.erase(pData);
			delete[] static_cast<const char*>(pData);

			return S_OK;
		}
	};

	static HMODULE loadShaderCompilerDLL() {
		HMODULE compiler = LoadLibraryA("d3dcompiler_47.dll");

		if (!compiler) {
			fprintf(stderr, "Error: can't load D3DCompile DLL\n");

			throw std::runtime_error("Can't load D3DX DLL");
		}

		return compiler;
	}

	static HMODULE loadShaderCompilerDurangoDLL() {
		HMODULE compiler = LoadLibraryA("D3DCompiler_47_xdk.dll");
		DWORD lastError = GetLastError();

		if (!compiler) {
			fprintf(stderr, "Error: can't load D3DCompile for xbox DLL\n");

			throw std::runtime_error("Can't load D3DX DLL");
		}

		return compiler;
	}

	template <typename T> static std::vector<T> consumeData(HRESULT hr, ID3DBlob* buffer, ID3DBlob* messages) {
		if (SUCCEEDED(hr)) {
			if (messages) {
				std::string log(static_cast<char*>(messages->GetBufferPointer()), messages->GetBufferSize());

				messages->Release();

				fprintf(stderr, "Shader compilation resulted in warnings: %s", log.c_str());
			}

			assert(buffer->GetBufferSize() % sizeof(T) == 0);
			std::vector<T> result(static_cast<T*>(buffer->GetBufferPointer()), static_cast<T*>(buffer->GetBufferPointer()) + buffer->GetBufferSize() / sizeof(T));

			buffer->Release();

			return result;
		}
		else if (messages) {
			std::string log(static_cast<char*>(messages->GetBufferPointer()), messages->GetBufferSize());

			messages->Release();

			throw std::runtime_error(log.c_str());
		}
		else {
			char buf[128];
			sprintf_s(buf, "Unknown error %x", hr);

			throw std::runtime_error(buf);
		}
	}

	ShaderProfile shaderProfile;

	typedef HRESULT(WINAPI* TypeD3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
	typedef HRESULT(WINAPI* TypeD3DPreprocess)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, ID3DBlob**, ID3DBlob**);

	TypeD3DCompile compileShader;
	TypeD3DPreprocess preprocessShader;
};

struct PackEntryMemory {
	std::vector<char> md5;
	std::vector<char> data;
};

struct PackEntryFile {
	char name[64];
	char md5[16];
	unsigned int offset;
	unsigned int size;
	char reserved[8];
};

static void readData(std::istream& in, void* data, int size) {
	in.read(static_cast<char*>(data), size);

	if (in.gcount() != size) {
		char buf[128];
		sprintf_s(buf, "Unexpected end of file while reading %d bytes", size);

		throw std::runtime_error(buf);
	}
}

static std::map<std::string, PackEntryMemory> readPack(const std::string& path) {
	std::map<std::string, PackEntryMemory> result;

	std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
	if (!in) return result;

	char header[4];
	readData(in, header, 4);

	if (memcmp(header, "RBXS", 4) != 0) {
		fprintf(stderr, "Warning: shader pack %s is corrupted\n", path.c_str());
		return result;
	}

	size_t count = 0u;
	readData(in, &count, 4);

	if (!count) return result;

	std::vector<PackEntryFile> entries(count);
	readData(in, &entries[0], count * sizeof(PackEntryFile));

	for (size_t i = 0u; i < count; ++i) {
		const PackEntryFile& e = entries[i];

		PackEntryMemory em;
		em.md5 = std::vector<char>(e.md5, e.md5 + sizeof(e.md5));
		em.data.resize(e.size);

		in.seekg(e.offset);
		readData(in, &em.data[0], e.size);

		result[e.name] = em;
	}

	return result;
}

static bool writePack(const std::string& path, const std::map<std::string, PackEntryMemory>& data) {
	std::vector<PackEntryFile> entries;
	std::vector<char> entryData;

	size_t baseDataOffset = 8u + data.size() * sizeof(PackEntryFile);

	for (std::map<std::string, PackEntryMemory>::const_iterator it = data.begin(); it != data.end(); ++it) {
		PackEntryFile e{};

		assert(it->first.length() < sizeof(e.name));
		strncpy_s(e.name, it->first.c_str(), sizeof(e.name));

		assert(it->second.md5.size() == sizeof(e.md5));
		memcpy(e.md5, &it->second.md5[0], sizeof(e.md5));

		e.offset = baseDataOffset + entryData.size();
		e.size = it->second.data.size();

		memset(e.reserved, 0, sizeof(e.reserved));

		entries.push_back(e);

		entryData.insert(entryData.end(), it->second.data.begin(), it->second.data.end());
	}

	std::ofstream out(path.c_str(), std::ios::out | std::ios::binary);

	if (!out) {
		fprintf(stderr, "Error: can't open shader pack %s for writing\n", path.c_str());
		return false;
	}

	size_t count = entries.size();

	out.write("RBXS", 4);
	out.write(reinterpret_cast<char*>(&count), sizeof(count));
	out.write(reinterpret_cast<char*>(&entries[0]), sizeof(PackEntryFile) * static_cast<std::streamsize>(count));
	out.write(reinterpret_cast<char*>(&entryData[0]), entryData.size());

	return true;
}

static std::string getStringOrEmpty(const rapidjson::Value& value) {
	return value.IsString() ? value.GetString() : "";
}

static bool isExcludedFromPack(const std::string& exclude, const std::string& packName) {
	if (!exclude.empty()) {
		std::istringstream iss(exclude);
		std::string item;

		while (iss >> item)
			if (item == packName)
				return true;
	}

	return false;
}

static bool compilePack(const std::string& folder, const std::string& packTarget, const std::string& builtinDefines, IShaderCompiler* compiler) {
	using namespace rapidjson;

	size_t succeeded = 0u;
	size_t failed = 0u;

	std::string shaderPackPath = folder + "/shaders_" + packTarget + ".pack";
	std::string sourceFolder = folder + "/source";

	std::string shaderDb = readFile(folder + "/shaders.json");

	std::map<std::string, PackEntryMemory> shaderPackOld = readPack(shaderPackPath);
	std::map<std::string, PackEntryMemory> shaderPackNew;

	Document root;
	root.Parse<kParseDefaultFlags>(shaderDb.c_str());

	if (root.HasParseError())
		return error("Failed to load shader.json: %s\n", root.GetParseError());

	assert(root.IsArray());

	for (Value::ValueIterator it = root.Begin(); it != root.End(); ++it) {
		const Value& name = (*it)["name"];
		const Value& source = (*it)["source"];
		const Value& target = (*it)["target"];
		const Value& entrypoint = (*it)["entrypoint"];
		const Value& defines = (*it)["defines"];
		const Value& exclude = (*it)["exclude"];

		if (!name.IsString() || !source.IsString() || !target.IsString() || !entrypoint.IsString()) {
			fprintf(stderr, "%s: Error parsing shader info: %s\n", packTarget.c_str(), getStringOrEmpty(name).c_str());

			failed++;

			continue;
		}

		if (isExcludedFromPack(getStringOrEmpty(exclude), packTarget))
			continue;

		try {
			std::string shaderSource = compiler->preprocess(sourceFolder, source.GetString(), getStringOrEmpty(defines) + " " + builtinDefines);

			char md5[16]{};

			MD5_CTX ctx;
			MD5_Init(&ctx);
			MD5_Update(&ctx, const_cast<char*>(shaderSource.c_str()), shaderSource.length());
			MD5_Update(&ctx, const_cast<char*>(target.GetString()), target.GetStringLength());
			MD5_Update(&ctx, const_cast<char*>(entrypoint.GetString()), entrypoint.GetStringLength());
			MD5_Final(reinterpret_cast<unsigned char*>(md5), &ctx);

			if (shaderPackOld.count(name.GetString())) {
				const PackEntryMemory& eOld = shaderPackOld[name.GetString()];

				if (memcmp(md5, &eOld.md5[0], sizeof(md5)) == 0) {
					shaderPackNew[name.GetString()] = eOld;

					continue;
				}
			}

			std::vector<char> shaderBytecode = compiler->compile(shaderSource, target.GetString(), entrypoint.GetString(), true);

			PackEntryMemory eNew;
			eNew.md5 = std::vector<char>(md5, md5 + sizeof(md5));
			eNew.data = shaderBytecode;

			shaderPackNew[name.GetString()] = eNew;

			succeeded++;
		}
		catch (const std::exception& e) {
			fprintf(stderr, "%s: Error compiling %s\n%s", packTarget.c_str(), name.GetString(), e.what());

			failed++;
		}
	}

	if (failed) {
		fprintf(stderr, "%s: failed to compile %d shaders\n", packTarget.c_str(), failed);

		return false;
	}

	if (!writePack(shaderPackPath, shaderPackNew))
		return false;

	if (succeeded)
		fprintf(stderr, "%s: updated %d shaders\n", packTarget.c_str(), succeeded);

	return true;
}

static int compilePacks(int argc, char** argv) {
	if (argc < 3)
		return error("Error: no folder specified");

	std::string folder = argv[2];

	for (int i = 3; i < argc; ++i) {
		std::string target = argv[i];
		ShaderCompilerD3D::ShaderProfile profileD3D = ShaderCompilerD3D::getD3DProfile(target);

		if (profileD3D != ShaderCompilerD3D::shaderProfile_Count) {
			ShaderCompilerD3D compiler(profileD3D);

			switch (profileD3D) {
			case ShaderCompilerD3D::shaderProfile_DX_11:
				if (!compilePack(folder, target, "DX11", &compiler))
					return 1;

				break;
			case ShaderCompilerD3D::shaderProfile_DX_11_DURANGO:
				if (!compilePack(folder, target, "DX11 DURANGO", &compiler))
					return 1;

				break;
			default:
				break;
			}
		}
		else
			return error("Unrecognized pack target: %s\n", target.c_str());
	}

	return 0;
}

int main(int argc, char** argv) {
	return compilePacks(argc, argv);
}


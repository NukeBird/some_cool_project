
#include <map>

#include <glbinding/gl/enum.h>
#include <globjects/globjects.h>
#include <spdlog/spdlog.h>

using namespace std::literals;

struct Program
{
	std::unique_ptr<globjects::Program> program;
	struct Shader
	{
		std::unique_ptr<globjects::AbstractStringSource> shader_source;
		std::unique_ptr<globjects::Shader> shader;
	};

	globjects::Program* operator->() const
	{
		return program.get();
	}

	std::vector<Shader> shaders;
};

//using ProgramRef = std::shared_ptr<Program>;

class ShaderImporter
{
public:
	static Program load(std::initializer_list<std::string> files)
	{
		static std::map<std::string_view, gl::GLenum> s_shaderTypes = {
			{".vs.glsl"sv, gl::GLenum::GL_VERTEX_SHADER },
			{".fs.glsl"sv, gl::GLenum::GL_FRAGMENT_SHADER },
			{".gs.glsl"sv, gl::GLenum::GL_GEOMETRY_SHADER },
			{".tes.glsl"sv, gl::GLenum::GL_TESS_EVALUATION_SHADER },
			{".tcs.glsl"sv, gl::GLenum::GL_TESS_CONTROL_SHADER },
			{".cs.glsl"sv, gl::GLenum::GL_COMPUTE_SHADER }
		};

		auto getExtension = [](std::string_view filename)
		{
			auto filenameFrom = filename.find_last_of("\\/"sv);
			if (filenameFrom == std::string_view::npos)
				filenameFrom = 0;

			return filename.substr(filename.find('.', filenameFrom));
		};

		Program result;
		result.program = globjects::Program::create();

		for (const auto& file : files)
		{
			try
			{
				const auto shaderType = s_shaderTypes.at(getExtension(file));

				auto source = globjects::Shader::sourceFromFile(file);
				auto shader = globjects::Shader::create(shaderType, source.get());

				result.program->attach(shader.get());

				result.shaders.push_back({ std::move(source), std::move(shader) });
			}
			catch (const std::out_of_range& exception)
			{
				spdlog::warn("ShaderImporter don't recognize extension of file {0}", file);
				return {};
			}
			
		}

		return result;
	}
};



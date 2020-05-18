
#include <optional>

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

	globjects::Program* operator->() { return program.get(); }
	const globjects::Program* operator->() const { return program.get(); }

	globjects::Program* operator* () { return program.get(); }
	const globjects::Program* operator*() const { return program.get(); }

	std::vector<Shader> shaders;
};

//using ProgramRef = std::shared_ptr<Program>;

class ShaderImporter
{
public:
	static Program load(std::initializer_list<std::string> files)
	{
		if (files.size() == 0)
		{
			spdlog::warn("ShaderImporter's file list should not be empty");
			return {};
		}

		Program result;
		result.program = globjects::Program::create();

		for (const auto& file : files)
		{
			const auto shaderType = determine_shader_type(file);
			if (!shaderType)
			{
				spdlog::warn("ShaderImporter don't recognize extension of file {0}", file);
				return {};
			}

			auto source = globjects::Shader::sourceFromFile(file);
			auto shader = globjects::Shader::create(*shaderType, source.get());

			result.program->attach(shader.get());

			if (result.shaders.empty()) //it's first file
				result.program->setName(file);

			result.shaders.push_back({ std::move(source), std::move(shader) });
		}

		spdlog::info("Shader program '{0}' created", result->name());
		return result;
	}

private:
	static std::optional<gl::GLenum> determine_shader_type(std::string_view filename)
	{
		using shader_map_pair = std::pair<std::string_view, gl::GLenum>;
		static std::vector<shader_map_pair> s_shaderTypes = {
			{".vs.glsl"sv, gl::GLenum::GL_VERTEX_SHADER },
			{".fs.glsl"sv, gl::GLenum::GL_FRAGMENT_SHADER },
			{".gs.glsl"sv, gl::GLenum::GL_GEOMETRY_SHADER },
			{".tes.glsl"sv, gl::GLenum::GL_TESS_EVALUATION_SHADER },
			{".tcs.glsl"sv, gl::GLenum::GL_TESS_CONTROL_SHADER },
			{".cs.glsl"sv, gl::GLenum::GL_COMPUTE_SHADER }
		};

		const auto it = std::find_if(std::begin(s_shaderTypes), std::end(s_shaderTypes), [filename](shader_map_pair pair)
		{
			if (filename.length() < pair.first.length())
				return false;

			return pair.first.compare(filename.substr(filename.length() - pair.first.length())) == 0;
		});

		return it != std::end(s_shaderTypes) ?  it->second : std::optional<gl::GLenum>{};
	}
};



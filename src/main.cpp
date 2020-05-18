#define GLFW_INCLUDE_NONE
#include <iostream>

#include "drawable.h"

#include <globjects/globjects.h>
#include <globjects/base/File.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <glbinding/gl/enum.h>
#include <globjects/VertexAttributeBinding.h>
#include <SOIL2/SOIL2.h>
#include <spdlog/spdlog.h>

#include "model_importer.h"
#include "texture_importer.h"

#include "glfw_window.h"
#include "camera.h"

using namespace std::literals;

std::shared_ptr<IDrawable> makeFullscreenQuad()
{
	static const std::array<glm::vec2, 4> raw{ {glm::vec2(+1.f,-1.f), glm::vec2(+1.f,+1.f), glm::vec2(-1.f,-1.f), glm::vec2(-1.f,+1.f) } };

	struct Quad : public IDrawable
	{
	    void draw(const MatrixStack& sceneTransforms) override
	    {
			program->use();
			program->setUniform("source", 0);

			program->setUniform("u_matProj", sceneTransforms.getProjection());
			program->setUniform("u_matModelView", sceneTransforms.getModelView());
			program->setUniform("u_matInverseProj", sceneTransforms.getProjectionInverse());
			program->setUniform("u_matInverseModelView", sceneTransforms.getModelViewInverse());

			vao->drawArrays(gl::GL_TRIANGLE_STRIP, 0, 4);
			program->release();
	    }

		VertexArrayRef vao;
		BufferRef vbo;

		std::unique_ptr<globjects::Program> program;
		std::unique_ptr<globjects::AbstractStringSource> vertex_shader_src, fragment_shader_src;
		std::unique_ptr<globjects::Shader> vertex_shader, fragment_shader;
	};

	auto mesh = std::make_shared<Quad>();

	//program
	mesh->vertex_shader_src = globjects::Shader::sourceFromFile("data/shaders/sky.vs.glsl");
	mesh->vertex_shader = globjects::Shader::create(gl::GLenum::GL_VERTEX_SHADER, mesh->vertex_shader_src.get());
	mesh->fragment_shader_src = globjects::Shader::sourceFromFile("data/shaders/sky.fs.glsl");
	mesh->fragment_shader = globjects::Shader::create(gl::GLenum::GL_FRAGMENT_SHADER, mesh->fragment_shader_src.get());
	mesh->program = globjects::Program::create();
	mesh->program->attach(mesh->vertex_shader.get(), mesh->fragment_shader.get());
	
	//auto block = mesh->program->uniformBlock("CameraBlock");

	//vao
	mesh->vao = globjects::VertexArray::create();

	//vbo
	mesh->vbo = globjects::Buffer::create();
    mesh->vbo->setData(raw, gl::GLenum::GL_STATIC_DRAW); //needed for some drivers

	auto binding = mesh->vao->binding(0);
	binding->setAttribute(0);
	binding->setBuffer(mesh->vbo.get(), 0, sizeof(glm::vec2));
	binding->setFormat(2, gl::GL_FLOAT, gl::GL_FALSE, 0);
	mesh->vao->enable(0);

	return mesh;
}

class App
{
public:
	void Run()
	{
		window.
			setLabel("Oh hi Mark"s).
			setSize(800, 600).
			setNumMsaaSamples(8);

		//setup callbacks
		window.InitializeCallback = [this] { OnInitialize(); };
		window.ResizeCallback = [this](const glm::ivec2& newSize) { OnResize(newSize); };
		window.RenderCallback = [this](double time, double dt) { OnRender(time, dt); };

		window.MouseMoveCallback = [&](const MousePos& pos)
		{
			const auto relativePos = pos.getPos() / static_cast<glm::vec2>(window.getSize());

			camera.setRotation(
				 
				glm::angleAxis(glm::mix(-glm::pi<float>(), glm::pi<float>(), relativePos.x), glm::vec3{ 0.0, -1.0, 0.0 }) *
				glm::angleAxis(glm::mix(-glm::pi<float>() / 2, glm::pi<float>() / 2, relativePos.y), glm::vec3{ 1.0, 0.0, 0.0 })
			);
		};

		window.MouseDownCallback = [this](int key)
		{
		};


		window.Run();
	}

private:
    void OnInitialize()
    {
		//camera.setProjection(glm::perspective())

		model = ModelImporter::load("data/matball.glb");

		skybox_texture = TextureImporter::load("data/skybox.dds");
		skybox_texture->generateMipmap();
		camera_buffer = globjects::Buffer::create();

		fullscreenQuad = makeFullscreenQuad();

		camera.setView(glm::lookAt(glm::vec3{ 100.0, 10.0, 0.0 }, { 0.0, 0.0, 0.0 }, {0.0, 1.0, 0.0}));
		auto f = camera.getForward();
		auto l = camera.getLeft();
		auto u = camera.getUp();
    }


	void OnResize(const glm::ivec2& newSize)
    {
		if (newSize == glm::ivec2{ 0, 0 })
			return;

		camera.setProjection(glm::perspectiveFov(glm::radians(90.0), static_cast<double>(newSize.x), static_cast<double>(newSize.y), 1.0, 1000.0));
    }

	void OnRender(double time, double deltaTime)
    {
		transforms.reset(camera.getView(), camera.getProjection());

        gl::glViewport(0, 0, window.getSize().x, window.getSize().y);

		gl::glActiveTexture(gl::GLenum::GL_TEXTURE0 + 0);
		skybox_texture->bind();
		fullscreenQuad->draw(transforms);

		assert(transforms.empty());
    }

private:
	GlfwWindow window;

	NodeRef model;
	TextureRef skybox_texture;

	BufferRef camera_buffer;

	std::shared_ptr<IDrawable> fullscreenQuad;

	Camera camera;
	MatrixStack transforms;
};


int main()
{
	try
	{
		App app;
		app.Run();
	}
	catch (const std::exception& e)
	{
		spdlog::error("Exception! {0}", e.what());
	}

	return EXIT_SUCCESS;
}
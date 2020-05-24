#include <iostream>
#include <unordered_set>


#include "drawable.h"

#include <globjects/globjects.h>
#include <globjects/base/File.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <globjects/VertexAttributeBinding.h>
#include <SOIL2/SOIL2.h>
#include <spdlog/spdlog.h>

#include "model_importer.h"
#include "texture_importer.h"
#include "shader_importer.h"

#include "glfw_application.h"
#include "camera.h"

using namespace std::literals;

static MeshRef makeFullscreenQuad()
{
	static const std::array<glm::vec2, 4> raw{ {glm::vec2(+1.f,-1.f), glm::vec2(+1.f,+1.f), glm::vec2(-1.f,-1.f), glm::vec2(-1.f,+1.f) } };

	auto mesh = std::make_shared<Mesh>();

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

struct RenderingContext
{
	Camera* active_camera = nullptr;
	globjects::Program* active_program = nullptr;

	MatrixStack transforms;

	void applyProgram(globjects::Program* program)
	{
		if (active_program != program)
		{
			program->use();
			active_program = program;
		}
	}

	void applyCameraUniforms()
	{
		assert(active_program);
		active_program->setUniform("u_matProj", transforms.getProjection());
		active_program->setUniform("u_matModelView", transforms.getModelView());
		active_program->setUniform("u_matInverseProj", transforms.getProjectionInverse());
		active_program->setUniform("u_matInverseModelView", transforms.getModelViewInverse());
		active_program->setUniform("u_matNormal", glm::transpose(glm::inverse(transforms.getModelView())));
	}

	//TODO: reuse textures that is already bound
	void applyTextures(std::initializer_list<std::pair<std::string, TextureRef>> textures)
	{
		assert(active_program);

		//TODO: request once
        gl::GLint maxTexUnits = 0;
		gl::glGetIntegerv(gl::GLenum::GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);

		if (textures.size() > maxTexUnits)
		{
			spdlog::warn("Trying to apply {0} textures, when hardware limit is just {1}", textures.size(), maxTexUnits);
		    return;
		}
		
		int activeTextureSlotNumber = 0;
	    for (auto pair : textures)
	    {
			gl::glActiveTexture(gl::GLenum::GL_TEXTURE0 + activeTextureSlotNumber);
			active_program->setUniform(pair.first, activeTextureSlotNumber);
			pair.second->bind();

			activeTextureSlotNumber++;
	    }
	}

	void applyMaterial(const MaterialRef& material)
	{
		applyTextures(
		{
			{"u_texAlbedo", material->albedo},
			{"u_texNormalMap", material->normalMap},
			{"u_texAoRoughnessMetallic", material->aoRoughnessMetallic}
		});
	}
};

class Scene
{
public:
	void addNode(NodeRef node, glm::mat4 modelMatrix = glm::mat4{1.0f})
	{
		node->transform *= modelMatrix;
		root.insert(std::move(node));
	}

	void render(RenderingContext& rc)
	{
		for (const auto& node : root)
			renderNode(node, rc);
	}

	void renderNode(const NodeRef& node, RenderingContext& rc)
	{
		rc.transforms.pushModelView(node->transform);

		for (const auto& mesh : node->meshes)
		{
			rc.applyCameraUniforms();
			rc.applyMaterial(mesh->material);

			mesh->vao->bind();
			mesh->vao->drawElements(gl::GLenum::GL_TRIANGLES, mesh->index_count, gl::GLenum::GL_UNSIGNED_INT);
		}

		for (const auto& child : node->childs)
			renderNode(child, rc);

		rc.transforms.popModelView();
	}

private:
	std::unordered_set<NodeRef> root;
};

class App
{
public:
	void Run()
	{
		GlfwApplication::get().OnNoActiveWindows = [] { GlfwApplication::get().stop(); };


		window = GlfwApplication::get().createWindow();

		window->
	        setLabel("Oh hi Mark"s).
			setSize(1024, 768);

		//setup callbacks
		window->InitializeCallback = [this] { OnInitialize(); };
		window->ResizeCallback = [this](const glm::ivec2& newSize) { OnResize(newSize); };
		window->RenderCallback = [this](double dt) { OnRender(dt); };

		window->MouseMoveCallback = [&](const MousePos& pos)
		{
			const auto relativePos = pos.getPos() / static_cast<glm::vec2>(window->getSize());

			camera.setRotation(
				glm::angleAxis(glm::mix(-glm::pi<float>(), glm::pi<float>(), relativePos.x), glm::vec3{ 0.0, -1.0, 0.0 }) *
				glm::angleAxis(glm::mix(-glm::pi<float>() / 2, glm::pi<float>() / 2, relativePos.y), glm::vec3{ 1.0, 0.0, 0.0 })
			);
		};

		window->UpdateCallback = [&](double dt)
		{
			const float moveSpeed = dt * 50.0f;
			if (window->isKeyDown(GLFW_KEY_W))
				camera.setPosition(camera.getPosition() + camera.getForward() * moveSpeed);
			if (window->isKeyDown(GLFW_KEY_S))
				camera.setPosition(camera.getPosition() - camera.getForward() * moveSpeed);
			if (window->isKeyDown(GLFW_KEY_A))
				camera.setPosition(camera.getPosition() + camera.getLeft() * moveSpeed);
			if (window->isKeyDown(GLFW_KEY_D))
				camera.setPosition(camera.getPosition() - camera.getLeft() * moveSpeed);

			const float expositionSpeed = dt;
			if (window->isKeyDown(GLFW_KEY_EQUAL))
				exposition += expositionSpeed;
			if (window->isKeyDown(GLFW_KEY_MINUS))
				exposition -= expositionSpeed;

			if (window->isKeyDown(GLFW_KEY_ESCAPE))
				window->setCloseFlag(true);

			exposition = glm::clamp(exposition, 0.01f, 10.0f);
		};

		window->MouseDownCallback = [this](int key)
		{
			
		};

		GlfwApplication::get().run();
	}

private:
    void OnInitialize()
    {
		//camera.setProjection(glm::perspective())

		scene.addNode(ModelImporter::load("data/matball.glb"), glm::scale(glm::mat4{1.0f}, {10, 10, 10}));

		program_skybox = ShaderImporter::load({ "data/shaders/sky.vs.glsl"s, "data/shaders/sky.fs.glsl"s });
		program_mesh = ShaderImporter::load({ "data/shaders/mesh.vs.glsl", "data/shaders/mesh.fs.glsl" });
		program_postprocess = ShaderImporter::load({"data/shaders/postprocess.vs.glsl", "data/shaders/postprocess.fs.glsl"});

		skybox_texture = TextureImporter::load("data/skybox.dds");
		skybox_texture->generateMipmap();

		//camera_buffer = globjects::Buffer::create();

		fullscreenQuad = makeFullscreenQuad();

		framebuffer_hdr = globjects::Framebuffer::create();
		framebuffer_hdr->clearBuffer(gl::GLenum::GL_COLOR, 0, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));

		camera.setView(glm::lookAt(glm::vec3{ 100.0, 10.0, 0.0 }, { 0.0, 0.0, 0.0 }, {0.0, 1.0, 0.0}));

		gl::glEnable(gl::GLenum::GL_MULTISAMPLE);
		gl::glEnable(gl::GLenum::GL_DEPTH_TEST);
		gl::glEnable(gl::GLenum::GL_CULL_FACE);
    }


	void OnResize(const glm::ivec2& newSize)
    {
		if (newSize == glm::ivec2{ 0, 0 })
			return;

		camera.setProjection(glm::perspectiveFov(glm::radians(70.0), static_cast<double>(newSize.x), static_cast<double>(newSize.y), 1.0, 1000.0));

		dirty_framebuffers = true;
    }

	void OnRender(double deltaTime)
	{
		if (dirty_framebuffers)
		{
			//recreate framebuffer_hdr
			framebuffer_hdr_color1 = globjects::Texture::createDefault(gl::GL_TEXTURE_2D_MULTISAMPLE);
			//framebuffer_hdr_color1->storage2D(1, gl::GL_RGBA16F, newSize);
			framebuffer_hdr_color1->storage2DMultisample(8, gl::GL_RGBA16F, window->getSize(), true);
			framebuffer_hdr_depth = globjects::Texture::createDefault(gl::GL_TEXTURE_2D_MULTISAMPLE);
			//framebuffer_hdr_depth->storage2D(1, gl::GL_DEPTH_COMPONENT32F, newSize);
			framebuffer_hdr_depth->storage2DMultisample(8, gl::GL_DEPTH_COMPONENT32F, window->getSize(), true);

			framebuffer_hdr->attachTexture(gl::GLenum::GL_COLOR_ATTACHMENT0, framebuffer_hdr_color1.get());
			framebuffer_hdr->attachTexture(gl::GLenum::GL_DEPTH_ATTACHMENT, framebuffer_hdr_depth.get());
			framebuffer_hdr->setDrawBuffers({ gl::GLenum::GL_COLOR_ATTACHMENT0, gl::GL_DEPTH_ATTACHMENT });
			framebuffer_hdr->printStatus();

			dirty_framebuffers = false;
		}

		rc.active_camera = &camera;
		rc.transforms.reset(camera.getView(), camera.getProjection());

		gl::glDepthMask(true);

		gl::glViewport(0, 0, window->getSize().x, window->getSize().y);
		//gl::glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);

		framebuffer_hdr->bind();
		framebuffer_hdr->clear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		//framebuffer_hdr->clearDepth(1.0f);
		

		//render background
		rc.applyProgram(*program_skybox);
		rc.applyCameraUniforms();

		gl::glDepthMask(false);
		rc.applyTextures({ 
			{"source", skybox_texture}
		});
		renderFullscreenQuad();

		//render scene
		gl::glDepthMask(true);
		rc.applyProgram(*program_mesh);
		scene.render(rc);

		framebuffer_hdr->unbind();

		
		//render postprocess
		gl::glClear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
		//gl::glDepthMask(false);

		rc.applyProgram(*program_postprocess);
		//rc.applyCameraUniforms();
		rc.active_program->setUniform("u_tmExposure", exposition);
		rc.applyTextures({ 
			{"u_colorMap", framebuffer_hdr_color1},
		    {"u_depthMap", framebuffer_hdr_depth}
		});
		renderFullscreenQuad();

		assert(rc.transforms.empty());
    }

	void renderFullscreenQuad()
    {
		fullscreenQuad->vao->bind();
		fullscreenQuad->vao->drawArrays(gl::GL_TRIANGLE_STRIP, 0, 4);
    }

private:
	std::shared_ptr<GlfwWindow> window;

	Scene scene;
	TextureRef skybox_texture;
	MeshRef fullscreenQuad;

	Program program_skybox, program_mesh, program_postprocess;

	//BufferRef camera_buffer;


	float exposition = 0.5f;
	std::shared_ptr<globjects::Texture> framebuffer_hdr_color1;
	std::shared_ptr<globjects::Texture> framebuffer_hdr_depth;
	std::unique_ptr<globjects::Framebuffer> framebuffer_hdr;

	bool dirty_framebuffers = true;
	

	Camera camera;
	RenderingContext rc;
};


int main()
{
	try
	{
		App app;
		app.Run();

		return EXIT_SUCCESS;
	}
	catch (const std::exception& e)
	{
		spdlog::error("Exception! {0}", e.what());

		return EXIT_FAILURE;
	}
}
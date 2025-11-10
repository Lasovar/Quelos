#include <qspch.h>
#include "Renderer.h"
#include <bgfx/bgfx.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Quelos/Core/Window.h"
#include "bx/math.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Quelos {

	struct PosColorVertex {
		float x;
		float y;
		float z;
		uint32_t abgr;
	};

	static PosColorVertex cubeVertices[] =
	{
		{-1.0f,  1.0f,  1.0f, 0xff000000 },
		{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
		{-1.0f, -1.0f,  1.0f, 0xff00ff00 },
		{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
		{-1.0f,  1.0f, -1.0f, 0xffff0000 },
		{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
		{-1.0f, -1.0f, -1.0f, 0xffffff00 },
		{ 1.0f, -1.0f, -1.0f, 0xffffffff },
	};

	static const uint16_t cubeTriList[] =
	{
		0, 1, 2,
		1, 3, 2,
		4, 6, 5,
		5, 6, 7,
		0, 2, 4,
		4, 2, 6,
		1, 5, 3,
		5, 7, 3,
		0, 4, 1,
		4, 5, 1,
		2, 3, 6,
		6, 3, 7,
	};

	static bgfx::ShaderHandle loadShader(const std::string& fileName)
	{
		std::string shaderPath;

		switch (bgfx::getRendererType()) {
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12: shaderPath = "Assets/shaders/dx11/";  break;
		case bgfx::RendererType::Gnm:        shaderPath = "Assets/shaders/pssl/";  break;
		case bgfx::RendererType::Metal:      shaderPath = "Assets/shaders/metal/"; break;
		case bgfx::RendererType::OpenGL:     shaderPath = "Assets/shaders/glsl/";  break;
		case bgfx::RendererType::OpenGLES:   shaderPath = "Assets/shaders/essl/";  break;
		case bgfx::RendererType::Vulkan:     shaderPath = "Assets/shaders/spirv/"; break;
		}

		std::filesystem::path filePath = shaderPath + fileName;

		std::ifstream file(filePath, std::ios::binary);
		if (!file.is_open()) {
			QS_CORE_ERROR("Couldn't open file '{}'", filePath.string());
			return BGFX_INVALID_HANDLE;
		}

		std::streamsize fileSize = std::filesystem::file_size(filePath);

		const bgfx::Memory* mem = bgfx::alloc(fileSize + 1);
		file.read(reinterpret_cast<char*>(mem->data), fileSize);

		mem->data[fileSize] = '\0';

		bgfx::ShaderHandle handle = bgfx::createShader(mem);
		bgfx::setName(handle, fileName.c_str(), fileName.length());

		return handle;
	}

	static bgfx::ProgramHandle program;

	static bgfx::VertexBufferHandle vbh;
	static bgfx::IndexBufferHandle ibh;
	static Ref<Window> s_Window;

	void Renderer::Init(const Ref<Window>& window) {
		s_Window = window;

		bgfx::PlatformData platformData;
		platformData.nwh = window->GetNativeWindow();

		bgfx::Init bgfxInit;
		bgfxInit.type = bgfx::RendererType::Count;
		bgfxInit.resolution.width = window->GetWidth();
		bgfxInit.resolution.height = window->GetHeight();
		bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
		bgfxInit.platformData = platformData;
		bgfx::init(bgfxInit);

		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);

		bgfx::VertexLayout pvcLayout;
		pvcLayout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();

		vbh = bgfx::createVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), pvcLayout);
		ibh = bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));

		bgfx::ShaderHandle vsh = loadShader("vs_cubes.bin");
		bgfx::ShaderHandle fsh = loadShader("fs_cubes.bin");

		if (!bgfx::isValid(vsh)) {
			QS_CORE_ERROR("Vertext shader failed to load");
		}

		if (!bgfx::isValid(fsh)) {
			QS_CORE_ERROR("Fragment shader failed to load");
		}

		program = bgfx::createProgram(vsh, fsh, true);
	}

	void Renderer::StartFrame()
	{
		bgfx::setViewRect(0, 0, 0, s_Window->GetWidth(), s_Window->GetHeight());
		bgfx::touch(0);
	}

	void Renderer::StartSceneRender(const CameraComponent& Camera, const TransformComponent& CameraTransform) {
		static int counter = 0;

		const glm::vec3 at(0);
		const glm::vec3 eye(0, 0, -5);

		glm::mat4 view = glm::translate(glm::mat4(1.0f), CameraTransform.Position) * glm::toMat4(CameraTransform.Rotation);
		view = glm::inverse(view);

		//bx::mtxLookAt(glm::value_ptr(view), bx::Vec3(0, 0, -5), bx::Vec3(0));
		glm::mat4 proj;
		bx::mtxProj(
			glm::value_ptr(proj),
			Camera.FOV,
			(float)s_Window->GetWidth() / s_Window->GetHeight(),
			Camera.Near,
			Camera.Far,
			bgfx::getCaps()->homogeneousDepth
		);

		bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj));

		glm::mat4 mat;
		bx::mtxRotateXY(glm::value_ptr(mat), counter * 0.01f, counter * 0.01f);

		bgfx::setTransform(glm::value_ptr(mat));

		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);

		bgfx::submit(0, program);
		counter++;

	}

	void Renderer::EndFrame()
	{
		bgfx::frame();
	}

	void Renderer::SubmitMesh() {
		
	}

	void Renderer::Shutdown() {
		bgfx::shutdown();
	}
}


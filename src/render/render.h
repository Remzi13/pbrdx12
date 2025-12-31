#pragma once

#include "core/math_utils.h"

#include "render/device/device_interface.h"

namespace render {

	class Render
	{
	public:
		bool init(HWND hwnd, int width, int height, core::SizeI viewportSize);
		void fini();

		void resize(int width, int height) const;
		void viewportResize(int width, int height);
		void draw() const;
		void present() const;

		void loadTexture(const char* file);
		[[nodiscard]] vector<string> texturesName() const;

		Texture* viewport() const { return viewport_.get(); }
		Texture* backBuffer() const;
		[[nodiscard]] GraphicsDevice* device() const { return device_.get(); }

		void createMaterials() const;
	private:
		SharedPtr<Texture> createTextureFromFile(const char* fileName) const;

	private:
		ResourceFormat format_{ ResourceFormat::RGBA8_UNORM };
		SharedPtr<GraphicsDevice> device_;
		SharedPtr<SwapChain> swapChain_;
		UniquePtr<CommandContext> commandContext_;
		SharedPtr<Texture> viewport_;
		SharedPtr<Texture> depthStencil_;
		SharedPtr<Texture> skyBox_;

		unordered_map<string, SharedPtr<Texture>> textures_;
	};
}
#pragma once
#include "../graphics/opengl/GLSLProgram.h"
#include "../scene/Scene.h"
#include "../resources/AssetManager.h"
#include "../core/paths.h"

namespace Lengine {
	class ShadowMap {
	public:

		ShadowMap(uint32_t shadowRes) : SHADOW_RES(shadowRes) {
			Init();
		}
		ShadowMap() = default;
		void Init();
		void renderDepthMap(
			const std::vector<std::unique_ptr<Entity>>& entities,
			const ComponentStorage<TransformComponent>& trs,
			const ComponentStorage<MeshFilter>& mfs,
			const UUID& mainDirectionalLight,
			AssetManager& assetManager,
			const glm::vec3& camPos
			);

		const GLuint& getDepthTexture() { return shadowDepthTex; }

		float nearPlane = 0.01f;
		float farPlane = 1000.5f;
	private:

		GLuint shadowFBO = 0;
		GLuint shadowDepthTex = 0;

		uint32_t SHADOW_RES = 1024;

		GLSLProgram depthShader;

		UUID prevLight = UUID::Null;
	};
}
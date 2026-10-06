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
			const std::vector<Entity> entities,
			const ComponentStorage<TransformComponent>& trs,
			const ComponentStorage<MeshFilter>& mfs,
			const ComponentStorage<AnimationComponent>& anims,
			const Entity& mainDirectionalLight,
			AssetManager& assetManager,
			const glm::vec3& camPos
			);

		const GLuint& getDepthTexture() { return shadowDepthTex; }

		const float shadowExtent = 20.0f;
		const float shadowNear = 0.1f;
		float shadowFar; 
		glm::mat4 lightSpaceMat = glm::mat4(1.0f);

		const uint32_t SHADOW_RES = 1024;

	private:

		GLuint shadowFBO = 0;
		GLuint shadowDepthTex = 0;


		GLSLProgram depthShader;

		Entity prevLight = NullEntity;
	};
}
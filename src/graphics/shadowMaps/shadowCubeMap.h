#pragma once

#include "../graphics/opengl/GLSLProgram.h"
#include "../core/paths.h"
#include "../resources/AssetManager.h"
namespace Lengine {
	class ShadowCubeMap {
	public:
		ShadowCubeMap() = default;
		ShadowCubeMap(unsigned int shadowRes) : SHADOW_RES(shadowRes) {
			Init();
		}
		void Init();
		void updateTransforms(const glm::vec3& lightPos);
		const glm::mat4& getShadowProj() { return shadowProj; }
		const GLuint& getDepthCubeMap() { return depthCubeMap; }
		const float& getNearPlane() { return nearPlane; }
		const float& getFarPlane() { return farPlane; }

		void renderDepthCubeMap(
			const std::vector<std::unique_ptr<Entity>>& entities,
			const ComponentStorage<TransformComponent>& trs,
			const ComponentStorage<MeshFilter>& mfs,
			const UUID& mainPointLight,
			AssetManager& assetManager
		);

	private:


		GLuint depthCubeMap = 0;
		GLuint depthMapFBO = 0;
		GLSLProgram depthCubeMapShader;

		unsigned int SHADOW_RES = 1024;
		
		const float nearPlane = 1.0f;
		const float farPlane = 25.0f;
		glm::mat4 shadowProj;
		std::vector<glm::mat4> shadowTransforms;

		UUID prevLight = UUID::Null;
	};
}
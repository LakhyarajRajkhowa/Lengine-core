#pragma once
#include <unordered_map>
#include <queue>
#include <memory>
#include <filesystem>


#include "../core/settings.h"
#include "../core/paths.h"
#include "../assets/AssetRegistry.h"

#include "../graphics/opengl/GLSLProgram.h"
#include "../graphics/opengl/GLTexture.h"
#include "../graphics/geometry/Mesh.h"
#include "../graphics/material/MaterialLoader.h"
#include "../resources/TextureCache.h"
#include "../utils/metaFileSystem.h"
#include "../utils/modelFileSystem.h"
#include "../utils/jsonHelper.h"
#include "../utils/C++20.h"
#include "../scene/Scene.h"

#include "resources/AssetEditor.h"
#include "resources/AssetImporter.h"

#include "resources/LoadingSystem.h"

#include "transform/TransformSystem.h"
namespace fs = std::filesystem;

namespace Lengine {
	enum class AssetState {
		Failed = 0,
		Importing = 1,
		Imported = 2,
		Loading = 3,
		LoadedToCPU = 4,
		LoadedToGPU = 5,
		Loaded = 6,
		Unloaded = 7,
		
	};

	struct Asset {
		AssetType type;
		AssetState state;
		UUID id;
	};

	struct AssetView
	{
		UUID uuid;
		std::string name;
		AssetType type;
		std::filesystem::path libraryPath;
		std::filesystem::path thumbnailPath;
	};

	struct PendingSubmeshRequest {
		Entity entityID;
		UUID meshID;
	};



	class AssetManager {
	private:
		std::unordered_map<std::string, std::shared_ptr<GLSLProgram>> shaders;
		std::unordered_map<UUID, std::shared_ptr<GLTexture>> textures;
		std::unordered_map<UUID, std::shared_ptr<PhongMaterial>> materials;
		std::unordered_map<UUID, std::shared_ptr<Material>> pbrMaterials;
		std::unordered_map<UUID, std::shared_ptr<Mesh>> submeshes;
		std::unordered_map<UUID, std::shared_ptr<Skeleton>> skeletons;
		std::unordered_map<UUID, std::shared_ptr<Animation>> animations;


		TextureCache textureCache;
		
	public:
		AssetManager(EngineSettings& set):
			settings(set)
		{
			
		}

		void Init();
		void LoadAllDefaultAssets();

		void Update(Scene& activeScene);

		std::vector<AssetView> allAssetViews;
		void UpdateAllAssetViews();

		std::vector<AssetView> submeshViews;
		std::vector<AssetView> GetAllSubmeshFromDatabase() const { return submeshViews; }

		std::vector<AssetView> pbrMaterialViews;
		std::vector<AssetView> GetAllPbrMaterialFromDatabase() const { return pbrMaterialViews; }

		std::vector<AssetView> TextureViews;
		std::vector<AssetView> GetAllTexturesFromDatabase() const { return TextureViews; }

		std::vector<AssetView> PrefabViews;
		std::vector<AssetView> GetAllPrefabsFromDatabase() const { return PrefabViews; }

		const AssetMetadata* GetAssetMetaData(const UUID& uuid) const;

		// SUBMESH
		void RequestSubmeshLoad(const UUID& meshID, const Entity& entityID);
		bool LoadSubmesh(const UUID& uuid);
		bool processPendingSubmesh(const UUID& id);
		Mesh* GetSubmesh(const UUID& id);

		// SKELETONS
		bool LoadSkeleton(const UUID& uuid);
		Skeleton* GetSkeleton(const UUID& id);

		// SKELETONS
		bool LoadAnimation(const UUID& uuid);
		Animation* GetAnimation(const UUID& id);

		// MATERIAL (PBR)
		void ImportMaterial(const std::string path);
		UUID CreateMaterial(const std::string name);
		void SaveMaterial(const UUID& id);
		bool LoadMaterial(const UUID& uuid);

		Material* GetMaterial(const UUID& id);

		// PREFAB
		void ImportMesh(const std::string& path);
		void ImportPrefab(const std::string& path);
		bool LoadPrefabToScene(const std::string& path);
		Entity* InstantiatePrefab(
			Scene& scene,
			const PrefabData& prefab
		);


		// TEXTURES
		UUID GetOrCreateTextureUUID(const std::string& path);
		void ImportTexture(const std::string& path, const UUID& assetID);
		void RequestTextureLoad(const UUID& texID, const UUID& matID, const TextureMapType& texMapType, bool srgb = false);
		void RequestTextureLoad_inst(const UUID& uuid, const Entity& entityID, const TextureMapType& texMapType, bool srgb = false);

		std::shared_ptr<ImageData> LoadTexture(const UUID& uuid);
		GLTexture* loadImage(const std::string& name, const std::string& path);
		bool processPendingTextures(const UUID& id);
		GLTexture* getTexture(const UUID& id);



		// SHADERS
		GLSLProgram* loadShader(const std::string& name,
			const std::string& vertPath,
			const std::string& fragPath);
		GLSLProgram* getShader(const std::string& name);


		// ASSET REGSISTRY


		// ASSET DATABASE
		void LoadAssetDatabase();
		void saveAssetDatabase();

		// SCENE
		std::unique_ptr<Scene> createScene(const std::string& name, const std::string& folderPath);
		void saveScene(const Scene& scene, const std::string& filePath);
		std::unique_ptr<Scene> loadScene(const std::string& filePath);


		std::unordered_map<UUID, AssetState> assetStates;
		std::mutex assetMutex;
		UUID currentLoadingAsset = UUID::Null;
		std::pair<UUID, std::string> currentImportingAsset = std::pair(UUID::Null, "Unknown");
		
		bool haveAssetState(const UUID& assetID);

		bool hasLoadingAssets();
		float getLoadingProgress(const UUID& id) const;
		void ProcessGpuUploads();
		void drawLoadingScreens();

		bool hasImportingAssets();
		float getImportingProgress(const UUID& id) const;
		void drawImportingScreens();

		void SyncAssetsToScene(Scene& activeScene);
		void UpdateAssetStates();
		void ProcessPendingTextureRequests(Scene& activeScene);


		UUID getCurrentLoadingAsset() { return currentLoadingAsset; }
		std::pair<UUID, std::string> getCurrentImportingAsset() { return currentImportingAsset; }

		AssetType getAssetType(const UUID& id);

	private:
		EngineSettings& settings;

	private:
		std::queue<PendingSubmeshRequest> pendingSubmeshes; // (entityID, submeshID)
		std::queue<TextureLoadRequest> pendingTextureRequests;
		std::queue<PrefabData> pendingPrefabs;

		std::unordered_map<std::string, UUID> texturePathToUUID;


	};
}
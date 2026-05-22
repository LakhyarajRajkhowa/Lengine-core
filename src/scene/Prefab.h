#pragma once

#include "../utils/UUID.h"
#include "animations/Skeleton.h"
#include "external/assimp/Importer.hpp"

namespace Lengine {

	// so basically this will be a tree structure
	// rootnode pointing to  children and so on

	// this exist only at importing time
	// at runtime PrefabNode -> Entity
	// so memory will be cleared after importing 
	class PrefabNode {
	public:
		std::string name = "NewPrefabNode";
		glm::mat4 localTransform = glm::mat4(1.0f);


		UUID meshID = UUID::Null;
		UUID materialID = UUID::Null;


		PrefabNode* parent;
		std::vector<PrefabNode*> children;

		~PrefabNode() {
			for (PrefabNode* c : children)
				delete c;
		}
	};


	// this will provide the root node (the root parent entity) 
	class Prefab {
	public:
		UUID id = UUID::Null;
		std::string name = "NewPrefab";
		std::filesystem::path path = "NewPrefab.prefab";

		std::unordered_map<const aiMesh*, UUID> loadedSubmeshes;
		std::unordered_map<const aiMaterial*, UUID> importedMaterials;


		UUID skeletonID = UUID::Null;
		std::vector<UUID> animationIDs = { UUID::Null };

		PrefabNode* rootPrefabNode; 


		~Prefab() {
			delete rootPrefabNode;
		}

	};

	struct PrefabNodeData
	{
		uint32_t index;          // this node index
		int32_t parentIndex = -1;     // -1 if root

		std::string name;
		glm::mat4 localTransform;

		UUID meshID = UUID::Null;
		UUID materialID = UUID::Null;


		std::optional<UUID> map_albedo;
	};

	struct PrefabData
	{
		UUID id;
		std::string name;

		UUID skeletonID = UUID::Null;
		std::vector<UUID> animationIDs = { UUID::Null };

		std::vector<PrefabNodeData> nodes ;
	};

	static void CollectPrefabNodes(
		PrefabNode* node,
		int parentIndex,
		std::vector<PrefabNodeData>& outNodes
	)
	{
		uint32_t myIndex = (uint32_t)outNodes.size();

		PrefabNodeData data;
		data.index = myIndex;
		data.parentIndex = parentIndex;
		data.name = node->name;
		data.localTransform = node->localTransform;
		data.meshID = node->meshID;
		data.materialID = node->materialID;

		outNodes.push_back(data);

		for (PrefabNode* child : node->children)
		{
			CollectPrefabNodes(child, myIndex, outNodes);
		}
	}

	static void SavePrefab(const Prefab& prefab)
	{
		PrefabData data;
		data.id = prefab.id;
		data.name = prefab.name;
		data.skeletonID = prefab.skeletonID;
		data.animationIDs = prefab.animationIDs;

		CollectPrefabNodes(prefab.rootPrefabNode, -1, data.nodes);

		std::ofstream out(prefab.path, std::ios::binary);

		// ---- Header ----
		out.write((char*)&data.id, sizeof(UUID));

		uint32_t nameLen = (uint32_t)data.name.size();
		out.write((char*)&nameLen, sizeof(uint32_t));
		out.write(data.name.data(), nameLen);

		uint32_t nodeCount = (uint32_t)data.nodes.size();
		out.write((char*)&nodeCount, sizeof(uint32_t));

		// skeleton
		out.write((char*)&data.skeletonID, sizeof(UUID));

		// animations
		uint32_t animCount = (uint32_t)data.animationIDs.size();
		out.write((char*)&animCount, sizeof(uint32_t));

		for (auto& animID : data.animationIDs)
			out.write((char*)&animID, sizeof(UUID));

		// ---- Nodes ----
		for (auto& n : data.nodes)
		{
			out.write((char*)&n.index, sizeof(uint32_t));
			out.write((char*)&n.parentIndex, sizeof(int32_t));

			uint32_t len = (uint32_t)n.name.size();
			out.write((char*)&len, sizeof(uint32_t));
			out.write(n.name.data(), len);

			out.write((char*)&n.localTransform, sizeof(glm::mat4));
			out.write((char*)&n.meshID, sizeof(UUID));
			out.write((char*)&n.materialID, sizeof(UUID));

			// optional albedo map
			bool hasAlbedo = n.map_albedo.has_value();
			out.write((char*)&hasAlbedo, sizeof(bool));

			if (hasAlbedo)
			{
				UUID id = n.map_albedo.value();
				out.write((char*)&id, sizeof(UUID));
			}
		}
	}

	static PrefabData LoadPrefabFile(const std::filesystem::path& path)
	{
		PrefabData data;
		std::ifstream in(path, std::ios::binary);

		in.read((char*)&data.id, sizeof(UUID));

		uint32_t nameLen;
		in.read((char*)&nameLen, sizeof(uint32_t));
		data.name.resize(nameLen);
		in.read(data.name.data(), nameLen);

		uint32_t nodeCount;
		in.read((char*)&nodeCount, sizeof(uint32_t));
		data.nodes.resize(nodeCount);

		// skeleton
		in.read((char*)&data.skeletonID, sizeof(UUID));

		// animations
		uint32_t animCount;
		in.read((char*)&animCount, sizeof(uint32_t));

		data.animationIDs.resize(animCount);
		for (uint32_t i = 0; i < animCount; i++)
			in.read((char*)&data.animationIDs[i], sizeof(UUID));

		// ---- Nodes ----
		for (uint32_t i = 0; i < nodeCount; i++)
		{
			auto& n = data.nodes[i];

			in.read((char*)&n.index, sizeof(uint32_t));
			in.read((char*)&n.parentIndex, sizeof(int32_t));

			uint32_t len;
			in.read((char*)&len, sizeof(uint32_t));
			n.name.resize(len);
			in.read(n.name.data(), len);

			in.read((char*)&n.localTransform, sizeof(glm::mat4));
			in.read((char*)&n.meshID, sizeof(UUID));
			in.read((char*)&n.materialID, sizeof(UUID));

			// optional albedo
			bool hasAlbedo;
			in.read((char*)&hasAlbedo, sizeof(bool));

			if (hasAlbedo)
			{
				UUID id;
				in.read((char*)&id, sizeof(UUID));
				n.map_albedo = id;
			}
		}

		return data;
	}

	static void DestroyPrefabNodeTree(PrefabNode* node)
	{
		for (PrefabNode* child : node->children)
			DestroyPrefabNodeTree(child);

		delete node;
	}



}
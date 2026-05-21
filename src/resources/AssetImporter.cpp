#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include "AssetImporter.h"

#include <string>
#include <fstream>

namespace fs = std::filesystem;

using namespace Lengine;


// u know all this Import code are almost same...should probably merge them
// TODO : Merge the import codes 
bool AssetImporter::ImportMeshFile(const fs::path& externalPath, const UUID& uuid)
{
    if (!fs::exists(externalPath)) {
        std::cout << "Asset file does not exist: " << externalPath << std::endl;
        return false;
    }


    fs::path assetsRoot = Paths::GameAssets_Mesh;

    // Decide asset name
    fs::path assetName;
    if (externalPath.extension() == ".gltf")
        assetName = externalPath.parent_path().filename(); 
    else
        assetName = externalPath.stem();

    fs::path assetDir = assetsRoot / assetName;

    // Handle folder name collisions
    int counter = 1;
    while (fs::exists(assetDir))
    {
        // if already exist means already imported !
        std::cout << assetDir.filename() << " already exists: " << externalPath << std::endl;

        return false;
       // assetDir = assetsRoot /
        //    (assetName.string() + "_" + std::to_string(counter++));
    }

    fs::create_directories(assetDir);

    // Final mesh path
    fs::path destMeshPath = assetDir / externalPath.filename();


    // Copy main mesh file
    fs::copy_file(externalPath, destMeshPath);

    // ---------------- GLTF DEPENDENCIES ----------------
    if (externalPath.extension() == ".gltf")
    {
        fs::path srcDir = externalPath.parent_path();
        fs::path dstDir = assetDir;

        // Copy .bin files
        for (auto& p : fs::directory_iterator(srcDir))
        {
            if (p.path().extension() == ".bin")
            {
                fs::copy_file(
                    p.path(),
                    dstDir / p.path().filename(),
                    fs::copy_options::overwrite_existing
                );
            }
        }

        // Copy textures folder (if exists)
        fs::path srcTextures = srcDir / "textures";
        if (fs::exists(srcTextures))
        {
            fs::copy(
                srcTextures,
                dstDir / "textures",
                fs::copy_options::recursive |
                fs::copy_options::overwrite_existing
            );
        }
    }

    // ---------------- META FILE ----------------
    UUID fileID = uuid;

    fs::path metaPath = destMeshPath;
    metaPath += ".meta";

    std::ofstream meta(metaPath);
    meta << "{\n";
    meta << "  \"fileID\": \"" << fileID.value() << "\",\n";
    meta << "  \"sourcePath\": \"" << destMeshPath.generic_string() << "\",\n";
    meta << "  \"importType\": \"Mesh\"\n";
    meta << "}\n";
    meta.close();

    // ---------------- IMPORT ----------------
    MeshImporter::Import(destMeshPath, fileID);

    return true;
}



static void AddBoneData(Vertex& v, int boneID, float weight)
{
    // Check if this bone is already added
    for (int i = 0; i < 4; i++)
    {
        if (v.boneIDs[i] == boneID)
        {
            v.weights[i] += weight;
            std::cout << v.weights[i] << std::endl;
            return;
        }
    }

    // Add to first empty slot
    for (int i = 0; i < 4; i++)
    {
        if (v.weights[i] == 0.0f)
        {
            v.boneIDs[i] = boneID;
            v.weights[i] = weight;
            return;
        }
    }

    // Vertex has more than 4 bones, take the largest weights
    int minIndex = 0;
    for (int i = 1; i < 4; i++)
        if (v.weights[i] < v.weights[minIndex])
            minIndex = i;

    if (weight > v.weights[minIndex])
    {
        v.boneIDs[minIndex] = boneID;
        v.weights[minIndex] = weight;
    }



}

static void ProcessSubMesh(
    const aiMesh* aiMesh,
    uint32_t subMeshIndex,
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& outDir,
    UUID parentMeshID,
    LMeshSubMeshRef& outRef,
    UUID meshID = UUID()
    
    )
{
    LSubMeshFile sm;
    sm.subMeshID = meshID;
    sm.parentMeshID = parentMeshID;
    sm.sourcePath = sourcePath.string();
    sm.subMeshIndex = subMeshIndex;

    // ---------------- Vertices ----------------
    sm.vertices.reserve(aiMesh->mNumVertices);
    sm.indices.reserve(aiMesh->mNumFaces * 3);

    for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
    {
        Vertex v{};

        // Position
        if (aiMesh->HasPositions() && aiMesh->mVertices)
        {
            v.position = glm::vec3(
                aiMesh->mVertices[i].x,
                aiMesh->mVertices[i].y,
                aiMesh->mVertices[i].z
            );
        }
        else
        {
            v.position = glm::vec3(0.0f);
        }

        // Normal
        if (aiMesh->HasNormals() && aiMesh->mNormals)
        {
            v.normal = glm::vec3(
                aiMesh->mNormals[i].x,
                aiMesh->mNormals[i].y,
                aiMesh->mNormals[i].z
            );
        }
        else
        {
            v.normal = glm::vec3(0.0f);
        }

        // Texture coordinates (UV0 only)
        if (aiMesh->HasTextureCoords(0) && aiMesh->mTextureCoords[0])
        {
            v.texCoord = glm::vec2(
                aiMesh->mTextureCoords[0][i].x,
                aiMesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            v.texCoord = glm::vec2(0.0f);
        }

        // Tangent & Bitangent
        if (aiMesh->HasTangentsAndBitangents() &&
            aiMesh->mTangents &&
            aiMesh->mBitangents)
        {
            v.tangent = glm::vec3(
                aiMesh->mTangents[i].x,
                aiMesh->mTangents[i].y,
                aiMesh->mTangents[i].z
            );

            v.bitangent = glm::vec3(
                aiMesh->mBitangents[i].x,
                aiMesh->mBitangents[i].y,
                aiMesh->mBitangents[i].z
            );
        }
        else
        {
            v.tangent = glm::vec3(0.0f);
            v.bitangent = glm::vec3(0.0f);
        }

        sm.vertices.push_back(v);
    }



    // ---------------- Indices ----------------
    for (uint32_t f = 0; f < aiMesh->mNumFaces; f++)
    {
        const aiFace& face = aiMesh->mFaces[f];
        for (uint32_t i = 0; i < face.mNumIndices; i++)
            sm.indices.push_back(face.mIndices[i]);
    }

    // ----------------- Name --------------------
    std::string name = "submesh_" + std::to_string(subMeshIndex);
    if (aiMesh->mName.C_Str())
    {
        name = aiMesh->mName.C_Str();
    }
    
    // ---------------- Write .lsubmesh ----------------
    std::filesystem::path subMeshPath =
        outDir / (name + ".lsubmesh");

    WriteLSubMesh(subMeshPath, sm);


    // ---------------- Register SubMesh asset ----------------
    AssetMetadata subMeshMeta;
    subMeshMeta.uuid = sm.subMeshID;
    subMeshMeta.name = name;
    subMeshMeta.type = AssetType::Mesh;
    subMeshMeta.libraryPath = subMeshPath;      // full path is better
    subMeshMeta.sourcePath = sourcePath;
    subMeshMeta.thumbnailPath = Paths::Icons + "submesh_icon.png";

    AssetDatabase::RegisterAsset(subMeshMeta);

    // ---------------- Fill lmesh reference ----------------
    outRef.subMeshID = sm.subMeshID;
    outRef.subMeshIndex = subMeshIndex;
    outRef.subMeshPath = subMeshPath.filename();
}

static void ProcessSubMesh_prefab(
    const aiMesh* aiMesh,
    uint32_t subMeshIndex,
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& outDir,
    UUID parentMeshID,
    LMeshSubMeshRef& outRef,
    std::unordered_map<std::string, int>& skeletonBoneMap,
    UUID meshID = UUID()

)
{
    LSubMeshFile sm;
    sm.subMeshID = meshID;
    sm.parentMeshID = parentMeshID;
    sm.sourcePath = sourcePath.string();
    sm.subMeshIndex = subMeshIndex;

    // ---------------- Vertices ----------------
    sm.vertices.reserve(aiMesh->mNumVertices);
    sm.indices.reserve(aiMesh->mNumFaces * 3);

    for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
    {
        Vertex v{};

        // Position
        if (aiMesh->HasPositions() && aiMesh->mVertices)
        {
            v.position = glm::vec3(
                aiMesh->mVertices[i].x,
                aiMesh->mVertices[i].y,
                aiMesh->mVertices[i].z
            );
        }
        else
        {
            v.position = glm::vec3(0.0f);
        }

        // Normal
        if (aiMesh->HasNormals() && aiMesh->mNormals)
        {
            v.normal = glm::vec3(
                aiMesh->mNormals[i].x,
                aiMesh->mNormals[i].y,
                aiMesh->mNormals[i].z
            );
        }
        else
        {
            v.normal = glm::vec3(0.0f);
        }

        // Texture coordinates (UV0 only)
        if (aiMesh->HasTextureCoords(0) && aiMesh->mTextureCoords[0])
        {
            v.texCoord = glm::vec2(
                aiMesh->mTextureCoords[0][i].x,
                aiMesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            v.texCoord = glm::vec2(0.0f);
        }

        // Tangent & Bitangent
        if (aiMesh->HasTangentsAndBitangents() &&
            aiMesh->mTangents &&
            aiMesh->mBitangents)
        {
            v.tangent = glm::vec3(
                aiMesh->mTangents[i].x,
                aiMesh->mTangents[i].y,
                aiMesh->mTangents[i].z
            );

            v.bitangent = glm::vec3(
                aiMesh->mBitangents[i].x,
                aiMesh->mBitangents[i].y,
                aiMesh->mBitangents[i].z
            );
        }
        else
        {
            v.tangent = glm::vec3(0.0f);
            v.bitangent = glm::vec3(0.0f);
        }

        sm.vertices.push_back(v);
    }


    if (aiMesh->HasBones())
    {
        for (uint32_t b = 0; b < aiMesh->mNumBones; b++)
        {
            aiBone* bone = aiMesh->mBones[b];
            bool hasWeight = false;

            // check if this bone has any non-zero weights
            for (uint32_t w = 0; w < bone->mNumWeights; w++)
            {
                if (bone->mWeights[w].mWeight > 0.0f)
                {
                    hasWeight = true;
                    break;
                }
            }

            if (!hasWeight)
                continue; // skip bones that don't influence any vertex

            // add bone index to palette
            sm.bonePalette.push_back(b);

            // now assign weights to vertices
            for (uint32_t w = 0; w < bone->mNumWeights; w++)
            {
                aiVertexWeight weight = bone->mWeights[w];
                int vertexID = weight.mVertexId;
                float value = weight.mWeight;

                AddBoneData(sm.vertices[vertexID], static_cast<int>(sm.bonePalette.size() - 1), value);
                // note: local ID = position in palette
            }
        }
    }
    // ---------------- Indices ----------------
    for (uint32_t f = 0; f < aiMesh->mNumFaces; f++)
    {
        const aiFace& face = aiMesh->mFaces[f];
        for (uint32_t i = 0; i < face.mNumIndices; i++)
            sm.indices.push_back(face.mIndices[i]);
    }

    // ----------------- Name --------------------
    std::string name = "submesh_" + std::to_string(subMeshIndex);
    if (aiMesh->mName.C_Str())
    {
        name = aiMesh->mName.C_Str();
    }

    // ---------------- Write .lsubmesh ----------------
    std::filesystem::path subMeshPath =
        outDir / (name + ".lsubmesh");

    WriteLSubMesh(subMeshPath, sm);


    // ---------------- Register SubMesh asset ----------------
    AssetMetadata subMeshMeta;
    subMeshMeta.uuid = sm.subMeshID;
    subMeshMeta.name = name;
    subMeshMeta.type = AssetType::Mesh;
    subMeshMeta.libraryPath = subMeshPath;      // full path is better
    subMeshMeta.sourcePath = sourcePath;
    subMeshMeta.thumbnailPath = Paths::Icons + "submesh_icon.png";

    AssetDatabase::RegisterAsset(subMeshMeta);

    // ---------------- Fill lmesh reference ----------------
    outRef.subMeshID = sm.subMeshID;
    outRef.subMeshIndex = subMeshIndex;
    outRef.subMeshPath = subMeshPath.filename();
}
void MeshImporter::Import(const std::filesystem::path& meshPath, UUID sourceFileID)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        meshPath.string(),
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices
    );


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes()) {
        std::cout << "Assimp error: "<< meshPath<<" :" << importer.GetErrorString() << std::endl;
        return ;
    }


    // Output directory
    std::filesystem::path assetName =
        meshPath.parent_path().filename();  

    std::filesystem::path outDir =
        Paths::GameLibrary_Assets_Mesh / assetName;

    std::filesystem::create_directories(outDir);


    // ---------------- Create lmesh ----------------
    LMeshFile lmesh;
    lmesh.meshID = UUID();
    lmesh.sourcePath = meshPath.string();

    lmesh.subMeshes.reserve(scene->mNumMeshes);


    // ---------------- Submeshes ----------------
    for (uint32_t i = 0; i < scene->mNumMeshes; i++)
    {
        LMeshSubMeshRef ref{};
        ProcessSubMesh(
            scene->mMeshes[i],
            i,
            meshPath,
            outDir,
            lmesh.meshID,
            ref
        );

        lmesh.subMeshes.push_back(ref);
    }

    // ----------------- Name --------------------
    std::string name = meshPath.stem().string();

    // ---------------- Write .lmesh ----------------
    std::filesystem::path lmeshPath =
        outDir / (name + ".lmesh");

    WriteLMesh(lmeshPath, lmesh);

    // ---------------- Register Mesh asset ----------------
    AssetMetadata meshMeta;
    meshMeta.uuid = lmesh.meshID;
    meshMeta.name = name;
    meshMeta.type = AssetType::Mesh;
    meshMeta.libraryPath = lmeshPath;
    meshMeta.sourcePath = meshPath;
    meshMeta.thumbnailPath = Paths::Icons + "mesh_icon.png";


    AssetDatabase::RegisterAsset(meshMeta);

}

bool AssetImporter::ImportPrefabFile(const fs::path& externalPath, const UUID& uuid)
{
    if (!fs::exists(externalPath)) {
        std::cout << "Asset file does not exist: " << externalPath << std::endl;
        return false;
    }


    fs::path assetsRoot = Paths::GameAssets_Prefab;

    // Decide asset name
    fs::path assetName;
    if (externalPath.extension() == ".gltf")
        assetName = externalPath.parent_path().filename();
    else
        assetName = externalPath.stem();

    fs::path assetDir = assetsRoot / assetName;

    // Handle folder name collisions
    int counter = 1;
    while (fs::exists(assetDir))
    {
         assetName = assetName.string() + "_" + std::to_string(counter++); 
         assetDir = assetsRoot / assetName;
    }

    fs::create_directories(assetDir);

    // Final mesh path
    fs::path destPath = assetDir / externalPath.filename();


    // Copy main mesh file
    fs::copy_file(externalPath, destPath);

    // ---------------- GLTF DEPENDENCIES ----------------
    if (externalPath.extension() == ".gltf")
    {
        fs::path srcDir = externalPath.parent_path();
        fs::path dstDir = assetDir;

        // Copy .bin files
        for (auto& p : fs::directory_iterator(srcDir))
        {
            if (p.path().extension() == ".bin")
            {
                fs::copy_file(
                    p.path(),
                    dstDir / p.path().filename(),
                    fs::copy_options::overwrite_existing
                );
            }
        }

        // Copy textures folder (if exists)
        fs::path srcTextures = srcDir / "textures";
        if (fs::exists(srcTextures))
        {
            fs::copy(
                srcTextures,
                dstDir / "textures",
                fs::copy_options::recursive |
                fs::copy_options::overwrite_existing
            );
        }
    }

    // ---------------- META FILE ----------------
    UUID fileID = uuid;

    fs::path metaPath = destPath;
    metaPath += ".meta";

    std::ofstream meta(metaPath);
    meta << "{\n";
    meta << "  \"fileID\": \"" << fileID.value() << "\",\n";
    meta << "  \"sourcePath\": \"" << destPath.generic_string() << "\",\n";
    meta << "  \"importType\": \"Prefab\"\n";
    meta << "}\n";
    meta.close();

    // ---------------- IMPORT ----------------
    Prefab* prefab = PrefabImporter::Import(destPath, fileID);

    // Save to library/prefab/prefabname.prefab
    SavePrefab(*prefab);
    
    // Register to AssetLibrary
    AssetMetadata metaData;
    metaData.uuid = prefab->id;
    metaData.name = assetName.string();
    metaData.sourcePath = destPath;
    metaData.libraryPath = prefab->path;
    metaData.type = AssetType::Prefab;
    metaData.thumbnailPath = Paths::Icons + "prefab_icon.png";

    AssetDatabase::RegisterAsset(metaData);

    return true;
}

Prefab* PrefabImporter::Import(const std::filesystem::path& assetPath, UUID sourceFileID)
{
    
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        assetPath.string(),
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices
    );
   


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes()) {
        std::cout << "Assimp error: " << assetPath << " :" << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    // Output directory
    std::filesystem::path assetName =
        assetPath.parent_path().filename();

    std::filesystem::path outDir =
        Paths::GameLibrary_Assets_Prefab / assetName;

    std::filesystem::path outMeshDir =
        Paths::GameLibrary_Assets_Mesh / assetName;

    std::filesystem::create_directories(outMeshDir);

    std::filesystem::path prefabPath = outDir / assetName.replace_extension(".prefab");
     


    // If file is a valid prefab file then proceed
    Prefab* prefab = new Prefab();
    prefab->id = sourceFileID;
    prefab->name = scene->mName.C_Str();
    prefab->path = prefabPath;

    std::filesystem::create_directories(outDir);


    // ---------------- Create lmesh ----------------
    // One Prefab = One Mesh but multiple submeshes
    LMeshFile lmesh;
    lmesh.meshID = prefab->id;
    lmesh.sourcePath = assetPath.string();
    lmesh.subMeshes.reserve(scene->mNumMeshes);

     // --- Skeleton ---

    bool hasSkeleton = false;
    std::unordered_map<std::string, int> skeletonBoneMap;
    std::vector<UUID> animationIDs;


    for (uint32_t i = 0; i < scene->mNumMeshes; i++)
    {
        if (scene->mMeshes[i]->HasBones())
        {
            hasSkeleton = true;
            break;
        }
    }

    
    LSkeletonFile skeleton;
    UUID skeletonID;

    if (hasSkeleton)
    {

        // skeleton dir
        std::filesystem::path skeletonDir =
            Paths::GameLibrary_Assets_Skeleton;

        std::filesystem::path outSkeletonDir =
                skeletonDir / assetName.replace_extension("").string();




        std::filesystem::create_directories(outSkeletonDir);

        skeletonID = UUID();

        SkeletonImporter::ImportSkeleton(
            scene,
            assetPath,
            outSkeletonDir,
            skeletonID,
            skeleton,
            skeletonBoneMap,
            std::to_string(skeletonID.toUint64())
        );

        AnimationImporter::ImportAnimations(
            scene,
            assetPath,
            outSkeletonDir,  
            skeleton,
            std::to_string(skeletonID.toUint64()),
            skeletonBoneMap,
            animationIDs
        );

    }

    prefab->skeletonID = skeletonID;
    prefab->animationIDs = animationIDs;

    // ---------------- Nodes ----------------

    // send root node of the scene for processing
    prefab->rootPrefabNode = LoadPrefabNode(
        scene,
        scene->mRootNode,
        prefab->loadedSubmeshes,
        prefab->importedMaterials,
        assetPath,
        outDir,
        lmesh,
        skeletonBoneMap,
        skeletonID,
        animationIDs
    );



    // ---------------- Write .lmesh ----------------
    std::filesystem::path lmeshPath =
        outMeshDir / (prefab->name + ".lmesh");

    WriteLMesh(lmeshPath, lmesh);

    return prefab;

}

PrefabNode* PrefabImporter::LoadPrefabNode(
    const aiScene* scene,
    const aiNode* node,
    std::unordered_map<const aiMesh*, UUID>& loadedSubmeshes,
    std::unordered_map<const aiMaterial*, UUID>& loadedMaterials,
    const std::filesystem::path& sourceAssetPath,
    const std::filesystem::path& outDir,
    LMeshFile& lmesh,
    std::unordered_map<std::string, int>& skeletonBoneMap,
    UUID skeletonID,
    std::vector<UUID>& animationIDs
    
)

{
    std::filesystem::path assetName =
        sourceAssetPath.parent_path().filename();

    std::filesystem::path outMeshDir =
        Paths::GameLibrary_Assets_Mesh / assetName;

    std::filesystem::path outMaterialDir =
        Paths::GameLibrary_Assets_Material / assetName;

    PrefabNode* parentNode = new PrefabNode();
    parentNode->name = node->mName.C_Str();
    parentNode->parent = nullptr;

    // ---- Transform ----
    aiMatrix4x4 m = node->mTransformation;
    parentNode->localTransform = glm::transpose(glm::make_mat4(&m.a1));

    // ---- Meshes ----
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        uint32_t submeshIndex = node->mMeshes[i];
        const aiMesh* submesh = scene->mMeshes[submeshIndex];

        const aiMaterial* aiMat =
            scene->mMaterials[submesh->mMaterialIndex];

        UUID materialUUID;
        UUID submeshUUID;

        if (submesh->HasBones())
        {
            skeletonID = skeletonID;
            animationIDs = animationIDs;
        }
        else
        {
            skeletonID = UUID::Null;
            animationIDs.clear();
        }


        auto it = loadedSubmeshes.find(submesh);
        if (it == loadedSubmeshes.end())
        {
            submeshUUID = UUID();

            LMeshSubMeshRef ref{};
            ProcessSubMesh_prefab(
                submesh,
                submeshIndex,
                sourceAssetPath,
                outMeshDir,
                lmesh.meshID,
                ref,
                skeletonBoneMap,
                submeshUUID
            );

            lmesh.subMeshes.push_back(ref);
            loadedSubmeshes[submesh] = submeshUUID;
        }
        else
        {
            submeshUUID = it->second;
        }

        auto mit = loadedMaterials.find(aiMat);
        if (mit == loadedMaterials.end())
        {
            materialUUID = UUID();

            MaterialImporter::ImportAssimpMaterial(
                scene,
                aiMat,
                sourceAssetPath,
                outMaterialDir,
                materialUUID
            );

            loadedMaterials[aiMat] = materialUUID;
        }
        else
        {
            materialUUID = mit->second;
        }


        PrefabNode* meshNode = new PrefabNode();
        meshNode->name = parentNode->name + "_submesh_" + std::to_string(i);
        meshNode->meshID = submeshUUID;
        meshNode->parent = parentNode;
        meshNode->materialID = materialUUID;
        parentNode->children.push_back(meshNode);
    }

    // ---- Children ----
    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        PrefabNode* child = LoadPrefabNode(
            scene,
            node->mChildren[i],
            loadedSubmeshes,
            loadedMaterials,
            sourceAssetPath,
            outDir,
            lmesh,
            skeletonBoneMap,
            skeletonID,
            animationIDs
        );

        child->parent = parentNode;
        parentNode->children.push_back(child);
    }

    return parentNode;
}



bool AssetImporter::ImportMaterialFile(const fs::path& externalPath, const UUID& uuid) {
    if (!fs::exists(externalPath)) {
        std::cout << "Asset file does not exist: " << externalPath << std::endl;
        return false;
    }

    if (externalPath.extension() != ".pbrmat") {
        std::cout << "Wrong file type: " << externalPath << std::endl;
        return false;
    }

    fs::path assetsRoot = Paths::GameAssets_Materials;

    // Decide asset name
    fs::path assetName;
  
    assetName = externalPath.stem();

    fs::path assetDir = assetsRoot / assetName;

    // Handle folder name collisions
    int counter = 1;
    while (fs::exists(assetDir))
    {
        // if already exist means already imported !
        std::cout << assetDir.filename() << " already exists: " << externalPath << std::endl;
        return UUID::Null;

    }

    fs::create_directories(assetDir);

    // Final  path
    fs::path destMaterialPath = assetDir / externalPath.filename();


    // Copy main  file
    fs::copy_file(externalPath, destMaterialPath);

    UUID fileID = uuid;

    fs::path metaPath = destMaterialPath;
    metaPath += ".meta";

    std::ofstream meta(metaPath);
    meta << "{\n";
    meta << "  \"fileID\": \"" << fileID.value() << "\",\n";
    meta << "  \"sourcePath\": \"" << destMaterialPath.generic_string() << "\",\n";
    meta << "  \"importType\": \"Material\"\n";
    meta << "}\n";
    meta.close();

    // ---------------- IMPORT ----------------
    MaterialImporter::Import(destMaterialPath, fileID);

    return true;

}

bool AssetImporter::ImportTextureFile(const fs::path& externalPath, const UUID& fileID) {
    if (!fs::exists(externalPath)) {
        std::cout << "Asset file does not exist: " << externalPath << std::endl;
        return false;
    }

    // check if this texture is from a prefab
    // if it is from prefab then dont copy to Paths::GameAssets_Textures
    
    fs::path destPath;

    fs::path prefabRoot =
        fs::weakly_canonical(
            Paths::GameAssets_Prefab
        );

    fs::path filePath =
        fs::weakly_canonical(
            externalPath
        );

    if (IsInsideDirectory(filePath, prefabRoot)) {
        destPath = externalPath;
    }
    else {
        fs::path assetsRoot = Paths::GameAssets_Textures;

        // Decide asset name
        fs::path assetName;

        assetName = externalPath.stem();

        fs::path assetDir = assetsRoot / assetName;

        // Handle folder name collisions
        int counter = 1;
        while (fs::exists(assetDir))
        {
            assetName = assetName.string() + "_" + std::to_string(counter++);
            assetDir = assetsRoot / assetName;
        }

        fs::create_directories(assetDir);

        // Final  path
       destPath = assetDir / externalPath.filename();


        // Copy main  file
        fs::copy_file(externalPath, destPath);


        fs::path metaPath = destPath;
        metaPath += ".meta";

        std::ofstream meta(metaPath);
        meta << "{\n";
        meta << "  \"fileID\": \"" << fileID.value() << "\",\n";
        meta << "  \"sourcePath\": \"" << destPath << "\",\n";
        meta << "  \"importType\": \"Texture\"\n";
        meta << "}\n";
        meta.close();
    }

  
   TextureImporter::Import(destPath, fileID);

    return true;

}

void MaterialImporter::Import(const fs::path& assetPath, UUID fileID) {
    std::string name = assetPath.stem().string();


    std::filesystem::path libPath = Paths::GameLibrary_Assets_Material + name + ".pbrmat";

    // copy the file to lib path
    fs::copy_file(assetPath, libPath);


    AssetMetadata materialMeta;
    materialMeta.uuid = fileID;
    materialMeta.name = name;
    materialMeta.type = AssetType::Material;
    materialMeta.libraryPath = libPath;
    materialMeta.sourcePath = assetPath;
    materialMeta.thumbnailPath = Paths::Icons + "material_icon.png";

    AssetDatabase::RegisterAsset(materialMeta);
}

void MaterialImporter::ImportAssimpMaterial(
    const aiScene* scene,
    const aiMaterial* mat,
    const fs::path& modelPath,
    const fs::path& outDir,
    UUID materialUUID
)
{
    std::string matName = mat->GetName().C_Str();
    if (matName.empty())
        matName = "Material_" + std::to_string(materialUUID.toUint64());

    fs::path matPath = outDir / (matName + ".pbrmat");

    // Create base material file if not present
    CreatePBRMaterial(matName, outDir);

    json j;
    {
        std::ifstream in(matPath);
        in >> j;
    }

    fs::path albedoPath;
    fs::path normalPath;
    fs::path metallicPath;
    fs::path roughnessPath;
    fs::path aoPath;

    fs::path modelRoot = modelPath.parent_path();
    fs::path texturesDir = modelRoot / "textures";

    if (!fs::exists(texturesDir))
    {
        fs::create_directories(texturesDir);
    }

    auto getTexPath = [&](aiTextureType type, fs::path& outPath)
        {
            aiString texPath;
            if (mat->GetTexture(type, 0, &texPath) == AI_SUCCESS)
            {
                // Embedded texture
                if (texPath.C_Str()[0] == '*')
                {
                    int index = atoi(texPath.C_Str() + 1);
                    const aiTexture* tex = scene->mTextures[index];

                    outPath = TextureImporter::ExtractEmbeddedTexture(
                        tex,
                        index,
                        texturesDir,   
                        matName
                    );
                    return;
                }

                // External texture
                outPath = fs::weakly_canonical(
                    modelRoot / texPath.C_Str()
                );
            }
        };
    getTexPath(aiTextureType_BASE_COLOR, albedoPath);
    getTexPath(aiTextureType_NORMALS, normalPath);
    getTexPath(aiTextureType_METALNESS, metallicPath);
    getTexPath(aiTextureType_DIFFUSE_ROUGHNESS, roughnessPath);
    getTexPath(aiTextureType_AMBIENT_OCCLUSION, aoPath);

    bool useARM = false;
    fs::path armPath;

    if (!metallicPath.empty() &&
        (metallicPath == roughnessPath || metallicPath == aoPath))
    {
        useARM = true;
        armPath = metallicPath;
    }
    else if (!roughnessPath.empty() && roughnessPath == aoPath)
    {
        useARM = true;
        armPath = roughnessPath;
    }

    if (!albedoPath.empty())
        j["textures"]["albedo"] = albedoPath.string();

    if (!normalPath.empty())
        j["textures"]["normal"] = normalPath.string();

    if (useARM)
    {
        j["textures"]["metallicRoughness"] = armPath.string();
    }
    else
    {
        if (!metallicPath.empty())
            j["textures"]["metallic"] = metallicPath.string();

        if (!roughnessPath.empty())
            j["textures"]["roughness"] = roughnessPath.string();

        if (!aoPath.empty())
            j["textures"]["ao"] = aoPath.string();
    }

    aiColor4D baseColor;
    if (aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS)
    {
        j["albedo"] = { baseColor.r, baseColor.g, baseColor.b };
    }

    float metallic = 0.0f;
    mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
    j["metallic"] = metallic;

    float roughness = 0.5f;
    mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
    j["roughness"] = roughness;

    {
        std::ofstream out(matPath);
        out << std::setw(4) << j;
    }


    fs::path metaPath = matPath;
    metaPath += ".meta";

    std::ofstream meta(metaPath);
    meta << "{\n";
    meta << "  \"fileID\": \"" << materialUUID.value() << "\",\n";
    meta << "  \"sourcePath\": \"" << matPath.generic_string() << "\",\n";
    meta << "  \"importType\": \"Material\"\n";
    meta << "}\n";
    meta.close();


    AssetMetadata materialMeta;
    materialMeta.uuid = materialUUID;
    materialMeta.name = matName;
    materialMeta.type = AssetType::Material;
    materialMeta.libraryPath = matPath;
    materialMeta.sourcePath = outDir;
    materialMeta.thumbnailPath = Paths::Icons + "material_icon.png";

    AssetDatabase::RegisterAsset(materialMeta);

}

fs::path TextureImporter::ExtractEmbeddedTexture(
    const aiTexture* tex,
    int index,
    const fs::path& texturesDir,
    const std::string& matName
)
{
    std::string baseName = matName + "_embedded_" + std::to_string(index);
    fs::path outPath;

    if (tex->mHeight == 0)
    {
        // Compressed (png/jpg already)
        std::string ext = tex->achFormatHint;
        if (ext.empty()) ext = "png";

        outPath = texturesDir / (baseName + "." + ext);

        std::ofstream out(outPath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(tex->pcData), tex->mWidth);
    }
    else
    {
        // Raw → convert to PNG
        outPath = texturesDir / (baseName + ".png");

        stbi_write_png(
            outPath.string().c_str(),
            tex->mWidth,
            tex->mHeight,
            4,
            tex->pcData,
            tex->mWidth * 4
        );
    }

    return fs::weakly_canonical(outPath);
}

void TextureImporter::Import(const fs::path& assetPath, UUID fileID)
{
    std::string name = assetPath.stem().string();
    auto ext = assetPath.extension();

    fs::path libPath =
        Paths::GameLibrary_Assets_Texture + (name + ext.string());

    // ---- CHECK ----
    if (fs::exists(libPath))
    {
        // Optional: log or early out
        std::cout << "Texture already exists in Library: "
            << libPath << "\n";

        return;
    }

    // ---- COPY ----
    fs::copy_file(
        assetPath,
        libPath,
        fs::copy_options::overwrite_existing // safe even with check
    );

    AssetMetadata meta;
    meta.uuid = fileID;
    meta.name = name;
    meta.type = AssetType::Texture;
    meta.libraryPath = libPath;
    meta.sourcePath = assetPath;
    meta.thumbnailPath = Paths::Icons + "texture_icon.png";

    AssetDatabase::RegisterAsset(meta);
}

static glm::mat4 ConvertMatrix(const aiMatrix4x4& m)
{
    return glm::transpose(glm::make_mat4(&m.a1));
}

void SkeletonImporter::ImportSkeleton(
    const aiScene* scene,
    const std::filesystem::path& assetPath,
    const std::filesystem::path& outDir,
    UUID skeletonID,
    LSkeletonFile& skeleton,
    std::unordered_map<std::string, int>& boneMap,
    std::string skeletonName

    )
{

    // PASS 1 — Collect bones from meshes
    for (uint32_t m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];

        for (uint32_t b = 0; b < mesh->mNumBones; b++)
        {
            aiBone* bone = mesh->mBones[b];
            std::string name = bone->mName.C_Str();

            if (boneMap.find(name) == boneMap.end())
            {
                int id = (int)skeleton.bones.size(); 
                boneMap[name] = id;

                LSkeletonBone newBone{};
                newBone.name = name;
                newBone.parentIndex = -1;
                newBone.inverseBindMatrix =
                    ConvertMatrix(bone->mOffsetMatrix);

                skeleton.bones.push_back(newBone);
            }
        }
    }

    // PASS 2 — Build hierarchy from nodes
    BuildBoneHierarchy(scene->mRootNode, -1, boneMap, skeleton);

    // Write .lskeleton

    std::string cleanName = SanitizeFilename(skeletonName);

    std::filesystem::path path = outDir / (cleanName + ".lskeleton" );


    WriteSkeleton(path, skeleton);

    AssetMetadata skeletonMeta;
    skeletonMeta.uuid = skeletonID;
    skeletonMeta.name = skeletonName;
    skeletonMeta.type = AssetType::Skeleton;
    skeletonMeta.libraryPath = path;
    skeletonMeta.sourcePath = assetPath;
    skeletonMeta.thumbnailPath = Paths::Icons + "mesh_icon.png";

    AssetDatabase::RegisterAsset(skeletonMeta);
}

void SkeletonImporter::BuildBoneHierarchy(
    const aiNode* node,
    int parentBone,
    std::unordered_map<std::string, int>& boneMap,
    LSkeletonFile& skeleton)
{
    std::string name = node->mName.C_Str();

    int currentBone = parentBone;

    auto it = boneMap.find(name);
    if (it != boneMap.end())
    {
        currentBone = it->second;
        skeleton.bones[currentBone].parentIndex = parentBone;
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        BuildBoneHierarchy(
            node->mChildren[i],
            currentBone,
            boneMap,
            skeleton
        );
    }
}

void AnimationImporter::ImportAnimations(
    const aiScene* scene,
    const std::filesystem::path& assetPath,
    const std::filesystem::path& outDir,
    const LSkeletonFile& skeleton,
    std::string skeletonName,
    std::unordered_map<std::string, int>& boneMap,
    std::vector<UUID>& animationIDs
)
{
    if (!scene->HasAnimations())
        return;

    for (uint32_t i = 0; i < scene->mNumAnimations; i++)
    {
        aiAnimation* aiAnim = scene->mAnimations[i];

        LAnimationFile animFile;

        animFile.animationID = UUID();
        animFile.skeletonID = skeleton.skeletonID;

        animationIDs.push_back(animFile.animationID);

        animFile.name = aiAnim->mName.length > 0 ?
            aiAnim->mName.C_Str() :
            "Animation_" + std::to_string(i);

        animFile.duration = (float)aiAnim->mDuration;
        animFile.ticksPerSecond = aiAnim->mTicksPerSecond != 0 ?
            (float)aiAnim->mTicksPerSecond : 25.0f;

        animFile.tracks.reserve(aiAnim->mNumChannels);
        animFile.boneTrackMap.resize(skeleton.bones.size(), -1);


        for (uint32_t c = 0; c < aiAnim->mNumChannels; c++)
        {
            aiNodeAnim* channel = aiAnim->mChannels[c];

            std::string boneName = channel->mNodeName.C_Str();

            // Ignore channels not in skeleton
            if (boneMap.find(boneName) == boneMap.end())
                continue;

            LAnimationTrack track;
            track.boneName = boneName;

            if (boneMap.find(track.boneName) != boneMap.end())
                track.boneIndex = boneMap[track.boneName];

            // POSITION KEYS
            for (uint32_t p = 0; p < channel->mNumPositionKeys; p++)
            {
                const aiVectorKey& key = channel->mPositionKeys[p];

                LKeyPosition pos;
                pos.position = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                pos.time = (float)key.mTime;

                track.positions.push_back(pos);
            }

            // ROTATION KEYS
            for (uint32_t r = 0; r < channel->mNumRotationKeys; r++)
            {
                const aiQuatKey& key = channel->mRotationKeys[r];

                LKeyRotation rot;
                rot.rotation = glm::quat(
                    key.mValue.w,
                    key.mValue.x,
                    key.mValue.y,
                    key.mValue.z
                );

                rot.time = (float)key.mTime;

                track.rotations.push_back(rot);
            }

            // SCALE KEYS
            for (uint32_t s = 0; s < channel->mNumScalingKeys; s++)
            {
                const aiVectorKey& key = channel->mScalingKeys[s];

                LKeyScale scale;
                scale.scale = glm::vec3(
                    key.mValue.x,
                    key.mValue.y,
                    key.mValue.z
                );

                scale.time = (float)key.mTime;

                track.scales.push_back(scale);
            }

            animFile.tracks.push_back(track);
            animFile.boneTrackMap[track.boneIndex] = (int)animFile.tracks.size() - 1;
        }

        // Output path
        std::string cleanName = SanitizeFilename(animFile.name);

        std::filesystem::path outPath =
            outDir / (skeletonName + "_" + std::to_string(animFile.animationID) + ".lanim");

        WriteLAnimation(outPath, animFile);


        AssetMetadata meta;
        meta.uuid = animFile.animationID;
        meta.name = animFile.name;
        meta.libraryPath = outPath;
        meta.sourcePath = assetPath;
        meta.thumbnailPath = Paths::Icons + "texture_icon.png";
        meta.type = AssetType::Animation;

        AssetDatabase::RegisterAsset(meta);
    }
}


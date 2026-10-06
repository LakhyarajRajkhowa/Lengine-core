#include "AssetManager.h"

using namespace Lengine;

void AssetManager::Init() {
    AssetDatabase::LoadDatabase();
    LoadAllDefaultAssets();
}

void AssetManager::LoadAllDefaultAssets() {

    for (auto id : SubmeshID::GetAllDefaults()) {
        LoadSubmesh(id);
    }

    for (auto id : MaterialID::GetAllDefaults()) {
        LoadMaterial(id);
    }

    for (auto shader : ShaderRegistry::GetAllDefaults()) {
        loadShader(shader.name, shader.vertexShaderPath, shader.fragmentShaderPath);
    }

    // for now load all particle emitter assets
    for (auto asset : AssetDatabase::GetAllAssets()) {
        if (asset.second.type == AssetType::ParticleEmitter) {
            LoadParticleEmitter(asset.second.uuid);
        }
    }
  
    for (auto asset : AssetDatabase::GetAllAssets()) {
        if (asset.second.type == AssetType::Texture) {
            texturePathToUUID[asset.second.sourcePath.string()] = asset.second.uuid;
        }
    }
}

void AssetManager::Update(Scene& activeScene) {
    UpdateAllAssetViews();
    SyncAssetsToScene(activeScene);
    ProcessGpuUploads();

    updateLoadingScreens();

    
}

void AssetManager::UpdateAllAssetViews()
{
    if (!AssetDatabase::needsUpdate)
        return;

    submeshViews.clear();
    pbrMaterialViews.clear();
    TextureViews.clear();
    PrefabViews.clear();
    SkeletonViews.clear();
    BoneMaskViews.clear();
    AnimationViews.clear();
    ParticleViews.clear();

    const auto& allAssets = AssetDatabase::GetAllAssets();

    for (const auto& [uuid, meta] : allAssets)
    {
        AssetView view;
        view.uuid = uuid;
        view.name = meta.name;
        view.type = meta.type;
        view.libraryPath = meta.libraryPath;
        view.thumbnailPath = meta.thumbnailPath;

        switch (meta.type)
        {
        case AssetType::Mesh:
            submeshViews.emplace_back(view);
            break;

        case AssetType::Material:
            pbrMaterialViews.emplace_back(view);
            break;

        case AssetType::Texture:
            TextureViews.emplace_back(view);
            break;
        case AssetType::Prefab:
            PrefabViews.emplace_back(view);
            break;
        case AssetType::Skeleton:
            SkeletonViews.emplace_back(view);
            break;
        case AssetType::BoneMask:
            BoneMaskViews.emplace_back(view);
            break;
        case AssetType::Animation:
            AnimationViews.emplace_back(view);
            break;
        case AssetType::ParticleEmitter:
            ParticleViews.emplace_back(view);
            break;

        default:
            break;
        }
    }

    AssetDatabase::needsUpdate = false;
}

void AssetManager::UpdateAssetName(const UUID id, const std::string name)
{
    if (auto* oldData = AssetDatabase::GetMetadata(id))
    {
        oldData->name = name;

        AssetDatabase::needsUpdate = true;
    }
}
//  ---- SUBMESH ---

void AssetManager::RequestSubmeshLoad(const UUID& meshID, const Entity& entityID)
{
    if (GetSubmesh(meshID)) {
        pendingSubmeshes.push({ entityID, meshID });
        assetStates[meshID] = AssetState::LoadedToGPU;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[meshID] = AssetState::Loading;
        pendingSubmeshes.push({ entityID, meshID });
    }

    std::thread([this, meshID]()
        {
            bool ok = LoadSubmesh(meshID);

            std::lock_guard<std::mutex> lock(assetMutex);
            assetStates[meshID] = ok
                ? AssetState::LoadedToCPU
                : AssetState::Failed;
        }).detach();
}


bool AssetManager::LoadSubmesh(const UUID& id) {
    auto sm = AssetDatabase::LoadAsset<Mesh>(id);

    if (sm)
        submeshes[id] = sm;
    else
        return false;

    return true;
}

bool AssetManager::processPendingSubmesh(const UUID& id) {

    auto& submesh = submeshes[id];
    if (submesh) {
        submesh->setupMesh();
        return true;
    }

    return false;
}

Mesh* AssetManager::GetSubmesh(const UUID& id) {
    auto it = submeshes.find(id);
    if (it == submeshes.end())
        return nullptr;

    return it->second.get();
}

// -------- SKELETON ----------

bool AssetManager::LoadSkeleton(const UUID& uuid) {
    auto skeleton = AssetDatabase::LoadAsset<Skeleton>(uuid);

    if (skeleton)
        skeletons[uuid] = skeleton;
    else
        return false;

    return true;
}

Skeleton* AssetManager::GetSkeleton(const UUID& id) {
    auto it = skeletons.find(id);
    if (it == skeletons.end())
        return nullptr;

    return it->second.get();
}


// -------- BONEMASK ---------

UUID AssetManager::CreateBoneMask(const std::string name) {
    return BoneMaskCreator::Create(name);
}

void AssetManager::SaveBoneMask(const UUID& id) {
    const BoneMask& bm = *GetBoneMask(id);
    const std::filesystem::path libPath = GetAssetMetaData(id)->libraryPath;

    BoneMaskSaver::Save(bm, libPath);
}

std::shared_ptr<BoneMask> AssetManager::GetBoneMask(const UUID& id)
{
    auto it = boneMasks.find(id);
    if (it == boneMasks.end())
        return nullptr;

    return it->second;
}

bool AssetManager::LoadBoneMask(const UUID& id) {

    if (GetBoneMask(id)) return true;

    auto bm = AssetDatabase::LoadAsset<BoneMask>(id);

    if (bm) {
        boneMasks[id] = bm;
        return true;

    }
    else 
        return false;
}


// -------- PARTICLE EMITTER ---------
UUID AssetManager::CreateParticleEmitter(const std::string name) {
    return ParticleEmitterCreator::Create(name);
}

void AssetManager::SaveParticleEmitter(const UUID& id) {
    const ParticleEmitterAsset& pe = *GetParticleEmitterAsset(id);
    const std::filesystem::path libPath = GetAssetMetaData(id)->libraryPath;

    ParticleEmitterSaver::Save(pe, libPath);
}

std::shared_ptr<ParticleEmitterAsset> AssetManager::GetParticleEmitterAsset(const UUID& id)
{
    auto it = particleEmitters.find(id);
    if (it == particleEmitters.end())
        return nullptr;

    return it->second;
}

bool AssetManager::LoadParticleEmitter(const UUID& id) {

    if (GetParticleEmitterAsset(id)) return true;

    auto pe = AssetDatabase::LoadAsset<ParticleEmitterAsset>(id);

    if (pe) {
        // load tex
        if (pe->textureID != UUID::Null) {
            RequestTextureLoad(pe->textureID, UUID::Null, TextureMapType::Albedo);
        }
        particleEmitters[id] = pe;
        return true;

    }
    else
        return false;
}

// -------- ANIMATION ----------

bool AssetManager::LoadAnimation(const UUID& uuid) {
    auto anim = AssetDatabase::LoadAsset<Animation>(uuid);

    if (anim)
        animations[uuid] = anim;
    else
        return false;

    return true;
}

Animation* AssetManager::GetAnimation(const UUID& id) {
    auto it = animations.find(id);

    if (it == animations.end())
        return nullptr;

    return it->second.get();
}


// -------- PBRMATERIAL ---------

void AssetManager::ImportMaterial(const std::string path) {
    const UUID& uuid = UUID();
    AssetImporter::ImportMaterialFile(path, uuid);
}

UUID AssetManager::CreateMaterial(const std::string name) {
    return MaterialCreator::Create(name);
}

void AssetManager::SaveMaterial(const UUID& id) {
    const Material& mat = *GetMaterial(id);
    const std::filesystem::path libPath = GetAssetMetaData(id)->libraryPath;

    MaterialSaver::Save(mat, libPath);
}

bool AssetManager::LoadMaterial(const UUID& matID) {

    if (GetMaterial(matID)) return true;

    auto mat = AssetDatabase::LoadAsset<Material>(matID);

    if (mat) {

        mat->id = matID;
        pbrMaterials[matID] = mat;

        // albedo
        if (!mat->map_albedo_path.empty()) {

            UUID texID = GetOrCreateTextureUUID(mat->map_albedo_path);

            if (haveAssetState(texID))
            {
                mat->map_albedo = texID;
            }
            else
            {
                ImportTexture(mat->map_albedo_path, texID);

                std::lock_guard<std::mutex> lock(assetMutex);
                pendingTextureRequests.push({
                    texID,
                    matID,
                    NullEntity,
                    TextureMapType::Albedo,
                    TextureTargetType::GlobalMaterial,
                    true
                    });
            }
        }

        // normal
        if (!mat->map_normal_path.empty()) {
            UUID texID = GetOrCreateTextureUUID(mat->map_normal_path);

            if (haveAssetState(texID))
            {
                mat->map_normal = texID;
            }
            else
            {
                ImportTexture(mat->map_normal_path, texID);

                std::lock_guard<std::mutex> lock(assetMutex);
                pendingTextureRequests.push({
                    texID,
                    matID,
                    NullEntity,
                    TextureMapType::Normal,
                    TextureTargetType::GlobalMaterial,
                    false
                    });
            }

        }

        // metallic
        if (!mat->map_metallic_path.empty()) {
            UUID texID = GetOrCreateTextureUUID(mat->map_metallic_path);

            if (haveAssetState(texID))
            {
                mat->map_metallic = texID;
            }
            else
            {
                ImportTexture(mat->map_metallic_path, texID);

                std::lock_guard<std::mutex> lock(assetMutex);
                pendingTextureRequests.push({
                    texID,
                    matID,
                    NullEntity,
                    TextureMapType::Metallic,
                    TextureTargetType::GlobalMaterial,
                    false
                    });
            }
        }

        // roughness
        if (!mat->map_roughness_path.empty()) {

            UUID texID = GetOrCreateTextureUUID(mat->map_roughness_path);

            if (haveAssetState(texID))
            {
                mat->map_roughness = texID;
            }
            else
            {
                ImportTexture(mat->map_roughness_path, texID);

                std::lock_guard<std::mutex> lock(assetMutex);
                pendingTextureRequests.push({
                    texID,
                    matID,
                    NullEntity,
                    TextureMapType::Roughness,
                    TextureTargetType::GlobalMaterial,
                    false
                    });
            }
        }

        // ambient occulision
        if (!mat->map_ao_path.empty()) {
            UUID texID = GetOrCreateTextureUUID(mat->map_ao_path);

            if (haveAssetState(texID))
            {
                mat->map_ao = texID;
            }
            else
            {
                ImportTexture(mat->map_ao_path, texID);

                std::lock_guard<std::mutex> lock(assetMutex);
                pendingTextureRequests.push({
                    texID,
                    matID,
                    NullEntity,
                    TextureMapType::AmbientOcclusion,
                    TextureTargetType::GlobalMaterial,
                    false
                    });
            }
        }

        // arm
        if (!mat->map_metallicRoughness_path.empty()) {
            UUID texID = GetOrCreateTextureUUID(mat->map_metallicRoughness_path);

            if (haveAssetState(texID))
            {
                mat->map_metallicRoughness = texID;
            }
            else
            {
                ImportTexture(mat->map_metallicRoughness_path, texID);

                std::lock_guard<std::mutex> lock(assetMutex);
                pendingTextureRequests.push({
                    texID,
                    matID,
                    NullEntity,
                    TextureMapType::MetallicRoughness,
                    TextureTargetType::GlobalMaterial,
                    false
                    });
            }
        }

        return true;

    }

    else
        return false;
}


const AssetMetadata* AssetManager::GetAssetMetaData(const UUID& uuid) const
{
    const auto& allAssets = AssetDatabase::GetAllAssets();

    auto it = allAssets.find(uuid);
    if (it == allAssets.end())
        return nullptr;

    return &it->second;
}


//    ----- MESH ----
void AssetManager::ImportPrefab(const std::string& path) {
    UUID assetID = UUID();
    {
        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[assetID] = AssetState::Importing;
        currentImportingAsset.second = path;
    }

    std::thread([this, path, assetID]() {

        bool ok = AssetImporter::ImportPrefabFile(path, assetID);

        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[assetID] = ok ? AssetState::Imported
            : AssetState::Failed;

        }).detach();
}

bool AssetManager::LoadPrefabToScene(const std::string& path) {

    PrefabData data = LoadPrefabFile(path);

    pendingPrefabs.push(data);

    return true;
}


Entity AssetManager::InstantiatePrefab(
    Scene& scene,
    const PrefabData& prefab
)
{
    Registry& registry = scene.GetRegistry();

    std::vector<Entity> entities(prefab.nodes.size());

    // ---- Create entities ----
    for (const auto& node : prefab.nodes)
    {
        Entity e = scene.createEntity_root(node.name);
        entities[node.index] = e;

        auto& t = registry.transforms.Add(e);

        TransformSystem::DecomposeMatrix(
            node.localTransform,
            t.localPosition,
            t.localRotation,
            t.localScale
        );

        t.localDirty = true;
        t.worldDirty = true;

        if (node.meshID != UUID::Null)
        {
            if (!registry.meshFilters.Has(e))
                registry.meshFilters.Add(e);

            RequestSubmeshLoad(node.meshID, e);

            UUID matID = node.materialID != UUID::Null
                ? node.materialID
                : MaterialID::DefaultPbr;

            auto& mr = registry.meshRenderers.Has(e)
                ? registry.meshRenderers.Get(e)
                : registry.meshRenderers.Add(e);

            if (node.materialID != UUID::Null && LoadMaterial(node.materialID))
            {
                mr.inst.baseMaterial = node.materialID;
                mr.inst.dirty = true;
            }
            else
            {
                mr.inst.baseMaterial = MaterialID::DefaultPbr;
            }
        }
    }

    // ---- Hierarchy ----
    for (const auto& node : prefab.nodes)
    {
        if (node.parentIndex != -1)
            scene.SetParent(entities[node.index], entities[node.parentIndex]);
    }

    // ---- Root parent on mesh filters ----
    for (auto& e : entities)
    {
        if (registry.meshFilters.Has(e))
            registry.meshFilters.Get(e).rootParent = entities[0];
    }

    // -------- SKELETON (root) --------
    if (prefab.skeletonID != UUID::Null)
    {
        if (!registry.skeletons.Has(entities[0]))
        {
            auto& sk = registry.skeletons.Add(entities[0]);
            sk.skeletonID = prefab.skeletonID;

            if (!GetSkeleton(sk.skeletonID) && !LoadSkeleton(sk.skeletonID))
                sk.skeletonID = UUID::Null;
            else {
                auto* skeleton = GetSkeleton(sk.skeletonID);
                if(skeleton) skeleton->BuildBoneNodeHierarchy();
            }
        }
    }

    // -------- ANIMATION (root) --------
    if (!prefab.animationIDs.empty())
    {
        if (!registry.animations.Has(entities[0]))
        {
            auto& anim = registry.animations.Add(entities[0]);
            anim.animationIDs = prefab.animationIDs;

            for (auto& animID : anim.animationIDs)
            {
                // Skip null sentinel
                if (animID == UUID::Null)
                    continue;

                bool loaded = GetAnimation(animID) != nullptr;
                if (!loaded)
                    loaded = LoadAnimation(animID);

                if (loaded)
                {
                    Animation* clip = GetAnimation(animID);
                    if (clip)
                    {
                        anim.animationNames[animID] = clip->name;
                        anim.animationNameToID[clip->name] = animID;
                    }
                }
            }

            // ---- T-Pose / bind pose init ----
            if (registry.skeletons.Has(entities[0]) && prefab.tposeAnimationID != UUID::Null)
            {
                const UUID& skelID = registry.skeletons.Get(entities[0]).skeletonID;
                Skeleton* skeleton = GetSkeleton(skelID);

                // Load T-pose clip if not already cached
                Animation* tposeClip = GetAnimation(prefab.tposeAnimationID);
                if (!tposeClip && LoadAnimation(prefab.tposeAnimationID))
                    tposeClip = GetAnimation(prefab.tposeAnimationID);

                if (skeleton && tposeClip)
                {
                    anim.tposeAnimationID = prefab.tposeAnimationID;

                    anim.finalBoneMatrices.resize(skeleton->bones.size(), glm::mat4(1.0f));
                    anim.globalBoneTransforms.resize(skeleton->bones.size(), glm::mat4(1.0f));
                    anim.tPose.Resize(skeleton->bones.size());

                    std::vector<glm::mat4> globalTransforms(skeleton->bones.size());

                    for (size_t i = 0; i < skeleton->bones.size(); i++)
                    {
                        int       trackIndex = tposeClip->boneTrackMap[i];
                        glm::mat4 localTransform(1.0f);


                        glm::vec3 pos = glm::vec3(0);
                        glm::quat rot = glm::quat(1, 0, 0, 0);
                        glm::vec3 scale = glm::vec3(1);


                        if (trackIndex != -1)
                        {
                            auto& track = tposeClip->tracks[trackIndex];

                             pos = track.positions.empty() ? glm::vec3(0) : track.positions[0].position;
                             rot = track.rotations.empty() ? glm::quat(1, 0, 0, 0) : track.rotations[0].rotation;
                             scale = track.scales.empty() ? glm::vec3(1) : track.scales[0].scale;

                            localTransform = glm::translate(glm::mat4(1.0f), pos)
                                * glm::mat4_cast(rot)
                                * glm::scale(glm::mat4(1.0f), scale);
                        }

                        int parent = skeleton->bones[i].parentIndex;

                        globalTransforms[i] = (parent == -1)
                            ? localTransform
                            : globalTransforms[parent] * localTransform;

                        // skinning matrix, used by the GPU skinning path
                        anim.finalBoneMatrices[i] = globalTransforms[i]
                            * skeleton->bones[i].inverseBindMatrix;


                        anim.globalBoneTransforms[i] = globalTransforms[i];

                        anim.tPose.bones[i] = { pos, rot, scale };


                    }
                }

            }
        }
    
    }

    return entities[0];
}


void AssetManager::ImportMesh(const std::string& path) {

    UUID assetID = UUID();

    {
        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[assetID] = AssetState::Importing;
        currentImportingAsset.second = path;
    }

    std::thread([this, path, assetID]() {

        bool ok = AssetImporter::ImportMeshFile(path, assetID);

        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[assetID] = ok ? AssetState::Imported
            : AssetState::Failed;

        }).detach();
}


Material* AssetManager::GetMaterial(const UUID& id)
{
    auto it = pbrMaterials.find(id);
    if (it == pbrMaterials.end())
        return nullptr;

    return it->second.get();
}


//          ----- TEXTURES -----

void AssetManager::ImportTexture(const std::string& path, const UUID& assetID) {

    {
        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[assetID] = AssetState::Importing;
        currentImportingAsset.second = path;
    }

    std::thread([this, path, assetID]() {

        bool ok = AssetImporter::ImportTextureFile(path, assetID);

        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[assetID] = ok ? AssetState::Imported
            : AssetState::Failed;

        }).detach();
}

UUID AssetManager::GetOrCreateTextureUUID(const std::string& path)
{
    std::lock_guard<std::mutex> lock(assetMutex);

    auto it = texturePathToUUID.find(path);
    if (it != texturePathToUUID.end())
        return it->second;

    UUID newID = UUID();
    texturePathToUUID[path] = newID;
    return newID;
}

// this one for only Global Material Asset
void AssetManager::RequestTextureLoad(
    const UUID& texID,
    const UUID& matID,
    const TextureMapType& mapType,
    bool srgb
)
{
    if (getTexture(texID)) {
        pendingTextureRequests.push({
           texID,
           matID,
           NullEntity,
           mapType,
           TextureTargetType::GlobalMaterial
            });
        assetStates[texID] = AssetState::LoadedToGPU;
        return;
    }

    auto tex = std::make_shared<GLTexture>();
    tex->srgb = srgb;

    {
        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[texID] = AssetState::Loading;

        pendingTextureRequests.push({
            texID,
            matID,
            NullEntity,
            mapType,
            TextureTargetType::GlobalMaterial
            });
    }

    std::thread([this, texID, tex]()
        {
            auto img = LoadTexture(texID);
            if (!img) return;

            tex->imageCPU = std::move(img);
            tex->pendingGPUUpload = true;
            textures[texID] = tex;

            std::lock_guard<std::mutex> lock(assetMutex);
            assetStates[texID] = AssetState::LoadedToCPU;
        }).detach();
}


// this one for inst material per meshrenderer
void AssetManager::RequestTextureLoad_inst(
    const UUID& texID,
    const Entity& entityID,
    const TextureMapType& mapType,
    bool srgb
)
{
    if (getTexture(texID)) {

        pendingTextureRequests.push({
            texID,
            UUID::Null,
            entityID,
            mapType,
            TextureTargetType::MeshRendererInstance
            });
        assetStates[texID] = AssetState::LoadedToGPU;
        return;
    }

    auto tex = std::make_shared<GLTexture>();
    tex->srgb = srgb;

    {
        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[texID] = AssetState::Loading;

        pendingTextureRequests.push({
            texID,
            UUID::Null,
            entityID,
            mapType,
            TextureTargetType::MeshRendererInstance
            });
    }

    std::thread([this, texID, tex]()
        {
            auto img = LoadTexture(texID);
            if (!img) {
                textures.erase(texID);
                return;
            }

            tex->imageCPU = std::move(img);
            tex->pendingGPUUpload = true;
            textures[texID] = tex;

            std::lock_guard<std::mutex> lock(assetMutex);
            assetStates[texID] = AssetState::LoadedToCPU;
        }).detach();
}


std::shared_ptr<ImageData> AssetManager::LoadTexture(const UUID& id) {
    std::shared_ptr<ImageData> img = AssetDatabase::LoadAsset<ImageData>(id);

    if (!img) {
        std::lock_guard<std::mutex> lock(assetMutex);

        assetStates[id] = AssetState::Failed;
        return nullptr;
    }

    return img;
}


GLTexture* AssetManager::loadImage(const std::string& name, const std::string& path) {

    MetaFile meta;

    std::string fileName = ExtractFileNameFromPath(path);
    if (!MetaFileSystem::HasMeta(path)) {
        meta.uuid = UUID();
        meta.type = "texture";
        meta.source = NormalizePath(Paths::Shaders + fileName);
        MetaFileSystem::Save(name, meta);
    }
    else {
        meta = MetaFileSystem::Load(path);
    }

    UUID id = meta.uuid;
    std::shared_ptr<GLTexture> tex = std::make_shared<GLTexture>();
    *tex = textureCache.getTexture(path, false); // sRGB = false
    textures[id] = tex;
    return tex.get();
}

bool AssetManager::processPendingTextures(const UUID& uuid)
{
    GLTexture* tex = getTexture(uuid);
    if (!tex || !tex->pendingGPUUpload || !tex->imageCPU)
        return false;

    GLenum format;
    switch (tex->imageCPU->channels)
    {
    case 1: format = GL_RED;  break;
    case 3: format = GL_RGB;  break;
    case 4: format = GL_RGBA; break;
    default:
        assetStates[uuid] = AssetState::Failed;
        return false;
    }

    GLenum internalFormat;
    if (tex->srgb)
    {
        if (format == GL_RGBA) internalFormat = GL_SRGB8_ALPHA8;
        else if (format == GL_RGB) internalFormat = GL_SRGB8;
        else internalFormat = GL_R8;
    }
    else
    {
        if (format == GL_RGBA) internalFormat = GL_RGBA8;
        else if (format == GL_RGB) internalFormat = GL_RGB8;
        else internalFormat = GL_R8;
    }

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    glTexImage2D(
        GL_TEXTURE_2D, 0, internalFormat,
        tex->imageCPU->width, tex->imageCPU->height,
        0, format, GL_UNSIGNED_BYTE,
        tex->imageCPU->pixels.data()
    );

    if (glGetError() != GL_NO_ERROR)
    {
        glDeleteTextures(1, &tex->id);
        tex->id = 0;
        assetStates[uuid] = AssetState::Failed;
        return false;
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    tex->imageCPU.reset();
    tex->pendingGPUUpload = false;

    return true;
}


GLTexture* AssetManager::getTexture(const UUID& id) {
    auto it = textures.find(id);
    return it != textures.end() ? it->second.get() : nullptr;
}


//          ----- SHADERS -----
GLSLProgram* AssetManager::loadShader(const std::string& name,
    const std::string& vert,
    const std::string& frag)
{
    if (shaders.count(name)) return shaders[name].get();

    auto shader = std::make_unique<GLSLProgram>();
    shader->compileShaders(vert.c_str(), frag.c_str());
    shader->linkShaders();

    shaders[name] = std::move(shader);

    return shaders[name].get();
}

std::shared_ptr<GLSLProgram> AssetManager::getShader(const std::string& name)
{
    auto it = shaders.find(name);

    if (it == shaders.end())
        return nullptr;

    return it->second;
}


//          ----- SCENE ASSET REGISTRY -----

void AssetManager::SaveSceneAssetRegistryForScene(const Scene& scene, const std::string& folderPath)
{
    namespace fs = std::filesystem;

    fs::path dir(folderPath);
    if (!fs::exists(dir))
        fs::create_directories(dir);

    std::string sceneName = scene.getName();
    fs::path finalPath = dir / (sceneName + "_assets.json");

    std::unordered_set<UUID> usedAssets;

    const Registry& registry = scene.GetRegistry();
    const auto& entities = scene.getEntities();

    // ---- Collect every asset UUID directly referenced by entities ----
    for (const auto& entityPtr : entities)
    {
        const Entity entityID = entityPtr;

        if (registry.meshFilters.Has(entityID))
        {
            const MeshFilter& mf = registry.meshFilters.Get(entityID);
            if (mf.meshID != UUID::Null)
                usedAssets.insert(mf.meshID);
        }

        if (registry.meshRenderers.Has(entityID))
        {
            const MeshRenderer& mr = registry.meshRenderers.Get(entityID);
            const MaterialInstance& inst = mr.inst;

            if (inst.baseMaterial != UUID::Null)          usedAssets.insert(inst.baseMaterial);
            if (inst.map_albedo.has_value())             usedAssets.insert(inst.map_albedo.value());
            if (inst.map_normal.has_value())             usedAssets.insert(inst.map_normal.value());
            if (inst.map_metallic.has_value())           usedAssets.insert(inst.map_metallic.value());
            if (inst.map_roughness.has_value())          usedAssets.insert(inst.map_roughness.value());
            if (inst.map_ao.has_value())                 usedAssets.insert(inst.map_ao.value());
            if (inst.map_metallicRoughness.has_value())  usedAssets.insert(inst.map_metallicRoughness.value());
        }

        if (registry.skeletons.Has(entityID))
        {
            const SkeletonComponent& s = registry.skeletons.Get(entityID);
            if (s.skeletonID != UUID::Null)
                usedAssets.insert(s.skeletonID);
        }

        if (registry.animations.Has(entityID))
        {
            const auto& a = registry.animations.Get(entityID);

            for (const auto& animID : a.animationIDs)
                if (animID != UUID::Null)
                    usedAssets.insert(animID);

            if (a.tposeAnimationID != UUID::Null)
                usedAssets.insert(a.tposeAnimationID);
        }
    }

    // ---- Pull in texture deps owned by the *material asset* itself ----
    // (an entity might only reference baseMaterial, with the material's own
    // texture maps not overridden per-instance, so they won't show up above)
    std::vector<UUID> materialDerivedAssets;
    for (const UUID& assetID : usedAssets)
    {
        Material* mat = GetMaterial(assetID);
        if (!mat) continue;

        if (mat->map_albedo != UUID::Null)            materialDerivedAssets.push_back(mat->map_albedo);
        if (mat->map_normal != UUID::Null)            materialDerivedAssets.push_back(mat->map_normal);
        if (mat->map_metallic != UUID::Null)          materialDerivedAssets.push_back(mat->map_metallic);
        if (mat->map_roughness != UUID::Null)         materialDerivedAssets.push_back(mat->map_roughness);
        if (mat->map_ao != UUID::Null)                materialDerivedAssets.push_back(mat->map_ao);
        if (mat->map_metallicRoughness != UUID::Null) materialDerivedAssets.push_back(mat->map_metallicRoughness);
    }
    for (const UUID& id : materialDerivedAssets)
        usedAssets.insert(id);

    // ---- Write registry JSON ----
    json jRegistry;
    jRegistry["scene"] = sceneName;
    jRegistry["assets"] = json::array();

    for (const UUID& assetID : usedAssets)
    {
        const AssetMetadata* meta = GetAssetMetaData(assetID);

        json jAsset;
        jAsset["uuid"] = assetID.toUint64();
        jAsset["type"] = meta ? AssetTypeToString(meta->type) : AssetTypeToString(AssetType::Unknown);
        jAsset["name"] = meta ? meta->name : std::string("<unknown>");
        jAsset["libraryPath"] = meta ? meta->libraryPath.string() : std::string();

        jRegistry["assets"].push_back(jAsset);
    }

    std::ofstream file(finalPath);
    file << jRegistry.dump(4);

    std::cout << "Saved asset registry for scene \"" << sceneName
        << "\" at: " << finalPath
        << " (" << usedAssets.size() << " assets)" << std::endl;
}

bool AssetManager::LoadSceneAssetRegistry(const std::string& filePath)
{
    namespace fs = std::filesystem;

    fs::path path(filePath);

    if (!fs::exists(path))
    {
        std::cerr << "Scene asset registry does not exist: " << filePath << "\n";
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open scene asset registry: " << filePath << "\n";
        return false;
    }

    json jRegistry;

    try
    {
        file >> jRegistry;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Invalid scene asset registry JSON: " << e.what() << "\n";
        return false;
    }

    file.close();

    if (!jRegistry.contains("assets"))
    {
        std::cerr << "Scene asset registry missing \"assets\" array: " << filePath << "\n";
        return false;
    }

    std::string sceneName = jRegistry.value("scene", std::string("<unknown>"));

    size_t loadedCount = 0;
    size_t failedCount = 0;

    for (const auto& jAsset : jRegistry["assets"])
    {
        UUID assetID;
        AssetType type;

        try
        {
            assetID = UUID(jAsset.at("uuid").get<uint64_t>());
            type = StringToAssetType(jAsset.at("type"));
        }
        catch (const json::exception& e)
        {
            std::cerr << "Skipping malformed asset entry in registry \"" << sceneName
                << "\": " << e.what() << std::endl;
            failedCount++;
            continue;
        }

        bool ok = false;

        switch (type)
        {
        case AssetType::Mesh:
            ok = GetSubmesh(assetID) != nullptr || LoadSubmesh(assetID);
            break;

        case AssetType::Material:
            // LoadMaterial also queues up its texture map dependencies
            // via pendingTextureRequests, so this cascades.
            ok = GetMaterial(assetID) != nullptr || LoadMaterial(assetID);
            break;

        case AssetType::Skeleton:
        {
            ok = GetSkeleton(assetID) != nullptr || LoadSkeleton(assetID);

            if (Skeleton* skeleton = GetSkeleton(assetID))
            {
                skeleton->BuildBoneNodeHierarchy();
            }

            break;
        }

        case AssetType::BoneMask:
            ok = GetBoneMask(assetID) != nullptr || LoadBoneMask(assetID);
            break;

        case AssetType::Texture:
            // Orphan textures (not pulled in via a material) are loaded
            // directly to CPU here; they'll pick up a GPU upload on the
            // next ProcessGpuUploads() pass once flagged LoadedToCPU.
            if (getTexture(assetID))
            {
                ok = true;
            }
            else
            {
                auto img = LoadTexture(assetID);
                if (img)
                {
                    auto tex = std::make_shared<GLTexture>();
                    tex->imageCPU = std::move(img);
                    tex->pendingGPUUpload = true;

                    {
                        std::lock_guard<std::mutex> lock(assetMutex);
                        textures[assetID] = tex;
                        assetStates[assetID] = AssetState::LoadedToCPU;
                    }

                    ok = true;
                }
            }
            break;

        case AssetType::Animation:
            ok = GetAnimation(assetID) != nullptr || LoadAnimation(assetID);
            break;

        }

        if (ok)
            loadedCount++;
        else
            failedCount++;
    }

    std::cout << "Loaded scene asset registry \"" << sceneName << "\" from: "
        << filePath << " (" << loadedCount << " loaded, "
        << failedCount << " failed)" << std::endl;

    return failedCount == 0;
}

// ASSET DATABASE

void AssetManager::LoadAssetDatabase() {
    AssetDatabase::LoadDatabase();
}

void AssetManager::SaveAssetDatabase() {
    AssetDatabase::SaveDatabase();
}


//          ----- SCENE -----

std::unique_ptr<Scene> AssetManager::createScene(const std::string& name, const std::string& folderPath)
{
    namespace fs = std::filesystem;

    UUID sID;
    std::unique_ptr<Scene> scene = std::make_unique<Scene>(name, sID);

    fs::path dir(folderPath);

    if (!fs::exists(dir))
        fs::create_directories(dir);

    std::string fileName = name + ".json";
    fs::path finalPath = dir / fileName;

    json jScene;

    jScene["uuid"] = scene->getUUID().toUint64();
    jScene["name"] = name;
    jScene["entities"] = json::array();

    std::ofstream file(finalPath);
    file << jScene.dump(4);

    std::cout << "Scene \"" << name << "\" created successfully at: "
        << finalPath.string() << std::endl;

    return scene;
}
void AssetManager::saveScene(const Scene& scene, const std::string& folderPath)
{
    namespace fs = std::filesystem;

    fs::path dir(folderPath);
    if (!fs::exists(dir))
        fs::create_directories(dir);

    std::string sceneName = scene.getName();
    fs::path finalPath = dir / (sceneName + ".json");

    json jScene;
    jScene["uuid"] = scene.getUUID().toUint64();
    jScene["name"] = sceneName;
    jScene["primaryCamera"] = scene.GetPrimaryCamera();
    jScene["entities"] = json::array();

    const Registry& registry = scene.GetRegistry();
    const auto& entities = scene.getEntities();

    for (const auto& entityPtr : entities)
    {
        const Entity entityID = entityPtr;

        std::string entityName = "Unknown";
        if (registry.nameTags.Has(entityID)) {
           entityName = registry.nameTags.Get(entityID).name;
        }

        json jEntity;
        jEntity["entityID"] = entityID;
        jEntity["name"] = entityName;

        if (registry.transforms.Has(entityID)) {
            const TransformComponent& entityTransform = registry.transforms.Get(entityID);

            jEntity["transform"] = {
                { "position", { entityTransform.localPosition.x, entityTransform.localPosition.y, entityTransform.localPosition.z } },
                { "rotation", { entityTransform.localRotation.x, entityTransform.localRotation.y, entityTransform.localRotation.z, entityTransform.localRotation.w } },
                { "scale",    { entityTransform.localScale.x,    entityTransform.localScale.y,    entityTransform.localScale.z } }
            };
        }

        if (registry.meshFilters.Has(entityID)) {
            const MeshFilter& mf = registry.meshFilters.Get(entityID);

            json jMeshFilter;
            jMeshFilter["meshID"] = mf.meshID.toUint64();
            jMeshFilter["rootParent"] = mf.rootParent;
            jEntity["meshFilter"] = jMeshFilter;
        }

        if (registry.meshRenderers.Has(entityID))
        {
            const MeshRenderer& mr = registry.meshRenderers.Get(entityID);
            json jMeshRenderer;
            jMeshRenderer["materialID"] = mr.inst.baseMaterial.toUint64();

            // --- Serialize material instance overrides ---
            json jOverrides;

            if (mr.inst.albedo)
                jOverrides["albedo"] = { mr.inst.albedo->x, mr.inst.albedo->y, mr.inst.albedo->z };

            if (mr.inst.metallic)
                jOverrides["metallic"] = *mr.inst.metallic;

            if (mr.inst.roughness)
                jOverrides["roughness"] = *mr.inst.roughness;

            if (mr.inst.ao)
                jOverrides["ao"] = *mr.inst.ao;

            if (mr.inst.opacity)
                jOverrides["opacity"] = *mr.inst.opacity;

            if (mr.inst.normalStrength)
                jOverrides["normalStrength"] = *mr.inst.normalStrength;

            if (mr.inst.map_albedo)
                jOverrides["map_albedo"] = mr.inst.map_albedo->toUint64();

            if (mr.inst.map_normal)
                jOverrides["map_normal"] = mr.inst.map_normal->toUint64();

            if (mr.inst.map_metallic)
                jOverrides["map_metallic"] = mr.inst.map_metallic->toUint64();

            if (mr.inst.map_roughness)
                jOverrides["map_roughness"] = mr.inst.map_roughness->toUint64();

            if (mr.inst.map_ao)
                jOverrides["map_ao"] = mr.inst.map_ao->toUint64();

            if (mr.inst.map_metallicRoughness)
                jOverrides["map_metallicRoughness"] = mr.inst.map_metallicRoughness->toUint64();

            // use_map_* toggles (always written - they're plain bools, not optional)
            jOverrides["use_map_albedo"] = mr.inst.use_map_albedo;
            jOverrides["use_map_normal"] = mr.inst.use_map_normal;
            jOverrides["use_map_metallic"] = mr.inst.use_map_metallic;
            jOverrides["use_map_roughness"] = mr.inst.use_map_roughness;
            jOverrides["use_map_ao"] = mr.inst.use_map_ao;
            jOverrides["use_map_metallicRoughness"] = mr.inst.use_map_metallicRoughness;

            jMeshRenderer["materialOverrides"] = jOverrides;

            jMeshRenderer["castShadows"] = mr.castShadows;
            jMeshRenderer["receiveShadows"] = mr.receiveShadows;
            jMeshRenderer["render"] = mr.render;

            jEntity["meshRenderer"] = jMeshRenderer;
        }

        if (registry.lights.Has(entityID)) {
            const Light& l = registry.lights.Get(entityID);

            json jLight;
            jLight["type"] = static_cast<int>(l.type);
            jLight["color"] = { l.color.x, l.color.y, l.color.z };
            jLight["intensity"] = l.intensity;
            jLight["range"] = l.range;
            jLight["innerAngle"] = l.innerAngle;
            jLight["outerAngle"] = l.outerAngle;
            jLight["castShadow"] = l.castShadow;
            jEntity["light"] = jLight;
        }

        if (registry.cameras.Has(entityID))
        {
            const CameraComponent& c = registry.cameras.Get(entityID);

            json jCam;
            jCam["projectionType"] = static_cast<int>(c.projectionType);
            jCam["fov"] = c.fov;
            jCam["nearClip"] = c.nearClip;
            jCam["farClip"] = c.farClip;
            jCam["aspectRatio"] = c.aspectRatio;
            jCam["orthoSize"] = c.orthoSize;
            jCam["target"] = c.target;
            jEntity["camera"] = jCam;
        }

        if (registry.skeletons.Has(entityID))
        {
            const SkeletonComponent& s = registry.skeletons.Get(entityID);

            json jSkeleton;

            jSkeleton["skeletonID"] = s.skeletonID.toUint64();

            auto SaveMat4Array = [](const std::vector<glm::mat4>& mats) {
                json arr = json::array();

                for (const auto& m : mats) {
                    json mat = json::array();
                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            mat.push_back(m[i][j]);

                    arr.push_back(mat);
                }
                return arr;
                };

            jSkeleton["localPose"] = SaveMat4Array(s.localPose);
            jSkeleton["globalPose"] = SaveMat4Array(s.globalPose);
            jSkeleton["finalMatrices"] = SaveMat4Array(s.finalMatrices);
            jEntity["skeleton"] = jSkeleton;
        }

        // ---- Bone Attachment ----
        if (registry.boneAttachments.Has(entityID))
        {
            const BoneAttachmentComponent& b =
                registry.boneAttachments.Get(entityID);

            json jBone;

            jBone["modelRoot"] = b.modelRoot;
            jBone["skinnedRoot"] = b.skinnedRoot;
            jBone["boneIndex"] = b.boneIndex;
            jBone["boneName"] = b.boneName;

            // Offset Position
            jBone["offsetPosition"] =
            {
                b.offset.position.x,
                b.offset.position.y,
                b.offset.position.z
            };

            // Offset Rotation (Quaternion)
            jBone["offsetRotation"] =
            {
                b.offset.rotation.x,
                b.offset.rotation.y,
                b.offset.rotation.z,
                b.offset.rotation.w
            };

            // Offset Scale
            jBone["offsetScale"] =
            {
                b.offset.scale.x,
                b.offset.scale.y,
                b.offset.scale.z
            };

            jEntity["boneAttachment"] = jBone;
        }

        if (registry.animations.Has(entityID))
        {
            const auto& a =
                registry.animations.Get(entityID);

            json jAnim;


            json animIDs = json::array();

            for (const auto& id : a.animationIDs)
                animIDs.push_back(id.toUint64());

            jAnim["animationIDs"] = animIDs;

            jAnim["tposeAnimationID"] =
                a.tposeAnimationID.toUint64();


            json animNames;

            for (const auto& [id, name] : a.animationNames)
            {
                animNames[
                    std::to_string(id.toUint64())
                ] = name;
            }

            jAnim["animationNames"] = animNames;


            jAnim["controller"] =
                AnimatorSerializer::Save(a.animator);

            jEntity["animation"] =
                jAnim;
        }

        if (registry.hierarchies.Has(entityID))
        {
            const auto& h = registry.hierarchies.Get(entityID);

            json jH;
            jH["parent"] = h.parent;

            json children = json::array();
            for (auto& c : h.children)
                children.push_back(c);

            jH["children"] = children;
            jEntity["hierarchy"] = jH;
        }

        if (registry.colliders.Has(entityID))
        {
            const ColliderComponent& c = registry.colliders.Get(entityID);

            json jCol;
            json jShapes = json::array();

            for (const auto& s : c.shapes)
            {
                json jShape;
                jShape["type"] = static_cast<int>(s.type);
                jShape["isTrigger"] = s.isTrigger;

                switch (s.type)
                {
                case ColliderShape::Type::Box:
                    jShape["size"] = { s.size.x, s.size.y, s.size.z };
                    break;

                case ColliderShape::Type::Sphere:
                    jShape["radius"] = s.radius;
                    break;

                case ColliderShape::Type::Capsule:
                    jShape["radius"] = s.radius;
                    jShape["height"] = s.height;
                    break;
                }

                jShapes.push_back(jShape);
            }

            jCol["shapes"] = jShapes;
            jEntity["collider"] = jCol;
        }

        // ---- Rigidbody ----
        if (registry.rigidBodies.Has(entityID))
        {
            const RigidbodyComponent& rb = registry.rigidBodies.Get(entityID);

            json jRb;
            jRb["mass"] = rb.mass;
            jRb["useGravity"] = rb.useGravity;
            jRb["isKinematic"] = rb.isKinematic;
            jRb["linearDamping"] = rb.linearDamping;
            jEntity["rigidbody"] = jRb;

            // velocity / acceleration / forces are runtime-only state —
            // they are intentionally not saved since they should always
            // start at zero when a scene is loaded fresh.
        }

        // ---- Movement ----
        if (registry.movements.Has(entityID))
        {
            const MovementComponent& m = registry.movements.Get(entityID);

            json jMov;
            jMov["moveInput"] = { m.moveInput.x, m.moveInput.y };
            jMov["jumpRequested"] = m.jumpRequested;
            jMov["sprinting"] = m.sprinting;
            jMov["walkSpeed"] = m.walkSpeed;
            jMov["sprintMultiplier"] = m.sprintMultiplier;
            jMov["jumpForce"] = m.jumpForce;
            jEntity["movement"] = jMov;
        }

        // ---- Controller ----
        if (registry.controllers.Has(entityID))
        {
            const ControllerComponent& c = registry.controllers.Get(entityID);

            json jCtrl;
            jCtrl["active"] = c.active;
            jCtrl["type"] = static_cast<int>(c.type);
            jCtrl["moveX"] = c.moveX;
            jCtrl["moveY"] = c.moveY;
            jCtrl["lookX"] = c.lookX;
            jCtrl["lookY"] = c.lookY;
            jCtrl["sprintHeld"] = c.sprintHeld;
            jCtrl["jumpPressed"] = c.jumpPressed;
            jCtrl["attackPressed"] = c.attackPressed;
            jCtrl["interactPressed"] = c.interactPressed;
            jCtrl["pausePressed"] = c.pausePressed;
            jEntity["controller"] = jCtrl;
        }

        // ---- Script ----
        if (registry.scripts.Has(entityID))
        {
            const ScriptComponent& s = registry.scripts.Get(entityID);

            json jScript;
            json names = json::array();
            for (const auto& name : s.scriptNames)
                names.push_back(name);

            jScript["scriptNames"] = names;
            jEntity["script"] = jScript;
        }

        jScene["entities"].push_back(jEntity);
    }

    std::ofstream file(finalPath);
    file << jScene.dump(4);

    std::cout << "Saved scene \"" << sceneName
        << "\" at: " << finalPath << std::endl;
}


std::unique_ptr<Scene> AssetManager::loadScene(const std::string& filePath)
{
    namespace fs = std::filesystem;

    try
    {
        fs::path path(filePath);

        if (!fs::exists(path))
        {
            std::cerr << "Scene file does not exist: " << filePath << "\n";
            return nullptr;
        }

        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open scene file: " << filePath << "\n";
            return nullptr;
        }

        json jScene;
        file >> jScene;
        file.close();

        std::string sceneName;
        uint64_t sceneUUID = 0;

        try {
            sceneName = jScene.at("name").get<std::string>();
            sceneUUID = jScene.at("uuid").get<uint64_t>();
        }
        catch (const json::exception& e) {
            std::cerr << "Invalid scene format: " << e.what() << std::endl;
            return nullptr;
        }

        std::unique_ptr<Scene> scene = std::make_unique<Scene>(sceneName, UUID(sceneUUID));

        const auto& jEntities = jScene.at("entities");

        std::unordered_map<Entity, Entity> idMap;

        for (const auto& jEntity : jEntities)
        {
            try {
                uint64_t oldID = jEntity.at("entityID").get<uint64_t>();
                std::string name = jEntity.at("name").get<std::string>();
                Entity newID = scene->CreateEntityImmediate(name);
                idMap[Entity(oldID)] = newID;
            }
            catch (const json::exception& e) {
                std::cerr << "Skipping entity (ID pass): " << e.what() << "\n";
            }
        }

      
        // set primary camera
        
        if (jScene.contains("primaryCamera")) {
            uint64_t primaryCamera = jScene.at("primaryCamera").get<uint64_t>();
            auto rootIt = idMap.find(primaryCamera);
            if (rootIt != idMap.end())
                scene->SetPrimaryCamera((Entity)rootIt->second);
        }
        else
            scene->SetPrimaryCamera(NullEntity);


        for (const auto& jEntity : jEntities)
        {
            try {
                uint64_t oldIDRaw = jEntity.at("entityID").get<uint64_t>();
                Entity oldID = Entity(oldIDRaw);

                auto it = idMap.find(oldID);
                if (it == idMap.end()) continue;

                const Entity entityID = it->second;
                Registry& registry = scene->GetRegistry();

                if (jEntity.contains("transform"))
                {
                    const auto& jt = jEntity.at("transform");

                    registry.transforms.Add(entityID, TransformComponent(
                        glm::vec3(jt.at("position")[0], jt.at("position")[1], jt.at("position")[2]),
                        glm::quat(jt.at("rotation")[3], jt.at("rotation")[0], jt.at("rotation")[1], jt.at("rotation")[2]),
                        glm::vec3(jt.at("scale")[0], jt.at("scale")[1], jt.at("scale")[2])
                    ));
                }

                if (jEntity.contains("meshFilter"))
                {
                    MeshFilter mf;

                    mf.meshID =
                        UUID(jEntity["meshFilter"]
                            .at("meshID")
                            .get<uint64_t>());

                    registry.meshFilters.Add(entityID, mf);
              
                }

                if (jEntity.contains("meshRenderer"))
                {
                    MeshRenderer mr;
                    const auto& jmr = jEntity["meshRenderer"];

                    mr.inst.baseMaterial = UUID(jmr.at("materialID").get<uint64_t>());
                    mr.inst.dirty = true; // force ResolveMaterial to recompute on next use

                    if (jmr.contains("materialOverrides"))
                    {
                        const auto& jOv = jmr["materialOverrides"];

                        if (jOv.contains("albedo"))
                        {
                            auto a = jOv["albedo"];
                            mr.inst.albedo = glm::vec3(a[0].get<float>(), a[1].get<float>(), a[2].get<float>());
                        }

                        if (jOv.contains("metallic"))
                            mr.inst.metallic = jOv["metallic"].get<float>();

                        if (jOv.contains("roughness"))
                            mr.inst.roughness = jOv["roughness"].get<float>();

                        if (jOv.contains("ao"))
                            mr.inst.ao = jOv["ao"].get<float>();

                        if (jOv.contains("opacity"))
                            mr.inst.opacity = jOv["opacity"].get<float>();

                        if (jOv.contains("normalStrength"))
                            mr.inst.normalStrength = jOv["normalStrength"].get<float>();

                        if (jOv.contains("map_albedo"))
                            mr.inst.map_albedo = UUID(jOv["map_albedo"].get<uint64_t>());

                        if (jOv.contains("map_normal"))
                            mr.inst.map_normal = UUID(jOv["map_normal"].get<uint64_t>());

                        if (jOv.contains("map_metallic"))
                            mr.inst.map_metallic = UUID(jOv["map_metallic"].get<uint64_t>());

                        if (jOv.contains("map_roughness"))
                            mr.inst.map_roughness = UUID(jOv["map_roughness"].get<uint64_t>());

                        if (jOv.contains("map_ao"))
                            mr.inst.map_ao = UUID(jOv["map_ao"].get<uint64_t>());

                        if (jOv.contains("map_metallicRoughness"))
                            mr.inst.map_metallicRoughness = UUID(jOv["map_metallicRoughness"].get<uint64_t>());

                        // use_map_* toggles - default to true if missing (matches in-class default)
                        mr.inst.use_map_albedo = jOv.value("use_map_albedo", true);
                        mr.inst.use_map_normal = jOv.value("use_map_normal", true);
                        mr.inst.use_map_metallic = jOv.value("use_map_metallic", true);
                        mr.inst.use_map_roughness = jOv.value("use_map_roughness", true);
                        mr.inst.use_map_ao = jOv.value("use_map_ao", true);
                        mr.inst.use_map_metallicRoughness = jOv.value("use_map_metallicRoughness", true);
                    }

                    if (jmr.contains("castShadows"))
                        mr.castShadows = jmr["castShadows"].get<bool>();

                    if (jmr.contains("receiveShadows"))
                        mr.receiveShadows = jmr["receiveShadows"].get<bool>();

                    if (jmr.contains("render"))
                        mr.render = jmr["render"].get<bool>();

                    registry.meshRenderers.Add(entityID, mr);
                }

                if (jEntity.contains("light"))
                {
                    const auto& jl = jEntity.at("light");

                    Light light(entityID);

                    if (jl.contains("type"))
                        light.type = static_cast<LightType>(jl.at("type").get<int>());

                    if (jl.contains("color")) {
                        light.color = {
                            jl.at("color")[0],
                            jl.at("color")[1],
                            jl.at("color")[2]
                        };
                    }

                    if (jl.contains("intensity"))  light.intensity = jl.at("intensity");
                    if (jl.contains("range"))       light.range = jl.at("range");
                    if (jl.contains("innerAngle"))  light.innerAngle = jl.at("innerAngle");
                    if (jl.contains("outerAngle"))  light.outerAngle = jl.at("outerAngle");
                    if (jl.contains("castShadow"))  light.castShadow = jl.at("castShadow");

                    registry.lights.Add(entityID, light);
                }

                if (jEntity.contains("camera"))
                {
                    const auto& jc = jEntity.at("camera");

                    CameraComponent c;

                    if (jc.contains("projectionType"))
                        c.projectionType = static_cast<ProjectionType>(
                            jc.at("projectionType").get<int>()
                            );

                    if (jc.contains("fov"))         c.fov = jc.at("fov");
                    if (jc.contains("nearClip"))     c.nearClip = jc.at("nearClip");
                    if (jc.contains("farClip"))      c.farClip = jc.at("farClip");
                    if (jc.contains("aspectRatio"))  c.aspectRatio = jc.at("aspectRatio");
                    if (jc.contains("orthoSize"))    c.orthoSize = jc.at("orthoSize");
                    if (jc.contains("target"))       c.target = jc.at("target").get<uint32_t>();

                    c.recalculateProjection();

                    registry.cameras.Add(entityID, c);
                }

                if (jEntity.contains("skeleton"))
                {
                    const auto& js = jEntity.at("skeleton");

                    SkeletonComponent s;

                    if (js.contains("skeletonID")) {
                        s.skeletonID = UUID(js.at("skeletonID").get<uint64_t>());
                    }

                    auto LoadMat4Array = [](const json& arr) {
                        std::vector<glm::mat4> mats;
                        for (const auto& matJson : arr) {
                            glm::mat4 m(1.0f);
                            int index = 0;
                            for (int i = 0; i < 4; i++)
                                for (int j = 0; j < 4; j++)
                                    m[i][j] = matJson[index++];
                            mats.push_back(m);
                        }
                        return mats;
                        };

                    if (js.contains("localPose"))     s.localPose = LoadMat4Array(js.at("localPose"));
                    if (js.contains("globalPose"))    s.globalPose = LoadMat4Array(js.at("globalPose"));
                    if (js.contains("finalMatrices")) s.finalMatrices = LoadMat4Array(js.at("finalMatrices"));

                    s.dirty = true;

                    registry.skeletons.Add(entityID, s);
                }

                if (jEntity.contains("boneAttachment"))
                {
                    const auto& jb = jEntity["boneAttachment"];

                    BoneAttachmentComponent b;

                    // These will be remapped later
                    if (jb.contains("modelRoot"))
                        b.modelRoot = Entity(jb["modelRoot"].get<uint64_t>());

                    if (jb.contains("skinnedRoot"))
                        b.skinnedRoot = Entity(jb["skinnedRoot"].get<uint64_t>());

                    if (jb.contains("boneIndex"))
                        b.boneIndex = jb["boneIndex"];

                    if (jb.contains("boneName"))
                        b.boneName = jb["boneName"].get<std::string>();
                    if (jb.contains("offsetPosition"))
                    {
                        auto& p = jb["offsetPosition"];
                        b.offset.position = glm::vec3(
                            p[0].get<float>(),
                            p[1].get<float>(),
                            p[2].get<float>());
                    }

                    if (jb.contains("offsetRotation"))
                    {
                        auto& r = jb["offsetRotation"];
                        b.offset.rotation = glm::quat(
                            r[3].get<float>(), // w
                            r[0].get<float>(), // x
                            r[1].get<float>(), // y
                            r[2].get<float>()  // z
                        );
                    }

                    if (jb.contains("offsetScale"))
                    {
                        auto& s = jb["offsetScale"];
                        b.offset.scale = glm::vec3(
                            s[0].get<float>(),
                            s[1].get<float>(),
                            s[2].get<float>());
                    }

                    b.offset.RecalculateMatrix();

                    registry.boneAttachments.Add(entityID, b);
                }

                if (jEntity.contains("animation"))
                {
                    const auto& ja = jEntity.at("animation");

                    AnimationComponent a;

                    if (ja.contains("animationIDs"))
                    {
                        for (const auto& id :
                            ja["animationIDs"])
                        {
                            a.animationIDs.push_back(
                                UUID(id.get<uint64_t>())
                            );
                        }
                    }

                    if (ja.contains("tposeAnimationID"))
                    {
                        a.tposeAnimationID =
                            UUID(
                                ja["tposeAnimationID"]
                                .get<uint64_t>()
                            );
                    }

                    if (ja.contains("animationNames"))
                    {
                        for (auto& [idStr, name] :
                            ja["animationNames"].items())
                        {
                            UUID id(
                                std::stoull(idStr)
                            );

                            a.animationNames[id] =
                                name.get<std::string>();

                            // rebuild reverse lookup
                            a.animationNameToID[
                                name.get<std::string>()
                            ] = id;
                        }
                    }

               
                    if (ja.contains("controller"))
                    {
                        AnimatorSerializer::Load(
                            ja["controller"],
                            a.animator
                        );
                    }

                    auto& anim = registry.animations.Add(
                        entityID,
                        a
                    );
                    
                    auto& skeleton = registry.skeletons.Get(entityID);
                    const UUID skeletonID = skeleton.skeletonID;
                    initAnimationComponentFromTpose(anim, skeletonID);
            
                }

                if (jEntity.contains("collider"))
                {
                    const auto& jc = jEntity.at("collider");

                    ColliderComponent c;

                    if (jc.contains("shapes"))
                    {
                        for (const auto& jShape : jc.at("shapes"))
                        {
                            ColliderShape s;

                            if (jShape.contains("type"))
                                s.type = static_cast<ColliderShape::Type>(jShape.at("type").get<int>());

                            if (jShape.contains("isTrigger"))
                                s.isTrigger = jShape.at("isTrigger");

                            switch (s.type)
                            {
                            case ColliderShape::Type::Box:
                                if (jShape.contains("size"))
                                {
                                    s.size = {
                                        jShape.at("size")[0],
                                        jShape.at("size")[1],
                                        jShape.at("size")[2]
                                    };
                                }
                                break;

                            case ColliderShape::Type::Sphere:
                                if (jShape.contains("radius"))
                                    s.radius = jShape.at("radius");
                                break;

                            case ColliderShape::Type::Capsule:
                                if (jShape.contains("radius"))
                                    s.radius = jShape.at("radius");
                                if (jShape.contains("height"))
                                    s.height = jShape.at("height");
                                break;
                            }

                            s.runtimeShape = nullptr;
                            s.dirty = true;

                            c.shapes.push_back(s);
                        }
                    }
                    registry.colliders.Add(entityID, c);
                }

                if (jEntity.contains("rigidbody"))
                {
                    const auto& jr = jEntity.at("rigidbody");

                    RigidbodyComponent rb;

                    if (jr.contains("mass"))          rb.mass = jr.at("mass");
                    if (jr.contains("useGravity"))    rb.useGravity = jr.at("useGravity");
                    if (jr.contains("isKinematic"))   rb.isKinematic = jr.at("isKinematic");
                    if (jr.contains("linearDamping")) rb.linearDamping = jr.at("linearDamping");

                    registry.rigidBodies.Add(entityID, rb);
                }

                if (jEntity.contains("movement"))
                {
                    const auto& jm = jEntity.at("movement");

                    MovementComponent m;

                    if (jm.contains("moveInput"))
                        m.moveInput = { jm.at("moveInput")[0], jm.at("moveInput")[1] };

                    if (jm.contains("jumpRequested"))    m.jumpRequested = jm.at("jumpRequested");
                    if (jm.contains("sprinting"))        m.sprinting = jm.at("sprinting");
                    if (jm.contains("walkSpeed"))        m.walkSpeed = jm.at("walkSpeed");
                    if (jm.contains("sprintMultiplier")) m.sprintMultiplier = jm.at("sprintMultiplier");
                    if (jm.contains("jumpForce"))        m.jumpForce = jm.at("jumpForce");

                    registry.movements.Add(entityID, m);
                }

                if (jEntity.contains("controller"))
                {
                    const auto& jc = jEntity.at("controller");

                    ControllerComponent c;

                    if (jc.contains("active"))          c.active = jc.at("active");
                    if (jc.contains("type"))            c.type = static_cast<ControllerType>(jc.at("type").get<int>());
                    if (jc.contains("moveX"))           c.moveX = jc.at("moveX");
                    if (jc.contains("moveY"))           c.moveY = jc.at("moveY");
                    if (jc.contains("lookX"))           c.lookX = jc.at("lookX");
                    if (jc.contains("lookY"))           c.lookY = jc.at("lookY");
                    if (jc.contains("sprintHeld"))      c.sprintHeld = jc.at("sprintHeld");
                    if (jc.contains("jumpPressed"))     c.jumpPressed = jc.at("jumpPressed");
                    if (jc.contains("attackPressed"))   c.attackPressed = jc.at("attackPressed");
                    if (jc.contains("interactPressed")) c.interactPressed = jc.at("interactPressed");
                    if (jc.contains("pausePressed"))    c.pausePressed = jc.at("pausePressed");

                    registry.controllers.Add(entityID, c);
                }

                if (jEntity.contains("script"))
                {
                    const auto& js = jEntity.at("script");

                    ScriptComponent s;

                    if (js.contains("scriptNames"))
                    {
                        for (const auto& name : js.at("scriptNames"))
                            s.scriptNames.push_back(name.get<std::string>());
                    }

                    registry.scripts.Add(entityID, s);
                }
            }
            catch (const json::exception& e) {
                std::cerr << "Skipping invalid entity in scene \"" << sceneName
                    << "\": " << e.what() << std::endl;
                continue;
            }
            catch (const std::exception& e) {
                std::cerr << "Error creating entity: " << e.what() << std::endl;
                continue;
            }
        }

        for (const auto& jEntity : jEntities)
        {
            if (!jEntity.contains("entityID")) continue;

            Entity oldID = Entity(jEntity["entityID"].get<uint64_t>());

            auto it = idMap.find(oldID);
            if (it == idMap.end()) continue;

            Entity newID = it->second;

            if (!jEntity.contains("hierarchy")) continue;

            const auto& jh = jEntity["hierarchy"];

            if (!jh.contains("parent")) continue;

            Entity oldParent = Entity(jh["parent"].get<uint64_t>());
            if (oldParent == NullEntity) continue;

            auto parentIt = idMap.find(oldParent);
            if (parentIt == idMap.end()) continue;

            scene->SetParent(newID, parentIt->second);
        }

        for (const auto& jEntity : jEntities)
        {
            if (!jEntity.contains("entityID")) continue;

            Entity oldID = Entity(jEntity["entityID"].get<uint64_t>());

            auto it = idMap.find(oldID);
            if (it == idMap.end()) continue;

            Entity newID = it->second;

            bool hasParent = false;

            if (jEntity.contains("hierarchy"))
            {
                const auto& jh = jEntity["hierarchy"];
                if (jh.contains("parent"))
                {
                    Entity oldParent = Entity(jh["parent"].get<uint64_t>());
                    if (oldParent != NullEntity && idMap.count(oldParent))
                        hasParent = true;
                }
            }

            if (!hasParent)
            {
                if (std::find(scene->GetRootEntities().begin(),
                    scene->GetRootEntities().end(),
                    newID) == scene->GetRootEntities().end())
                {
                    scene->GetRootEntities().push_back(newID);
                }
            }
        }

        // remap meshFilters.rootEntity
        for (auto e : scene->GetRegistry().meshFilters.GetEntities()) {
            auto& mf = scene->GetRegistry().meshFilters.Get(e);

            mf.rootParent = scene->GetRootParent(e);
        }

        // Remap BoneAttachment entity references
        for (auto entity : scene->GetRegistry().boneAttachments.GetEntities())
        {
            auto& b = scene->GetRegistry().boneAttachments.Get(entity);

            if (b.modelRoot != NullEntity)
            {
                auto it = idMap.find(b.modelRoot);
                if (it != idMap.end())
                    b.modelRoot = it->second;
                else
                    b.modelRoot = NullEntity;
            }

            if (b.skinnedRoot != NullEntity)
            {
                auto it = idMap.find(b.skinnedRoot);
                if (it != idMap.end())
                    b.skinnedRoot = it->second;
                else
                    b.skinnedRoot = NullEntity;
            }
        }

        std::cout << "Loaded scene: " << scene->getName() << "\n";
        return scene;
    }
    catch (const json::exception& e)
    {
        std::cerr << "JSON error while loading scene: " << e.what() << "\n";
        return nullptr;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error while loading scene: " << e.what() << "\n";
        return nullptr;
    }
}


void AssetManager::ProcessGpuUploads()
{
    std::vector<UUID> gpuReadyAssets;

    {
        std::lock_guard<std::mutex> lock(assetMutex);

        for (auto& [id, state] : assetStates)
        {
            if (state == AssetState::LoadedToCPU)
                gpuReadyAssets.push_back(id);
        }
    }

    for (UUID id : gpuReadyAssets)
    {
        AssetType type = getAssetType(id);
        bool ok = false;

        switch (type)
        {
        case AssetType::Mesh:
            ok = processPendingSubmesh(id);
            break;

        case AssetType::Texture:
            ok = processPendingTextures(id);
            break;
        }

        std::lock_guard<std::mutex> lock(assetMutex);
        assetStates[id] = ok
            ? AssetState::LoadedToGPU
            : AssetState::Failed;
    }
}


void AssetManager::SyncAssetsToScene(Scene& activeScene) {

    if (pendingPrefabs.empty() && pendingSubmeshes.empty() && pendingTextureRequests.empty()) {
        isLoadingAssets = false;
        return;
    }

    isLoadingAssets = true;

    if (!pendingPrefabs.empty()) {
        InstantiatePrefab(activeScene, pendingPrefabs.front());
        pendingPrefabs.pop();
    }

    if (!pendingSubmeshes.empty())
    {
        auto req = pendingSubmeshes.front();

        if (assetStates[req.meshID] == AssetState::LoadedToGPU)
        {
            auto& mf = activeScene.GetRegistry().meshFilters.Get(req.entityID);
            mf.meshID = req.meshID;
            mf.ClearPendingSubmesh();

            assetStates[req.meshID] = AssetState::Loaded;
            pendingSubmeshes.pop();
        }
        else if (assetStates[req.meshID] == AssetState::Failed)
        {
            pendingSubmeshes.pop();
        }
    }

    ProcessPendingTextureRequests(activeScene);
}

// this func is for clearing up the Failed or Loaded assets of assetStates 
// as processGpuUploads() iterarate through assetStates every loop
// it is better to keep only the assets which are needed for loading or importing
void AssetManager::UpdateAssetStates()
{
    for (auto it = assetStates.begin(); it != assetStates.end(); )
    {
        AssetState state = it->second;

        if (
            state == AssetState::Failed ||
            state == AssetState::Unloaded
            )
        {
            it = assetStates.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


void AssetManager::ProcessPendingTextureRequests(Scene& activeScene)
{
    if (pendingTextureRequests.empty())
        return;

    Registry& registry = activeScene.GetRegistry();

    size_t count = pendingTextureRequests.size();

    while (count--)
    {
        TextureLoadRequest req = pendingTextureRequests.front();
        pendingTextureRequests.pop();

        AssetState state = assetStates[req.textureID];

        if (state == AssetState::Importing)
        {
            pendingTextureRequests.push(req);
            continue;
        }

        // 🔹 Stage 2: Imported but not loading → trigger load
        if (state == AssetState::Imported)
        {
            RequestTextureLoad(
                req.textureID,
                req.matID,
                req.mapType,
                req.srgb
            );
            pendingTextureRequests.push(req);
            continue;
        }

        // 🔹 Stage 3: Waiting for GPU upload
        if (state != AssetState::LoadedToGPU)
        {
            pendingTextureRequests.push(req);
            continue;
        }

        // 🔹 Stage 4: Safe to bind
        if (req.targetType == TextureTargetType::GlobalMaterial)
        {
            Material* mat = GetMaterial(req.matID);
            if (!mat) continue;

            switch (req.mapType)
            {
            case TextureMapType::Albedo:             mat->map_albedo = req.textureID; break;
            case TextureMapType::Normal:             mat->map_normal = req.textureID; break;
            case TextureMapType::Metallic:           mat->map_metallic = req.textureID; break;
            case TextureMapType::Roughness:          mat->map_roughness = req.textureID; break;
            case TextureMapType::AmbientOcclusion:   mat->map_ao = req.textureID; break;
            case TextureMapType::MetallicRoughness:  mat->map_metallicRoughness = req.textureID; break;
            }

            mat->localDirty = true;
            assetStates[req.textureID] = AssetState::Loaded;
        }
        else
        {
            if (!registry.meshRenderers.Has(req.entityID))
                continue;

            MeshRenderer& mr = registry.meshRenderers.Get(req.entityID);
            MaterialInstance& inst = mr.inst;

            switch (req.mapType)
            {
            case TextureMapType::Albedo:             inst.map_albedo = req.textureID; break;
            case TextureMapType::Normal:             inst.map_normal = req.textureID; break;
            case TextureMapType::Metallic:           inst.map_metallic = req.textureID; break;
            case TextureMapType::Roughness:          inst.map_roughness = req.textureID; break;
            case TextureMapType::AmbientOcclusion:   inst.map_ao = req.textureID; break;
            case TextureMapType::MetallicRoughness:  inst.map_metallicRoughness = req.textureID; break;
            }

            inst.dirty = true;
            assetStates[req.textureID] = AssetState::Loaded;
        }
    }
}

bool AssetManager::haveAssetState(const UUID& assetID)
{
    return assetStates.find(assetID) != assetStates.end();
}


bool AssetManager::hasLoadingAssets()
{
    for (const auto& [id, state] : assetStates)
    {
        if (state == AssetState::Loading || state == AssetState::LoadedToCPU)
        {
            currentLoadingAsset = id;
            return true;
        }
    }
    currentLoadingAsset = UUID::Null;
    return false;
}

bool AssetManager::hasImportingAssets()
{
    for (const auto& [id, state] : assetStates)
    {
        if (state == AssetState::Importing)
        {
            currentImportingAsset.first = id;
            return true;
        }
    }
    currentImportingAsset.first = UUID::Null;
    return false;
}

float AssetManager::getLoadingProgress(const UUID& id) const
{
    auto it = assetStates.find(id);
    if (it == assetStates.end())
        return -1.0f;

    AssetState state = it->second;
    switch (state) {
    case AssetState::Failed:      return -1.0f;
    case AssetState::Loading:     return 0.25f;
    case AssetState::LoadedToCPU: return 0.50f;
    case AssetState::LoadedToGPU: return 0.75f;
    case AssetState::Loaded:      return 1.0f;
    case AssetState::Unloaded:    return -1.0f;
    default:                      return 0.0f;
    }
}

float AssetManager::getImportingProgress(const UUID& id) const
{
    auto it = assetStates.find(id);
    if (it == assetStates.end())
        return -1.0f;

    AssetState state = it->second;
    switch (state) {
    case AssetState::Failed:    return -1.0f;
    case AssetState::Importing: return 0.25f;
    case AssetState::Imported:  return 1.0f;
    default:                    return 0.0f;
    }
}

void AssetManager::updateLoadingStatus() {

    if (!loadingScreen || !hasLoadingAssets())
        return;

    UUID id = getCurrentLoadingAsset();

    LoadingScreenInfo info;

    info.progress = getLoadingProgress(id);

    switch (getAssetType(id))
    {
    case AssetType::Mesh:
        info.title = "Loading Mesh";
        break;

    case AssetType::Texture:
        info.title = "Loading Texture";
        break;
    }

    info.message =
        "Loading: " +
        GetAssetMetaData(id)->libraryPath.string();

    loadingScreen->Show(info);


}

void AssetManager::updateImportStatus() {

    if (!loadingScreen  || !hasImportingAssets() )
        return;


    UUID id = getCurrentImportingAsset().first;

    LoadingScreenInfo info;

    info.title = "Importing Asset";
    info.progress = getImportingProgress(id);

    info.message =
        "Importing: " +
        getCurrentImportingAsset().second;

    loadingScreen->Show(info);


}

void AssetManager::updateLoadingScreens(){
    if (!hasLoadingAssets() && !hasImportingAssets()) {
        loadingScreen->Hide();
        return;
    }

    updateLoadingStatus();
    updateImportStatus();
}

AssetType AssetManager::getAssetType(const UUID& id) {
    const AssetMetadata* meta = GetAssetMetaData(id);
    if (!meta) return AssetType::Unknown;

    if (meta->type == AssetType::Material)  return AssetType::Material;
    else if (meta->type == AssetType::Texture) return AssetType::Texture;
    else if (meta->type == AssetType::Mesh)    return AssetType::Mesh;
    else return AssetType::Unknown;
}

void AssetManager::initAnimationComponentFromTpose(
    AnimationComponent& anim,
    const UUID& skelID)
{
    if (anim.tposeAnimationID == UUID::Null) return;
    

    Skeleton* skeleton = GetSkeleton(skelID);

    if (!skeleton) {
      LoadSkeleton(skelID);
      skeleton = GetSkeleton(skelID);
      skeleton->BuildBoneNodeHierarchy();
    }


    Animation* tposeClip = GetAnimation(anim.tposeAnimationID);
    if (!tposeClip && LoadAnimation(anim.tposeAnimationID))
        tposeClip = GetAnimation(anim.tposeAnimationID);

    if (!skeleton || !tposeClip) {
        return;
    } 



    size_t boneCount = skeleton->bones.size();

    anim.finalBoneMatrices.resize(boneCount, glm::mat4(1.0f));
    anim.globalBoneTransforms.resize(boneCount, glm::mat4(1.0f));
    anim.tPose.Resize(boneCount);

    std::vector<glm::mat4> globalTransforms(boneCount);

    for (size_t i = 0; i < boneCount; i++)
    {
        int trackIndex = tposeClip->boneTrackMap[i];

        glm::vec3 pos = glm::vec3(0.0f);
        glm::quat rot = glm::quat(1, 0, 0, 0);
        glm::vec3 scale = glm::vec3(1.0f);

        if (trackIndex != -1)
        {
            auto& track = tposeClip->tracks[trackIndex];
            pos = track.positions.empty() ? glm::vec3(0) : track.positions[0].position;
            rot = track.rotations.empty() ? glm::quat(1, 0, 0, 0) : track.rotations[0].rotation;
            scale = track.scales.empty() ? glm::vec3(1) : track.scales[0].scale;
        }

        glm::mat4 local = glm::translate(glm::mat4(1.0f), pos)
            * glm::mat4_cast(rot)
            * glm::scale(glm::mat4(1.0f), scale);

        int parent = skeleton->bones[i].parentIndex;
        globalTransforms[i] = (parent == -1) ? local : globalTransforms[parent] * local;

        anim.finalBoneMatrices[i] = globalTransforms[i] * skeleton->bones[i].inverseBindMatrix;
        anim.globalBoneTransforms[i] = globalTransforms[i];
        anim.tPose.bones[i] = { pos, rot, scale };
    }
}
#include "SceneManager.h"

using namespace Lengine;


void SceneManager::loadScenes(
    const std::vector<std::string>& scenesToBeLoaded)
{
    for (auto& sceneName : scenesToBeLoaded)
    {
        auto scene = assetManager.loadScene(
            Paths::GameScenes + sceneName + ".json"
        );

        if (!activeScene)
            setActiveScene(scene.get());

        scenes.push_back(std::move(scene));
    }
}
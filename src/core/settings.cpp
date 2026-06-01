#include "settings.h"

std::filesystem::path exePath = std::filesystem::current_path();
std::string configPath = (exePath / "../config.json").string();

EngineSettings::EngineSettings() {
    loadSettings();
}

const bool EngineSettings::loadSettings() {
    std::ifstream file(configPath);
    if (file.fail()) {
        perror(configPath.c_str());
        Lengine::fatalError("Error to open " + configPath);
    }

    json j;
    file >> j;
    
    windowName = j.value("windowName", windowName);
    windowWidth = j.value("windowWidth", windowWidth);
    windowHeight = j.value("windowHeight", windowHeight);

    std::string renderPathStr = j.value("renderPath", "forward");


    resolution_X = j.value("resolution_X", resolution_X);
    resolution_Y = j.value("resolution_Y", resolution_Y);

    shadowMapResolution = j.value("shadowMapResolution", shadowMapResolution);


    std::string modeStr = j.value("windowMode", "borderless");

    if (modeStr == "borderless")
        windowMode = BORDERLESS;
    else if (modeStr == "invisible")
        windowMode = INVISIBLE;
    else if (modeStr == "fullscreen")
        windowMode = FULLSCREEN;
    else {
        std::cerr << "Unknown windowMode: " << modeStr
            << ", defaulting to borderless.\n";
        windowMode = BORDERLESS;
    }

    cameraPosX = j.value("cameraPosX", cameraPosX);
    cameraPosY = j.value("cameraPosY", cameraPosY);
    cameraPosZ = j.value("cameraPosZ", cameraPosZ);
    cameraFov = j.value("cameraFov", cameraFov);

    gameFolderPath = j.value("gameFolderPath", gameFolderPath);
    engineFolderPath = j.value("engineFolderPath", engineFolderPath);
    editorFolderPath = j.value("editorFolderPath", editorFolderPath);
    gameExecutableFolder = j.value("gameExecutableFolder", gameExecutableFolder);

    Lengine::Paths::setPaths(gameFolderPath, engineFolderPath, editorFolderPath, gameExecutableFolder);

    std::cout << "Loaded settings from " << configPath << "\n";
    return true;
}

const bool EngineSettings::saveSettings() 
{
    json j;

    // --- Window ---
    j["windowName"] = windowName;
    j["windowWidth"] = windowWidth;
    j["windowHeight"] = windowHeight;

    switch (windowMode)
    {
    case BORDERLESS: j["windowMode"] = "borderless"; break;
    case INVISIBLE:  j["windowMode"] = "invisible";  break;
    case FULLSCREEN: j["windowMode"] = "fullscreen"; break;
    }

   
    j["resolution_X"] = resolution_X;
    j["resolution_Y"] = resolution_Y;
    j["shadowMapResolution"] = shadowMapResolution;


    // --- Camera ---
    j["cameraPosX"] = cameraPosX;
    j["cameraPosY"] = cameraPosY;
    j["cameraPosZ"] = cameraPosZ;
    j["cameraFov"] = cameraFov;

    // --- Paths ---
    j["gameFolderPath"] = gameFolderPath;

    std::ofstream file(configPath);
    if (!file.is_open())
    {
        perror(configPath.c_str());
        return false;
    }

    // Pretty print with indentation (engine-quality)
    file << j.dump(4);

    std::cout << "Saved settings to " << configPath << "\n";
    return true;
}

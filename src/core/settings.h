#pragma once

#include <iostream>
#include <fstream>

#include "../core/Errors.h"
#include "../core/paths.h"


#include "../external/json.hpp"

using json = nlohmann::json;

enum WindowMode {
	BORDERLESS = 0,
	INVISIBLE = 1,
	FULLSCREEN = 2
};

enum class EditorMode {
	EDIT = 0,
	PLAY = 1,
	PAUSE = 2
};

enum class RenderPath {
	Forward,
	Deferred
};

struct RenderSettings {
	 RenderPath renderPath = RenderPath::Forward;

	float exposure = 1.0f;

	bool enableBloom = false;
	float bloomBlur = 1.0f;

	bool MSAA = false;
	int msaaSamples = 4;

	bool needsReload = true;

	uint32_t resolution_X = 1280;
	uint32_t resolution_Y = 720;

};


class EngineSettings {
public:
	EngineSettings();
	std::string windowName = "Lengine";
	uint32_t  windowWidth = 1280;
	uint32_t  windowHeight = 720 ;
	WindowMode windowMode = WindowMode::BORDERLESS;


	uint32_t resolution_X = 1280;
	uint32_t resolution_Y = 720;

	uint32_t shadowMapResolution = 1024;


	float cameraPosX = 0;
	float cameraPosY = 0;
	float cameraPosZ = 0;
	float cameraFov = 45;

	std::string gameFolderPath ;
	std::string engineFolderPath;
	std::string editorFolderPath;
	std::string gameExecutableFolder;

	const bool loadSettings();
	const bool saveSettings();
};


#include "InputManager.h"

namespace Lengine {


	InputManager::InputManager() : _mouseCoords(0, 0) {}
	InputManager::~InputManager() {}

	void InputManager::Update()
	{
		for (auto& it : _keyMap)
			_previousKeyMap[it.first] = it.second;

		for (auto& it : _mouseButtonMap)
			_previousMouseButtonMap[it.first] = it.second;

		updateMouseCoords();

	}

	void InputManager::pressButton(unsigned int keyID) {
		_mouseButtonMap[keyID] = true;
	}
	void InputManager::pressKey(unsigned int keyID) {
		_keyMap[keyID] = true;
	}
	void InputManager::releaseButton(unsigned int buttonID) {
		_mouseButtonMap[buttonID] = false;
	}
	void InputManager::releaseKey(unsigned int keyID) {
		_keyMap[keyID] = false;
	}

	void InputManager::updateMouseCoords()
	{
		_previousMouseCoords = _mouseCoords;

		int mouseX, mouseY;
		SDL_GetMouseState(&mouseX, &mouseY);

		_mouseCoords.x = static_cast<float>(mouseX);
		_mouseCoords.y = static_cast<float>(mouseY);

		_mouseDelta = _mouseCoords - _previousMouseCoords;
	}

	bool InputManager::isMouseButtonDown(unsigned int buttonID) {
		auto it = _mouseButtonMap.find(buttonID);
		if (it != _mouseButtonMap.end()) {
			return it->second;
		}
		else {
			return false;
		}
	}
	bool InputManager::isKeyDown(unsigned int keyID) {
		auto it = _keyMap.find(keyID);
		if (it != _keyMap.end()){
			return it->second;
		}
		else {
			return false;
		}
	}
	bool InputManager::isMouseButtonPressed(unsigned int buttonID) {
		if (isMouseButtonDown(buttonID) && !wasMouseButtonDown(buttonID)) {
			return true;
		}
		return false;
	}


	bool InputManager::isKeyPressed(unsigned int keyID) {
		if (isKeyDown(keyID) && !wasKeyDown(keyID)) {
			return true;
		}
		return false;
	}
	bool InputManager::wasMouseButtonDown(unsigned int buttonID) {
		auto it = _previousMouseButtonMap.find(buttonID);
		if (it != _previousMouseButtonMap.end()) {
			return it->second;
		}
		else {
			return false;
		}
	}
	bool InputManager::wasKeyDown(unsigned int keyID) {
		auto it = _previousKeyMap.find(keyID);
		if (it != _previousKeyMap.end()) {
			return it->second;
		}
		else {
			return false;
		}
	}
	void InputManager::setScroll(int x, int y) {
		scrollX = x;
		scrollY = y;
	}

	void InputManager::resetScroll() {
		scrollX = 0;
		scrollY = 0;
	}

	void InputManager::processEvent(const SDL_Event& event)
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			pressKey(event.key.keysym.sym);
			break;

		case SDL_KEYUP:
			releaseKey(event.key.keysym.sym);
			break;

		case SDL_MOUSEBUTTONDOWN:
			pressButton(event.button.button);
			break;

		case SDL_MOUSEBUTTONUP:
			releaseButton(event.button.button);
			break;

		case SDL_MOUSEWHEEL:
			setScroll(event.wheel.x, event.wheel.y);
			break;
		}
	}
}


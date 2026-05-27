#pragma once

#include <unordered_map>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>


namespace Lengine {


	class InputManager
	{
	public:
		InputManager();
		~InputManager();

		void Update();
		void processEvent(const SDL_Event& event);


		void pressKey(unsigned int keyID);
		void releaseKey(unsigned int keyID);
		void pressButton(unsigned int buttonID);
		void releaseButton(unsigned int buttonID);
		void updateMouseCoords();
		void setScroll(int x, int y);
		void resetScroll();

		bool isKeyPressed(unsigned int keyID);
		bool isKeyDown(unsigned int keyID);
		bool wasKeyDown(unsigned int keyID);
		bool wasMouseButtonDown(unsigned int buttonID);
		bool isMouseButtonDown(unsigned int buttonID);
		bool isMouseButtonPressed(unsigned int buttonID);

		int getScrollX() const { return scrollX; }
		int getScrollY() const { return scrollY; }

		glm::vec2 getMouseCoords() const { return _mouseCoords; }
		glm::vec2 getMouseDelta() const { return _mouseDelta; }


	private:
		

		std::unordered_map<unsigned int, bool> _keyMap;
		std::unordered_map<unsigned int, bool> _previousKeyMap;

		std::unordered_map<unsigned int, bool> _mouseButtonMap;
		std::unordered_map<unsigned int, bool> _previousMouseButtonMap;

		glm::vec2 _mouseCoords;
		glm::vec2 _previousMouseCoords;
		glm::vec2 _mouseDelta;

		int scrollX = 0;
		int scrollY = 0;

	};
}



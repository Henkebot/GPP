#include "InputHandler.h"
#include <Windows.h> // For GetAsyncKeyState
#include <iostream> // For cout
#include "ICommand.h"


ICommand* InputHandler::HandleInput()
{
	if (_isKeyPressed('C')) return m_buttonX;
	else if (_isKeyPressed('F')) return m_buttonY;
	return nullptr;
}

void InputHandler::BindCommand(int button, ICommand * command)
{
	if (button == 1)
	{
		m_buttonX = command;
	}
	else if (button == 2)
	{
		m_buttonY = command;
	}


}

bool InputHandler::_isKeyPressed(char button) const
{
	return GetAsyncKeyState((int) button);
}

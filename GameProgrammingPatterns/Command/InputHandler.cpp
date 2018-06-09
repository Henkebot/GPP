#include "InputHandler.h"
#include <Windows.h> // For GetAsyncKeyState
#include <iostream> // For cout
#include "ICommand.h"
#include "GameActor.h"


// ##########################COMMANDS##########################
class JumpCommand : public ICommand
{
private:
	GameActor* m_Unit;
public:
	JumpCommand(GameActor* unit_) : m_Unit(unit_) {}
	virtual void execute()
	{
		m_Unit->Jump();
	}
	virtual void undo()
	{

	}
};

class FireCommand : public ICommand
{
private:
	GameActor* m_Unit;
public:
	FireCommand(GameActor* unit_) : m_Unit(unit_) {}
	virtual void execute()
	{
		m_Unit->Fire();
	}
	virtual void undo()
	{

	}
};

class MoveCommand : public ICommand
{
private:
	float xBefore;
	float yBefore;
	float x;
	float y;
	GameActor* m_Unit;
public:
	MoveCommand(GameActor* unit_, int x_, int y_) : m_Unit(unit_), x(x_), y(y_)
	{
		xBefore = 0;
		yBefore = 0;
	}
	virtual void execute()
	{
		xBefore = m_Unit->getX();
		yBefore = m_Unit->getY();

		m_Unit->Move(x, y);
	}
	virtual void undo()
	{
		m_Unit->Move(xBefore, yBefore);
	}
};

InputHandler::~InputHandler()
{
	while (false == m_commandList.empty())
	{
		ICommand* ptr = m_commandList.front();
		delete ptr;
		ptr = nullptr;
		m_commandList.pop_front();
	}
}

ICommand* InputHandler::HandleInput()
{
	ICommand* command = nullptr;

	if (_isKeyPressed('C')) command = new FireCommand(&m_unit);
	else if (_isKeyPressed('F')) command = new JumpCommand(&m_unit);
	else if ((_isKeyPressed('W'))) command = new MoveCommand(&m_unit, m_unit.getX(), m_unit.getY() + 1);
	else if ((_isKeyPressed('A'))) command = new MoveCommand(&m_unit, m_unit.getX() - 1, m_unit.getY());
	else if ((_isKeyPressed('D'))) command = new MoveCommand(&m_unit, m_unit.getX() + 1, m_unit.getY());
	else if ((_isKeyPressed('S'))) command = new MoveCommand(&m_unit, m_unit.getX(), m_unit.getY() - 1);
	
	if(command) 
		m_commandList.push_front(command); 

	return command;
}

ICommand * InputHandler::ReverseInput()
{
	ICommand* command = nullptr;
	if (false == m_commandList.empty())
	{
		command = m_commandList.front();
		m_commandList.pop_front();
	}
	return command;
}


bool InputHandler::_isKeyPressed(char button) const
{
	return GetAsyncKeyState((int) button);
}

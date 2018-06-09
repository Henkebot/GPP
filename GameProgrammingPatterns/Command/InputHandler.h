#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#include <forward_list> // For storing all the recent commands
#include "GameActor.h"

class ICommand;

class InputHandler
{
private:
	// Used to store all the commands which have been made
	std::forward_list<ICommand*> m_commandList;
	
	// Test actor for this InputHandler
	GameActor m_unit;
public:
	virtual ~InputHandler();
	// Returns the recent input, can be if no input was recieved
	ICommand* HandleInput();
	// Undos the recent input made
	ICommand* ReverseInput();
private:
	bool _isKeyPressed(char button) const;
};

#endif // !INPUT_HANDLER_H


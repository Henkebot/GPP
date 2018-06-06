#ifndef ICOMMAND_H
#define ICOMMAND_H

class GameActor;
// Interface class

class ICommand
{
public:
	virtual ~ICommand() {}
	// Execute a command
	virtual void execute() = 0;
	// Undo a command
	virtual void undo() = 0;
};

#endif // !COMMAND_H


#ifndef ICOMMAND_H
#define ICOMMAND_H

class GameActor;
// Interface class

class ICommand
{
public:
	virtual ~ICommand() {}
	virtual void execute(GameActor& actor) = 0;
};

#endif // !COMMAND_H


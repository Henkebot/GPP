#include "../Command/InputHandler.h"
#include "../Command/ICommand.h"
#include <iostream>


int main()
{
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(flag);

	InputHandler ih;
	int counter = 0;
	std::cout << "Request commands!\n";
	while (counter != 50)
	{
		ICommand* command = ih.HandleInput();
		if (command)
		{
			command->execute();
			counter++;
		}
		
	}

	std::cout << "Reverse commands!\n";

	for (int i = 0; i < counter + 5; i++)
	{
		ICommand* command = ih.ReverseInput();
		if (command)
		{
			command->undo();
			delete command;
			command = nullptr;
		}
	}
	
	system("pause");

	return 0;
}
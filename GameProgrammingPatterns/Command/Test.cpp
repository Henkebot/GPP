#include "InputHandler.h"
#include "ICommand.h"
#include <iostream>


int main()
{
	InputHandler ih;
	int counter = 0;
	std::cout << "Request commands!\n";
	while (true)
	{
		ICommand* command = ih.HandleInput();
		if (command)
		{
			command->execute();
			counter++;
		}

		if (counter == 50)
			break;
	}
	std::cout << "Reverse commands!\n";
	for (int i = 0; i < counter + 5; i++)
	{
		ICommand* command = ih.ReverseInput();
		if (command)
			command->undo();
	}
	
	system("pause");

	return 0;
}
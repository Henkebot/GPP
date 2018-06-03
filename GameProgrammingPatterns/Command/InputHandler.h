#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

class ICommand;

class InputHandler
{
private:
	ICommand* m_buttonX;
	ICommand* m_buttonY;
public:
	void Record();
	void Playback();

	ICommand* HandleInput();
	void BindCommand(int button, ICommand* command);
private:
	bool _isKeyPressed(char button) const;
};

#endif // !INPUT_HANDLER_H


#include "TestWindow.h"
#include <CommCtrl.h>
TestWindow::TestWindow(LPCTSTR _lpszCaption, HINSTANCE _hInstance, LPCTSTR _lpszClassName)
	:BaseWindow(_lpszCaption,_hInstance,_lpszClassName)
{
}

LRESULT TestWindow::wndProc(UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	

	switch (_uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
	
		break;



	}
	
	return DefWindowProc(m_hHandle, _uMsg, _wParam, _lParam);
}


#include <Windows.h>
#include <StdIO.h>

// -------------------------------------------------------------------------
// Command identifiers:
// -------------------------------------------------------------------------
#define IDM_OPEN	1001
#define IDM_SAVE_AS	1002
#define IDM_IMG_TILED	1003
#define IDM_IMG_FIT	1004
#define IDM_IMG_CENTERED	1005
#define IDM_IMG_NORMAL	1006

// -------------------------------------------------------------------------
// Prototypes:
// -------------------------------------------------------------------------
LRESULT CALLBACK BmpEditWndProc(
	HWND hWnd, UINT uiMsg, WPARAM wp, LPARAM lp);

// -------------------------------------------------------------------------
// Add some readability:
// -------------------------------------------------------------------------
#define WRECT(area)		((area).right - (area).left)
#define HRECT(area)		((area).bottom - (area).top)

// -------------------------------------------------------------------------
WCHAR* ProvideClassName()
{
	static WCHAR* pstrClassName = NULL;

	if (!pstrClassName) {
		pstrClassName = L"xxiBmpWriter";
	}
	return pstrClassName;
}

// -------------------------------------------------------------------------
class TWnd
{
private:
	HWND m_hWnd;
	UINT m_uiAppearance;
	HBITMAP m_hLoadedImage;
	SIZE m_ImageSize;

public:
	// Constructor:									  
	TWnd(HWND hWnd) {
		m_hWnd = hWnd;
		m_uiAppearance = IDM_IMG_NORMAL;
		m_hLoadedImage = NULL;
		OpenImage("lena_gray.bmp");
	}

	// Destructor:
	~TWnd() {
		ClearLoadedImage();
	}

	// Clear image:
	VOID ClearLoadedImage() {
		if (m_hLoadedImage) {
			DeleteObject(m_hLoadedImage);
			m_hLoadedImage = NULL;
		}
	}

	// One function for both "Open" and "SaveAs" commands:
	BOOL RunFileDialog(CHAR* pstrResultPath, BOOL bSaveAs) {
		OPENFILENAMEA ofn = { 0 };
		BOOL bRetCode;

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_hWnd;
		ofn.Flags = OFN_PATHMUSTEXIST;
		ofn.lpstrFile = pstrResultPath;
		ofn.lpstrFilter = "Bitmap Files (*.Bmp)\0*.Bmp\0";
		ofn.lpstrDefExt = "Bmp";
		ofn.nMaxFile = MAX_PATH;
		pstrResultPath[0] = '\0';

		if (bSaveAs) {
			ofn.Flags |= OFN_OVERWRITEPROMPT;
			bRetCode = GetSaveFileNameA(&ofn);
		}
		else {
			ofn.Flags |= OFN_FILEMUSTEXIST;
			bRetCode = GetOpenFileNameA(&ofn);
		}
		return bRetCode;
	}

	// Force to redraw the client area:
	VOID Refresh() {
		InvalidateRect(m_hWnd, NULL, TRUE);
	}

	// Local menu:
	VOID WmRBtnUp() {
		POINT pt;
		HMENU hMenu = CreatePopupMenu();

		AppendMenuA(hMenu, MF_STRING, IDM_OPEN, "&Open Bitmap...");
		AppendMenuA(hMenu, MF_STRING, IDM_SAVE_AS, "Save Bitmap &As...");
		AppendMenuA(hMenu, MF_SEPARATOR, -1, NULL);
		AppendMenuA(hMenu, MF_STRING, IDM_IMG_NORMAL, "&Normal");
		AppendMenuA(hMenu, MF_STRING, IDM_IMG_CENTERED, "&Centered");
		AppendMenuA(hMenu, MF_STRING, IDM_IMG_FIT, "Stretched to &Fit");
		AppendMenuA(hMenu, MF_STRING, IDM_IMG_TILED, "&Tiled");

		GetCursorPos(&pt);

		TrackPopupMenu(hMenu,
			TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
			pt.x, pt.y, 0, m_hWnd, NULL);

		DestroyMenu(hMenu);
	}

	// Drawing operations:
	VOID WmPaint() {
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(m_hWnd, &ps);
		HDC hMemDC;

		if ((hMemDC = CreateCompatibleDC(hDC)) != NULL) {
			HBITMAP hDCBitmap = (HBITMAP)SelectObject(hMemDC, m_hLoadedImage);
			RECT rcClient;

			GetClientRect(m_hWnd, &rcClient);
			switch (m_uiAppearance) {
			case IDM_IMG_NORMAL:
				// Drop it at location (0,0):
				BitBlt(hDC, 0, 0, m_ImageSize.cx, m_ImageSize.cy,
					hMemDC, 0, 0, SRCCOPY);
				break;
			case IDM_IMG_CENTERED:
				// Centered in client area:
			{
				RECT areaImg;

				GetCenteredImageRect(areaImg);

				BitBlt(hDC, areaImg.left, areaImg.top,
					m_ImageSize.cx, m_ImageSize.cy,
					hMemDC, 0, 0, SRCCOPY);
			}
			break;
			case IDM_IMG_FIT:
				// Stretch both sides to fit the client area:
				StretchBlt(hDC, 0, 0, rcClient.right, rcClient.bottom,
					hMemDC, 0, 0, m_ImageSize.cx, m_ImageSize.cy, SRCCOPY);
				break;
			case IDM_IMG_TILED:
				// Tile the client area:
			{
				int yTile = 0;
				int xTile;

				while (yTile < rcClient.bottom) {
					xTile = 0;
					while (xTile < rcClient.right) {
						BitBlt(hDC, xTile, yTile,
							m_ImageSize.cx, m_ImageSize.cy,
							hMemDC, 0, 0, SRCCOPY);

						xTile += m_ImageSize.cx;
					}
					yTile += m_ImageSize.cy;
				}
			}
			break;
			}
			SelectObject(hMemDC, hDCBitmap);
			DeleteDC(hMemDC);
		}
		EndPaint(m_hWnd, &ps);
	}

	// Open another image:
	VOID OpenImage(CHAR* pstrPath) {
		ClearLoadedImage(); // Kill the previous, if any...

		m_hLoadedImage = (HBITMAP)
			LoadImageA(NULL, pstrPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		BITMAP bmp;

		// Get the size:
		GetObjectA(m_hLoadedImage, sizeof(bmp), &bmp);
		m_ImageSize.cx = bmp.bmWidth;
		m_ImageSize.cy = bmp.bmHeight;




		Refresh();
	}

	// Make a bitmap out of a client area:
	HBITMAP MakeClientSnapshot(RECT& client) {
		HDC hDC = GetDC(m_hWnd);
		RECT rcClient;
		BOOL bOk = FALSE;
		HBITMAP hImage = NULL;

		GetClientRect(m_hWnd, &rcClient);
		client = rcClient;
		if ((hImage = CreateCompatibleBitmap(hDC,
			rcClient.right, rcClient.bottom)) != NULL) {

			HDC hMemDC;
			HBITMAP hDCBmp;

			if ((hMemDC = CreateCompatibleDC(hDC)) != NULL) {
				hDCBmp = (HBITMAP)SelectObject(hMemDC, hImage);

				BitBlt(hMemDC, 0, 0, rcClient.right, rcClient.bottom,
					hDC, 0, 0, SRCCOPY);

				SelectObject(hMemDC, hDCBmp);
				DeleteDC(hMemDC);
				bOk = TRUE;
			}
		}
		ReleaseDC(m_hWnd, hDC);
		if (!bOk) {
			if (hImage) {
				DeleteObject(hImage);
				hImage = NULL;
			}
		}
		return hImage;
	}

	// Save HDC as a bitmap:
	VOID WriteImage(CHAR* pstrPath) {
		HBITMAP hSnapshot;
		RECT client;

		if ((hSnapshot = MakeClientSnapshot(client)) != NULL) {
			UINT uiBytesPerRow = 3 * client.right; // RGB takes 24 bits
			UINT uiRemainderForPadding;

			if ((uiRemainderForPadding = uiBytesPerRow % sizeof(DWORD)) > 0) {
				uiBytesPerRow += (sizeof(DWORD) - uiRemainderForPadding);
			}

			UINT uiBytesPerAllRows = uiBytesPerRow * client.bottom;
			PBYTE pDataBits;

			if ((pDataBits = new BYTE[uiBytesPerAllRows]) != NULL) {
				BITMAPINFOHEADER bmi = { 0 };
				BITMAPFILEHEADER bmf = { 0 };
				HDC hDC = GetDC(m_hWnd);

				// Prepare to get the data out of HBITMAP:
				bmi.biSize = sizeof(bmi);
				bmi.biPlanes = 1;
				bmi.biBitCount = 24;
				bmi.biHeight = client.bottom;
				bmi.biWidth = client.right;

				// Get it:
				GetDIBits(hDC, hSnapshot, 0, client.bottom,
					pDataBits, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

				ReleaseDC(m_hWnd, hDC);

				// Fill the file header:
				bmf.bfOffBits = sizeof(bmf) + sizeof(bmi);
				bmf.bfSize = bmf.bfOffBits + uiBytesPerAllRows;
				bmf.bfType = 0x4D42;

				// Writing:
				FILE* pFile;

				if ((pFile = fopen(pstrPath, "wb")) != NULL) {
					fwrite(&bmf, sizeof(bmf), 1, pFile);
					fwrite(&bmi, sizeof(bmi), 1, pFile);
					fwrite(pDataBits, sizeof(BYTE), uiBytesPerAllRows, pFile);
					fclose(pFile);
				}
				delete[] pDataBits;
			}
			DeleteObject(hSnapshot);
		}
	}

	// Command mapping here:
	VOID WmCommand(WORD wMenuItemID) {
		CHAR strPath[MAX_PATH];

		switch (wMenuItemID) {
		case IDM_OPEN:
			if (RunFileDialog(strPath, FALSE)) {
				OpenImage(strPath);
			}
			break;
		case IDM_SAVE_AS:
			if (RunFileDialog(strPath, TRUE)) {
				// I can't believe my eyes! If I will not refresh
				// the window, I will capture the part of the open
				// file dialog, but IT IS ENDED!! Try to comment the
				// following line (1-st line only) and see what happens...
				Refresh(); UpdateWindow(m_hWnd);
				WriteImage(strPath);
			}
			break;
		case IDM_IMG_CENTERED:
		case IDM_IMG_NORMAL:
		case IDM_IMG_TILED:
		case IDM_IMG_FIT:
			m_uiAppearance = (UINT)wMenuItemID; // Set the flag
			Refresh();
			break;
		}
	}

	// Make menu 'know' what can be done:
	VOID AdjustUserInterface(HMENU hPopup) {
		if (!m_hLoadedImage) {
			// Cannot save:
			EnableMenuItem(hPopup, IDM_SAVE_AS, MF_GRAYED);
		}

		// Put a check mark where needed:
		CheckMenuItem(hPopup, m_uiAppearance, MF_CHECKED);
	}

	// What area taken by the image if centered?
	VOID GetCenteredImageRect(RECT& area) {
		RECT rcClient;

		SetRect(&area, 0, 0, m_ImageSize.cx, m_ImageSize.cy);
		GetClientRect(m_hWnd, &rcClient);

		OffsetRect(&area,
			(WRECT(rcClient) - m_ImageSize.cx) / 2,
			(HRECT(rcClient) - m_ImageSize.cy) / 2);
	}

	// Avoiding flicker:
	VOID WmEraseBackground(HDC hDC)
	{
		HBRUSH hWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
		RECT rcClient, rcArea;

		GetClientRect(m_hWnd, &rcClient);
		if (!m_hLoadedImage)
		{
			// Nothing to draw, all filled with white:
			FillRect(hDC, &rcClient, hWhite);
		}
		else
		{
			// We have to calculate what is painted:
			// (Notice, that we do not process
			// IDM_IMG_TILED and IDM_IMG_FIT, because these
			// drawing options do not have visible background)
			switch (m_uiAppearance) {
			case IDM_IMG_CENTERED:
			{
				RECT area1, area2;

				GetCenteredImageRect(rcArea);
				if (rcArea.left > 0) {
					area1 = rcClient;
					area1.right = rcArea.left;
					area2 = rcClient;
					area2.left = rcArea.right;
					FillRect(hDC, &area1, hWhite);
					FillRect(hDC, &area2, hWhite);
				}

				if (rcArea.top > 0) {
					area1 = rcClient;
					area1.bottom = rcArea.top;
					area2 = rcClient;
					area2.top = rcArea.bottom;
					FillRect(hDC, &area1, hWhite);
					FillRect(hDC, &area2, hWhite);
				}
			}
			break;
			case IDM_IMG_NORMAL:
			{
				BOOL bRightSidePainted = FALSE;

				if (m_ImageSize.cx < rcClient.right) {
					// Uncovered from the right side:
					rcArea = rcClient;
					rcArea.left = m_ImageSize.cx;
					FillRect(hDC, &rcArea, hWhite);
					bRightSidePainted = TRUE;
				}

				if (m_ImageSize.cy < rcClient.bottom) {
					// Uncovered from the bottom side:
					rcArea = rcClient;
					rcArea.top = m_ImageSize.cy;
					if (bRightSidePainted) {
						rcArea.right = m_ImageSize.cx;
					}
					FillRect(hDC, &rcArea, hWhite);
				}
			}
			break;
			}
		}
	}

	// Message mapping here:
	LRESULT MsgMap(UINT uiMsg, WPARAM wp, LPARAM lp) {
		switch (uiMsg) {
		case WM_INITMENUPOPUP:
			AdjustUserInterface((HMENU)wp);
			return 0;
		case WM_ERASEBKGND:
			// The WNDCLASS contained 0 as background, so
			// we have to draw it ourselves. It helps avoid flicker.
			// Try to disable it and see how much flicker you get!
			WmEraseBackground((HDC)wp);
			return 1;
		case WM_PAINT:
			if (!m_hLoadedImage) {
				break; // No image loaded. Default processing.
			}
			WmPaint();
			return 0;
		case WM_RBUTTONUP:
			WmRBtnUp();
			return 0;
		case WM_COMMAND:
			WmCommand(LOWORD(wp));
			return 0;
		}
		return DefWindowProc(m_hWnd, uiMsg, wp, lp);
	}
};

// -------------------------------------------------------------------------
#define GETPWND		(TWnd*) (GetWindowLongW (hWnd, 0))
#define SETPWND(pWnd)		SetWindowLongW (hWnd, 0, (LONG) (pWnd))

// -------------------------------------------------------------------------
LRESULT CALLBACK BmpEditWndProc(HWND hWnd, UINT uiMsg, WPARAM wp, LPARAM lp)
{
	TWnd* pWnd;
	LRESULT lRetCode = 0; // For most messages...

	switch (uiMsg) {
	case WM_CREATE:
		SETPWND(new TWnd(hWnd));
		break;
	case WM_DESTROY:
		pWnd = GETPWND; // Retrieve the object 'this' pointer
		delete pWnd; // Call destructor of the object
		SETPWND(NULL); // Object life ends here.
		PostQuitMessage(0); // Signal to quit message loop
		break;
	default:
		if ((pWnd = GETPWND) != NULL) {
			// Between WM_CREATE and WM_DESTROY:
			lRetCode = pWnd->MsgMap(uiMsg, wp, lp);
		}
		else {
			// Before WM_CREATE and after WM_DESTROY:
			lRetCode = DefWindowProc(hWnd, uiMsg, wp, lp);
		}
	}
	return lRetCode;
}

// -------------------------------------------------------------------------
BOOL RegisterMainWnd(HINSTANCE hModule)
{
	WNDCLASSW wc = { 0 }; // All members to zero

	wc.cbWndExtra = sizeof(TWnd*);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hModule;
	wc.lpfnWndProc = BmpEditWndProc;
	wc.lpszClassName = ProvideClassName();
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.style = CS_VREDRAW | CS_HREDRAW;

	return (RegisterClassW(&wc) != NULL);
}

// -------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hModule, HINSTANCE, LPSTR, int iCmdShow)
{
	if (RegisterMainWnd(hModule))
	{
		HWND hMain = CreateWindowExW(0, ProvideClassName(),
			L"Test", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, hModule, NULL);

		if (hMain)
		{
			MSG message;

			ShowWindow(hMain, iCmdShow);
			UpdateWindow(hMain);

			while (GetMessageW(&message, NULL, 0, 0))
			{
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
		}
	}

	return 0;
}

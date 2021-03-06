#include "NewConsole.hpp"
#include <gdiplus.h>

#include "ConsoleHostServer.hpp"
#include "ConsoleWnd.hpp"

#pragma comment(lib, "gdiplus.lib")

NewConsole::NewConsole() : mainDC_(CreateCompatibleDC(nullptr)), mainBitmap_(nullptr), redrawQueued_(false)
{
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &mainThread_, 0, FALSE, DUPLICATE_SAME_ACCESS);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, nullptr);
	CoInitialize(nullptr);

	ConsoleHostServer::initialize();
}

NewConsole::~NewConsole()
{
	consoles_.clear();
	CloseHandle(mainThread_);
	Gdiplus::GdiplusShutdown(gdiplusToken_);
	CoUninitialize();

	DeleteDC(mainDC_);
}

int NewConsole::run(int nShowCmd)
{
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = TEXT("Console");
	wcex.lpfnWndProc = (WNDPROC)&NewConsole::WndProc_;
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.hCursor = LoadCursor(NULL, IDC_IBEAM);

	mainWnd_ = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW | WS_EX_LAYERED, (LPCWSTR)RegisterClassEx(&wcex), TEXT("Console"), 
							  WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, this);

	ShowWindow(mainWnd_, nShowCmd);

	consoles_.push_back(std::make_shared<ConsoleWnd>(L"C:\\windows\\system32\\cmd.exe", shared_from_this(), L"Consolas", 10.f));
	activeConsole_ = *consoles_.begin();
	activeConsole_.lock()->activated();

	COORD size = activeConsole_.lock()->querySize(80, 24);
	SetWindowPos(mainWnd_, nullptr, 0, 0, size.X, size.Y, SWP_NOMOVE | SWP_NOZORDER);

	redraw();

	MSG msg;
	while(true)
	{
		MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLEVENTS, MWMO_ALERTABLE);
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if(msg.message == WM_QUIT)
				break;
		}
		if(msg.message == WM_QUIT)
			break;
	}

	return 0;
}

void NewConsole::contentsUpdated(std::weak_ptr<ConsoleWnd> wnd)
{
	if(wnd.lock() == activeConsole_.lock())
		redraw();
}

void CALLBACK NewConsole::redrawCallback_(ULONG_PTR dwParam)
{
	reinterpret_cast<NewConsole *>(dwParam)->redrawCallback();
}

void NewConsole::redrawCallback()
{
	std::shared_ptr<ConsoleWnd> activeConsole = activeConsole_.lock();
	if(!activeConsole)
		return;
	redrawQueued_ = false;

	RECT rt;
	GetWindowRect(mainWnd_, &rt);
	int width = rt.right - rt.left;
	int height = rt.bottom - rt.top;
	if(!mainBitmap_)
	{
		HDC desktopDC = GetDC(nullptr);
		mainBitmap_ = CreateCompatibleBitmap(desktopDC, width, height);
		ReleaseDC(nullptr, desktopDC);
		SelectObject(mainDC_, mainBitmap_);
	}

	activeConsole->drawScreenContents(mainDC_, 0, 0, width, height, 0, 0);

	BLENDFUNCTION bf;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;

	POINT pt = {0, 0};
	POINT origin = {rt.left, rt.top};
	SIZE size = {width, height};
	UpdateLayeredWindow(mainWnd_, mainDC_, &origin, &size, mainDC_, &pt, RGB(0, 0, 0), &bf, ULW_ALPHA);
}

void NewConsole::redraw()
{
	std::shared_ptr<ConsoleWnd> activeConsole = activeConsole_.lock();
	if(!activeConsole)
		return;
	if(redrawQueued_)
		return;

	QueueUserAPC(&NewConsole::redrawCallback_, mainThread_, reinterpret_cast<ULONG_PTR>(this));
	redrawQueued_ = true;
}

HWND NewConsole::gethWnd()
{
	return mainWnd_;
}

LRESULT NewConsole::WndProc(UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch(iMessage)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_NCHITTEST:
		if(GetKeyState(VK_MENU) & 0x8000)
			return HTCAPTION;
		return HTCLIENT;
	case WM_SIZE:
		if(mainBitmap_)
			DeleteObject(mainBitmap_);
		mainBitmap_ = nullptr;
		return 0;
	case WM_KEYDOWN:
		if(!activeConsole_.expired())
		{
			BYTE state[256];
			GetKeyboardState(state);
			std::wstring temp(10, L' ');
			int len = ToUnicode(static_cast<UINT>(wParam), HIWORD(lParam) & 0xff, state, &temp[0], 10, 0);
			if(len > 0)
			{
				temp.resize(len);
				if(temp[0] == L'\r' && len == 1)
					temp[0] = L'\n';
				activeConsole_.lock()->appendCharacter(temp);
			}
			else
				activeConsole_.lock()->onKeyDown(static_cast<int>(wParam));
		}
		return 0;
	case WM_SETFOCUS:
		if(!activeConsole_.expired())
			activeConsole_.lock()->activated();
		return 0;
	}
	return DefWindowProc(mainWnd_, iMessage, wParam, lParam);
}

LRESULT CALLBACK NewConsole::WndProc_(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	NewConsole *this_ = nullptr;
	if(iMessage == WM_NCCREATE)
	{
		CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
		this_ = reinterpret_cast<NewConsole *>(cs->lpCreateParams);
		this_->mainWnd_ = hWnd;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this_));
	}
	else
		this_ = reinterpret_cast<NewConsole *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if(this_)
		return this_->WndProc(iMessage, wParam, lParam);
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	std::shared_ptr<NewConsole> instance = std::make_shared<NewConsole>();
	return instance->run(nShowCmd);	
}

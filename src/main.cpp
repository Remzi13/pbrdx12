// main.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <exception>

#include "input.h"

#include "app.h"

#include <ios>

static uint32_t getModifiers( WPARAM wParam )
{
	uint32_t m = 0;
	if ( GetKeyState( VK_SHIFT ) & 0x8000 ) m |= 1;
	if ( GetKeyState( VK_CONTROL ) & 0x8000 ) m |= 2;
	if ( GetKeyState( VK_MENU ) & 0x8000 ) m |= 4;
	return m;
}

void enableConsole()
{
	AllocConsole();
	FILE* fDummy;
	freopen_s( &fDummy, "CONOUT$", "w", stdout );
	freopen_s( &fDummy, "CONOUT$", "w", stderr );
	std::ios::sync_with_stdio();
}

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ); // Функция "обработчика" окна

// ---- Точка входа WinMain ----
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow )
{
	enableConsole();

	// 1. Регистрация класса окна
	const wchar_t CLASS_NAME[] = L"RTDX12";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc; // Наша функция-обработчик
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass( &wc );

	std::uint16_t width = 800;
	std::uint16_t height = 600;
	// 2. Создание окна
	HWND hwnd = CreateWindowEx(
		0, CLASS_NAME,
		L"RTDX12", // Заголовок окна
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, // Размеры окна
		nullptr, nullptr, hInstance, nullptr
	);

	if ( hwnd == nullptr )
	{
		return 0;
	}

	App app;

	// 3. Инициализация DirectX 12
	try
	{
		app.init( hwnd, width, height );
	}
	catch ( const std::exception& e )
	{
		MessageBoxA( hwnd, e.what(), "Error", MB_OK );
		return 1;
	}

	// 4. Показываем окно
	ShowWindow( hwnd, nCmdShow );

	// 5. Главный цикл сообщений (пока окно не закроют)
	MSG msg = {};

	while ( msg.message != WM_QUIT )
	{
		// Обработка сообщений Windows (если они есть)
		if ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			app.update();
		}
	}
	// 6. Очистка ресурсов
	app.fini();

	return 0;
}

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_PAINT:
		ValidateRect( hwnd, nullptr );
		return 0;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;

		// Keyboard
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		InputEvent ev;
		ev.type = InputEvent::Type::KeyDown;
		ev.key = static_cast<uint32_t>( wParam );
		ev.modifiers = getModifiers( wParam );
		Input::pushEvent( ev );
		return 0;
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		InputEvent ev;
		ev.type = InputEvent::Type::KeyUp;
		ev.key = static_cast<uint32_t>( wParam );
		ev.modifiers = getModifiers( wParam );
		Input::pushEvent( ev );
		return 0;
	}
	case WM_CHAR:
	{
		InputEvent ev;
		ev.type = InputEvent::Type::Char;
		ev.ch = static_cast<wchar_t>( wParam );
		ev.modifiers = getModifiers( wParam );
		Input::pushEvent( ev );
		return 0;
	}

	// Mouse
	case WM_MOUSEMOVE:
	{
		int x = GET_X_LPARAM( lParam );
		int y = GET_Y_LPARAM( lParam );
		InputEvent ev;
		ev.type = InputEvent::Type::MouseMove;
		ev.x = x; ev.y = y;
		ev.modifiers = getModifiers( wParam );
		Input::pushEvent( ev );
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	{
		int x = GET_X_LPARAM( lParam );
		int y = GET_Y_LPARAM( lParam );
		InputEvent ev;
		ev.type = InputEvent::Type::MouseDown;
		ev.x = x; ev.y = y;
		if ( uMsg == WM_LBUTTONDOWN ) ev.mouse_button = 1;
		else if ( uMsg == WM_RBUTTONDOWN ) ev.mouse_button = 2;
		else ev.mouse_button = 3;
		ev.modifiers = getModifiers( wParam );
		Input::pushEvent( ev );
		return 0;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		int x = GET_X_LPARAM( lParam );
		int y = GET_Y_LPARAM( lParam );
		InputEvent ev;
		ev.type = InputEvent::Type::MouseUp;
		ev.x = x; ev.y = y;
		if ( uMsg == WM_LBUTTONUP ) ev.mouse_button = 1;
		else if ( uMsg == WM_RBUTTONUP ) ev.mouse_button = 2;
		else ev.mouse_button = 3;
		ev.modifiers = getModifiers( wParam );
		Input::pushEvent( ev );
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM( wParam );
		int x = GET_X_LPARAM( lParam );
		int y = GET_Y_LPARAM( lParam );
		InputEvent ev;
		ev.type = InputEvent::Type::MouseWheel;
		ev.wheel_delta = delta;
		ev.x = x; ev.y = y;
		ev.modifiers = getModifiers( wParam );
		Input::pushEvent( ev );
		return 0;
	}

	case WM_KILLFOCUS:
	{
		InputEvent ev;
		ev.type = InputEvent::Type::FocusLost;
		Input::pushEvent( ev );
		return 0;
	}
	case WM_SETFOCUS:
	{
		InputEvent ev;
		ev.type = InputEvent::Type::FocusGained;
		Input::pushEvent( ev );
		return 0;
	}
	} // switch
	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}
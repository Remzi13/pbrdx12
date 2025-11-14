//
// Created by zakkh on 10.11.2025.
//
#include "app.h"

#include "input.h"

#include <iostream>
#include <chrono>

namespace {
	std::chrono::time_point g_lastTime = std::chrono::high_resolution_clock::now();
	float g_fpsTimer = 0.0f;
	int g_frameCount = 0;
	int g_fps = 0;
	float g_frameTime = 0.0f;
}
bool App::init( HWND hwnd, std::uint16_t width, std::uint16_t height )
{
	hwnd_ = hwnd;
	camera_.setPos( { 0, 0, 2 } );
	g_lastTime = std::chrono::high_resolution_clock::now();
	return render_.init( hwnd, width, height );
}

void App::update()
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	// Считаем разницу между кадрами в секундах
	float deltaTime = std::chrono::duration<float>( currentTime - g_lastTime ).count();
	g_lastTime = currentTime;

	// Сохраняем frametime в миллисекундах
	g_frameTime = deltaTime * 1000.0f;

	// Накапливаем время и кадры для подсчета FPS
	g_fpsTimer += deltaTime;
	g_frameCount++;

	// Обновляем заголовок окна один раз в секунду
	if ( g_fpsTimer >= 1.0f )
	{
		// Сохраняем FPS
		g_fps = g_frameCount;

		// Сбрасываем счетчики
		g_frameCount = 0;
		g_fpsTimer -= 1.0f; // Вычитаем 1.0, а не обнуляем, для точности

		// Форматируем строку заголовка
		wchar_t title[128];
		swprintf_s( title, 128, L"PBRDX12 - FPS: [ %d | %.2f ms]", g_fps, g_frameTime );

		// Устанавливаем заголовок окна
		SetWindowText( hwnd_, title );
	}

	inputUpdate();
	render_.update( camera_, scene_, isDirty_, deltaTime );
	render_.draw();
}

void App::inputUpdate()
{
	// обработка событий, пока они есть
	while ( !Input::empty() ) {
		auto evOpt = Input::pop();
		if ( !evOpt ) break;
		InputEvent ev = *evOpt;

		switch ( ev.type ) {
		case InputEvent::Type::KeyDown:
		case InputEvent::Type::KeyUp:
			handleKeyEvent( ev );
			break;
		case InputEvent::Type::Char:
			//handleChar(ev.ch);
			break;
		case InputEvent::Type::MouseMove:
			//handleMouseMove(ev.x, ev.y);
			break;
		case InputEvent::Type::MouseDown:
			//handleMouseDown(ev.mouse_button, ev.x, ev.y);
			break;
		case InputEvent::Type::MouseUp:
			//handleMouseUp(ev.mouse_button, ev.x, ev.y);
			break;
		case InputEvent::Type::MouseWheel:
			//handleMouseWheel(ev.wheel_delta, ev.x, ev.y);
			break;
		case InputEvent::Type::FocusLost:
			//onFocusLost();
			break;
		case InputEvent::Type::FocusGained:
			//onFocusGained();
			break;
		}
	}
}

void App::handleKeyEvent( const InputEvent& event )
{
	// Проверяем, что это клавиатурное событие
	if ( event.type != InputEvent::Type::KeyDown &&
		event.type != InputEvent::Type::KeyUp )
		return;

	bool pressed = ( event.type == InputEvent::Type::KeyDown );

	const float shift = 0.1f;
	switch ( event.key )
	{
	case VK_LEFT:
		if ( pressed )
		{
			
			const auto pos = camera_.pos();			
			camera_.setPos( { pos.x() - shift, pos.y(), pos.z() } );

			std::cout << "Left pressed" << camera_.pos() << "\n";
		}
		else
		{			
			std::cout << "Left released\n";
		}
		// Например, измени направление персонажа:
		// moveDirection.x = pressed ? -1.0f : 0.0f;
		break;

	case VK_RIGHT:
		if ( pressed )
		{
			const auto pos = camera_.pos();
			camera_.setPos( { pos.x() + shift, pos.y(), pos.z() } );
			std::cout << "Right pressed" << camera_.pos() << "\n"; 
		}
		else
			std::cout << "Right released\n";
		// moveDirection.x = pressed ? +1.0f : 0.0f;
		break;

	case VK_UP:
		if ( pressed )
		{
			const auto pos = camera_.pos();
			camera_.setPos( { pos.x() , pos.y(), pos.z() + shift } );
			std::cout << "Up pressed\n";
		}
		else
			std::cout << "Up released\n";
		// moveDirection.y = pressed ? +1.0f : 0.0f;
		break;

	case VK_DOWN:
		if ( pressed )
		{
			const auto pos = camera_.pos();
			camera_.setPos( { pos.x() , pos.y(), pos.z() - shift } );
			std::cout << "Down pressed\n";
		}
		else
			std::cout << "Down released\n";
		// moveDirection.y = pressed ? -1.0f : 0.0f;
		break;
	case 'A':
		if (pressed)
		{
			Primitive newSphere;
			newSphere.type = 0; // TYPE_SPHERE
			newSphere.radius = 0.3f;
			// Случайная позиция
			newSphere.position_point = { (rand() % 10) - 5.0f, ( rand() % 10 ) - 5.0f, (rand() % 5) + 1.0f, 0.0f };
			newSphere.normal_color = { 1.0f, 0.1f, 0.1f, 1.0f }; // Красный

			scene_.push_back(newSphere);
			isDirty_ = true;
		}

	default:
		break;
	}
}

void App::fini()
{
	render_.fini();
}


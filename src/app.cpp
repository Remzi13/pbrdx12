//
// Created by zakkh on 10.11.2025.
//
#include "app.h"

#include "input.h"

#include <iostream>
#include <chrono>

namespace {
	std::chrono::time_point g_lastTime = std::chrono::high_resolution_clock::now();
}
bool App::init( HWND hwnd, std::uint16_t width, std::uint16_t height )
{
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

	inputUpdate();
	render_.update( camera_, deltaTime );
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

	default:
		break;
	}
}

void App::fini()
{
	render_.fini();
}


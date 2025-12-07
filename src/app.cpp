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

bool App::init( HWND hwnd )
{
	hwnd_ = hwnd;

	g_lastTime = std::chrono::high_resolution_clock::now();
	//scene_.load( "../pbr/scenes/03-scene-easy.txt" );
	//scene_.load("../pbr/scenes/03-scene-medium.txt");
	//scene_.load("../pbr/scenes/03-scene-hard.txt");
	scene_.load( "../pbr/scenes/04-scene-easy.txt" );
	//scene_.load("../pbr/scenes/04-scene-medium.txt");
	return render_.init( hwnd, scene_);
}

void App::update()
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float>( currentTime - g_lastTime ).count();	
	g_lastTime = currentTime;

	g_frameTime = deltaTime * 1000.0f;

	g_fpsTimer += deltaTime;
	g_frameCount++;

	if ( g_fpsTimer >= 1.0f )
	{
		g_fps = g_frameCount;

		g_frameCount = 0;
		g_fpsTimer -= 1.0f;

		wchar_t title[128];
		swprintf_s( title, 128, L"PBRDX12 - FPS: [ %d | %.2f ms]", g_fps, g_frameTime );

		SetWindowText( hwnd_, title );
	}

	inputUpdate();
	render_.update( scene_, isDirty_, deltaTime );
	isDirty_ = false;
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
			auto camera = scene_.camera();
			camera.pos[0] -= shift;
			scene_.setCamera(camera);

			//const auto pos = camera_.pos();
			//camera_.setPos( { pos.x() - shift, pos.y(), pos.z() } );
			//std::cout << "Left pressed" << camera_.pos() << "\n";
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
			auto camera = scene_.camera();
			camera.pos[0] += shift;
			scene_.setCamera(camera);

			//const auto pos = camera_.pos();
			//camera_.setPos( { pos.x() + shift, pos.y(), pos.z() } );
			//std::cout << "Right pressed" << camera_.pos() << "\n"; 
		}
		else
			std::cout << "Right released\n";
		// moveDirection.x = pressed ? +1.0f : 0.0f;
		break;

	case VK_UP:
		if ( pressed )
		{
			auto camera = scene_.camera();
			camera.pos[2] += shift;
			camera.target[2] += shift;
			scene_.setCamera(camera);

			//const auto pos = camera_.pos();
			// 
			//camera_.setPos( { pos.x() , pos.y(), pos.z() + shift } );
			//std::cout << "Up pressed\n";
		}
		else
			std::cout << "Up released\n";
		// moveDirection.y = pressed ? +1.0f : 0.0f;
		break;

	case VK_DOWN:
		if ( pressed )
		{
			auto camera = scene_.camera();
			camera.pos[2] -= shift;
			camera.target[2] -= shift;
			scene_.setCamera(camera);

			//const auto pos = scene_.camera().pos;
			//scene_.camera().pos = { pos.x() , pos.y(), pos.z() - shift };
			//std::cout << "Down pressed\n";
		}
		else
			std::cout << "Down released\n";
		// moveDirection.y = pressed ? -1.0f : 0.0f;
		break;
	case 'A':
		if (pressed)
		{
			Sphere sphere;
			sphere.radius = 0.3f;
			// Случайная позиция
			sphere.pos = Vector3({ (rand() % 10) - 5.0f, ( rand() % 10 ) - 5.0f, (rand() % 5) + 1.0f});
			sphere.matIndex = 0;

			//auto s = scene_.samples();
			//scene_.setSamples( s + 1 );
			scene_.addSphere(sphere);
			isDirty_ = true;
		}
		break;
	case 'Z':
		if ( pressed )
		{
			auto s = scene_.samples();
			scene_.setSamples( s - 1 );
		}
		break;
	default:
		break;
	}
}

void App::fini()
{
	render_.fini();
}


#include "app.h"

#include <chrono>
#include <thread>


int main()
{
	App app(100, 30);
	
	while (app.isRun())
	{
		app.input();
		app.update();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}
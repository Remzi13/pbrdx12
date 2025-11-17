#include "app.h"

#include <chrono>
#include <thread>


int main()
{
	App app(100, 30);
	
	while (app.isRun())
	{
		if (app.input())
		{
			app.update();
		}
		else
		{
			std::this_thread::yield();
		}
	}

	return 0;
}
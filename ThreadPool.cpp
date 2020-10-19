// ConsoleApplication21.cpp : Defines the entry point for the console application.

//TODO
	//add 3 different function

#include "stdafx.h"

#include "ThreadPool.h"
#include <iostream>
#include <windows.h>
#include <chrono>


void TestFunc()
{
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(2s);
	std::cout << "thread: " << std::this_thread::get_id() << std::endl;
	return;
}

//return 0 if negative num
double Factorial(int _n)
{
	if (_n < 0)
		return 0;

	int sum = 1;

	for (int i = 1; i <= _n; ++i)
		sum *= i;
	
	Sleep(2000);
	std::cout << "thread: " << std::this_thread::get_id() << ". sum: " << sum << std::endl;

	return sum;
}

int main()
{
	ThreadPool pool(3);

	for (int i = 5; i < 15; ++i)
	{
		pool.Push(TestFunc);//(Factorial);

		//pool.Push(Factorial, i);
		//std::cout << returnValue.get() << std::endl;
	}
		
	getchar();
	return 0;
}
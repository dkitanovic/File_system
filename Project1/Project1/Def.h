#pragma once

#include<iostream>
#include<fstream>
#include<cstdlib>
#include<windows.h>
#include<ctime>

#define signal(x) ReleaseSemaphore(x,1,NULL)
#define wait(x) WaitForSingleObject(x,INFINITE)

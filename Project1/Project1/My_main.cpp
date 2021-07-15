/*#include <iostream>
#include "fs.h"
#include "KernelFS.h"
#include "part.h"
#include "file.h"
using namespace std;

int main() {
	
	Partition* partition = new Partition((char*)"p1.ini");
	FS::mount(partition);
	FS::format();
	char buffer[100000];
	
	char filepath[] = "/fajl1.dat";
	File* f = FS::open(filepath, 'w');
	if (f) cout << "File je uspesno otvoren" << endl;
	
	for (int i = 0; i < 100000; i++) buffer[i] = 'U';


	cout <<"Pisanje "<< f->write(100000, buffer) << endl;
	FS::pisi();

	
	delete f;
	FS::pisi();
	return 0;
}*/
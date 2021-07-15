#include <iostream>
#include <vector>
#include "fs.h"
#include "KernelFS.h"
using namespace std;

KernelFS* FS::myImpl = new KernelFS();

char FS::mount(Partition* partition) {
	return myImpl->mount(partition);
}

char FS::unmount() {
	return myImpl->unmount();
}

char FS::format() {
	return myImpl->format();
}

FileCnt FS::readRootDir() {
	return myImpl->readRootDir();
}

char FS::doesExist(char* fname){
	return myImpl->doesExist(fname);
}

File* FS::open(char* fname, char mode) {
	return myImpl->open(fname,mode);
}

char FS::deleteFile(char* fname) {
	return myImpl->deleteFile(fname);
}

void FS::pisi() {
	for (int i = 0; i < 1000; i++) cout << myImpl->get_valid(i) << " ";
}

FS::~FS() {

}
FS::FS() {
	
}
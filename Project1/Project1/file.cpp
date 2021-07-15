#include "file.h"
#include <iostream>
#include "fs.h"
#include "KernelFile.h"
using namespace std;


File::~File() {
	delete myImpl;
}

char File::write(BytesCnt bytes, char* buffer) {
	return myImpl->write(bytes, buffer);
}

BytesCnt File::read(BytesCnt bytes, char* buffer) {
	return myImpl->read(bytes, buffer);
}

char File::seek(BytesCnt bytes) {
	return myImpl->seek(bytes);
}

BytesCnt File::filePos() {
	return myImpl->filePos();
}

char File::eof() {
	return myImpl->eof();
}

BytesCnt File::getFileSize() {
	return myImpl->getFileSize();
}

char File::truncate() {
	return myImpl->truncate();
}

File::File(int* info, int level_one,int last_cluster, KernelFS* kernel,char rw,int c) {
	myImpl = new KernelFile(info,level_one,last_cluster,kernel,rw,c);
}

int File::fja() {
	return myImpl->fja();
}
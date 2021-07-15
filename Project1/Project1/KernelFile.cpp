#include "KernelFile.h"
#include "fs.h"
#include "part.h"
#include <iostream>
using namespace std;
bool prom = false;
HANDLE KernelFile::take=CreateSemaphore(NULL, 1, 32, NULL);
KernelFile::KernelFile(int* info, int level_one,int size, KernelFS* kernel,char rw,int c) {
	//this->curr_cluster=0;
	this->size = size;
	this->curr = c;
	this->open = true;
	this->info_pos = info;
	if (size == 0) this->clusters_used = size / 2048;
	else this->clusters_used = size / 2048 + 1;
	this->end_of_last_cluster = size % 2048;
	this->level_one = level_one;
	this->rw = rw;
	this->myKernel = kernel;
	if (level_one!=0) {
		char* buffer = new char[2048];
		myKernel->readCluster(this->level_one, buffer);
		char number[4];
		number[0] = buffer[0];
		number[1] = buffer[1];
		number[2] = buffer[2];
		number[3] = buffer[3];
		int* num = (int*)number;
		myKernel->readCluster(*num, buffer);
		number[0] = buffer[0];
		number[1] = buffer[1];
		number[2] = buffer[2];
		number[3] = buffer[3];
		num = (int*)number;
		this->curr_cluster  = *num;
	    int second = size / 2048;
		int first = second / 512;
		second = second % 512;
	
		myKernel->readCluster(this->level_one, buffer);
		number[0] = buffer[first * 4];
		number[1] = buffer[first * 4 + 1];
		number[2] = buffer[first * 4 + 2];
		number[3] = buffer[first * 4 + 3];
		 num = (int*)number;
		myKernel->readCluster(*num, buffer);
		number[0] = buffer[second * 4];
		number[1] = buffer[second * 4 + 1];
		number[2] = buffer[second * 4 + 2];
		number[3] = buffer[second * 4 + 3];
		num = (int*)number;
		this->last_cluster = *num;
	}
	
	//Treba da se inicijalizuje poslednji klaster;
	
	this->content = new char[2048];
}

KernelFile::~KernelFile() {	
	char buffer[2048];
	rw = 0;
	curr = 0;
	myKernel->readCluster(info_pos[0],buffer);
	char* size = new char[4];
	int* velicina=new int();
	*velicina = this->size;
	size = (char*)velicina;
	buffer[info_pos[1] * 32 + 16] = size[0];
	buffer[info_pos[1] * 32 + 17] = size[1];
	buffer[info_pos[1] * 32 + 18] = size[2];
	buffer[info_pos[1] * 32 + 19] = size[3];

	buffer[info_pos[1] * 32 + 20] = 1<<5;
	myKernel->writeCluster(info_pos[0], buffer);
	myKernel->close_File();
}

char KernelFile::write(BytesCnt bytes, char* buffer) {
	if (!(rw & 2)) return '0';
	BytesCnt count = 0;
	if (level_one == 0) {
		level_one = myKernel->first_free_cluster();

		char buffer1[2048];
		myKernel->readCluster(info_pos[0], buffer1);
		char* index = new char[4];
		index = (char*)(&level_one);

		buffer1[info_pos[1] * 32 + 12] = index[0];
		buffer1[info_pos[1] * 32 + 13] = index[1];
		buffer1[info_pos[1] * 32 + 14] = index[2];
		buffer1[info_pos[1] * 32 + 15] = index[3];
		myKernel->writeCluster(info_pos[0], buffer1);

		myKernel->set_v(level_one);
		myKernel->writeCluster(level_one, KernelFS::root_init);
		i_want_new_cluster();
	}
	while (count < bytes) {
		myKernel->readCluster(last_cluster, content);
		while (end_of_last_cluster != 2048 && count<bytes) {
			content[end_of_last_cluster++] = buffer[count++];
			size++;
			if (size == 512 * 512 * 2048) return count;
		}
		myKernel->writeCluster(last_cluster,content);
		if (count < bytes) {
			i_want_new_cluster();
		}

	}
	curr = size;
	char buffer1[2048];
	myKernel->readCluster(info_pos[0], buffer1);
	char* new_size = new char[4];
	new_size = (char*)(&size);
	
	buffer1[info_pos[1] * 32 + 16] = new_size[0];
	buffer1[info_pos[1] * 32 + 17] = new_size[1];
	buffer1[info_pos[1] * 32 + 18] = new_size[2];
	buffer1[info_pos[1] * 32 + 19] = new_size[3];
	myKernel->writeCluster(info_pos[0], buffer1);
	return '1';
}

BytesCnt KernelFile::read(BytesCnt bytes, char* buffer) {
	
	if (!(rw & 1)) return 0;
	if (curr == size) return 0;
	int count = 0;
	int position = curr % 2048;
	if ((curr != 0) && (curr / 2048 > 0) && (curr % 2048 == 0)) {
		give_me_next_cluster();
	}
	while (count < bytes) {
		//if (prom) cout << curr_cluster << " ";
		myKernel->readCluster(curr_cluster, content);
		while (position!=2048 && count < bytes) {
			buffer[count++] = content[position++];
			curr++;
			if (curr == size) return count;
		}
		if (count < bytes) {
			give_me_next_cluster();
		}
		position = 0;
	}
	return count;
}

char KernelFile::seek(BytesCnt bytes) {
	if (!(rw & 3)) return '0';
	if (bytes > size) return '0';
	curr = bytes;
	
	int clusterNo = curr / 2048;
	int entry = clusterNo / 512;
	
	myKernel->readCluster(level_one, content);
	char* number = new char[4];
	number[0] = content[entry * 4    ];
	number[1] = content[entry * 4 + 1];
	number[2] = content[entry * 4 + 2];
	number[3] = content[entry * 4 + 3];
	int* num = (int*)number;

	entry = clusterNo % 512;
	myKernel->readCluster(*num, content);
	number[0] = content[entry * 4];
	number[1] = content[entry * 4 + 1];
	number[2] = content[entry * 4 + 2];
	number[3] = content[entry * 4 + 3];
	num = (int*)number;

	curr_cluster = *num;
	return '1';
}

BytesCnt KernelFile::filePos() {
	if (!(rw &3)) return 0;
	return curr;
}

char KernelFile::eof() {

	if (!(rw & 3)) return 1;
	if (curr == this->size) {
		return 2;
	}
	else {
		return 0;
	}
}

BytesCnt KernelFile::getFileSize() {
	if (!(rw & 3)) return 0;
	return this->size;
}

char KernelFile::truncate() {
	if (!(rw & 2)) return '0';
	if (size == curr) return '0';
	if (size == 0) return '1';
	int clusterNo;
	if (curr % 2048 == 0) clusterNo = curr / 2048;
	else clusterNo = curr / 2048 + 1;
	int entry2 = clusterNo % 512;
	int entry1 = clusterNo / 512;
	myKernel->free_clusters(level_one,entry1,entry2,size-curr);
	size = curr;
	return '1';
}

void KernelFile::i_want_new_cluster() {
	wait(take);
	clusters_used++;
	int entry = (clusters_used-1) / 512;
	char index[2048];
	myKernel->readCluster(level_one,index);

	int* value;
	char* valuee=new char[4];
	valuee[0] = index[entry * 4];
	valuee[1] = index[entry * 4 + 1];
	valuee[2] = index[entry * 4 + 2];
	valuee[3] = index[entry * 4 + 3];
	value = (int*)valuee;
	if (*value == -1) {
		int* free = new int();
		*free = myKernel->first_free_cluster();
		myKernel->set_v(*free);
		valuee = (char*)free;
		index[entry * 4] = valuee[0];
		index[entry * 4 + 1] = valuee[1];
		index[entry * 4 + 2] = valuee[2];
		index[entry * 4 + 3] = valuee[3];
		myKernel->writeCluster(level_one, index);
		*value = *free;
		
	}
	if (clusters_used % 512 == 0) entry = (clusters_used - 1) % 512;
	else entry = (clusters_used % 512 -1);              //entry = 0;
	
	myKernel->readCluster(*value, index);
	int* free=new int();
	*free = myKernel->first_free_cluster();
	myKernel->set_v(*free);
	valuee = (char*)free;
	index[entry * 4    ] = valuee[0] ;
	index[entry * 4 + 1] = valuee[1];
	index[entry * 4 + 2] = valuee[2];
	index[entry * 4 + 3] = valuee[3];
	myKernel->writeCluster(*value, index);
	end_of_last_cluster = 0;
	last_cluster = *free;
	if (curr_cluster == 0) curr_cluster = last_cluster;
	signal(take);
}

void KernelFile::give_me_next_cluster() {

	int clusterNo = curr / 2048;
	int index2 = clusterNo % 512;
	int index1 = clusterNo / 512;
	char buffer[2048];
	myKernel->readCluster(level_one,buffer);
	int* num;
	char numm[4];
	numm[0] = buffer[index1 * 4];
	numm[1] = buffer[index1 * 4 + 1];
	numm[2] = buffer[index1 * 4 + 2];
	numm[3] = buffer[index1 * 4 + 3];
	num = (int*)numm;
	myKernel->readCluster(*num, buffer);
	numm[0] = buffer[index2 * 4];
	numm[1] = buffer[index2 * 4 + 1];
	numm[2] = buffer[index2 * 4 + 2];
	numm[3] = buffer[index2 * 4 + 3];
	num = (int*)numm;
	//if (curr_cluster == 515) cout << "aaaaaaaaaaaaaaaaaaaaaaa" << *num<<endl;
	curr_cluster = *num;
}

int KernelFile::fja() {
	prom = true;
	return 0;
}
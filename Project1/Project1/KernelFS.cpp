#include "KernelFS.h"
#include <iostream>
#include <vector>
#include <bitset>
#include "Def.h"
#include "file.h"
using namespace std;

const char KernelFS::all_ones = 1 | 2 | 4 | 8 | 16 | 32 | 64 | 128;
char KernelFS::valid_init[2048];
char KernelFS::root_init[2048];

KernelFS::KernelFS() {
	this->can_open = true;
	this->part = nullptr;
	this->mount_w = 0;
	this->number_of_open = 0;
	this->unmount_sem = CreateSemaphore(NULL, 0, 32, NULL);
	this->closed_sem = CreateSemaphore(NULL, 0, 32, NULL);
	this->mutex1= CreateSemaphore(NULL, 1, 32, NULL);
	memset(valid_init, all_ones, 2048);
	valid_init[0] = 1 | 2 | 4 | 8 | 16 | 32;
	memset(root_init, -1, 2048);
}

char KernelFS::mount(Partition* partition) {
	if (part) { 
		mount_w++;
		wait(unmount_sem); 
		mount_w--;
	}
	part = partition;
	return '1';
}

char KernelFS::unmount() {
	if (part == nullptr) return '0';
	if (number_of_open) wait(closed_sem);
	part = nullptr;
	if (mount_w > 0) signal(unmount_sem);
	return '1';
}

char KernelFS::format() {
	if (part == nullptr) return '0';
	bool can_open = false;
	if (number_of_open) wait(closed_sem);
	number_of_files = 0;
	mount_w = 0;                          //Pitanje je da li ovo treba
	part->writeCluster(0, valid_init);
	for (int i = 1; i < 10000; i++) part->writeCluster(i, root_init);
	return '1';
}

FileCnt KernelFS::readRootDir() {
	if (part == nullptr) return -1;
	return number_of_files;
}

char KernelFS::doesExist(char* fname) {
	if (part == nullptr) return '0';
	int* positions = where_is_file(fname);
	if (positions == nullptr) return '0';
	return '1';
}

File* KernelFS::open(char* fname, char mode) {
	
	if (part == nullptr) return 0;
	if (mode == 'r' || mode == 'a') {
		if (number_of_files == 0) return nullptr; // Greska
		int* position = where_is_file(fname);
		if (position == nullptr) return nullptr;  // Greska

		char buffer[2048];
		part->readCluster(position[0], buffer);

		char open = buffer[position[1] * 32 + 20];
		char* size=new char[4];
		char* index=new char[4];
		index[0] = buffer[position[1] * 32 + 12];
		index[1] = buffer[position[1] * 32 + 13];
		index[2] = buffer[position[1] * 32 + 14];
		index[3] = buffer[position[1] * 32 + 15];
		
		size[0] = buffer[position[1] * 32 + 16];
		size[1] = buffer[position[1] * 32 + 17];
		size[2] = buffer[position[1] * 32 + 18];
		size[3] = buffer[position[1] * 32 + 19];

		if (mode == 'r') {
			while (open & (1 << 1)) {
				wait(mutex1);
				part->readCluster(position[0], buffer);
			    open = buffer[position[1] * 32 + 20];
				if (open & (1 << 1)) signal(mutex1);
			}
			index[0] = buffer[position[1] * 32 + 12];
			index[1] = buffer[position[1] * 32 + 13];
			index[2] = buffer[position[1] * 32 + 14];
			index[3] = buffer[position[1] * 32 + 15];

			size[0] = buffer[position[1] * 32 + 16];
			size[1] = buffer[position[1] * 32 + 17];
			size[2] = buffer[position[1] * 32 + 18];
			size[3] = buffer[position[1] * 32 + 19];
	
			buffer[position[1] * 32 + 20] |= 1;
			part->writeCluster(position[0], buffer);
			number_of_open++;
			return new File(position,*(int*)index,*(int*)size,this,buffer[position[1]*32+20],0); //Ovde se vraca vrednost tipa File* ali mozda treba otici skroz do fajla
		}

		if (mode == 'a') {
			while (open & ((1 << 1) | 1)) {
				wait(mutex1);
				part->readCluster(position[0], buffer);
				char open = buffer[position[1] * 32 + 20];
				if (open & ((1 << 1) | 1)) signal(mutex1);
			}
			index[0] = buffer[position[1] * 32 + 12];
			index[1] = buffer[position[1] * 32 + 13];
			index[2] = buffer[position[1] * 32 + 14];
			index[3] = buffer[position[1] * 32 + 15];

			size[0] = buffer[position[1] * 32 + 16];
			size[1] = buffer[position[1] * 32 + 17];
			size[2] = buffer[position[1] * 32 + 18];
			size[3] = buffer[position[1] * 32 + 19];
			buffer[position[1] * 32 + 20] |= 3;
			part->writeCluster(position[0], buffer);
			number_of_open++;
			return new File(position, *(int*)index, *(int*)size, this, buffer[position[1] * 32 + 20],*(int*)size);        //Ovde se vraca dobra vrednost
		}
	}  
	if (mode == 'w') {
		int* position = where_is_file(fname);
		if (position != nullptr) {
			char buffer[2048];
			part->readCluster(position[0], buffer);
			char open = buffer[position[1] * 32 + 20];
			
			while (open & ((1 << 1) | 1)) {
				wait(mutex1);
				part->readCluster(position[0], buffer);
				char open = buffer[position[1] * 32 + 20];
				if (open & ((1 << 1) | 1)) signal(mutex1);
			}
			deleteFile(fname);
			this->open(fname, mode);
		}
		if (position == nullptr) {
			//Trazimo poziciju odnosno klaster u kome se nalaze podaci
			position = find_free_space();
			
			set_v(position[0]);
			char buffer[2048];
			part->readCluster(position[0], buffer);
			int curr_position = position[1] * 32;
			
			int i = 0;
			while (fname[i] != '.') {
				buffer[curr_position] = fname[i];
				i++;
				curr_position++;
			}
			int j = i;
			while (j < 8) {
				buffer[curr_position++] = ' ';
				j++;
			}
			i++;
			j = 0;
			while (fname[i] && j<3) {
				buffer[curr_position++] = fname[i++];
				j++;
			}
			while (j < 3) {
				buffer[curr_position++] = ' ';
				i++;
			}
			//Ovde smo zavrsili sa upisivanje imena fajla i ekstenzije sad idemo na popunjavanje ostalih bajtova
			buffer[curr_position++] = '0';

			char* size=new char[4];
			int* sizee = new int();
			*sizee = 0;
			size = (char*)sizee;

			/*buffer[curr_position++] = '0';			
			buffer[curr_position++] = '0';
			buffer[curr_position++] = '0';
			buffer[curr_position++] = '0';

			buffer[curr_position++] = '0';
			buffer[curr_position++] = '0';
			buffer[curr_position++] = '0';
			buffer[curr_position++] = '0';*/
			buffer[curr_position++] = 0;
			buffer[curr_position++] = 0;
			buffer[curr_position++] = 0;
			buffer[curr_position++] = 0;

			buffer[curr_position++] = 0;
			buffer[curr_position++] = 0;
			buffer[curr_position++] = 0;
			buffer[curr_position++] = 0;

			buffer[curr_position++] = 3;
			buffer[curr_position] = 'Y';
			number_of_files++;
			number_of_open++;
			part->writeCluster(position[0], buffer);
			return new File(position, 0, 0, this, buffer[position[1] * 32 + 20],0);
		}
	}
	
}

char KernelFS::deleteFile(char* fname) {
	int count = number_of_files;
	if (part == nullptr) return '0';
	char* buffer = new char[2048];
	part->readCluster(1, buffer);   //Citanje klastera

	char* num = new char[4];
	num[0] = buffer[0]; num[1] = buffer[1]; num[2] = buffer[2]; num[3] = buffer[3];
	int number = *((int*)num);                                                        // Uzimanje 32 bita(int)

	int pos = 0;
	int new_pos = 32;
	int step = 4;
	char index[4];
	char size[4];
	while (number != -1) {
		char* buffer_new = new char[2048];
		part->readCluster(number, buffer_new);

		for (int i = 0; i < 64; i++) {                     //Petlja ide do 64 sto znaci da cita sve do kraja klastera
			char* name = new char[12];
			int j = 0;

			if (buffer_new[pos+21] != 'Y') {                   //Ako je Y znaci da je neki fajl koji je tu bio ranije obrisan i idemo na sledeci ulaz
				pos = new_pos;
				new_pos += 32;
				continue;
			}

			while ((buffer_new[pos] != ' ') && (j < 8)) {
				name[j++] = buffer_new[pos++];
			}
			pos += (8 - j);
			name[j++] = '.';
			int more = 0;

			while (buffer_new[pos] != ' ' && more < 3) {
				name[j + more] = buffer_new[pos++];
				more++;
			}
			name[j + more] = '\0';
			if (strcmp(name, fname) == 0) {
				int old_pos = pos - (pos % 32);
				int pos = old_pos + 20;
				
				char open = buffer_new[pos];          //Uzimam 21 bajt da vidim da li je otvoren
				char maska = (1 | (1 << 1));
				if (open & maska) return '0';

				buffer_new[old_pos + 21] = 'X';
				part->writeCluster(number, buffer_new);
				number_of_files--;

				
				index[0] = buffer_new[old_pos + 12];
				index[1] = buffer_new[old_pos + 13];
				index[2] = buffer_new[old_pos + 14];
				index[3] = buffer_new[old_pos + 15];
				
				size[0] = buffer_new[old_pos + 16];
				size[1] = buffer_new[old_pos + 17];
				size[2] = buffer_new[old_pos + 18];
				size[3] = buffer_new[old_pos + 19];

				free_clusters(*(int*)index,0,0,*(int*)size);
				return '1';
				//Kad se stavi X u 22 bajt ulaza to znaci da je ulaz slobodan odnosno da ne sadrzi podatke ni o kakvom fajlu
				//Ovde je nadjen fajl sada ga treba obrisati
				//Treba dodati i da ako je ovo jedini fajl da se postavi na 0 prvi nivo
			}
			if (--count == 0) return '0';
			pos = new_pos;
			new_pos += 32;
		}
		num[0] = buffer[step + 0]; num[1] = buffer[step + 1]; num[2] = buffer[step + 2]; num[3] = buffer[step + 3];
		number = *((int*)num);
		step += 4;
	}
	return '0';
}

bool KernelFS::get_valid(int clusterNo){
	char buffer[2048];
	part->readCluster(0, buffer);
	int byte = clusterNo / 8;
	int num = clusterNo % 8;
	char number = buffer[byte];
	char maska = (1 << (7 - num));
	return (number & maska);
}

int KernelFS::first_free_cluster() {
	int number_of_clusters = part->getNumOfClusters();
	for (int i = 0; i < number_of_clusters; i++) {
		if (get_valid(i)) return i;
	}
}

int* KernelFS::where_is_file(char* fname) {
	int* result = new int[2]; // U ovom nizu se smesta rezultat
	int cnt = number_of_files;
	char* buffer = new char[2048];
	part->readCluster(1, buffer);   //Citanje klastera

	char* num = new char[4];
	num[0] = buffer[0]; num[1] = buffer[1]; num[2] = buffer[2]; num[3] = buffer[3];
	int number = *((int*)num);                                                        // Uzimanje 32 bita(int)

	int pos = 0;
	int new_pos = 32;
	int step = 4;
	char buffer_new[2048];
	while (number != -1) {
		part->readCluster(number, buffer_new);

		for (int i = 0; i < 64; i++) {                     //Petlja ide do 64 sto znaci da cita sve do kraja klastera
			char* name=new char[12];
			int j = 0;

			char c = buffer_new[pos + 21];
			
			if (c != 'Y') {                   //Ako je X znaci da je neki fajl koji je tu bio ranije obrisan i idemo na sledeci ulaz
				pos = new_pos;
				new_pos += 32;
				continue;
			}
			while ((buffer_new[pos] != ' ') && (j < 8)) {
				name[j++] = buffer_new[pos++];
			}

			pos += (8 - j);
			name[j++] = '.';
			int more = 0;

			while (buffer_new[pos] != ' ' && more < 3) {
				name[j + more] = buffer_new[pos++];
				more++;
			}
			name[j + more] = '\0';
			//Mozda treba na kraj imena da se stavi /0
			if (strcmp(name, fname) == 0) {
				result[0] = number;
				result[1] = pos / 32;
				return result;
			}
			if (--cnt == 0) return nullptr;
			pos = new_pos;
			new_pos += 32;
		}
		num[0] = buffer[step + 0]; num[1] = buffer[step + 1]; num[2] = buffer[step + 2]; num[3] = buffer[step + 3];
		number = *((int*)num);
		step += 4;
	}
	return nullptr;
}

int* KernelFS::find_free_space() {
	int* position = new int[2];
	char buffer[2048];
	part->readCluster(1, buffer);

	if (number_of_files % 64 == 0) {
		position[0] = first_free_cluster();
		int entry = number_of_files / 64;

		int* value = new int();
		*value = position[0];
		char* valuee = new char[4];
		valuee = (char*)value;

		buffer[entry * 4] = valuee[0];
		buffer[entry * 4 + 1] = valuee[1];
		buffer[entry * 4 + 2] = valuee[2];
		buffer[entry * 4 + 3] = valuee[3];

		part->writeCluster(1, buffer);
	}
	else {
		
		int entry = number_of_files / 64;
		int* value;
		char* valuee = new char[4];
		valuee[0] = buffer[entry * 4    ];
		valuee[1] = buffer[entry * 4 + 1];
		valuee[2] = buffer[entry * 4 + 2];
		valuee[3] = buffer[entry * 4 + 3];
		value = (int*)valuee;
		position[0] = *value;
	}
	part->readCluster(position[0], buffer); //Nasli smo slobodan cluster sad trazimo slobodan ulaz
	int i = 21;
	while (buffer[i] == 'Y') i += 32;
	position[1] = i / 32;
	return position;
}

void KernelFS::readCluster(int ClusterNo, char* buffer) {
	part->readCluster(ClusterNo, buffer);
}

void KernelFS::writeCluster(int ClusterNo, char* buffer) {
	part->writeCluster(ClusterNo, buffer);
}

void KernelFS::free_clusters(int index1,int entry1,int entry2,int size) {
	
	char buffer[2048];
	part->readCluster(index1, buffer);
	if (index1 == 0) return;
	char number[4];
	number[0] = buffer[entry1]; number[1] = buffer[entry1+1]; number[2] = buffer[entry1+2]; number[3] = buffer[entry1+3];
	int* num = (int*)number;
	
	int step = entry1 + 4;
	char buffer1[2048];

	while (size > 0) {
		int position = 0;
		if (step == entry1 + 4) position = entry2 * 4;
		int old_position = position;
		
		while (position < 2048) {
			part->readCluster(*num, buffer1);
			char number[4];
			number[0] = buffer1[position++]; 
			number[1] = buffer1[position++]; 
			number[2] = buffer1[position++]; 
			number[3] = buffer1[position++];
			int* num = (int*)number;
			free_cluster(*num);
			size-=2048;
			if (size <= 0) { 
				if (entry1==0 && entry2==0)free_cluster(index1);
				return; 
			}
		}
		if (old_position==0)free_cluster(*num);
		number[0] = buffer[step++]; number[1] = buffer[step++]; number[2] = buffer[step++]; number[3] = buffer[step++];
		int* num = (int*)number;
	}
	exit(1);
	if (entry1==0 && entry2==0)free_cluster(index1);
}

void KernelFS::set_v(int i) {
	int byte = i / 8;
	char buffer[2048];
	part->readCluster(0,buffer);

	char maska = ~(1 << (7 - (i % 8)));
	buffer[byte] = buffer[byte] & maska;
	
	part->writeCluster(0, buffer);
}

void KernelFS::close_File() {
	number_of_open--;
	signal(mutex1);
	if (number_of_open == 0) {
		signal(closed_sem);
	}
	
}

void KernelFS::free_cluster(int ClusterNo) {
	int i = ClusterNo;
	int byte = i / 8;
	char buffer[2048];
	part->readCluster(0, buffer);

	char maska = (1 << (7 - (i % 8)));
	buffer[byte] = buffer[byte] | maska;

	part->writeCluster(0, buffer);
}
#define _CRT_SECURE_NO_WARNINGS
#define DOWNLOAD 1
#define UPLOAD 2
#define REMOVE 3
#define EXIT 4
#define PORT "1234"

#pragma comment(lib, "rpcrt4.lib")

#include <iostream>
#include "Interface_h.h"
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
using namespace std;
RPC_BINDING_HANDLE hBinding = NULL;
string Path;

void* __RPC_USER midl_user_allocate(size_t size) {
	return malloc(size);
}
void __RPC_USER midl_user_free(void* p) {
	free(p);
}

void Download() {
	cout << "Enter the path of the file you want to copy:" << endl;
	Path.clear();
	cin >> Path;
	unsigned int Size = Get_Size_File(hBinding, (unsigned char*)Path.c_str());
	if (Size == 0) {
		cout << "Error: invalid path" << endl;
		return;
	}
	else {
		unsigned char* Arr = new unsigned char[Size + 1];
		if (Download_File(hBinding, (unsigned char*)Path.c_str(), Arr, Size)) {
			cout << "Enter the path of the file you want to save the data to:" << endl;
			Path.clear();
			cin >> Path;
			ofstream File((const char*)Path.c_str(), ios::binary);
			if (File.is_open()) {
				File.write((const char*)Arr, (streamsize)Size);
				File.close();
				cout << "The file has been uploaded successfully!" << endl;
			}
			else cout << "Error: invalid file path on client!" << endl;
		}
		else {
			cout << "Error: invalid file path on server" << endl;
			return;
		}
	}
}
void Upload() {
	cout << "Enter the path of the file you want to upload:" << endl;
	Path.clear();
	cin >> Path;

	ifstream File((const char*)Path.c_str(), ios::binary);
	if (File.is_open()) {
		const auto Begin = File.tellg();
		File.seekg(0, ios::end);
		const auto End = File.tellg();
		long Size = (long)(End - Begin);
		File.close();
		ifstream File((const char*)Path.c_str(), ios::binary);
		unsigned char* Arr = new unsigned char[Size + 1];

		File.read((char*)Arr, (streamsize)Size);
		File.close();
		cout << "Enter the path of the file where you want to save it on the server:" << endl;
		Path.clear();
		cin >> Path;
		if (Upload_File(hBinding, (unsigned char*)Path.c_str(), Arr, Size)) {
			cout << "The file was successfully uploaded!" << endl;
			return;
		}
		else {
			cout << "Error: invalid file path on server!" << endl;
			return;
		}
	}
	else cout << "ERROR: invalid file path on client!" << endl;
}
void Remove() {
	cout << "Enter the path of the file you want to delete:" << endl;
	Path.clear();
	cin >> Path;
	if (Delete_File(hBinding, (unsigned char*)Path.c_str()))
		cout << "The file was successfully deleted!" << endl;
	else
		cout << "Error: invalid file path" << endl;
}
int Menu_Init(void) {
	char Choice = 0;
	cout << "Select an action:\n1:Enter a new user\n2:Exit" << endl;
	cin >> Choice;
	while ((getchar()) != '\n');
	if (Choice == '1') return 0;
	else return 1;
}
void Menu(void) {
	while (true) {
		char Choice = 0;
		cout << "Select an action:\n1:Download File\n2:Upload File\n3:Remove File\n4:Exit" << endl;
		cin >> Choice;
		while ((getchar()) != '\n');
		if (Choice == '4') break;
		else if (Choice == '1') {
			Download();
			continue;
		}
		else if (Choice == '2') {
			Upload();
			continue;
		}
		else if (Choice == '3') {
			Remove();
			continue;
		}
		else cout << "Input error. Try again" << endl;
	}
}
void Check_Status(RPC_STATUS Status) {
	if (Status != RPC_S_OK) { //RPC_S_OK = 0
		cout << "Error code: " << Status << endl;
		exit(Status);
	}
}
int Login() {
	string UserName;
	string Password;
	while (true) {
		cout << "Enter username: " << endl;
		cin >> UserName;
		cout << "Enter password: " << endl;
		cin >> Password;
		if (Authentication(hBinding, (unsigned char*)UserName.c_str(), (unsigned char*)Password.c_str())) return 0;
		if (UserName == "exit" || Password == "exit")return 1;
		cout << "Error when entering data. Try again" << endl;
		UserName.clear();
		Password.clear();
	}
}
int Connect() {
	RPC_STATUS Status = 0;
	RPC_CSTR StringBinding;

	string IP;
	cout << "Enter server IP: " << endl;
	cin >> IP;
	//Создает дескриптор привязки строки
	Status = RpcStringBindingComposeA(NULL, (RPC_CSTR)("ncacn_ip_tcp"), (RPC_CSTR)(IP.c_str()), (RPC_CSTR)(PORT), NULL, &StringBinding);
	Check_Status(Status);
	//Возвращает дескриптор привязки из строкового представления дескриптора привязки
	Status = RpcBindingFromStringBindingA(StringBinding, &hBinding);
	Check_Status(Status);
	if (Login())return 0;
}
int main(void*) {
	Connect();
	while (1) {
		Menu();
		if (Menu_Init())return 0;
		else {
			if (Login())return 0;
		}
	}
	return 0;
}
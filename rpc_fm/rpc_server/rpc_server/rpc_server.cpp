#define _CRT_SECURE_NO_WARNINGS
#define ERROR_FUNCTION 1
#define PORT "1234"

#pragma comment(lib, "rpcrt4.lib")

#include <iostream>
#include "Interface_h.h"
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>

using namespace std;
RPC_BINDING_HANDLE hToken;
char UserName[128] = { 0 };

void* __RPC_USER MIDL_user_allocate(size_t size) {
	return malloc(size);
}

void __RPC_USER MIDL_user_free(void* p) {
	free(p);
}
int Exit_user(handle_t hBinding) {
	cout << "User " << UserName << " has finished the session!" << endl;
	memset(UserName, 0, 100);
	return 0;
}
long Get_Size_File(handle_t hBinding, unsigned char* Path) {
	if (!ImpersonateLoggedOnUser(hToken)) {
		Exit_user(hBinding);
		return false;
	}
	ifstream File((char*)Path, ios::binary);
	if (File.is_open()) {
		const auto Begin = File.tellg();
		File.seekg(0, ios::end);
		const auto End = File.tellg();
		long Size = (long)(End - Begin);
		File.close();

		cout << "The file " << Path << " has a size of " << Size << endl;
		return (long)Size;
	}
	else {
		cout << "Access error when getting file size " << Path << " by " << UserName << endl;
		return 0;
	}
}
int Upload_File(handle_t hBinding, unsigned char* Path, unsigned char* Arr, long Size) {
	if (!ImpersonateLoggedOnUser(hToken)) {
		Exit_user(hBinding);
		return false;
	}
	ofstream File((const char*)Path, ios::binary);
	if (File.is_open()) {
		File.write((char*)Arr, (streamsize)Size);
		File.close();
		cout << "The file " << Path << " has been uploaded successfully by user " << UserName << "!" << endl;
		return true;
	}
	else {
		cout << "Access error when uploading a file " << Path << " by " << UserName << endl;
		return false;
	}
	return false;

}
int Download_File(handle_t hBinding, unsigned char* Path, unsigned char* Arr, long Size) {
	if (!ImpersonateLoggedOnUser(hToken)) {
		Exit_user(hBinding);
		return false;
	}
	ifstream File((const char*)Path, ios::binary);
	if (File.is_open()) {
		File.read((char*)Arr, (streamsize)Size);
		File.close();
		cout << "The file " << Path << " has been copied successfully by " << UserName << "!" << endl;
		return true;
	}
	else {
		cout << "Access error when downloading a file " << Path << " by " << UserName << endl;
		return false;
	}
	return false;

}
int Delete_File(handle_t hBinding, const unsigned char* Path) {
	//Проверка пользователя
	if (!ImpersonateLoggedOnUser(hToken)) {
		Exit_user(hBinding);
		return false;
	}
	//Удаление
	bool Status = remove((LPSTR)Path);
	if (!Status) {
		cout << "File " << Path << " was deleted by user " << UserName << "!" << endl;
		return true;
	}
	else {
		cout << "Access error when deleting a file " << Path << " by " << UserName << endl;
		return false;
	}
}

int Authentication(handle_t hBinding, unsigned char* Login, unsigned char* Password) {
	for (int i = 0; i < 100; i++) {
		UserName[i] = Login[i];
	}
	//Пытается авторизоваться на локальном компьютере
	if (LogonUserA((LPCSTR)Login, NULL, (LPCSTR)Password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hBinding) && ImpersonateLoggedOnUser(hBinding)) {
		cout << "The user " << Login << " has connected!" << endl;
		hToken = hBinding;
		return true;
	}
	//Если это не удалось сделать
	else {
		cout << "Error when entering user data" << endl;
		return false;
	}
}
void Check_Status(RPC_STATUS Status) {
	if (Status != RPC_S_OK) { //RPC_S_OK = 0
		cout << "Error code: " << Status << endl;
		exit(Status);
	}
}
int main(void) {
	RPC_STATUS Status = 0;
	//Для регистрации одной последовательности протокола в библиотеке времени
	Status = RpcServerUseProtseqEpA((RPC_CSTR)("ncacn_ip_tcp"), RPC_C_PROTSEQ_MAX_REQS_DEFAULT, (RPC_CSTR)(PORT), NULL);
	//Проверка успешного выполнения функции
	Check_Status(Status);

	Status = RpcServerRegisterIf2(INTERFACENAME_v1_0_s_ifspec, NULL, NULL, RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH, RPC_C_LISTEN_MAX_CALLS_DEFAULT, -1, NULL);
	Check_Status(Status);

	std::cout << "The server is waiting for connection..." << endl;

	Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 0);

	return Status;
}

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <string>
#include <Lm.h>
#include <sddl.h>
#include <Lsalookup.h>
#include <Ntsecapi.h>
#include <stdlib.h>
#include <GPEdit.h>
#include <comdef.h>


using namespace std;

NTSTATUS(WINAPI* LsaOpenPolicy_)(PLSA_UNICODE_STRING, PLSA_OBJECT_ATTRIBUTES, ACCESS_MASK, PLSA_HANDLE);

// Объявления указателей функций и переменных 
NET_API_STATUS(NET_API_FUNCTION* NetUserEnum_)(LPCWSTR, DWORD, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD, PDWORD);            // получение всех пользователей
BOOL(WINAPI* LookupAccountNameW_)(LPCWSTR, LPCWSTR, PSID, LPDWORD, LPWSTR, LPDWORD, PSID_NAME_USE);                         // получение информации о пользователе
BOOL(WINAPI* ConvertSidToStringSidW_)(PSID, LPWSTR*);                                                                       // перевод SID в строковый вид
NTSTATUS(WINAPI* LsaEnumerateAccountRights_)(LSA_HANDLE, PSID, PLSA_UNICODE_STRING*, PULONG);                               // получение прав
NET_API_STATUS(NET_API_FUNCTION* NetApiBufferFree_)(_Frees_ptr_opt_ LPVOID);                                                // освобождение памяти
NET_API_STATUS(NET_API_FUNCTION* NetUserGetLocalGroups_)(LPCWSTR, LPCWSTR, DWORD, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD); // получение списка групп, в которых состоит пользователь
NET_API_STATUS(NET_API_FUNCTION* NetLocalGroupEnum_)(LPCWSTR, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD, PDWORD_PTR);         // получение списка локальных групп
NET_API_STATUS(NET_API_FUNCTION* NetUserAdd_)(LPCWSTR, DWORD, LPBYTE, LPDWORD);                                             // добавление пользователя
NET_API_STATUS(NET_API_FUNCTION* NetUserDel_)(LPCWSTR, LPCWSTR);                                                            // удаление пользователя
NET_API_STATUS(NET_API_FUNCTION* NetLocalGroupAdd_)(LPCWSTR, DWORD, LPBYTE, LPDWORD);                                       // добавление локальной группы в систему
NET_API_STATUS(NET_API_FUNCTION* NetLocalGroupDel_)(LPCWSTR, LPCWSTR);                                                      // удаление группы из системы
NTSTATUS(WINAPI* LsaAddAccountRights_)(LSA_HANDLE, PSID, PLSA_UNICODE_STRING, ULONG);                                       // добавление указанного права для пользователя
NTSTATUS(WINAPI* LsaRemoveAccountRights_)(LSA_HANDLE, PSID, BOOLEAN, PLSA_UNICODE_STRING, ULONG);                           // удаление указанного права
NET_API_STATUS(NET_API_FUNCTION* NetLocalGroupAddMembers_)(LPCWSTR, LPCWSTR, DWORD, LPBYTE, DWORD);                         // добавление пользователей в группу
NET_API_STATUS(NET_API_FUNCTION* NetLocalGroupDelMembers_)(LPCWSTR, LPCWSTR, DWORD, LPBYTE, DWORD);                         // удаление пользователей

LSA_OBJECT_ATTRIBUTES ObjectAttributes;
LSA_HANDLE PolicyHandle;

PSID GetSID(LPCWSTR name, SID_NAME_USE sid_name_use) { // Поиск SID по имени
    DWORD cbSid = 0; //размер
    DWORD cchReferencedDomainName = 0; //имя домена
    PSID sid = 0;
    LPWSTR domain = 0;
    // Первичный вызов для получения необходимых размеров для SID и домена
    LookupAccountNameW_(NULL, name, NULL, &cbSid, NULL, &cchReferencedDomainName, &sid_name_use);
    sid = (PSID)calloc(1, cbSid);
    domain = (LPWSTR)calloc(1, sizeof(TCHAR) * (cchReferencedDomainName));
    // Вторичный вызов для получения реального SID и имени домена
    LookupAccountNameW_(NULL, name, sid, &cbSid, domain, &cchReferencedDomainName, &sid_name_use);
    return sid;
}

struct RightDescription {
    LPCWSTR rightName;
    LPCWSTR description;
};
// Список прав и их описания
RightDescription rightsList[] = {
    { L"SeInteractiveLogonRight", L"Разрешает локальный вход в систему" },
    { L"SeRemoteInteractiveLogonRight", L"Разрешает удаленный вход через RDP" },
    { L"SeBackupPrivilege", L"Позволяет создавать резервные копии файлов и каталогов" },
    { L"SeRestorePrivilege", L"Позволяет восстанавливать файлы и каталоги из резервных копий" },
    { L"SeShutdownPrivilege", L"Позволяет завершать работу системы" },
    { L"SeDebugPrivilege", L"Позволяет отлаживать процессы и изменять память других процессов" },
    { L"SeChangeNotifyPrivilege", L"Позволяет обходить проверку обхода дерева каталогов" },
    { L"SeSecurityPrivilege", L"Позволяет управлять аудитом и журналом безопасности" },
    { L"SeTakeOwnershipPrivilege", L"Позволяет брать владение объектами" },
    { L"SeLoadDriverPrivilege", L"Позволяет загружать и выгружать драйверы устройств" },
    { L"SeSystemtimePrivilege", L"Позволяет изменять системное время" },
    { L"SeCreateTokenPrivilege", L"Позволяет создавать токены доступа" },
    { L"SeAssignPrimaryTokenPrivilege", L"Позволяет назначать первичный токен процессу" },
    { L"SeIncreaseQuotaPrivilege", L"Позволяет увеличивать квоты памяти для процессов" },
    { L"SeMachineAccountPrivilege", L"Позволяет добавлять компьютеры в домен" },
    { L"SeTcbPrivilege", L"Позволяет действовать как часть операционной системы" },
    { L"SeCreatePermanentPrivilege", L"Позволяет создавать постоянные объекты" },
    { L"SeAuditPrivilege", L"Позволяет генерировать записи аудита в журнале безопасности" },
    { L"SeImpersonatePrivilege", L"Позволяет олицетворять клиента после аутентификации" },
    { L"SeIncreaseBasePriorityPrivilege", L"Позволяет увеличивать базовый приоритет процесса" },
    { L"SeLockMemoryPrivilege", L"Позволяет блокировать страницы памяти в оперативной памяти" },
    { L"SeProfileSingleProcessPrivilege", L"Позволяет профилировать один процесс" },
    { L"SeSystemProfilePrivilege", L"Позволяет профилировать всю систему" },
    { L"SeUndockPrivilege", L"Позволяет отсоединять компьютер от док-станции" },
    { L"SeManageVolumePrivilege", L"Позволяет управлять томами файловой системы" },
    { L"SeRemoteShutdownPrivilege", L"Позволяет удаленно завершать работу системы" },
    { L"SeSyncAgentPrivilege", L"Позволяет синхронизировать данные каталогов" },
    { L"SeEnableDelegationPrivilege", L"Позволяет включать делегирование учетных записей" },
    { L"SeCreateGlobalPrivilege", L"Позволяет создавать глобальные объекты" },
    { L"SeTrustedCredManAccessPrivilege", L"Позволяет получать доступ к диспетчеру учетных данных" },
    { L"SeRelabelPrivilege", L"Позволяет изменять метки целостности объектов" },
    { L"SeIncreaseWorkingSetPrivilege", L"Позволяет увеличивать рабочий набор процесса" },
    { L"SeTimeZonePrivilege", L"Позволяет изменять часовой пояс системы" },
    { L"SeCreateSymbolicLinkPrivilege", L"Позволяет создавать символические ссылки" }
};

PSID GetGroupSID(const wchar_t* groupName) { // Поиск SID группы по имени
    PSID sid = nullptr;
    SID_NAME_USE sidType;
    wchar_t domainName[256];
    DWORD sidSize = 0;
    DWORD domainNameSize = 256;

    LookupAccountNameW(nullptr, groupName, nullptr, &sidSize, domainName, &domainNameSize, &sidType);
    sid = (PSID)malloc(sidSize);
    if (!LookupAccountNameW(nullptr, groupName, sid, &sidSize, domainName, &domainNameSize, &sidType)) {
        free(sid);
        return nullptr;
    }
    return sid;
}

void InfoAboutUser() {
    LPUSER_INFO_0 buffer = NULL;
    DWORD entriesread = 0; //успешно
    DWORD totalentries = 0; //все
    DWORD resume_handle = 0; // по страницам, если слишком большой

    NET_API_STATUS netapi_status = NetUserEnum_(NULL, 0, FILTER_NORMAL_ACCOUNT, (LPBYTE*)&buffer, MAX_PREFERRED_LENGTH, &entriesread, &totalentries, &resume_handle);
    if (netapi_status != NERR_Success) {
        cout << "Ошибка при получении списка пользователей" << endl;
    }

    PSID sid = 0;
    LPWSTR str_sid = NULL;
    for (DWORD i = 0; i < entriesread; i++) {
        sid = GetSID((buffer + i)->usri0_name, SidTypeUser);
        ConvertSidToStringSidW_(sid, &str_sid);

        wprintf(L"Пользователь: %s\n \t%s\n", (buffer + i)->usri0_name, str_sid);

        PLSA_UNICODE_STRING UserRights;
        ULONG CountOfRights = 0;

        LsaEnumerateAccountRights_(PolicyHandle, sid, &UserRights, &CountOfRights);
        cout << "\tПрава: " << endl;
        for (ULONG j = 0; j < CountOfRights; j++) {
            wprintf(L"\t\t%s\n", UserRights[j].Buffer);
        }

        LPLOCALGROUP_USERS_INFO_0 buff = NULL;
        DWORD group_count = 0;
        DWORD group_count_total = 0;
        PLSA_UNICODE_STRING Rights;
        ULONG CountOfRights_g = 0;
        PSID sid_g;
        LPWSTR name_ = (buffer + i)->usri0_name;
        NetUserGetLocalGroups_(NULL, name_, 0, LG_INCLUDE_INDIRECT, (LPBYTE*)&buff, MAX_PREFERRED_LENGTH, &group_count, &group_count_total);
        cout << "\tГруппы: " << endl;
        for (DWORD k = 0; k < group_count; k++) {
            wprintf(L"\t\t%s\n", (buff + k)->lgrui0_name);
            PSID groupSid = GetGroupSID((buff + k)->lgrui0_name);
            if (groupSid) {
                PLSA_UNICODE_STRING UserRights;
                ULONG CountOfRights = 0;
                if (LsaEnumerateAccountRights_(PolicyHandle, groupSid, &UserRights, &CountOfRights) == 0) {
                    cout << "\tПрава группы:" << endl;
                    for (ULONG j = 0; j < CountOfRights; j++) {
                        wcout << L"\t\t" << UserRights[j].Buffer << endl;
                    }
                }
                else {
                    wcerr << "Ошибка при получении прав группы" << endl;
                }

                free(groupSid);
            }
            else {
                wcerr << L"Ошибка при получении SID группы " << (buff + k)->lgrui0_name << endl;
            }
        }
    }

    NetApiBufferFree_(buffer);
}

void InfoAboutGroup() {
    PGROUP_INFO_0 buffer = NULL;
    DWORD entriesread = 0;
    DWORD totalentries = 0;
    PDWORD_PTR resume_handle = 0;
    PSID sid = 0;
    LPWSTR str_sid = NULL;

    NetLocalGroupEnum_(NULL, 0, (LPBYTE*)&buffer, MAX_PREFERRED_LENGTH, &entriesread, &totalentries, resume_handle);

    for (DWORD i = 0; i < entriesread; i++) {
        sid = GetSID((buffer + i)->grpi0_name, SidTypeGroup);
        ConvertSidToStringSidW_(sid, &str_sid);

        wprintf(L"Группа: %s\n \t%s\n", (buffer + i)->grpi0_name, str_sid);

        PLSA_UNICODE_STRING Rights;
        ULONG CountOfRights = 0;
        LsaEnumerateAccountRights_(PolicyHandle, sid, &Rights, &CountOfRights);
        cout << "\tПрава: " << endl;
        for (ULONG j = 0; j < CountOfRights; j++) {
            wprintf(L"\t\t%s\n", Rights[j].Buffer);
        }
    }
    NetApiBufferFree_(buffer);
}

void AddUser() {
    USER_INFO_1 user;
    wchar_t user_name[128];
    wchar_t password[128];
    cout << "Имя: ";
    wscanf(L"%s", &user_name);
    cout << "Пароль: ";
    wscanf(L"%s", &password);

    user.usri1_name = user_name;
    user.usri1_password = password;
    user.usri1_priv = USER_PRIV_USER;
    user.usri1_home_dir = (LPWSTR)TEXT("");
    user.usri1_comment = NULL;
    user.usri1_flags = UF_SCRIPT;
    user.usri1_script_path = (LPWSTR)TEXT("");

    NetUserAdd_(NULL, 1, (LPBYTE)&user, NULL);
}

void DeleteUser() {
    wchar_t user_name[128];
    cout << "Имя: ";
    wscanf(L"%s", &user_name);
    NetUserDel_(NULL, user_name);
}

void AddGroup() {
    _LOCALGROUP_INFO_0 buffer;
    wchar_t group_name[128];
    cout << "Имя: ";
    wscanf(L"%s", &group_name);
    buffer.lgrpi0_name = group_name;
    int t = NetLocalGroupAdd_(NULL, 0, (LPBYTE)&buffer, NULL);
    cout << "error " << t << endl;
}

void DeleteGroup() {
    wchar_t group_name[128];
    cout << "Имя: ";
    wscanf(L"%s", &group_name);
    NetLocalGroupDel_(NULL, group_name);
}

LSA_UNICODE_STRING InitLsaStr(PWSTR str) { // хранение строк в Local Security Authority
    LSA_UNICODE_STRING ret;
    ret.Buffer = str;
    ret.MaximumLength = 128;
    ret.Length = wcslen(str) * sizeof(WCHAR);
    return ret;
}

void AddRightsToUser() {
    wchar_t user_name[128];
    wchar_t right_user[128];
    cout << "Имя: ";
    wscanf(L"%s", &user_name);
    cout << "Право: ";
    wscanf(L"%s", &right_user);
    LSA_UNICODE_STRING str_right = InitLsaStr(right_user);
    LsaAddAccountRights_(PolicyHandle, GetSID(user_name, SidTypeUser), &str_right, 1);
}

void DeleteRightsToUser() {
    wchar_t user_name[128];
    wchar_t right_user[128];
    cout << "Имя: ";
    wscanf(L"%s", &user_name);
    cout << "Право: ";
    wscanf(L"%s", &right_user);
    LSA_UNICODE_STRING str_right = InitLsaStr(right_user);
    LsaRemoveAccountRights_(PolicyHandle, GetSID(user_name, SidTypeUser), FALSE, &str_right, 1);
}

void AddRightsToGroup() {
    wchar_t group_name[128];
    wchar_t right_user[128];
    cout << "Имя: ";
    wscanf(L"%s", &group_name);
    cout << "Право: ";
    wscanf(L"%s", &right_user);
    LSA_UNICODE_STRING str_right = InitLsaStr(right_user);
    LsaAddAccountRights_(PolicyHandle, GetSID(group_name, SidTypeGroup), &str_right, 1);
}

void DeleteRightsToGroup() {
    wchar_t group_name[128];
    wchar_t right_user[128];
    cout << "Имя: ";
    wscanf(L"%s", &group_name);
    cout << "Право: ";
    wscanf(L"%s", &right_user);
    LSA_UNICODE_STRING str_right = InitLsaStr(right_user);
    LsaRemoveAccountRights_(PolicyHandle, GetSID(group_name, SidTypeGroup), FALSE, &str_right, 1);
}

void AddUserInGroup() {
    wchar_t user_name[128];
    wchar_t group_name[128];
    cout << "Имя пользователя: ";
    wscanf(L"%s", &user_name);
    cout << "Имя группы: ";
    wscanf(L"%s", &group_name);
    _LOCALGROUP_MEMBERS_INFO_0 buffer;
    buffer.lgrmi0_sid = GetSID(user_name, SidTypeUser);
    NetLocalGroupAddMembers_(NULL, group_name, 0, (LPBYTE)&buffer, 1);
}

void DeleteUserFromGroup() {
    wchar_t user_name[128];
    wchar_t group_name[128];
    cout << "Имя пользователя: ";
    wscanf(L"%s", &user_name);
    cout << "Имя группы: ";
    wscanf(L"%s", &group_name);
    _LOCALGROUP_MEMBERS_INFO_0 buffer;
    buffer.lgrmi0_sid = GetSID(user_name, SidTypeUser);
    NetLocalGroupDelMembers_(NULL, group_name, 0, (LPBYTE)&buffer, 1);
}

void ListAllRights() {
    cout << "Список всех возможных прав и их описаний:" << endl;
    for (const auto& right : rightsList) {
        wprintf(L"%s: %s\n", right.rightName, right.description);
    }
}



// Функция для инициализации LSA строки
LSA_UNICODE_STRING InitLsaStr(const wchar_t* str) {
    LSA_UNICODE_STRING lsaStr;
    lsaStr.Buffer = const_cast<wchar_t*>(str);
    lsaStr.Length = (USHORT)(wcslen(str) * sizeof(wchar_t));
    lsaStr.MaximumLength = (USHORT)((wcslen(str) + 1) * sizeof(wchar_t));
    return lsaStr;
}






std::vector<std::wstring> ReadBlacklist(const std::wstring& filename) {
    std::vector<std::wstring> blacklist;
    std::wifstream file(filename);
    std::wstring line;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            blacklist.push_back(line);
        }
    }

    return blacklist;
}

bool SetDisallowRunEnabled(bool enabled) {
    HKEY hKey = NULL;
    const wchar_t* subKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";
    LONG result = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        subKeyPath,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );

    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Ошибка создания/открытия ключа реестра: " << result << std::endl;
        return false;
    }
    DWORD value = enabled ? 1 : 0;

    result = RegSetValueEx(
        hKey,
        L"DisallowRun",
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(value)
    );

    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::wcerr << L"Ошибка установки значения в реестре: " << result << std::endl;
        return false;
    }

    RegCloseKey(hKey);
    return true;
}

bool AddToBlacklist(const std::vector<std::wstring>& blacklist) {
    HKEY hKey;
    LONG result = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun"),
        0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);

    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Не получилось создать Disallow: " << result << std::endl;
        return false;
    }

    for (size_t i = 0; i < blacklist.size(); ++i) {
        wchar_t valueName[32];
        swprintf_s(valueName, L"%d", i + 1);

        result = RegSetValueEx(
            hKey, valueName, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(blacklist[i].c_str()),
            (blacklist[i].size() + 1) * sizeof(wchar_t));

        if (result != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return false;
        }
    }

    RegCloseKey(hKey);
    return true;
}

/////
int main() {
    setlocale(LC_ALL, "Russian");
    HMODULE netapi32 = LoadLibrary(L"C:\\Windows\\System32\\netapi32.dll");
    HMODULE advapi32 = LoadLibrary(L"C:\\Windows\\System32\\Advapi32.dll");

    (FARPROC&)NetUserEnum_ = GetProcAddress(netapi32, "NetUserEnum");
    (FARPROC&)LookupAccountNameW_ = GetProcAddress(advapi32, "LookupAccountNameW");
    (FARPROC&)ConvertSidToStringSidW_ = GetProcAddress(advapi32, "ConvertSidToStringSidW");
    (FARPROC&)LsaEnumerateAccountRights_ = GetProcAddress(advapi32, "LsaEnumerateAccountRights");
    (FARPROC&)LsaOpenPolicy_ = GetProcAddress(advapi32, "LsaOpenPolicy");
    (FARPROC&)NetApiBufferFree_ = GetProcAddress(netapi32, "NetApiBufferFree");
    (FARPROC&)NetUserGetLocalGroups_ = GetProcAddress(netapi32, "NetUserGetLocalGroups");
    (FARPROC&)NetLocalGroupEnum_ = GetProcAddress(netapi32, "NetLocalGroupEnum");
    (FARPROC&)NetUserAdd_ = GetProcAddress(netapi32, "NetUserAdd");
    (FARPROC&)NetUserDel_ = GetProcAddress(netapi32, "NetUserDel");
    (FARPROC&)NetLocalGroupAdd_ = GetProcAddress(netapi32, "NetLocalGroupAdd");
    (FARPROC&)NetLocalGroupDel_ = GetProcAddress(netapi32, "NetLocalGroupDel");
    (FARPROC&)LsaAddAccountRights_ = GetProcAddress(advapi32, "LsaAddAccountRights");
    (FARPROC&)LsaRemoveAccountRights_ = GetProcAddress(advapi32, "LsaRemoveAccountRights");
    (FARPROC&)NetLocalGroupAddMembers_ = GetProcAddress(netapi32, "NetLocalGroupAddMembers");
    (FARPROC&)NetLocalGroupDelMembers_ = GetProcAddress(netapi32, "NetLocalGroupDelMembers");

    NTSTATUS ntstatus = 0;
    ntstatus = LsaOpenPolicy_(NULL, &ObjectAttributes, POLICY_LOOKUP_NAMES | POLICY_CREATE_ACCOUNT, &PolicyHandle);

    int N;
    while (1) {
        cout << "Выберите действие: \n";
        cout << "1. Показать информацию о пользователях\n";
        cout << "2. Показать информацию о группах\n";
        cout << "3. Добавить пользователя\n";
        cout << "4. Удалить пользователя\n";
        cout << "5. Добавить группу\n";
        cout << "6. Удалить группу\n";
        cout << "7. Добавить права пользователю\n";
        cout << "8. Удалить права пользователя\n";
        cout << "9. Добавить права группе\n";
        cout << "10. Удалить права группы\n";
        cout << "11. Добавить пользователя в группу\n";
        cout << "12. Удалить пользователя из группы\n";
        cout << "13. Вывод всевозможных прав\n";
        cout << "14. Добавить программы в черный список\n";
        cout << "0. Выход\n";
        cout << ">> ";
        cin >> N;

        switch (N) {
        case 1:
            InfoAboutUser();
            break;
        case 2:
            InfoAboutGroup();
            break;
        case 3:
            AddUser();
            break;
        case 4:
            DeleteUser();
            break;
        case 5:
            AddGroup();
            break;
        case 6:
            DeleteGroup();
            break;
        case 7:
            AddRightsToUser();
            break;
        case 8:
            DeleteRightsToUser();
            break;
        case 9:
            AddRightsToGroup();
            break;
        case 10:
            DeleteRightsToGroup();
            break;
        case 11:
            AddUserInGroup();
            break;
        case 12:
            DeleteUserFromGroup();
            break;
        case 13:
            ListAllRights();
            break;
        case 14:
        {
            if (!SetDisallowRunEnabled(true)) {
                printf("Set disallow out error\n");
                return 1;
            }
            std::vector<std::wstring> blacklist = ReadBlacklist(L"blacklist.txt");

            if (!AddToBlacklist(blacklist)) {
                printf("AddToBlacklist error\n");
                return 1;
            }

            printf("Черный список успешно применен.\n");
            system("gpupdate /force");
            printf("Групповые политики обновлены.");
            break;
        }

        case 0:
            return 0;
        default:
            cout << "Неверный выбор. Попробуйте снова.\n";
            break;
        }
        cout << "Нажмите любую клавишу, чтобы продолжить\n";
        system("pause");
        system("cls");
    }
}
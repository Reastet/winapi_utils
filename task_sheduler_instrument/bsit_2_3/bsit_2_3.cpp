#include <comdef.h>
#include <taskschd.h>
#include <iostream>
#include <Windows.h>
#define _WIN32_DCOM
#pragma comment(lib, "taskschd.lib")

using namespace std;

void ActiveTaskInFolder(ITaskFolder* pRootFolder)
{
	BSTR a = NULL;
	pRootFolder->get_Name(&a);
	wprintf(L"Folder: %ws\n", a);
	SysFreeString(a);

	IRegisteredTaskCollection* pTaskCollection = NULL;
	HRESULT result = pRootFolder->GetTasks(NULL, &pTaskCollection); //получает список таск
	LONG count_Tasks = 0;
	result = pTaskCollection->get_Count(&count_Tasks); //количество задач в текущей папке
	cout << "Count tasks: " << count_Tasks << endl;
	if (count_Tasks == 0)
	{
		pTaskCollection->Release();
	}

	TASK_STATE taskState;
	// вывод информации о задаче
	for (long i = 1; i <= count_Tasks; i++)
	{
		IRegisteredTask* pRegisteredTask = NULL;
		result = pTaskCollection->get_Item(_variant_t(i), &pRegisteredTask);

		if (SUCCEEDED(result))
		{
			BSTR taskName = NULL;
			result = pRegisteredTask->get_Name(&taskName);

			if (SUCCEEDED(result))
			{
				wprintf(L"\t%ws\n", taskName);
				SysFreeString(taskName);

				result = pRegisteredTask->get_State(&taskState);
				if (SUCCEEDED(result))
				{
					switch (taskState)
					{
					case TASK_STATE_UNKNOWN:
					{
						printf("\t\t Status: TASK_STATE_UNKNOWN\n");
						break;
					}
					case TASK_STATE_DISABLED:
					{
						printf("\t\t Status: TASK_STATE_DISABLED\n");
						break;
					}
					case TASK_STATE_QUEUED:
					{
						printf("\t\t Status: TASK_STATE_QUEUED\n");
						break;
					}
					case TASK_STATE_READY:
					{
						printf("\t\t Status: TASK_STATE_READY\n");
						break;
					}
					case TASK_STATE_RUNNING:
					{
						printf("\t\t Status: TASK_STATE_RUNNING\n");
						break;
					}
					default:
						break;
					}
				}
				else
					cout << "Error get_State()" << endl;
			}
			else
				cout << "Error get_Name()" << endl;
			pRegisteredTask->Release();
		}
		else
			cout << "Error get_Item() at index= " << i + 1 << result << endl;
	}
	cout << endl;
	//рекурсия для подпапок
	ITaskFolderCollection* folder_collection;
	result = pRootFolder->GetFolders(NULL, &folder_collection);
	LONG folder_count;
	result = folder_collection->get_Count(&folder_count);
	for (long i = 1; i <= folder_count; i++)
	{
		ITaskFolder* task_subfolder;
		result = folder_collection->get_Item(_variant_t(i), &task_subfolder);
		ActiveTaskInFolder(task_subfolder);
		task_subfolder->Release();
	}
	folder_collection->Release();
}

int ActiveTask()
{
	HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED); //Инициализация COM
	ITaskService* pService = NULL;
	result = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService); //создание объекта планироващика задач
	result = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()); //подключение к планировщику
	if (FAILED(result))
	{
		cout << "Error Connect()" << endl;
		CoUninitialize();
		return 0;
	}

	ITaskFolder* pRootFolder = NULL;
	result = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(result))
	{
		cout << "Error Root Folder" << endl;
		CoUninitialize();
		return 0;
	}

	ActiveTaskInFolder(pRootFolder);
	pService->Release();
	pRootFolder->Release();
	CoUninitialize();
	return 1;
}

void CreateTask(const LPCWSTR task_name, const LPCWSTR task_path, const LPCWSTR author_name,
				const LPCWSTR trigger_filter, const bool putValueQueries, const LPCWSTR value_name,
				const LPCWSTR value_num, const LPCWSTR args, int mode, LPCWSTR st)
{
	HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(result))
	{
		cout << "Error CoInitializeEx()" << endl;
		return;
	}

	LPCWSTR wszTaskName = task_name;
	ITaskService* pService = NULL;
	result = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(result))
	{
		cout << "Error CoCreateInstance()" << endl;
		CoUninitialize();
		return;
	}

	result = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(result))
	{
		cout << "Error Connect()" << endl;
		return;
	}

	ITaskFolder* pRootFolder = NULL;
	result = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(result))
	{
		cout << "Error GetFolder()" << endl;
		return;
	}
	pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0); // Если уже существует с таким названием

	ITaskDefinition* pTask = NULL;
	result = pService->NewTask(0, &pTask);
	pService->Release();
	if (FAILED(result))
	{
		cout << "Error NewTask()" << endl;
		return;
	}

	IRegistrationInfo* pRegInfo = NULL;
	result = pTask->get_RegistrationInfo(&pRegInfo);
	if (FAILED(result))
	{
		cout << "Error get_RegistrationInfo()" << endl;
		return;
	}

	result = pRegInfo->put_Author(_bstr_t(author_name));
	pRegInfo->Release();
	if (FAILED(result))
	{
		cout << "Error put_Author()" << endl;
		return;
	}

	ITaskSettings* pSettings = NULL;
	result = pTask->get_Settings(&pSettings);
	if (FAILED(result))
	{
		cout << "Error get_Settings()" << endl;
		return;
	}

	result = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
	pSettings->Release();
	if (FAILED(result))
	{
		cout << "Error put_StartWhenAvailable()" << endl;
		return;
	}

	ITriggerCollection* pTriggerCollection = NULL;
	result = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(result))
	{
		cout << "Error get_Triggers()" << endl;
		return;
	}

	ITrigger* pTrigger = NULL;
	if (mode == 0)
	{
		result = pTriggerCollection->Create(TASK_TRIGGER_EVENT, &pTrigger);
	pTriggerCollection->Release();
	if (FAILED(result))
	{
		cout << "Error Create Trigger" << endl;
		return;
	}

	IEventTrigger* pEventTrigger = NULL;
	result = pTrigger->QueryInterface(IID_IEventTrigger, (void**)&pEventTrigger);
	pTrigger->Release();
	if (FAILED(result))
	{
		cout << "Error QueryInterface()" << endl;
		return;
	}

	result = pEventTrigger->put_Id(_bstr_t(L"Triggeer_"));
	if (FAILED(result))
	{
		cout << "Error put_Id()" << endl;
		return;
	}

	result = pEventTrigger->put_Subscription(_bstr_t(trigger_filter));
	if (FAILED(result))
	{
		cout << "Error put_Subscription()" << endl;
		return;
	}

	if (putValueQueries == TRUE)
	{
		ITaskNamedValueCollection* value_queries;
		pEventTrigger->get_ValueQueries(&value_queries);
		value_queries->Create(_bstr_t(value_name), _bstr_t(value_num), NULL);
		result = pEventTrigger->put_ValueQueries(value_queries);
		if (FAILED(result))
		{
			cout << "Error put_ValueQueries()" << endl;
			return;
		}
	}
	pEventTrigger->Release();
	}
	else if(mode == 1)
	{
		result = pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pTrigger);
		pTriggerCollection->Release();
		IDailyTrigger* pDailyTrigger = NULL;
		if (SUCCEEDED(result))
		{
			result = pTrigger->QueryInterface(IID_IDailyTrigger, (void**)&pDailyTrigger);
			pTrigger->Release();
			if (SUCCEEDED(result))
			{
				result=pDailyTrigger->put_Id(_bstr_t(L"Trigger1"));
				if (FAILED(result))
				{
					cout << "Error put_Id()" << endl;
					return;
				}
				result=pDailyTrigger->put_StartBoundary(_bstr_t(st));
				if (FAILED(result))
				{
					cout << "Error put_StartBoundary()" << endl;
					return;
				}

				result = pDailyTrigger->put_EndBoundary(_bstr_t(L"2027-05-02T12:05:00"));
				if (FAILED(result))
				{
					cout << "Error put_EndBoundary()" << endl;
					return;
				}
				result = pDailyTrigger->put_DaysInterval((short)1);
				if (FAILED(result))
				{
					cout << "Error put_DaysInterval()" << endl;
					return;
				}
				pDailyTrigger->Release();
			}
			else
			{
				cout << "Error reate time event()" << endl;
				exit(1);
			}
		}
		else
		{
			cout << "Error reate time event()" << endl;
			exit(1);
		}
	}
	else
	{
		result = pTriggerCollection->Create(TASK_TRIGGER_BOOT, &pTrigger);
		pTriggerCollection->Release();
		IBootTrigger* pBootTrigger = NULL;
		if (SUCCEEDED(result))
		{
			result = pTrigger->QueryInterface(IID_IBootTrigger, (void**)&pBootTrigger);
			pTrigger->Release();
			if (SUCCEEDED(result))
			{
				result = pBootTrigger->put_Id(_bstr_t(L"Trigger1"));
				if (FAILED(result))
				{
					printf("\nCannot put the trigger ID: %x", result);
				};
				result = pBootTrigger->put_StartBoundary(_bstr_t(L"2005-01-01T12:05:00"));
				if (FAILED(result))
					printf("\nCannot put the start boundary: %x", result);

				result = pBootTrigger->put_EndBoundary(_bstr_t(L"2035-05-02T08:00:00"));
				if (FAILED(result))
					printf("\nCannot put the end boundary: %x", result);
				result = pBootTrigger->put_Delay(_bstr_t(L"PT60S"));
				if (FAILED(result))
				{
					printf("\nCannot put deleay: %x", result);
				};
				pBootTrigger->Release();
			}
			else
			{
				exit(1);
			}
		}
		else
		{
			exit(1);
		}
	}
	IActionCollection* pActionCollection = NULL;
	result = pTask->get_Actions(&pActionCollection);
	if (FAILED(result))
	{
		cout << "Error get_Actions()" << endl;
		return;
	}

	IAction* pAction = NULL;
	result = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollection->Release();
	if (FAILED(result))
	{
		cout << "Error Create Action Collection" << endl;
		return;
	}

	IExecAction2* pExecAction = NULL;
	result = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	pAction->Release();
	if (FAILED(result))
	{
		cout << "Error QueryInterface" << endl;
		return;
	}

	result = pExecAction->put_Path(_bstr_t(task_path));
	if (FAILED(result))
	{
		cout << "Error put_Path()" << endl;
		return;
	}

	result = pExecAction->put_Arguments(_bstr_t(args));
	if (FAILED(result))
	{
		cout << "Error put_Arguments()" << endl;
		exit(1);
	}
	pExecAction->Release();

	IRegisteredTask* pRegisteredTask = NULL;
	result = pRootFolder->RegisterTaskDefinition(_bstr_t(task_name), pTask, TASK_CREATE_OR_UPDATE, _variant_t(_bstr_t(L"")),
		_variant_t(_bstr_t(L"")), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask);
	if (FAILED(result))
	{
		cout << "Error RegisterTaskDefinition()" << endl;
		exit(1);
	}

	cout << "Task added" << endl;
	pTask->Release();
	pRegisteredTask->Release();
	CoUninitialize();
}

void DeleteTasks()
{
	HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(result))
	{
		cout << "Error CoInitializeEx()" << endl;
		return;
	}

	ITaskService* pService = NULL;
	result = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(result))
	{
		cout << "Error CoCreateInstance()" << endl;
		CoUninitialize();
		return;
	}

	result = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(result))
	{
		cout << "Error Connect()" << endl;
		return;
	}

	ITaskFolder* pRootFolder = NULL;
	result = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(result))
	{
		cout << "Error GetFolder()" << endl;
		return;
	}

	pRootFolder->DeleteTask(_bstr_t(L"Task_WD"), 0);
	pRootFolder->DeleteTask(_bstr_t(L"Task_FW"), 0);
	pRootFolder->DeleteTask(_bstr_t(L"Task_Ping"), 0);
	pRootFolder->DeleteTask(_bstr_t(L"Task_Boot"), 0);
	pRootFolder->DeleteTask(_bstr_t(L"Task_Time"), 0);
}


int main()
{
	setlocale(LC_ALL, "Russian");
	int choice;

	while (1)
	{
		cout << "1 - Active tasks\n2 - Create a task WD\n3 - Create a task FW\n4 - Create a task ping\n5 - Create a time task\n6 - Create a boot task\n7 - Delete tasks" << endl;
		cin >> choice;

		switch (choice)
		{
		case 1:
		{
			ActiveTask();
			break;
		}
		case 2:
		{
			CreateTask(L"Task_WD", L"C:\\Users\\AleksYum\\source\\repos\\stub_console_app\\x64\\Debug\\stub_console_app.exe", L"user",
				L"<QueryList>\
				    <Query Id=\"0\" Path=\"Microsoft-Windows-Windows Defender/Operational\">\
				    <Select Path=\"Microsoft-Windows-Windows Defender/Operational\">*[System[Provider[@Name='Microsoft-Windows-Windows Defender'] and (EventID = 5007)]]</Select>\
				    <Select Path=\"Microsoft-Windows-Windows Defender/WHC\">*[System[Provider[@Name='Microsoft-Windows-Windows Defender'] and (EventID = 5007)]]</Select>\
				    </Query>\
				    </QueryList>", FALSE, L"", L"", L"d", 0, L"");
			break;
		}
		case 3:
		{
			CreateTask(L"Task_FW", L"C:\\Users\\AleksYum\\source\\repos\\stub_console_app\\x64\\Debug\\stub_console_app.exe", L"user",
				L"<QueryList>\
				    <Query Id=\"0\" Path=\"Microsoft-Windows-Windows Firewall With Advanced Security/Firewall\">\
				    <Select Path=\"Microsoft-Windows-Windows Firewall With Advanced Security/Firewall\">*</Select>\
				    </Query>\
				    </QueryList>", FALSE, L"", L"", L"f", 0, L"");
			break;
		}
		case 4:
		{
			CreateTask(L"Task_Ping", L"C:\\Users\\AleksYum\\source\\repos\\stub_console_app\\x64\\Debug\\stub_console_app.exe", L"user",
				L"<QueryList><Query Id='0' Path='Security'><Select Path='Security'>*[System[Provider[@Name='Microsoft-Windows-Security-Auditing'] and EventID=5152]]</Select></Query></QueryList>",
				TRUE, L"eventData", L"Event/EventData/Data[@Name='SourceAddress']", L"$(eventData)", 0, L"");
			break;
		}
		case 5:
		{
			CreateTask(L"Task_Time", L"C:\\Users\\AleksYum\\source\\repos\\stub_console_app\\x64\\Debug\\stub_console_app.exe", L"user",
				L"",
				FALSE, L"", L"", L"t", 1, L"2025-09-11T13:14:00");
			break;
		}
		case 6:
		{
			CreateTask(L"Task_Boot", L"C:\\Users\\AleksYum\\source\\repos\\stub_console_app\\x64\\Debug\\stub_console_app.exe", L"user",
				L"",
				FALSE, L"", L"", L"b", 2, L"");
			break;
		}
		case 7:
		{
			DeleteTasks();
			break;
		}
		default:
			break;
		}
	}
}
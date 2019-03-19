// ScatServer.cpp: определяет точку входа для консольного приложения.
//
#include "stdafx.h"
#include "ScatServerService.h"
#include <iostream>

#define SERVICE_NAME             L"SCATLicenseServer"
#define SERVICE_DISPLAY_NAME     L"SCAT License Server"
#define SERVICE_START_TYPE       SERVICE_AUTO_START //SERVICE_DEMAND_START
#define SERVICE_DEPENDENCIES     L""
#define SERVICE_ACCOUNT          L"NT AUTHORITY\\LocalService"
#define SERVICE_PASSWORD         0

void InstallService(PWSTR, PWSTR, DWORD, PWSTR, PWSTR, PWSTR);
void UninstallService(PWSTR);
int openRegForm(const wchar_t* str);
int outputStatus();

extern bool fStopping;

int _tmain(int argc, _TCHAR* argv[])
{
//#ifdef _DEBUG
//	ScatServerService service(SERVICE_NAME);
//	service.ServiceWorkerThread();
//	return 0;
//#endif
	if ((argc>1) && ((*argv[1]==L'-' || (*argv[1]==L'/')))) {
		if (!_wcsicmp(L"install", argv[1] + 1))
			InstallService(SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_START_TYPE, SERVICE_DEPENDENCIES, SERVICE_ACCOUNT, SERVICE_PASSWORD);
		else if (!_wcsicmp(L"remove", argv[1] + 1))
			UninstallService(SERVICE_NAME);
		else if (!_wcsnicmp(L"reg", argv[1] + 1, 3))
			return openRegForm(argv[1] + 1); //argc, (char**)argv
		else if (!_wcsicmp(L"status", argv[1] + 1))
			return outputStatus();
		else {
			wprintf(L"Parameters:\n");
			wprintf(L" -install  to install the service.\n");
			wprintf(L" -remove   to remove the service.\n");
			wprintf(L" -reg   to registration the license.\n");
			wprintf(L" -status   to registration and current connections info.\n");
		}
	}
	//else {
	//Sleep(60000);
		ScatServerService service(SERVICE_NAME);
		if(ScatServerService::fStopping) return -1;
		ServiceBase::Run(service);
	//		wprintf(L"Parameters:\n");
	//		wprintf(L" -install  to install the service.\n");
	//		wprintf(L" -remove   to remove the service.\n");
	//		wprintf(L" -reg   to registration the license.\n");
	//		wprintf(L" -status   to registration and current connections info.\n");
	//	}
	//}
	return 0;
}
int openRegForm(const wchar_t* str)
{
	//if (*(str + 3) != ' ') {
	//	wprintf(L"Parameter incorrect!\n");
	//	return -1;
	//}
	ScatServerService::s_key = ScatServerService::wcharToU16str(str + 4);
	if (ScatServerService::validRegNumber()) {
		ScatServerService::saveRegistrationData();
		wprintf(L"Server been registered on : ");
		wprintf(std::to_wstring(ScatServerService::s_licCount).c_str());
		wprintf(L" licenses!\n");
	}
	else {
		wprintf(L"Invalid registration number!\nKey: ");
		wprintf(ScatServerService::u16ToWstr(ScatServerService::getSerialNumber().c_str()).c_str());
		wprintf(L"\n");
	}
	return 0;
	//ShowWindow(GetConsoleWindow(), SW_HIDE);
	//int out_argc(0);
	//char out_argv(0);
	//QApplication a(out_argc, (char**)&out_argv);
	//qt w;
	//w.registrationDataToForm();
	//w.show();
	//return a.exec();
}
int outputStatus()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult) goto WSACleanup;

	sockaddr_in ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	//unsigned long serv_ip(0);
	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //SOCK_STREAM, 0);
	if (udpSocket == INVALID_SOCKET) {
		std::cout << "Error create UDP-socket!";
		return -1;
	}
	DWORD timeout(3000); // Таймаут ожидания пакета 1 сек.
	ServerAddr.sin_addr.s_addr = INADDR_BROADCAST;
	iResult = setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	if (iResult == SOCKET_ERROR) {
		std::cout << "Error set socket parameters!";
		closesocket(udpSocket);
		return -1;
	}
	char val(1);
	iResult = setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));
	if (iResult == SOCKET_ERROR) {
		std::cout << "Error set socket parameters!";
		closesocket(udpSocket);
		return -1;
	}
	int SizeSockaddr = sizeof(ServerAddr);
	iResult = sendto(udpSocket, (const char*)NEW_CLIENT, (std::u16string(NEW_CLIENT).size() * 2) + 2, 0, (sockaddr*)&ServerAddr, SizeSockaddr);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Error send BROADCAST message!";
		closesocket(udpSocket);
		return -1;
	}
	char recvData[MIN_RECV_BUFFER] = { 0 };
	iResult = recvfrom(udpSocket, recvData, MIN_RECV_BUFFER, 0, (sockaddr*)&ServerAddr, &SizeSockaddr);
	closesocket(udpSocket);
	if (iResult == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err == WSAETIMEDOUT) {
			std::cout << "SCAT License Server not found!";
		}
		else {
			std::cout << "Error connection!";
		}
		WSACleanup();
		return -1;
	}

	SOCKET tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpSocket == INVALID_SOCKET) {
		std::cout << "Error create socket!";
		goto WSACleanup;
	}
	iResult = connect(tcpSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if (iResult == SOCKET_ERROR) {
		std::cout << "Error set connection with local license server!";
		goto WSACleanup;
	}
	iResult = send(tcpSocket, "status", 7, 0);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Error send query to license server!";
		goto WSACleanup;
	}
#define RECV_BUFFER_LEN 10000
	char buf[RECV_BUFFER_LEN] = { 0 };
	iResult = recv(tcpSocket, buf, RECV_BUFFER_LEN, 0);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Error receive data from license server!";
		goto WSACleanup;
	}
	//std::cout << "Current connections:\n";
	std::cout << buf;
WSACleanup:
	closesocket(tcpSocket);
	WSACleanup();
	//char a;
	//std::cin >> a;
	return 0;
}
void InstallService(PWSTR pszServiceName, PWSTR pszDisplayName, DWORD dwStartType, PWSTR pszDependencies, PWSTR pszAccount, PWSTR pszPassword)
{
    wchar_t szPath[MAX_PATH];
    SC_HANDLE schSCManager = 0;
    SC_HANDLE schService = 0;
	//SERVICE_STATUS ssSvcStatus;

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0) {
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

	schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Install the service into SCM by calling CreateService
    schService = CreateService(schSCManager,pszServiceName,pszDisplayName,SERVICE_QUERY_STATUS,SERVICE_WIN32_OWN_PROCESS,      // Service type
        dwStartType,SERVICE_ERROR_NORMAL,szPath,0,0,pszDependencies,pszAccount,pszPassword);
    if (!schService) {
        wprintf(L"CreateService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }
	wprintf(L"%s is installed.\n", pszServiceName);

	schService=OpenService(schSCManager, SERVICE_NAME, SERVICE_START);	// SERVICE_ALL_ACCESS
	if(!schService) {
		wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
		goto Cleanup;
	}
	if (!StartService(schService, 0, NULL)) {
		wprintf(L"StartService failed w/err 0x%08lx\n", GetLastError());
		goto Cleanup;
	}
	wprintf(L"%s is started.\n", pszServiceName);

Cleanup:
    if (schService) CloseServiceHandle(schService);
	if (schSCManager) CloseServiceHandle(schSCManager);
};

void UninstallService(PWSTR pszServiceName)
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!schSCManager) {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (!schService) {
        wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus)) {
        wprintf(L"Stopping %s.", pszServiceName);
        Sleep(1000);
        while (QueryServiceStatus(schService, &ssSvcStatus)) {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                wprintf(L".");
                Sleep(1000);
            }
            else break;
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED) wprintf(L"\n%s is stopped.\n", pszServiceName);
        else wprintf(L"\n%s failed to stop.\n", pszServiceName);
    }

    if (!DeleteService(schService)) {
        wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    wprintf(L"%s is removed.\n", pszServiceName);

Cleanup:
    if (schSCManager) {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService) {
        CloseServiceHandle(schService);
        schService = NULL;
    }
};

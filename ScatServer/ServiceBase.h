
#pragma once
#include "stdafx.h"
//#include <windows.h>
//#pragma comment(lib, "ws2_32.lib")

class ServiceBase
{
public:
    static bool Run(ServiceBase &service);
    ServiceBase(PWSTR pszServiceName, bool fCanStop=true, bool fCanShutdown=true, bool fCanPauseContinue=false);
    virtual ~ServiceBase(void);
    void Stop();

protected:
    virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv);
    virtual void OnStop();
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();
    void SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode = NO_ERROR, DWORD dwWaitHint = 0);
	DWORD GetServiceStatus();
    static void WriteEventLogEntry(PWSTR pszMessage, WORD wType);
    void WriteErrorLogEntry(PWSTR pszFunction, DWORD dwError = GetLastError());

private:
    static void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);
    static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);
    void Start(DWORD dwArgc, PWSTR *pszArgv);
    void Pause();
    void Continue();
    void Shutdown();

    static ServiceBase *s_service;

	static PWSTR m_name;
    SERVICE_STATUS m_status;
    SERVICE_STATUS_HANDLE m_statusHandle;
};
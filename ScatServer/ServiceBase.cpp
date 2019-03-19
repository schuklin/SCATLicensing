
#include "ServiceBase.h"
#include <assert.h>
#include <strsafe.h>

ServiceBase *ServiceBase::s_service(0);
LPWSTR ServiceBase::m_name(0);

bool ServiceBase::Run(ServiceBase &service)
{
	//if(!ScatServerService::registrationData.countLic) return false;
    s_service = &service;
    SERVICE_TABLE_ENTRY serviceTable[]={
        {service.m_name, ServiceMain},
        {NULL, NULL}
    };
    return StartServiceCtrlDispatcher(serviceTable);
}

void WINAPI ServiceBase::ServiceMain(DWORD dwArgc, PWSTR *pszArgv)
{
	assert(s_service);
	s_service->m_statusHandle = RegisterServiceCtrlHandler(s_service->m_name, ServiceCtrlHandler);
    if (!s_service->m_statusHandle) throw GetLastError();

    s_service->Start(dwArgc, pszArgv);
}

void WINAPI ServiceBase::ServiceCtrlHandler(DWORD dwCtrl)
{
    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP: s_service->Stop(); break;
    case SERVICE_CONTROL_PAUSE: s_service->Pause(); break;
    case SERVICE_CONTROL_CONTINUE: s_service->Continue(); break;
    case SERVICE_CONTROL_SHUTDOWN: s_service->Shutdown(); break;
    case SERVICE_CONTROL_INTERROGATE: break;
    default: break;
    }
}

ServiceBase::ServiceBase(PWSTR pszServiceName, bool fCanStop, bool fCanShutdown, bool fCanPauseContinue)
{
    m_name = (pszServiceName) ? pszServiceName : L"";
	m_statusHandle = 0;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_START_PENDING;
    DWORD dwControlsAccepted = 0;
    
	if (fCanStop) dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    if (fCanShutdown) dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    if (fCanPauseContinue) dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
    
	m_status.dwControlsAccepted = dwControlsAccepted;
    m_status.dwWin32ExitCode = NO_ERROR;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

ServiceBase::~ServiceBase(void)
{
}

void ServiceBase::Start(DWORD dwArgc, PWSTR *pszArgv)
{
    try {
        SetServiceStatus(SERVICE_START_PENDING);
        OnStart(dwArgc, pszArgv);
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD dwError) {
        WriteErrorLogEntry(L"Service Start", dwError);
        SetServiceStatus(SERVICE_STOPPED, dwError);
    }
    catch (...) {
        WriteEventLogEntry(L"Service failed to start.", EVENTLOG_ERROR_TYPE);
        SetServiceStatus(SERVICE_STOPPED);
    }
}

void ServiceBase::OnStart(DWORD dwArgc, PWSTR *pszArgv)
{
}

void ServiceBase::Stop()
{
    DWORD dwOriginalState = m_status.dwCurrentState;
    try {
        SetServiceStatus(SERVICE_STOP_PENDING);
        OnStop();
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD dwError) {
        WriteErrorLogEntry(L"Service Stop", dwError);
        SetServiceStatus(dwOriginalState);
    }
    catch (...) {
        WriteEventLogEntry(L"Service failed to stop.", EVENTLOG_ERROR_TYPE);
        SetServiceStatus(dwOriginalState);
    }
}

void ServiceBase::OnStop()
{
}

void ServiceBase::Pause()
{
    try {
        SetServiceStatus(SERVICE_PAUSE_PENDING);
        OnPause();
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (DWORD dwError) {
        WriteErrorLogEntry(L"Service Pause", dwError);
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (...) {
        WriteEventLogEntry(L"Service failed to pause.", EVENTLOG_ERROR_TYPE);
        SetServiceStatus(SERVICE_RUNNING);
    }
}

void ServiceBase::OnPause()
{
}

void ServiceBase::Continue()
{
    try {
        SetServiceStatus(SERVICE_CONTINUE_PENDING);
        OnContinue();
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD dwError) {
        WriteErrorLogEntry(L"Service Continue", dwError);
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (...) {
        WriteEventLogEntry(L"Service failed to resume.", EVENTLOG_ERROR_TYPE);
        SetServiceStatus(SERVICE_PAUSED);
    }
}

void ServiceBase::OnContinue()
{
}

void ServiceBase::Shutdown()
{
    try {
        OnShutdown();
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD dwError) {
        WriteErrorLogEntry(L"Service Shutdown", dwError);
    }
    catch (...) {
        WriteEventLogEntry(L"Service failed to shut down.", EVENTLOG_ERROR_TYPE);
    }
}

void ServiceBase::OnShutdown()
{
}

void ServiceBase::SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

	m_status.dwCurrentState = dwCurrentState;
    m_status.dwWin32ExitCode = dwWin32ExitCode;
    m_status.dwWaitHint = dwWaitHint;
    m_status.dwCheckPoint = ((dwCurrentState==SERVICE_RUNNING) || (dwCurrentState==SERVICE_STOPPED)) ? 0 : dwCheckPoint++;

    ::SetServiceStatus(m_statusHandle, &m_status);
}

DWORD ServiceBase::GetServiceStatus()
{
	return m_status.dwCurrentState;
}

void ServiceBase::WriteEventLogEntry(PWSTR pszMessage, WORD wType)
{
    HANDLE hEventSource = NULL;
    LPCWSTR lpszStrings[2] = {NULL, NULL};

    hEventSource = RegisterEventSource(NULL, m_name);
    if (hEventSource) {
        lpszStrings[0] = m_name;
        lpszStrings[1] = pszMessage;

        ReportEvent(hEventSource, wType, 0, 0, NULL, 2, 0, lpszStrings, NULL);
        DeregisterEventSource(hEventSource);
    }
}

void ServiceBase::WriteErrorLogEntry(PWSTR pszFunction, DWORD dwError)
{
    wchar_t szMessage[260];
    StringCchPrintf(szMessage, ARRAYSIZE(szMessage), L"%s failed w/err 0x%08lx", pszFunction, dwError);
    WriteEventLogEntry(szMessage, EVENTLOG_ERROR_TYPE);
}

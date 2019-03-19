
#include "stdafx.h"
//#include <wchar.h>
#include <string>
#include "ScatClient.h"
#include "time.h"

std::vector<std::wstring> disabledServers = {}; // "00000000-0000-0000-0000-000000000000", "" \};

WSADATA wsaData;
//int iResult;
//IAddInDefBase* g_iConnect;

static const wchar_t *g_PropNames[] = {L"Key"};
static const wchar_t *g_MethodNames[] = {L"GetLicense", L"UserMessage"};
static const wchar_t *g_PropNamesRu[] = { L"Ключ" };
static const wchar_t *g_MethodNamesRu[] = { L"ПолучитьЛицензию", L"СообщитьПользователю" };
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;

static const wchar_t g_kClassNames[] = L"ScatClient"; //|OtherClass1|OtherClass2";
static WcharWrapper s_names(g_kClassNames);

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);

bool CAddInNative::fStopping = false;
SOCKET CAddInNative::s_sock = INVALID_SOCKET;

long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (!*pInterface)
	{
		*pInterface = new CAddInNative();
		return (long)*pInterface;
	}
	return 0;
}
long DestroyObject(IComponentBase** pIntf)
{
	if (!*pIntf)
		return -1;

	delete *pIntf;
	*pIntf = 0;
	return 0;
}
const WCHAR_T* GetClassNames()
{
	return s_names;
}
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}
CAddInNative::CAddInNative()
	: m_iMemory(0), m_iConnect(0)
{
}
CAddInNative::~CAddInNative()
{
}
bool CAddInNative::Init(void* pConnection)
{ 
	fStopping = false;
	int iResult=WSAStartup(MAKEWORD(2, 2), &wsaData);
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL && !iResult;
	//g_iConnect	= (IAddInDefBase*) pConnection;
	//iResult = WSAStartup( MAKEWORD(2,2), &wsaData);
	////MessageBox(0, L"Init", 0, MB_OK);
	//return g_iConnect && !iResult;
}
bool CAddInNative::setMemManager(void* memManager)
{ 
	m_iMemory	= (IMemoryManager*) memManager;
	return m_iMemory != 0;
}
long CAddInNative::GetInfo()
{ 
    return 20190312; 
}
void CAddInNative::Done()
{
	WSACleanup();
	fStopping = true;
}
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsLanguageExt)
{ 
	const wchar_t *wsExtension = GetClassNames();
	size_t iActualSize = wcslen(wsExtension) + 1;
	if (m_iMemory) {
		if (m_iMemory->AllocMemory((void**)wsLanguageExt, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsLanguageExt, wsExtension, iActualSize);
		return true;
	}
	return false;
}
long CAddInNative::GetNProps()
{ 
    return eLastProp;
}
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{
	long plPropNum = -1;
	wchar_t* propName = 0;

	::convFromShortWchar(&propName, wsPropName);
	plPropNum = findName(g_PropNames, propName, eLastProp);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, eLastProp);

	delete[] propName;

	return plPropNum;
}
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{ 
	if (lPropNum >= eLastProp)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsPropName = NULL;
	size_t iActualSize = 0;

	switch (lPropAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{ 
	//TV_VT(pvarPropVal) = VTYPE_PWSTR;
	//switch (lPropNum)
	//{
	//default:
	//	return false;
	//}
	return true;
}
bool CAddInNative::SetPropVal(const long lPropNum, tVariant* varPropVal)
{ 
 //   switch(lPropNum)
 //   { 
	//case eKey:
 //       if (TV_VT(varPropVal) != VTYPE_PWSTR)
 //           return false;
 //       m_Key = TV_WSTR(varPropVal);
 //       
	//	if(connecting()) return false;
	//	break;
	//case EventOnKeyPressed:
    //    if (TV_VT(varPropVal) != VTYPE_BOOL)
    //        return false;
    //    m_EventOnKeyPressed = TV_BOOL(varPropVal);
    //    break;
    //case HookEnabled:
    //    if (TV_VT(varPropVal) != VTYPE_BOOL)
    //        return false;
    //    m_HookEnabled = TV_BOOL(varPropVal);
    //    break;
    //case FirstInterception:
    //    if (TV_VT(varPropVal) != VTYPE_BOOL)
    //        return false;
    //    m_FirstInterception = TV_BOOL(varPropVal);
    //    break;
    //case KeyboardLocked:
    //    if (TV_VT(varPropVal) != VTYPE_BOOL)
    //        return false;
    //    m_KeyboardLocked = TV_BOOL(varPropVal);
    //    break;
    //default:
    //    return false;
    //}

    return true;
}
bool CAddInNative::IsPropReadable(const long lPropNum)
{ 
 //   switch(lPropNum)
 //   { 
	//case eKey:
    //case EventOnKeyPressed:
    //case HookEnabled:
    //case FirstInterception:
    //case KeyboardLocked:
    //    return true;
    //default:
    //    return false;
    //}

    return false;
}
bool CAddInNative::IsPropWritable(const long lPropNum)
{
 //   switch(lPropNum)
 //   { 
	//case eKey:
		//case EventOnKeyPressed:
    //case HookEnabled:
    //case FirstInterception:
    //case KeyboardLocked:
    //    return true;
    //default:
    //    return false;
    //}

    return false;
}
long CAddInNative::GetNMethods()
{ 
    return eLastMethod;
}
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{ 
	long plMethodNum = -1;
	wchar_t* name = 0;

	::convFromShortWchar(&name, wsMethodName);
	plMethodNum = findName(g_MethodNames, name, eLastMethod);
	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eLastMethod);

	delete[] name;
	return plMethodNum;
}
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{ 
	if (lMethodNum >= eLastMethod)
		return 0;

	wchar_t *wsCurrentName = 0;
	WCHAR_T *wsMethodName = 0;
	size_t iActualSize = 0;

	switch (lMethodAlias)
	{
	case 0:
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1:
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;
	if (m_iMemory && wsCurrentName) {
		if (m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	return wsMethodName;
}
long CAddInNative::GetNParams(const long lMethodNum)
{ 
	switch (lMethodNum)
	{
	case eGetLicense:
		return 2;
	case eSendMessage:
		return 2;
	default:
		break;
	}
	return 0;
}
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant *pvarParamDefValue)
{ 
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;
	return false;
} 
bool CAddInNative::HasRetVal(const long lMethodNum)
{ 
    switch(lMethodNum)
    { 
    case eGetLicense:
        return true;
	case eSendMessage:
		return true;
	default:
        break;
    }
    return false;
}
bool CAddInNative::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{ 
    return false;
}
bool CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{ 
	switch(lMethodNum)
	{
	case eGetLicense:
	{
		std::string ip;
		std::u16string emp;
		if (paParams && TV_VT(paParams) == VTYPE_PWSTR && wcslen(paParams->pwstrVal)) ip = wcharToStr(paParams->pwstrVal);
		if (paParams && paParams + 1 && TV_VT(paParams + 1) == VTYPE_PWSTR && wcslen((paParams + 1)->pwstrVal)) emp = wcharToU16str((paParams + 1)->pwstrVal);
		if (connecting(ip, emp)) {
			TV_VT(pvarRetValue) = VTYPE_PWSTR;
			auto hash = createHash();
			wchar_t* ptr = new wchar_t[hash.size() + 1];
			u16ToWstr(ptr, hash.c_str(), hash.size() + 1);
			TV_WSTR(pvarRetValue) = createReturnPtr(ptr);
			pvarRetValue->wstrLen = wcslen(TV_WSTR(pvarRetValue)) & 0xFFFFFFFF;
			delete[] ptr;
		}
		else {
			TV_VT(pvarRetValue) = VTYPE_BOOL;
			TV_BOOL(pvarRetValue) = false;
		}
		return true;
	}
	case eSendMessage:
	{
		const wchar_t* msg(0);
		const wchar_t* emp(0);
		if (paParams && TV_VT(paParams) == VTYPE_PWSTR && wcslen(paParams->pwstrVal)) msg = paParams->pwstrVal;
		if (paParams && paParams + 1 && TV_VT(paParams + 1) == VTYPE_PWSTR && wcslen((paParams + 1)->pwstrVal)) emp = (paParams + 1)->pwstrVal;
		TV_VT(pvarRetValue) = VTYPE_BOOL;
		TV_BOOL(pvarRetValue) = false;
		if (s_sock != INVALID_SOCKET) {
			std::wstring sendData(msg);
			sendData.push_back(1);
			sendData += emp;
			auto res(send(s_sock, (const char*)sendData.c_str(), ((sendData.size() * 2) + 2) & 0xFFFFFFFF, 0));
			if (res == SOCKET_ERROR) {
				m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Соединение с сервером недоступно!", res);
				closeSocket();
			}
			else TV_BOOL(pvarRetValue) = true;
		}
		return true;
	}
	default:
        break;
    }
	return false; 
}
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
    _wsetlocale(LC_ALL, loc);
}
void CAddInNative::closeSocket()
{
	if (s_sock != INVALID_SOCKET) {
		closesocket(s_sock);
		s_sock = INVALID_SOCKET;
	}
}
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name, int Count) const
{
	long ret = -1;
	for (int i = 0; i < Count; i++) {
		if (!wcscmp(names[i], name)) {
			ret = i;
			break;
		}
	}
	return ret;
}
bool CAddInNative::connecting(const std::string& ip, const std::u16string& emp)
{
	sockaddr_in ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	//unsigned long serv_ip(0);
	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //SOCK_STREAM, 0);
	if (udpSocket == INVALID_SOCKET) {
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Ошибка создания сокета! стр.337", 0);
		return false;
	}
	DWORD timeout(3000); // Таймаут ожидания пакета 1 сек.
	ServerAddr.sin_addr.s_addr = INADDR_BROADCAST;
	if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Ошибка установки параметров соединения! стр.344", 0);
		closesocket(udpSocket);
		return false;
	}
	if (!ip.size()) {
		char val(1);
		if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) == SOCKET_ERROR) {
			m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Ошибка установки параметров соединения! стр.352", 0);
			closesocket(udpSocket);
			return false;
		}
	}
	else
		ServerAddr.sin_addr.s_addr = inet_addr(ip.c_str());

	int SizeSockaddr = sizeof(ServerAddr);
	if (sendto(udpSocket, (const char*)NEW_CLIENT, ((std::u16string(NEW_CLIENT).size() * sizeof(char16_t)) + sizeof(char16_t)) & 0xFFFFFFFF, 0, (sockaddr*)&ServerAddr, SizeSockaddr) == SOCKET_ERROR) {
		//wprintf(L"error %d\n", WSAGetLastError());
		//CAddInNative::SendEvent(L"Ошибка отправки широковещательного запроса!");
		m_iConnect->ExternalEvent(L"ScatClient", L"Ошибка отправки широковещательного запроса!", 0);
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Ошибка отправки широковещательного запроса! стр.363", 0);
		closesocket(udpSocket);
		return false;
	}
	wchar_t recvData[MIN_RECV_BUFFER] = { 0 };
	if (recvfrom(udpSocket, (char*)recvData, (MIN_RECV_BUFFER * 2) + 2, 0, (sockaddr*)&ServerAddr, &SizeSockaddr) == SOCKET_ERROR) {
		auto err(WSAGetLastError());
		if (err == WSAETIMEDOUT || (ip.size() && err == WSAECONNRESET)) {
			//CAddInNative::SendEvent(L"Сервер лицензий не обнаружен!");
			m_iConnect->ExternalEvent(L"ScatClient", L"Сервер лицензий не обнаружен!", 0);
			m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Сервер лицензий не обнаружен!", 0);
		}
		else {
			std::wstring msg(L"Ошибка соединения с сервером (");
			wchar_t buf[10];
			msg += _itow_s(err, buf, 10);
			msg += L") стр.373";
			m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", msg.c_str(), 0);
		}
		closesocket(udpSocket);
		return false;
	}
	if (serverDisabled(recvData)) {
		//CAddInNative::SendEvent(L"Сервер не действителен!");
		m_iConnect->ExternalEvent(L"ScatClient", L"Сервер не действителен!", 0);
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Сервер не действителен!", 0);
		closesocket(udpSocket);
		return false;
	}
	closesocket(udpSocket);
	return connectingTCP(ServerAddr.sin_addr.s_addr, emp);
}
bool CAddInNative::connectingTCP(unsigned long ip, const std::u16string& emp)
{
	sockaddr_in ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = ip;
	s_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (s_sock == INVALID_SOCKET) {
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Ошибка создания сокета! стр.401", 0);
		return false;
	}

	//DWORD timeout(1000);
	//int iResult = setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	//if (iResult == SOCKET_ERROR) { closesocket(tcpSocket); return false; }

	if (connect(s_sock, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR) {
		//CAddInNative::SendEvent(L"Не удалось установить соединение с сервером лицензий!");
		m_iConnect->ExternalEvent(L"ScatClient", L"Не удалось установить соединение с сервером лицензий!", 0);
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Ошибка установки соединения! стр.411", 0);
		closeSocket();
		return false;
	}
	//std::string sendData=createDataForServer();
	DWORD sessionID;
	ProcessIdToSessionId(GetCurrentProcessId(), &sessionID);
	char16_t buf[MIN_RECV_BUFFER] = { 0 };
	//_ltow_s(sessionID, buf, 10);
	std::u16string sendData(u16StrFromInt(sessionID));
	sendData.push_back(1);
	sendData += emp;
	if (send(s_sock, (const char*)sendData.c_str(), ((sendData.size() * sizeof(char16_t)) + sizeof(char16_t)) & 0xFFFFFFFF, 0) == SOCKET_ERROR) {
		//CAddInNative::SendEvent(L"Сервер лицензий более недоступен!");
		m_iConnect->ExternalEvent(L"ScatClient", L"Сервер лицензий более недоступен!", 0);
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Сервер лицензий более недоступен! стр.424", 0);
		closeSocket();
		return false;
	}
	memset(buf, 0, MIN_RECV_BUFFER * sizeof(char16_t));
	if (recv(s_sock, (char*)buf, (MIN_RECV_BUFFER * sizeof(char16_t)) + sizeof(char16_t), 0) == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT) {	// вернуть ошибку "Сервер лицензий не обнаружен"
			//CAddInNative::SendEvent(L"Сервер лицензий более недоступен!");
			m_iConnect->ExternalEvent(L"ScatClient", L"Сервер лицензий более недоступен!", 0);
			m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Сервер лицензий более недоступен! стр.433", 0);
			closeSocket();
			return false;
		}
	}
	if (std::u16string(buf) == SERVER_NO_LICENS) {
		//CAddInNative::SendEvent(L"Нет свободных лицензий!");
		m_iConnect->ExternalEvent(L"ScatClient", L"Нет свободных лицензий!", 0);
		m_iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Нет свободных лицензий!", 0);
		closeSocket();
		return false;
	}
	_beginthread(ConnectThread, 0, m_iConnect); //new ThreadData{ tcpSocket, m_iConnect });
	return true;
}
//inline std::string CAddInNative::wtostr(const wchar_t * in_str)
//{
//	std::string res;
//	for (const wchar_t* tmp = in_str; *tmp; tmp++)
//		res.push_back(*tmp);
//	return res;
//}
inline bool CAddInNative::serverDisabled(const wchar_t* in_ID)
{
	return std::find(disabledServers.begin(), disabledServers.end(), in_ID) != disabledServers.end();
}
// bool getOwnIP(char* buf, size_t len)
//{
//	if (!gethostname(buf, len)) {
//		hostent *sh;
//		sh=gethostbyname(buf);
//		if (sh!=NULL) {
//
//		}
//	{
//}
void CAddInNative::ConnectThread(void* val)
{
	//SOCKET tcpSocket = static_cast<CAddInNative::ThreadData*>(val)->sock;
	IAddInDefBase* iConnect = static_cast<IAddInDefBase*>(val);
	//delete val;
	while(true) {
		try {
			std::u16string data;
			char16_t buffer[DEFAULT_RECV_BUFFER];
			int size(0);
			do {
				memset(buffer, 0, DEFAULT_RECV_BUFFER);
				size = recv(s_sock, (char*)buffer, DEFAULT_RECV_BUFFER * sizeof(char16_t), 0);
				if (size <= 0) {
					if (!fStopping) {
						//else if (WSAGetLastError() == WSAETIMEDOUT) {
						iConnect->ExternalEvent(L"ScatClient", L"Сервер лицензий более недоступен!", 0);
						iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Сервер лицензий более недоступен!", 0);
					}
					goto exit;
				}
				data += buffer;
			} while (size == DEFAULT_RECV_BUFFER * sizeof(char16_t));
			if (!data.size() || data.at(0) == 1) continue;

			wchar_t* ptr = new wchar_t[data.size() + 1];
			u16ToWstr(ptr, data.c_str(), data.size() + 1);
			iConnect->ExternalEvent(L"ScatClient", ptr, 0);
			delete[] ptr;
			//if (recv(s_sock, (char*)buffer, DEFAULT_RECV_BUFFER * sizeof(wchar_t), 0) == SOCKET_ERROR) {
			//	if (fStopping) break;
			//	else if (WSAGetLastError() == WSAETIMEDOUT) {	// вернуть ошибку "Сервер лицензий не обнаружен"
			//													//CAddInNative::SendEvent(L"Сервер лицензий более недоступен!");
			//		iConnect->ExternalEvent(L"ScatClient", L"Сервер лицензий более недоступен!", 0);
			//		iConnect->AddError(ADDIN_E_MSGBOX_FAIL, L"ScatClient", L"Сервер лицензий более недоступен!", 0);
			//		//MessageBox(0, L"Сервер лицензий более недоступен!", 0, MB_OK);
			//		break;
			//	}
			//}
		}
		catch (...) { break; }
	}
exit:
	closeSocket();
	_endthread();
}

std::u16string CAddInNative::wcharToU16str(const wchar_t * ptr)
{
	std::u16string result;
	for (auto ch = ptr; *ch; ch++) result.push_back(*ch & 0xFFFF);
	return result;
}

std::string CAddInNative::wcharToStr(const wchar_t * ptr)
{
	std::string result;
	for (auto ch = ptr; *ch; ch++) result.push_back(*ch & 0xFF);
	return result;
}

void CAddInNative::char16FromInt(unsigned long long val, char16_t * dst, size_t dst_size)
{
	memcpy_s(dst, dst_size * sizeof(char16_t), u16StrFromInt(val).c_str(), (u16StrFromInt(val).size() * sizeof(char16_t)) + sizeof(char16_t));
}

uint16_t CAddInNative::u16StrToUint16(const std::u16string & in_str)
{
	std::string buffer;
	for (auto& ch : in_str) buffer.push_back(ch & 0xFF);
	return std::stoi(buffer);
}

std::u16string CAddInNative::u16StrFromInt(unsigned long long val, int radix)
{
	std::u16string result;
	if (radix != 10) {
		char buffer[20];
		_ui64toa_s(val, buffer, 20, radix);
		if(!buffer[1]) result.push_back(u'0');
		for (auto ch = buffer; *ch; ch++) result.push_back(*ch);
	}
	else
		for (auto& ch : std::to_string(val)) result.push_back(ch);
	return result;
}

void CAddInNative::u16ToWstr(wchar_t * dst, const char16_t * src, size_t src_size)
{
	auto dptr(dst);
	auto sptr(src);
	for (; src_size; dptr++, sptr++, src_size--) *dptr = *sptr;
}

wchar_t* CAddInNative::createReturnPtr(const wchar_t* in_data)
{
	wchar_t* result(0);
	int iActualSize = ::wcslen(in_data) + 1;
	if (m_iMemory) {
		if (m_iMemory->AllocMemory((void**)&result, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&result, in_data, iActualSize);
	}
	return result;
}

std::u16string CAddInNative::createHash()
{	
	std::u16string key(u"443b0b1a-2b1f-4703-1b27-723e4f541377");

	time_t curTime;
	time(&curTime);
	tm structTime;
	localtime_s(&structTime, &curTime);

	int sec(structTime.tm_sec);
	std::u16string result(u16StrFromInt(sec, 16));
	int year1((structTime.tm_year + 1900) / 100);
	int year2((structTime.tm_year + 1900) - year1 * 100);
	int dateTime[6] = { year1, year2, structTime.tm_mon + 1, structTime.tm_mday, structTime.tm_hour, structTime.tm_min };
	
	char16_t buf[5];
	auto it = key.begin() + 2;
	for (int i = 0; it != key.end(); it++, i++) {
		if (*it == u'-') { result.push_back(*it); it++; }
		memset(buf, 0, 5 * sizeof(char16_t));
		*buf = *it;
		it++;
		*(buf+1) = *it;
		int val(0);
		wchar_t wbuf[5]; 
		u16ToWstr(wbuf, buf, 5);
		swscanf_s(wbuf, L"%x", &val);
		val += dateTime[i];// + sec;
		result += u16StrFromInt(val, 16);
		if (i == 5) i = -1;
	}
	return result;
}

WcharWrapper::WcharWrapper(const wchar_t* str) :
#ifdef LINUX_OR_MACOS
	m_str_WCHAR(NULL),
#endif 
	m_str_wchar(NULL)
{
	if (str)
	{
		int len = wcslen(str);
		m_str_wchar = new wchar_t[len + 1];
		memset(m_str_wchar, 0, sizeof(wchar_t) * (len + 1));
		memcpy(m_str_wchar, str, sizeof(wchar_t) * len);
#ifdef LINUX_OR_MACOS
		::convToShortWchar(&m_str_WCHAR, m_str_wchar);
#endif
	}

}
//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
#ifdef LINUX_OR_MACOS
	if (m_str_WCHAR)
	{
		delete[] m_str_WCHAR;
		m_str_WCHAR = NULL;
	}
#endif
	if (m_str_wchar)
	{
		delete[] m_str_wchar;
		m_str_wchar = NULL;
	}
}
//---------------------------------------------------------------------------//

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
	if (!len) len = ::wcslen(Source) + 1;

	if (!*Dest) *Dest = new WCHAR_T[len];

	WCHAR_T* tmpShort = *Dest;
	wchar_t* tmpWChar = (wchar_t*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(WCHAR_T));
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
		*tmpShort = (WCHAR_T)*tmpWChar;
	return res;
}
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
	uint32_t res = 0;
	WCHAR_T *tmpShort = (WCHAR_T*)Source;

	while (*tmpShort++)
		++res;

	return res;
}
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
	if (!len) len = getLenShortWcharStr(Source) + 1;

	if (!*Dest) *Dest = new wchar_t[len];

	wchar_t* tmpWChar = *Dest;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(wchar_t));
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
		*tmpWChar = (wchar_t)*tmpShort;
	return res;
}

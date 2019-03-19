
#include "ScatServerService.h"
#include "ThreadPool.h"
#include <WS2tcpip.h>
#include <iostream>
#include <Psapi.h>
//#include <locale>
//#include <codecvt>
//#include <algorithm>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Уникальные идентификаторы серверов
//char* serverID =
//"4BB0D019-420E-459B-9CCF-0B42651ED612"; // ООО "Лаборатория-33"
//"CF7FA840-6A1A-42CA-8CF4-F3052745D8F5"; // ООО "ТРЕЙД"
//"CF64D44E-FE73-4A5C-8566-CDAA1761035C"; // ООО «КлеверПак»
//"89DD8C4D-9B96-4AE9-AB14-93D54CB29CD3"; // ООО «Пельменная компания» (Дулисов)
//"24990C39-B6C3-4015-BCE2-2CD2C9BB4FCA"; // ООО "Строй-Инструмент"
//"6B45F521-0631-428F-B368-291D80EADD62"; // ООО "ТД ХотФрост"
//"A6D36CED-E252-4FF7-8AC4-CBBC8B6DB109"; // Группа компаний "РТЛ"
//"9486750C-6ED5-4385-8880-EA4A26D2649C"; // ООО "Альянс-групп"
//"B2247C65-63E7-4AFB-AD0D-B7B668B57690"; // ООО "Ува-молоко"
//"1BB781F1-85E2-4111-831B-912AE180648C"; // ООО "Браверс"
//"7417D2A4-4D3E-48C5-8D7A-4AD12F8E1403"; // ООО "Молсбыт" (Ижевск)
//"992F07EA-D2AB-4355-B00F-0EFF5D435E48"; // ООО "Молсбыт" (Москва)
//"0119AAB2-B59E-4BD0-8734-EE7128059B81"; // Ижевскхиммаш
//"6C664171-AFF4-4DEB-A6A9-57CEC12776E7"; // ООО "ПКФ УМЗ"
//"20532024-9C71-444F-BCDE-63C089C7E0AB"; // ООО "ТДС"
////////////////////////////////////////////////////////////////////////////////////////////////////////

CRITICAL_SECTION ScatServerService::csWin;
bool ScatServerService::fStopping = false;
HANDLE ScatServerService::m_hStoppedEvent = 0;
std::u16string ScatServerService::s_serialNumber;
std::u16string ScatServerService::s_key;
size_t ScatServerService::s_licCount = 0;

//ScatServerService::RegistrationData ScatServerService::registrationData{};
ScatServerService::Clients ScatServerService::s_clients{};

ScatServerService::ScatServerService(PWSTR pszServiceName, bool fCanStop, bool fCanShutdown, bool fCanPauseContinue)
	: ServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	fStopping = false;
	readRegistrationData();
	//if(!registrationData.serialNumber) readRegistrationData();
	if(!validRegNumber()) {
#ifdef _DEBUG
		printMessage(u"SCAT License Server не лицензирован!");
#else
		WriteEventLogEntry(L"SCAT License Server не лицензирован!", EVENTLOG_ERROR_TYPE);
#endif
		fStopping = true;
		return;
	}
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL) throw GetLastError();
}
ScatServerService::~ScatServerService(void)
{
    if (m_hStoppedEvent) {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}
void ScatServerService::OnStart(DWORD dwArgc, PWSTR *lpszArgv)
{
    WriteEventLogEntry(L"SCAT License Server in OnStart", EVENTLOG_INFORMATION_TYPE);
	ThreadPool::QueueUserWorkItem(&ScatServerService::ServiceWorkerThread, this);
}
void ScatServerService::OnStop()
{
    WriteEventLogEntry(L"SCAT License Server in OnStop", EVENTLOG_INFORMATION_TYPE);
    fStopping = true;
    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0) throw GetLastError();
}
void ScatServerService::ServiceWorkerThread(void)
{
	if (fStopping) return;
	InitializeCriticalSection(&csWin);

	addrinfo hints;
	addrinfo* result;
	//MessageBox(0, L"ServiceWorkerThread", 0, MB_OK);
	WSADATA wsaData;
	int iResult;
	//char chInfo[64];

	sockaddr_in ClientAddr;
	//sockaddr_in ServerAddr;

	int SizeSockaddr = sizeof(ClientAddr);
	//ServerAddr.sin_family = AF_INET;
	//ServerAddr.sin_port = htons(PORT);
	//ServerAddr.sin_addr.s_addr = INADDR_BROADCAST;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult) goto WSACleanup;

	///////////////////////////////////////////////////////////////////////////////////
	//#define NI_MAXSERV    32
	//#define NI_MAXHOST  1025
	//
	//	//int iResult = 0;
	//	DWORD dwRetval;
	//	struct sockaddr_in saGNI;
	//	char hostname[NI_MAXHOST];
	//	char servInfo[NI_MAXSERV];
	//	u_short port = 27015;
	//	saGNI.sin_family = AF_INET;
	//	saGNI.sin_addr.s_addr = inet_addr("192.168.1.5");
	//	saGNI.sin_port = htons(port);
	//	dwRetval = getnameinfo((struct sockaddr *) &saGNI, sizeof(struct sockaddr), hostname, NI_MAXHOST, servInfo, NI_MAXSERV, NI_NUMERICSERV);
	///////////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, PORT_STR, &hints, &result);
	if (iResult)  goto WSACleanup;
	/////// Формируем строку, включающую все ip-адреса тек. сервера	
	//    if (!gethostname(chInfo,sizeof(chInfo))) {
	//        hostent *sh;
	//        sh=gethostbyname(chInfo);
	//        if (sh!=NULL) {
	//            int nAdapter=0;
	//            while (sh->h_addr_list[nAdapter]) {
	//                struct   sockaddr_in   adr;
	//                memcpy(&adr.sin_addr,sh->h_addr_list[nAdapter],sh->h_length);
	// 				strIP+=inet_ntoa(adr.sin_addr);
	//				strIP += '\n';
	//                nAdapter++;
	//            }
	//        }
	//    }
	/////////////////////////////////////
		//SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	SOCKET udpSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (udpSocket == INVALID_SOCKET) goto Freeaddrinfo;

	char val(1);
	iResult = setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));
	if (iResult == SOCKET_ERROR) goto udpSocket;

	DWORD timeout(1000);
	iResult = setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	if (iResult == SOCKET_ERROR) goto udpSocket;

	iResult = bind(udpSocket, result->ai_addr, result->ai_addrlen);
	if (iResult == SOCKET_ERROR) goto udpSocket;

	_beginthread(&AcceptThread, 0, 0);

	char16_t recvData[MIN_RECV_BUFFER]{ 0 };
	while (!fStopping) {
	recvfrom:
		iResult = recvfrom(udpSocket, (char*)recvData, MIN_RECV_BUFFER * 2, 0, (sockaddr*)&ClientAddr, &SizeSockaddr);
		if (iResult == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				if (GetServiceStatus() == SERVICE_STOPPED || GetServiceStatus() == SERVICE_STOP_PENDING) goto udpSocket;
				goto recvfrom;
			}
			else goto udpSocket;
		}
		//if(!wcscmp(recvData, NEW_CLIENT)) {
		if (std::u16string(recvData) == NEW_CLIENT)
			if (sendto(udpSocket, (const char*)s_key.c_str(), (s_key.size() * sizeof(char16_t)) + sizeof(char16_t), 0, (sockaddr*)&ClientAddr, SizeSockaddr) == SOCKET_ERROR) goto udpSocket;
	}
udpSocket:	
	closesocket(udpSocket);
Freeaddrinfo:
	freeaddrinfo(result);
WSACleanup:	
	WSACleanup();
	fStopping=true;
    SetEvent(m_hStoppedEvent);
	DeleteCriticalSection(&csWin);
}
std::string ScatServerService::wcharToStdString(const uint16_t* in_str)
{
	std::string result;
	for (const uint16_t* tmp = in_str; *tmp; tmp++) {
		char* first((char*)tmp);
		if(*first) result.push_back(*first);
		if(*(first+1)) result.push_back(*(first + 1));
	}
	return result;
}
std::u16string ScatServerService::charToStdWString(const char* in_str)
{
	std::u16string result;
	for(const char* tmp=in_str; *tmp; tmp++)
		result.push_back(*tmp);
	return result;
}
const std::u16string& ScatServerService::getSerialNumber()
{
	if(s_serialNumber.size()) return s_serialNumber;
	LPWSTR VolumeNameBuffer = 0;
	char16_t FileSystemNameBuffer[100];
	LPDWORD sz=0, fs=0;
	unsigned long drive_sn = 0;
	GetVolumeInformation(NULL, VolumeNameBuffer, 100, &drive_sn, sz, fs, (LPWSTR)FileSystemNameBuffer, 100);
	s_serialNumber = u16StrFromInt(extendSN(drive_sn));
	return s_serialNumber;
}
bool ScatServerService::readRegistrationData()
{
	//snAsWString(registrationData.serialNumberStr);
	std::u16string str;
	uint16_t hash(0);
	uint16_t offset(REG_OFFSET);
	const uint16_t step(REG_STEP);
	bool validKey(false);
	HKEY rKey(0);
	RegCreateKey(HKEY_LOCAL_MACHINE, L"Software\\Laboratory-33\\SCAT License Server", &rKey);
	if (!rKey) {
		WriteEventLogEntry(L"Ошибка создания (открытия) ключа реестра!", EVENTLOG_ERROR_TYPE);
		return false;
	}
	DWORD sizeData(0);
	DWORD dwType(0);
	RegGetValue(rKey, NULL, NULL, RRF_RT_REG_BINARY, NULL, NULL, &sizeData);
	if (!sizeData) {
		RegCloseKey(rKey);
		return false;
	}
	BYTE* licData = new BYTE[sizeData+sizeof(uint16_t)];
	memset(licData, 0, sizeData + sizeof(uint16_t));
	RegGetValue(rKey, NULL, NULL, RRF_RT_REG_BINARY, NULL, licData, &sizeData);
	RegCloseKey(rKey);

	int count(0);
	s_serialNumber = getSerialNumber();
	for (uint16_t* ptr = (uint16_t*)licData; *ptr; ptr++) {
		uint16_t size = *ptr;
		size += offset;
		hash += size;
		offset += step;
		if (size) {
			str.push_back(size);
		}
		else {
			switch (count) {
			case 0:
				//registrationData.organization = str;
				break;
			case 1:
				//registrationData.addres = str;
				break;
			case 2:
				//registrationData.phone = str;
				break;
			case 3:
				//registrationData.e_mail = str;
				break;
			case 4:
				s_key = str;
				break;
			default:
				break;
			}
			str.clear();
			count++;
		}
	}
	delete[] licData;

	///////// Контрольная сумма
	if (!hash) {
		WriteEventLogEntry(L"Некорректны параметры регистрации!", EVENTLOG_ERROR_TYPE);
		return false;
	}
	offset -= step;
	str[0] -= offset;
	hash -= str.at(0);
	hash -= offset;
	bool regFileTrue(hash==str.at(0));
	if (!regFileTrue) {
		WriteEventLogEntry(L"Некорректный HASH параметров регистрации!", EVENTLOG_ERROR_TYPE);
		s_serialNumber.clear();
		s_key.clear();
		s_licCount = 0;
	}
	//countLicFromRegNumber();
	return regFileTrue;
}
void ScatServerService::saveRegistrationData()
{
	uint16_t hash(0);	// формируется суммированием кодов символов всех полей
	uint16_t offset(REG_OFFSET);
	const uint16_t step(REG_STEP);
	//FILE* fkey(0);
	//if (_wfopen_s(&fkey, L"license.key", L"w")) {
	//	MessageBox(0, L"Ошибка открытия файла лицензии!", L"Ошибка", MB_OK | MB_ICONERROR);
	//	return;
	//}
	//writeToFile(registrationData.organization, fkey, hash, offset, step); // Организация
	//writeToFile(registrationData.addres, fkey, hash, offset, step); // Адрес
	//writeToFile(registrationData.phone, fkey, hash, offset, step); // Телефон
	//writeToFile(registrationData.e_mail, fkey, hash, offset, step); // E-mail
	//writeToFile(registrationData.key, fkey, hash, offset, step); // Рег. номер
	//fwrite(&hash, sizeof(wchar_t), 1, fkey);
	//fclose(fkey);

	HKEY rKey(0);
	//RegCreateKey(HKEY_CURRENT_USER, L"Software\\Laboratory-33\\SCAT License Server", &rKey);
	RegCreateKey(HKEY_LOCAL_MACHINE, L"Software\\Laboratory-33\\SCAT License Server", &rKey);
	if (!rKey) {
		messageBox(std::u16string(u"Ошибка создания ключа реестра!"));
		//WriteEventLogEntry(L"Ошибка создания ключа реестра!", EVENTLOG_ERROR_TYPE);
		return;
	}
	std::u16string licData;
	//writeToStdString(registrationData.organization, licData, hash, offset, step);
	//writeToStdString(registrationData.addres, licData, hash, offset, step);
	//writeToStdString(registrationData.phone, licData, hash, offset, step);
	//writeToStdString(registrationData.e_mail, licData, hash, offset, step);
	writeToStdString(u"", licData, hash, offset, step);
	writeToStdString(u"", licData, hash, offset, step);
	writeToStdString(u"", licData, hash, offset, step);
	writeToStdString(u"", licData, hash, offset, step);
	writeToStdString(s_key, licData, hash, offset, step);
	licData += hash;
	//licData = L"Test";
	LONG result = RegSetValueEx(rKey, NULL, 0, REG_BINARY, (BYTE*)licData.c_str(), licData.size() * sizeof(uint16_t));
	if (result != ERROR_SUCCESS)
		messageBox(std::u16string(u"Ошибка записи данных регистрации в реестр!"));
	//RegSetValue(rKey, NULL, REG_SZ, licData.c_str(), 0);
	RegCloseKey(rKey);
}
bool ScatServerService::validRegNumber(std::u16string* in_regNum)
{
	if (!in_regNum) in_regNum = &s_key; //&registrationData.key;
	if (!in_regNum->size()) {
		WriteEventLogEntry(L"Отсутствует рег. номер!", EVENTLOG_ERROR_TYPE);
		return false;
	}
	std::u16string magicNumber = u"144";
	std::u16string countLicStr;
		if (in_regNum->size() > 6 && in_regNum->substr(2, 3) == magicNumber) {
			countLicStr = in_regNum->substr(0, 2);
			if (countLicStr.at(0) == '0') countLicStr.erase(0, 1);
			s_licCount = u16StrToUll(countLicStr) & 0xFFFFFFFF;
		}
		else
		{
			WriteEventLogEntry(L"Не корректный рег.номер!", EVENTLOG_ERROR_TYPE);
			return false;
		}
	
	std::u16string openNumber(countLicStr);
	if(openNumber.size()==1)
		openNumber.insert(openNumber.begin(), '0');
	openNumber+= magicNumber;

	bool forward(true); // лево
	std::u16string result;
	if(!s_serialNumber.size()) s_serialNumber = getSerialNumber();
	std::u16string number(s_serialNumber);
	for(int n=1; n<25; n++) {
		result.clear();
		for (size_t c = 1; c < number.size(); c++) {
			char16_t buf[3]={0};
			buf[0]=number.at(c-1);
			int leftNumber = u16StrToUll(buf) & 0xFFFFFFFF;
			buf[0]=number.at(c);
			int reightNumber = u16StrToUll(buf) & 0xFFFFFFFF;
			char16FromInt(leftNumber + reightNumber, buf, sizeof(buf));
			result += buf[1] ? buf[1] : buf[0];
		}
		if(forward) {number=number.at(0); number+=result;}
		else { uint16_t tmp=number.at(number.size()-1); number=result; number+=tmp;}
		forward=!forward;
	}
	unsigned long long cnTmp = u16StrToUll(number) * s_licCount * 144;
	char16_t buf2[MIN_RECV_BUFFER];
	char16FromInt(cnTmp, buf2, sizeof(buf2));
	std::u16string closeNumber(buf2);
	std::u16string regNumber=openNumber;
	regNumber+='-';
	regNumber+=closeNumber.substr(0, 5);
	regNumber+='-';
	regNumber+=closeNumber.substr(5, 5);
	bool res(!in_regNum->compare(regNumber));
	if (!res && in_regNum == &s_key) {
		s_key.clear();
		s_licCount = 0;
		//registrationData.countLicStr.clear();
	}
	return res;
}
void ScatServerService::messageBox(const std::u16string& msg)
{
	MessageBox(0, (LPCWSTR)msg.c_str(), 0, MB_OK);
}
unsigned long long ScatServerService::extendSN(unsigned long in_SN)
{
	if(in_SN>999999999 && in_SN<10000000000) return in_SN;
	if (in_SN > 999999999) {
		char buf[100];
		std::string val(_ui64toa(in_SN, buf, 10));
		return _atoi64(val.substr(0,10).c_str());
	}
	while (in_SN<1000000000) {
		unsigned char endNum(in_SN & 0xFF);
		for (; endNum > 9; endNum /= 2);
		in_SN*=10;
		in_SN += endNum;
	}
	return in_SN;
}
//void ScatServerService::snAsWString(std::u16string& dstBuf)
//{
//	//char16_t tmpBuf[11]={0};
//	//_ui64tow_s(getSerialNumber(), tmpBuf, 11, 10);
//	dstBuf = u16StrFromInt(getSerialNumber());
//}
void ScatServerService::_countLicFromRegNumber()
{
	if(!s_key.size()) return;
	s_key.clear();
	char16_t buf[3]={0};
	if (s_key.at(0) != '0') {
		buf[0] = s_key.at(0);
		buf[1] = s_key.at(1);
		s_key.push_back(buf[0]);
		s_key.push_back(buf[1]);
	}
	else {
		buf[0] = s_key.at(1);
		s_key.push_back(buf[1]);
	}
	s_licCount = u16StrToUll(buf) & 0xFFFFFFFF;
}
inline bool ScatServerService::parseRecvData(std::string & out_id)
{
	return false;
}
unsigned long long ScatServerService::u16StrToUll(const std::u16string & in_str)
{
	std::string buffer;
	for (auto& ch : in_str) buffer.push_back(ch & 0xFF);
	return std::stoull(buffer);
}
std::u16string ScatServerService::u16StrFromInt(unsigned long long val, int radix)
{
	std::u16string result;
	if (radix != 10) {
		char buffer[20];
		_ui64toa_s(val, buffer, 20, radix);
		for (auto ch = buffer; *ch; ch++) result.push_back(*ch);
	}
	else
		for (auto& ch : std::to_string(val)) result.push_back(ch);
	return result;
	//std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::little_endian>, char16_t> conv;
	//return conv.from_bytes(std::to_string(val));
}
std::u16string ScatServerService::wcharToU16str(const wchar_t * ptr)
{
	std::u16string result;
	for (auto ch = ptr; *ch; ch++) result.push_back(*ch & 0xFFFF);
	return result;
}
std::wstring ScatServerService::u16ToWstr(const char16_t* src)
{
	std::wstring result;
	for (auto it = src; *it; it++) result.push_back(*it);
	return result;
}
void ScatServerService::char16FromInt(unsigned long long val, char16_t * dst, size_t dst_size)
{
	memcpy_s(dst, dst_size * sizeof(char16_t), u16StrFromInt(val).c_str(), u16StrFromInt(val).size() * sizeof(char16_t));
}
void ScatServerService::writeToFile(const std::u16string & in_data, FILE * file, uint16_t& hash, uint16_t& offset, const uint16_t& step)
{
	for (size_t i = 0; i < in_data.size(); i++) {
		uint16_t sym=in_data.at(i);
		hash += sym;
		sym -= offset;
		fwrite(&sym, sizeof(uint16_t), 1, file);
		offset += step;
	}
	uint16_t sym(-offset);
	fwrite(&sym, sizeof(uint16_t), 1, file);
	offset += step;
}
void ScatServerService::writeToStdString(const std::u16string & in_data, std::u16string & out_data, uint16_t & hash, uint16_t & offset, const uint16_t & step)
{
	for (size_t i = 0; i < in_data.size(); i++) {
		uint16_t sym = in_data.at(i);
		hash += sym;
		sym -= offset;
		out_data.push_back(sym);
		//fwrite(&sym, sizeof(wchar_t), 1, file);
		offset += step;
	}
	out_data.push_back(-offset);
	//wchar_t sym(-offset);
	//fwrite(&sym, sizeof(wchar_t), 1, file);
	offset += step;
}
void ScatServerService::AcceptThread(void* pData)
{
	int iResult;
	DWORD timeout(10000);
	sockaddr_in ServerAddr;

	int SizeSockaddr=sizeof(ServerAddr);
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = INADDR_ANY;

	SOCKET tcpSocket=socket(AF_INET, SOCK_STREAM, 0);
	if (tcpSocket==INVALID_SOCKET) goto endthread;	
	
	iResult = bind(tcpSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr));	
	if (iResult == SOCKET_ERROR) goto tcpSocket;

	iResult = listen(tcpSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) goto tcpSocket;

	sockaddr_in ClientAddr;
	int len(sizeof(ClientAddr));
	while (!fStopping) {
		SOCKET newSock = accept(tcpSocket, (sockaddr*)&ClientAddr, &len);
		if (newSock == INVALID_SOCKET) continue;
		else if (setsockopt(newSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
			closesocket(newSock);
			continue;
		}
//		std::string ip(inet_ntoa(ClientAddr.sin_addr));
//		EnterCriticalSection(&csWin);
//		iMap=mapLic.find(newSock);
//		if(iMap!=mapLic.end()) {
//#ifdef _DEBUG
//			std::wstring mes(L"New connect existing client");
//			mes+=ScatServerService::charToStdWString(ip.c_str());
//			mes+='\n';
//			wprintf(mes.c_str());
//#endif
//			mapLic[newSock]++;
//		}
//		else if(ScatServerService::registrationData.countLic == mapLic.size()) {
//			delete newSock;
//			LeaveCriticalSection(&csWin);
//			continue;
//		}
//		else {
//			iResult = setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
//			if (iResult == SOCKET_ERROR) {
//				delete newSock;
//				LeaveCriticalSection(&csWin);
//				continue;
//			}
//			mapLic.insert(std::map<SOCKET*, int>::value_type(newSock,1));
//			//wprintf(L"new client\n");
//		}
//		LeaveCriticalSection(&csWin);
		ConnectionData* cd = new ConnectionData;
		cd->addr = ClientAddr;
		cd->sock = newSock;
		_beginthread(&ConnectionThread, 0, (void*)cd);
	}

tcpSocket:	
	closesocket(tcpSocket);
endthread:
	fStopping=true;
    SetEvent(m_hStoppedEvent);
	_endthread();
}
void ScatServerService::ConnectionThread(void* val)
{
	SOCKET ClientSocket = ((ConnectionData*)val)->sock;
	std::string client = inet_ntoa(((ConnectionData*)val)->addr.sin_addr);
	delete val;

	std::u16string sendData;
	std::u16string empId;
	std::thread::id* thr = &std::this_thread::get_id();
	char16_t* tmpbuf = new char16_t[MIN_RECV_BUFFER];
	memset(tmpbuf, 0, MIN_RECV_BUFFER * sizeof(uint16_t));
	int iResult = recv(ClientSocket, (char*)tmpbuf, MIN_RECV_BUFFER * 2, 0);
	std::u16string recvData(tmpbuf);
	delete[] tmpbuf;
	std::u16string msg_def{ 1 };
	if (iResult == SOCKET_ERROR) goto Endthread;

	if (recvData == u"status") {
		std::u16string info(s_clients.info());
		iResult = send(ClientSocket, (const char*)info.c_str(), (info.size() * 2) + 2, 0);
		goto Endthread;
	}

	client.push_back('/');
	bool flag(false);
	for (auto& ch : recvData) {
		if (flag) empId.push_back(ch);
		else if (ch == 1) flag = true;
		else if (ch > 0x2F && ch < 0x7B) client.push_back(ch & 0xFF);
	}

	sendData = s_clients.addClient(client, empId, thr);
	iResult = send(ClientSocket, (const char*)sendData.c_str(), (sendData.size() * 2) + 2, 0);
	if (iResult == SOCKET_ERROR || sendData != SERVER_OK) goto Endthread;

	while(true) {
		//Sleep(10000);
		std::u16string data;
		char16_t buffer[DEFAULT_RECV_BUFFER];
		int size(0);
		do {
			memset(buffer, 0, DEFAULT_RECV_BUFFER);
			size = recv(ClientSocket, (char*)buffer, DEFAULT_RECV_BUFFER * sizeof(char16_t), 0);
			if(size > 0) data += buffer;
		} while (size == DEFAULT_RECV_BUFFER * sizeof(char16_t));
		if (data.size()) {
			std::u16string guidEmp;
			std::u16string msg;
			bool isMsg(true);
			for (auto& ch : data) {
				if (ch == 1) isMsg = false;
				else if (isMsg) msg.push_back(ch);
				else guidEmp.push_back(ch);
			}
			if(msg.size()) s_clients.addMessage(guidEmp, msg);
		}
		const std::u16string* msg(s_clients.getMessage(client, empId, thr));
		if (!msg) msg = &msg_def;
		do {
			size = send(ClientSocket, (const char*)msg->c_str(), (msg->size() * sizeof(char16_t)) + sizeof(char16_t), 0);
			if (size == SOCKET_ERROR) {
				s_clients.removeClient(client, empId, thr);
				goto Endthread;
			}
			if (msg != &msg_def) s_clients.removeMessage(client, empId, thr);
			msg = s_clients.getMessage(client, empId, thr);
		} while (msg);
	}
Endthread:
	closesocket(ClientSocket);
	_endthread();
}

void ScatServerService::printMessage(const char16_t* text)
{
#ifdef _DEBUG
	std::cout << text << std::endl; //printf(text.c_str());
#endif
}

std::u16string ScatServerService::Clients::addClient(const std::string & ipId, const std::u16string & guidEmp, const std::thread::id* thr)
{
	std::u16string result(SERVER_OK);
	EnterCriticalSection(&csWin);
	if (s_licCount == clients.size()) result = SERVER_NO_LICENS;
	else clients[ipId].users[guidEmp].thrs.insert(thr);
	LeaveCriticalSection(&csWin);
	return result;
}

void ScatServerService::Clients::addMessage(const std::u16string & guidEmp, const std::u16string& msg)
{
	EnterCriticalSection(&csWin);
	for (auto& term : clients) {
		if (guidEmp.size() && !term.second.users.count(guidEmp)) continue;
		for (auto& user : term.second.users) if (user.first == guidEmp) user.second.addMessage(msg);
	}
	LeaveCriticalSection(&csWin);
}

const std::u16string * ScatServerService::Clients::getMessage(const std::string & ipId, const std::u16string & guidEmp, const std::thread::id * thr) const
{
	const std::u16string* result(0);
	EnterCriticalSection(&csWin);
	if(clients.count(ipId) && clients.at(ipId).users.count(guidEmp))
		for (auto& message : clients.at(ipId).users.at(guidEmp).msgs)
			if (message.second.count(thr)) result = &message.first;
	LeaveCriticalSection(&csWin);
	return result;
}

void ScatServerService::Clients::removeMessage(const std::string & ipId, const std::u16string & guidEmp, const std::thread::id * thr)
{
	std::vector<std::vector<UsersData::Messages::Message>::iterator> eraseIndexes;
	EnterCriticalSection(&csWin);
	auto& msgs = clients.at(ipId).users.at(guidEmp).msgs;
	for (auto& messageIt = msgs.begin(); messageIt != msgs.end(); messageIt++) {
		messageIt->second.erase(thr);
		if (!messageIt->second.size()) eraseIndexes.push_back(messageIt);
	}
	for (auto it = eraseIndexes.rbegin(); it != eraseIndexes.rend(); it++) msgs.erase(*it);
	LeaveCriticalSection(&csWin);
}

void ScatServerService::Clients::removeClient(const std::string & ipId, const std::u16string & guidEmp, const std::thread::id* thr)
{
	EnterCriticalSection(&csWin);
	if (clients.count(ipId) && clients.at(ipId).users.count(guidEmp)) {
		clients[ipId].users[guidEmp].removeSession(thr);
		if (!clients[ipId].users[guidEmp].thrs.size()) clients[ipId].users.erase(guidEmp);
		if (!clients[ipId].users.size()) clients.erase(ipId);
	}
	LeaveCriticalSection(&csWin);
}

std::u16string ScatServerService::Clients::info()
{
	std::u16string result;
	if (!s_licCount) {
		result = u"\nKey: ";
		result += s_key;
		result += u" (ungeristered)";
		return result;
	}
	result = u"\nRegistered on ";
	result += u16StrFromInt(s_licCount);
	result += u" licenses\nSerial number: ";
	result += s_serialNumber;
	result += u"\nKey: ";
	result += s_key;
	result += u"\n\nTotal connections: ";

	EnterCriticalSection(&csWin);
	result += u16StrFromInt(clients.size());
	result.push_back('\n');
	for (auto& it : clients) {
		for (auto& ch : it.first) result.push_back(ch);
		result.push_back('(');
		result += u16StrFromInt(count(it.second)); //_itow_s(count(it.second), buf, 10);
		result += u")\n";
	}
	LeaveCriticalSection(&csWin);
	return result;
}

inline size_t ScatServerService::Clients::count(const ScatServerService::Clients::UsersData& in_data)
{
	size_t rez(0);
	for (auto& it : in_data.users) rez += it.second.thrs.size();
	return rez;
}

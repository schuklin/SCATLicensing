
#pragma once
#include "ServiceBase.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <thread>

#define PORT 6543
#define PORT_STR "6543"
#define MIN_RECV_BUFFER 100
#define DEFAULT_RECV_BUFFER 1024
#define NEW_CLIENT u"%new_client%"
#define SERVER_OK u"%ok%"
//#define SERVER_DISABLED "%disabled%"
#define SERVER_NO_LICENS u"%nolic%"
#define REG_OFFSET 0x0220
#define REG_STEP 0x0007

class ScatServerService : public ServiceBase
{
public:
	struct ConnectionData { SOCKET sock; sockaddr_in addr; };
	static struct Clients{
		struct UsersData {
			struct Messages {
				typedef std::pair<std::u16string, std::set<const std::thread::id*>> Message;
				std::set<const std::thread::id*> thrs;
				std::vector<Message> msgs;
				void removeSession(const std::thread::id* idSession)
				{
					std::vector<std::vector<Message>::iterator> eraseIndexes;
					for (auto msg = msgs.begin(); msg != msgs.end(); msg++) {
						if (msg->second.count(idSession)) {
							msg->second.erase(idSession);
							if (!msg->second.size()) eraseIndexes.push_back(msg);
						}
					}
					for (auto it = eraseIndexes.rbegin(); it != eraseIndexes.rend(); it++) msgs.erase(*it);
					thrs.erase(idSession);
				};
				void addMessage(const std::u16string& msg)
				{
					if (thrs.size()) msgs.push_back({ msg, thrs });
				};
			};
			std::map<std::u16string, Messages> users; // empId, thr
		};

		std::u16string addClient(const std::string& ipId, const std::u16string& guidEmp, const std::thread::id* thr);
		void addMessage(const std::u16string& guidEmp, const std::u16string& msg);
		const std::u16string* getMessage(const std::string& ipId, const std::u16string& guidEmp, const std::thread::id* thr) const;
		void removeMessage(const std::string& ipId, const std::u16string& guidEmp, const std::thread::id* thr);
		void removeClient(const std::string& ipId, const std::u16string& guidEmp, const std::thread::id* thr);
		std::u16string info();
		inline size_t count(const UsersData& in_data);
		std::map<std::string, UsersData> clients; // ip
	} s_clients;
	static CRITICAL_SECTION csWin;
	static bool fStopping;
	static HANDLE m_hStoppedEvent;
	static std::u16string s_serialNumber;
	static std::u16string s_key;
	static size_t s_licCount;


    ScatServerService(PWSTR pszServiceName, bool fCanStop=true, bool fCanShutdown=true, bool fCanPauseContinue=false);
    virtual ~ScatServerService(void);
#ifdef _DEBUG
    void ServiceWorkerThread(void);
#endif
	static std::string wcharToStdString(const uint16_t* in_str);
	static std::u16string charToStdWString(const char* in_str);
	static const std::u16string& getSerialNumber();
	static bool readRegistrationData();
	static void saveRegistrationData();
	static bool validRegNumber(std::u16string* in_regNum = 0);
	static void messageBox(const std::u16string& msg);
	static void char16FromInt(unsigned long long val, char16_t* dst, size_t dst_size);
	static unsigned long long u16StrToUll(const std::u16string& in_str);
	static std::u16string u16StrFromInt(unsigned long long val, int radix = 10);
	static std::u16string wcharToU16str(const wchar_t * ptr);
	static std::wstring u16ToWstr(const char16_t* src);

protected:
    virtual void OnStart(DWORD dwArgc=0, PWSTR *pszArgv=0);
    virtual void OnStop();
#ifndef _DEBUG
    void ServiceWorkerThread(void);
#endif

private:
	void printMessage(const char16_t* text);
	static void AcceptThread(void*);
	static void ConnectionThread(void*);
	static inline unsigned long long extendSN(unsigned long in_SN);
	//static inline void snAsWString(std::u16string& dstBuf);
	static inline void _countLicFromRegNumber();
	static inline bool parseRecvData(std::string& out_id);
	static void writeToFile(const std::u16string& in_data, FILE* file, uint16_t& hash, uint16_t& offset, const uint16_t& step);
	static void writeToStdString(const std::u16string& in_data, std::u16string& out_data, uint16_t& hash, uint16_t& offset, const uint16_t& step);
};

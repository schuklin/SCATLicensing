
#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include <wtypes.h>
#include "ComponentBase.h"
#include "DefBase.h"
#include "IMemoryManager.h"
#include <vector>

#define PORT 6543
#define MIN_RECV_BUFFER 100
#define DEFAULT_RECV_BUFFER 1024
#define NEW_CLIENT u"%new_client%"
#define SERVER_OK u"%ok%"
//#define SERVER_DISABLED "%disabled%"
#define SERVER_NO_LICENS u"%nolic%"

class CAddInNative : public IComponentBase
{
public:
	enum Props
	{
		eLastProp=0      // Always last
	};

	enum Methods
	{
		eGetLicense = 0,
		eSendMessage,
		eLastMethod      // Always last
	};
	//struct ThreadData {
	//	SOCKET sock;
	//	IAddInDefBase* iConnect;
	//};
	CAddInNative(void);
	virtual ~CAddInNative();
	// IInitDoneBase
	virtual bool ADDIN_API Init(void*);
	virtual bool ADDIN_API setMemManager(void* mem);
	virtual long ADDIN_API GetInfo();
	virtual void ADDIN_API Done();
	// ILanguageExtenderBase
	virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsLanguageExt);
	virtual long ADDIN_API GetNProps();
	virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName);
	virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias);
	virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal);
	virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal);
	virtual bool ADDIN_API IsPropReadable(const long lPropNum);
	virtual bool ADDIN_API IsPropWritable(const long lPropNum);
	virtual long ADDIN_API GetNMethods();
	virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
	virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum,
		const long lMethodAlias);
	virtual long ADDIN_API GetNParams(const long lMethodNum);
	virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum,
		tVariant *pvarParamDefValue);
	virtual bool ADDIN_API HasRetVal(const long lMethodNum);
	virtual bool ADDIN_API CallAsProc(const long lMethodNum,
		tVariant* paParams, const long lSizeArray);
	virtual bool ADDIN_API CallAsFunc(const long lMethodNum,
		tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	operator IComponentBase*() { return (IComponentBase*)this; }
	// LocaleBase
	virtual void ADDIN_API SetLocale(const WCHAR_T* loc);
	static void closeSocket();

private:
	// Attributes
	IAddInDefBase      *m_iConnect;
	IMemoryManager     *m_iMemory;
	char16_t*			m_Key;
	static bool fStopping;
	static SOCKET s_sock;

	long findName(const wchar_t* names[], const wchar_t* name, int Count) const;
	//wchar_t* getNewWSTR(const wchar_t* source);
	bool connecting(const std::string& ip, const std::u16string& emp);
	bool connectingTCP(unsigned long ip, const std::u16string& emp);
	//inline std::string wtostr(const wchar_t* in_str);
	inline bool serverDisabled(const wchar_t* in_ID);
	wchar_t* createReturnPtr(const wchar_t* in_data);
	std::u16string createHash();
	static void ConnectThread(void* val = 0);
	static std::u16string wcharToU16str(const wchar_t* ptr);
	static std::string wcharToStr(const wchar_t* ptr);
	static void char16FromInt(unsigned long long val, char16_t* dst, size_t dst_size);
	static uint16_t u16StrToUint16(const std::u16string& in_str);
	static std::u16string u16StrFromInt(unsigned long long val, int radix = 10);
	static void u16ToWstr(wchar_t* dst, const char16_t* src, size_t src_size);
};

class WcharWrapper
{
public:
#ifdef LINUX_OR_MACOS
	WcharWrapper(const WCHAR_T* str);
#endif
	WcharWrapper(const wchar_t* str);
	~WcharWrapper();
#ifdef LINUX_OR_MACOS
	operator const WCHAR_T*() { return m_str_WCHAR; }
	operator WCHAR_T*() { return m_str_WCHAR; }
#endif
	operator const wchar_t*() { return m_str_wchar; }
	operator wchar_t*() { return m_str_wchar; }
private:
	WcharWrapper& operator = (const WcharWrapper& other) {};
	WcharWrapper(const WcharWrapper& other) {};
private:
#ifdef LINUX_OR_MACOS
	WCHAR_T* m_str_WCHAR;
#endif
	wchar_t* m_str_wchar;
};

#endif //__ADDINNATIVE_H__

#pragma once

#include <stddef.h>
#include <string>

#ifdef __ANDROID__
#include <stdlib.h>
#endif

#ifdef _WIN32

#ifdef LOG_DLL
#	define LOGAPI __declspec(dllexport)	
#else
#	define LOGAPI __declspec(dllexport)
#endif

#else

#ifdef LOG_DLL
#	define LOGAPI __attribute__ ((visibility("default")))	
#else
#   define LOGAPI
#endif


#endif

#define FASTLOG(group,message) do { if(group) FLog::FastLog(group, message"", 0); } while(0)
#define FASTLOG1(group,message,arg1) do { if(group) FLog::FastLog(group, message"", reinterpret_cast<const void*>(arg1)); } while(0)
#define FASTLOG2(group,message,arg1,arg2) do { if(group) FLog::FastLog(group, message"", reinterpret_cast<const void*>(arg1), reinterpret_cast<const void*>(arg2),0); } while(0)
#define FASTLOG3(group,message,arg1,arg2,arg3) do { if(group) FLog::FastLog(group, message"", reinterpret_cast<const void*>(arg1), reinterpret_cast<const void*>(arg2), reinterpret_cast<const void*>(arg3)); } while(0)
#define FASTLOG4(group,message,arg1,arg2,arg3,arg4) do { if(group) FLog::FastLog(group, message"", reinterpret_cast<const void*>(arg1), reinterpret_cast<const void*>(arg2), reinterpret_cast<const void*>(arg3), reinterpret_cast<const void*>(arg4), 0); } while(0)
#define FASTLOG5(group,message,arg1,arg2,arg3,arg4,arg5) do { if(group) FLog::FastLog(group, message"", reinterpret_cast<const void*>(arg1), reinterpret_cast<const void*>(arg2), reinterpret_cast<const void*>(arg3), reinterpret_cast<const void*>(arg4), reinterpret_cast<const void*>(arg5)); } while(0)

#define FASTLOGS(group,message,sarg) do { if(group) FLog::FastLogS(group, message"", sarg); } while(0)
#define FASTLOG1F(group,message,arg1) do { if(group) FLog::FastLogF(group, message"", arg1); } while(0)
#define FASTLOG2F(group,message,arg1,arg2) do { if(group) FLog::FastLogF(group, message"", arg1, arg2); } while(0)
#define FASTLOG3F(group,message,arg1,arg2,arg3) do { if(group) FLog::FastLogF(group, message"", arg1, arg2, arg3); } while(0)
#define FASTLOG4F(group,message,arg1,arg2,arg3,arg4) do { if(group) FLog::FastLogF(group, message"", arg1, arg2, arg3, arg4); } while(0)


#define FASTLOGNOFILTER(channel,message) FLog::FastLog(channel, message"", 0)
#define FASTLOGNOFILTER2(channel,message,arg1,arg2) FLog::FastLog(channel, message"", reinterpret_cast<const void*>(arg1), reinterpret_cast<const void*>(arg2), 0)

#define FASTLOGFORMATTED(group,message,...) do { if(group) FLog::FastLogFormatted(group, message"", ##__VA_ARGS__); } while(0)
#define FASTLOGFORMATTEDNOADORN(group,message,...) do { if(group) FLog::FastLogFormattedNoAdorn(group, message"", ##__VA_ARGS__); } while(0)


#define LOG_HISTORY 2048 // Has to be a power of two. If changing, modify debugger entries below

#define LOGGROUP(group) namespace FLog { extern Channel group; }
#define LOGVARIABLE(group,defaulton) \
	namespace FLog { Channel group = defaulton; int group##Initer = RegisterLogGroup(#group, &group, NULL, FASTVARTYPE_STATIC); }

#define DYNAMIC_LOGGROUP(group) namespace DFLog { extern FLog::Channel group; }
#define DYNAMIC_LOGVARIABLE(group,defaulton) \
	namespace DFLog { FLog::Channel group = defaulton; int group##Initer = FLog::RegisterLogGroup(#group, &group, NULL, FASTVARTYPE_DYNAMIC); }

#define SYNCHRONIZED_LOGGROUP(group) namespace SFLog { extern FLog::Channel get##group(); }
#define SYNCHRONIZED_LOGVARIABLE(group,defaulton) \
    namespace SFLog { FLog::Channel group = defaulton; bool* group##IsSync; int group##Initer = FLog::RegisterLogGroup(#group, &group, &group##IsSync, FASTVARTYPE_SYNC); \
    FLog::Channel get##group() { assert(*group##IsSync == true); return group; } }

#define FASTSTRING(var) namespace FString { extern std::string var; }
#define FASTSTRINGVARIABLE(var, defaultvalue) \
    namespace FString { std::string var = defaultvalue; int var##Initer = FLog::RegisterString(#var, &var, NULL, FASTVARTYPE_STATIC); }

#define FASTFLAG(var) namespace FFlag { extern bool var; }
#define FASTFLAGVARIABLE(var, defaultvalue) \
    FASTSTRINGVARIABLE(PlaceFilter_##var, "") \
    namespace FFlag { bool var = defaultvalue; int var##Initer = FLog::RegisterFlag(#var, &var, NULL, FASTVARTYPE_STATIC); }

#define DYNAMIC_FASTFLAG(var) namespace DFFlag { extern bool var; }
#define DYNAMIC_FASTFLAGVARIABLE(var, defaultvalue) \
    FASTSTRINGVARIABLE(PlaceFilter_##var, "") \
	namespace DFFlag { bool var = defaultvalue; int var##Initer = FLog::RegisterFlag(#var, &var, NULL, FASTVARTYPE_DYNAMIC); }

#ifndef _WIN32
#if defined(__i386__)
#define RBX_DEBUG_BREAK() { __asm__ __volatile__ ( "int $3" ); }(void)0
#else
#define RBX_DEBUG_BREAK() { ::abort(); }(void)0
#endif
#else
#define RBX_DEBUG_BREAK() { __debugbreak(); }(void)0
#endif

#if defined(_DEBUG) || defined(_NOOPT) || defined(RBX_TEST_BUILD)
    #define VALIDATE_SYNCHRONIZED_FLAG(var) { if (!var) RBX_DEBUG_BREAK(); } ((void)0)
#else
    #define VALIDATE_SYNCHRONIZED_FLAG(var) ((void)0)
#endif

#define SYNCHRONIZED_FASTFLAG(var) namespace SFFlag { extern bool get##var(); }
#define SYNCHRONIZED_FASTFLAGVARIABLE(var, defaultvalue) \
    FASTSTRINGVARIABLE(PlaceFilter_##var, "") \
    namespace SFFlag { bool var = defaultvalue; bool* var##IsSync; int var##Initer = FLog::RegisterFlag(#var, &var, &var##IsSync, FASTVARTYPE_SYNC); \
    bool get##var() { VALIDATE_SYNCHRONIZED_FLAG(*var##IsSync); return var; } }

#define FASTINT(var) namespace FInt { extern int var; }
#define FASTINTVARIABLE(var, defaultvalue) \
    FASTSTRINGVARIABLE(PlaceFilter_##var, "") \
	namespace FInt { int var = defaultvalue; int var##Initer = FLog::RegisterInt(#var, &var, NULL, FASTVARTYPE_STATIC); }

#define DYNAMIC_FASTINT(var) namespace DFInt { extern int var; }
#define DYNAMIC_FASTINTVARIABLE(var, defaultvalue) \
    FASTSTRINGVARIABLE(PlaceFilter_##var, "") \
	namespace DFInt { int var = defaultvalue; int var##Initer = FLog::RegisterInt(#var, &var, NULL, FASTVARTYPE_DYNAMIC); }

#define SYNCHRONIZED_FASTINT(var) namespace SFInt { extern int get##var(); }
#define SYNCHRONIZED_FASTINTVARIABLE(var, defaultvalue) \
    FASTSTRINGVARIABLE(PlaceFilter_##var, "") \
    namespace SFInt { int var = defaultvalue; bool* var##IsSync; int var##Initer = FLog::RegisterInt(#var, &var, &var##IsSync, FASTVARTYPE_SYNC); \
    int get##var() { VALIDATE_SYNCHRONIZED_FLAG(*var##IsSync); return var; } }

#define DYNAMIC_FASTSTRING(var) namespace DFString { extern std::string var; }
#define DYNAMIC_FASTSTRINGVARIABLE(var, defaultvalue) \
	namespace DFString { std::string var = defaultvalue; int var##Initer = FLog::RegisterString(#var, &var, NULL, FASTVARTYPE_DYNAMIC); }

#define SYNCHRONIZED_FASTSTRING(var) namespace SFString { extern std::string get##var(); }
#define SYNCHRONIZED_FASTSTRINGVARIABLE(var, defaultvalue) \
	namespace SFString { std::string var = defaultvalue; bool* var##IsSync; int var##Initer = FLog::RegisterString(#var, &var, &var##IsSync, FASTVARTYPE_SYNC); \
	std::string get##var() { VALIDATE_SYNCHRONIZED_FLAG(*var##IsSync); return var; } }

#define ABTEST_NEWUSERS(var) namespace ABNewUsers { extern int var; }
#define ABTEST_NEWUSERS_VARIABLE(var) \
	namespace ABNewUsers { int var = 0; int var##Initer = FLog::RegisterInt(#var, &var, NULL, FASTVARTYPE_AB_NEWUSERS); }

#define ABTEST_NEWSTUDIOUSERS(var) namespace ABNewStudioUsers { extern int var; }
#define ABTEST_NEWSTUDIOUSERS_VARIABLE(var) \
	namespace ABNewStudioUsers { int var = 0; int var##Initer = FLog::RegisterInt(#var, &var, NULL, FASTVARTYPE_AB_NEWSTUDIOUSERS); }

#define ABTEST_ALLUSERS(var) namespace ABAllUsers { extern int var; }
#define ABTEST_ALLUSERS_VARIABLE(var) \
	namespace ABAllUsers { int var = 0; int var##Initer = FLog::RegisterInt(#var, &var, NULL, FASTVARTYPE_AB_ALLUSERS); }

#define LOGCHANNELS 5

#define FILECHANNEL(id) (LOGCHANNELS+id)

enum ABTestVariation
{
	Control = 0,
	Variation1 = 1,
	Variation2 = 2,
	Variation3 = 3,
	Variation4 = 4,
	Variation5 = 5
};

enum FastVarType
{
    FASTVARTYPE_STATIC				= 0,
    FASTVARTYPE_DYNAMIC				= 1,
    FASTVARTYPE_SYNC				= 2,
    FASTVARTYPE_AB_NEWUSERS			= 4,
	FASTVARTYPE_AB_NEWSTUDIOUSERS	= 8,
	FASTVARTYPE_AB_ALLUSERS			= 16,
	FASTVARTYPE_ANY					= 1+2+4+8+16

};

namespace FLog {

	// Mini guide:

	// ---------------------------------
	// Using logs
	// ---------------------------------
	// Use FASTLOG macros _ONLY_


	// Examples:

	// Declare a group, then log using this group:
	// LOGGROUP(Test);
	// Important(!): Add group to v8dataModel/FastLogSettings.cpp

	// Just message - only constant strings are allowed (no generated strings), use arguments instead
	// FASTLOG(FLog::Test, "Cannot initialize graphics engine");

	// Message with arguments like numbers and pointers
	// FASTLOG2(FLog::Test, "Sound systems %p has %u effects", system, effectcount);

	// Message with floating-point arguments - has to use flf() function:
	// FASTLOG1F(FLog::Test, "Average block size %f", avesize);

	// Message with string (works for both char* and std::strings) - only one string is allowed
	// FASTLOGS(FLog::Test, "Object name: %s", name);

	// Channel 1: Non-chatty / important events (Game started, loaded UI script) -- more permanent messages
	// Channel 2: Per frame data
	// Channel 3-5: User defined / used for debugging / more temporary

	// ---------------------------------
	// Looking at the logs
	// ---------------------------------
	// Helpful debugger entries - select and mouse over:
	// Channel 1 -			{,,Log}g_FastLog[0]+({,,Log}g_LogCounters[0]%2048)
	// Channel 2 -			{,,Log}g_FastLog[1]+({,,Log}g_LogCounters[1]%2048)
	// Channel 3 -			{,,Log}g_FastLog[2]+({,,Log}g_LogCounters[2]%2048)
	// Channel 4 -			{,,Log}g_FastLog[3]+({,,Log}g_LogCounters[3]%2048)
	// Channel 5 -			{,,Log}g_FastLog[4]+({,,Log}g_LogCounters[4]%2048)

	typedef unsigned char Channel;

	// Make "too many actual parameters for macro" a error
#pragma warning ( error : 4002 )

	typedef double(*TimeFunc)();
	typedef void(*ExternalLogFunc)(Channel channel, const char* message);
	void LOGAPI Init(TimeFunc timeF);
	void LOGAPI SetExternalLogFunc(ExternalLogFunc logF);

	void LOGAPI FastLog(Channel channel, const char* message, const void* arg0);
	void LOGAPI FastLog(Channel channel, const char* message, const void* arg0, const void* arg1, const void* arg2);
	void LOGAPI FastLog(Channel channel, const char* message, const void* arg0, const void* arg1, const void* arg2, const void* arg3, const void* arg4);
	void LOGAPI FastLogS(Channel channel, const char* message, const char* sarg);
	void LOGAPI FastLogS(Channel channel, const char* message, const std::string& sarg);
	void LOGAPI FastLogF(Channel channel, const char* message, float arg1, float arg2 = 0.0f, float arg3 = 0.0f, float arg4 = 0.0f);
    void LOGAPI FastLogFormatted( Channel channel, const char* message, ... );
    void LOGAPI FastLogFormattedNoAdorn( Channel channel, const char* message, ... );

	int LOGAPI GetFastLogCounter(Channel channel);

	int LOGAPI RegisterLogGroup(const char* groupName, Channel* groupVar, bool** isSync = NULL, FastVarType fastVarType = FASTVARTYPE_STATIC);
	int LOGAPI RegisterFlag(const char* flagName, bool* flagVar, bool** isSync = NULL, FastVarType fastVarType = FASTVARTYPE_STATIC);
	int LOGAPI RegisterInt(const char* flagName, int* flagVar, bool** isSync = NULL, FastVarType fastVarType = FASTVARTYPE_STATIC);
	int LOGAPI RegisterString(const char* flagName, std::string* flagVar, bool** isSync = NULL, FastVarType fastVarType = FASTVARTYPE_STATIC);

	typedef void(*VariableVisitor)(const std::string& name, const std::string& value, void* context);
	void LOGAPI ForEachVariable(VariableVisitor visitor, void* context, FastVarType flagType);

	bool LOGAPI SetValue(const std::string& name, const std::string& value, FastVarType fastVarType = FASTVARTYPE_ANY, bool loadedFromServer = false);
	bool LOGAPI GetValue(const std::string& name, std::string& value, bool alsoCheckUnknownFlags = false);
    bool LOGAPI SetValueFromServer(const std::string& name, const std::string& value);

	double LOGAPI NowFast();
	void LOGAPI WriteFastLogDump(const char *fileName, int numEntries);

    unsigned short LOGAPI GetNumSynchronizedVariable();
	void LOGAPI ResetSynchronizedVariablesState();

    namespace Detail
    {
        typedef void(*BinaryLogFunc)(void* context, const void* data, size_t size);
        void LOGAPI SetBinaryLog(void* context, BinaryLogFunc callback);
    }
}

LOGGROUP(Legacy)

namespace FLog { 
	const Channel Always = 1; 
	const Channel Error = 1; 
	const Channel Warning = 1; 
};


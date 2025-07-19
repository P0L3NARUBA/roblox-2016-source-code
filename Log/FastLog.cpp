#include "FastLog.h"

#ifdef _WIN32
#include <windows.h>
#elif __ANDROID__
#include <sys/atomics.h>
#else
#include <libkern/OSAtomic.h>
#endif

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>

#include "rbx/atomic.h"
#include "rbx/rbxTime.h"
#include "rbx/boost.hpp"
#include "RbxPlatform.h"
#include "RbxFormat.h"

#include <fstream>
#include <string>
#include <boost/unordered_map.hpp>
#include <iomanip>
#include <algorithm>

#include <set>
#include <deque>


#define FLOAT_MARKER 0xFFFF10AD

#define MAX_LOG_MESSAGE 260

LOGVARIABLE(FastLogValueChanged, 1)

namespace FLog
{

	struct IValueGetSet
	{
	public:
		virtual void set(const std::string& valueData, FastVarType _fflagMask) = 0;
		virtual std::string get() = 0;
        virtual FastVarType getType() = 0;
        virtual void setIsSync(bool) = 0;
        virtual bool getIsSync() = 0;
        virtual bool* getIsSyncAddr() = 0;
	};

	template<class T> T convertFromString(const std::string& valueData);

	template<> int convertFromString<int>(const std::string& valueData)
	{
		return atoi(valueData.c_str());
	}

	template<> bool convertFromString<bool>(const std::string& valueData)
	{
		return (valueData == "true" || valueData == "True") ? true : false;
	}

	template<> std::string convertFromString<std::string>(const std::string& valueData)
	{
		return valueData;
	}

	template<> Channel convertFromString<unsigned char>(const std::string& valueData)
	{
		return (unsigned char)atoi(valueData.c_str());
	}

	std::string convertToString(bool a)
	{
		return a ? "true" : "false";
	}

	std::string convertToString(int a)
	{
#pragma warning(push)
#pragma warning(disable: 4996)
		char temp[200];
		snprintf(temp, 200-1, "%d", a);
		return temp;
#pragma warning(pop)
	}
	std::string convertToString(std::string a)
	{
		return a;
	}


	template<class T> class ValueGetSet : public IValueGetSet
	{
		T* value;
        FastVarType fastVarType;
        bool isSync;
	public:
		ValueGetSet(T* value, FastVarType _fastVarType) : value(value), fastVarType(_fastVarType), isSync(true) {}

		void set(const std::string& valueData, FastVarType _fflagMask) 
		{
            if (_fflagMask != FASTVARTYPE_ANY)
            {
                //assert(fastVarType == _fflagMask);
    			if(fastVarType != _fflagMask)
	    			return;
            }

			*value = convertFromString<T>(valueData); 
		}

		std::string get()
		{
			return convertToString(*value);
		}

        FastVarType getType()
        {
            return fastVarType;
        }

        void setIsSync(bool sync)
        {
            isSync = sync; 
        }

        bool getIsSync()
        {
            return isSync;
        }

        bool* getIsSyncAddr()
        {
            return &isSync;
        }
	};

	typedef boost::unordered_map<std::string, IValueGetSet*> Variables;
	typedef boost::unordered_map<std::string, std::string> UnknownVariables;

	Variables* gVariables;
	UnknownVariables* gUnknownVariables;


	typedef boost::unordered_map<std::string, Channel*> LogGroups;
	LogGroups* gLogGroups = 0;
	LogGroups* gDynamicLogGroups = 0;

	typedef boost::unordered_map<std::string, Channel> UnknownLogGroups;
	UnknownLogGroups* gUnknownLogGroups = 0;

	typedef boost::unordered_map<std::string, bool*> FastFlags;
	FastFlags* gFastFlags = 0;
	FastFlags* gDynamicFastFlags = 0;

	typedef boost::unordered_map<std::string, bool> UnknownFastFlags;
	UnknownFastFlags* gUnknownFastFlags = 0;

    
#if defined (__aarch64__)
    typedef double FASTFLAG_VAR_FLOAT;
#else
    typedef float FASTFLAG_VAR_FLOAT ;
#endif

	double timeDummy(){
		return 0.0;
	}

	TimeFunc timeF = timeDummy;

	ExternalLogFunc logF = NULL;
	

	// Exactly 32 bytes on 32bit platform, add stuff responsibly
	struct LogEntry
	{
		const char* message;
		float timestamp;
		unsigned threadid;
		union Args
		{
			struct IntArgs
			{
				const void* arg0;
				const void* arg1;
				const void* arg2;
				const void* arg3;
				const void* arg4;
			} intArgs;
			struct FloatArgs
			{
                FASTFLAG_VAR_FLOAT arg0f;
                FASTFLAG_VAR_FLOAT arg1f;
                FASTFLAG_VAR_FLOAT arg2f;
                FASTFLAG_VAR_FLOAT arg3f;
                FASTFLAG_VAR_FLOAT arg4f;
			} floatArgs;

			char sarg[sizeof(void*)*5];
		} args;
	};


	static LogEntry g_FastLog[LOGCHANNELS][LOG_HISTORY];
	
    static rbx::atomic<unsigned int> g_LogCounters[LOGCHANNELS] = {0, 0, 0, 0, 0};

	// flf == FastLogFloat
	inline void* flf(FASTFLAG_VAR_FLOAT value)
	{
		return reinterpret_cast<void*(&)>(value);
	}

	inline float flfBack(const void* v)
	{
        return static_cast<float>(reinterpret_cast<FASTFLAG_VAR_FLOAT&>(v));
	}

	void printMessage(
		char* result, 
		unsigned maxPrint, 
		size_t threadid,
        float timeStamp,
		const char* message, 
		const void* arg0, 
		const void* arg1, 
		const void* arg2, 
		const void* arg3, 
		const void* arg4,
		float* lastTimeStamp = NULL)
	{
#pragma warning(push)
#pragma warning(disable: 4996)
		float delta = 0;
		if(lastTimeStamp)
		{
			delta = (*lastTimeStamp - timeStamp)*1000.0f;
			*lastTimeStamp = timeStamp;
		}

		int printed = snprintf(result, maxPrint-1, "%.5f %.1f ms %x:", timeStamp, delta, unsigned(threadid));
		if(static_cast<int>(reinterpret_cast<long>(arg0)) == FLOAT_MARKER)
			snprintf(result + printed, MAX_LOG_MESSAGE-printed-1, message, flfBack(arg1), flfBack(arg2), flfBack(arg3), flfBack(arg4));
		else
			snprintf(result + printed, MAX_LOG_MESSAGE-printed-1, message, arg0, arg1, arg2, arg3, arg4);
#pragma warning(pop)
		return;
	}

    void printMessageFormatted(
        char* result, 
        unsigned maxPrint, 
        unsigned threadid,
        float timeStamp,
        const char* message, 
        float* lastTimeStamp = NULL)
    {
#pragma warning(push)
#pragma warning(disable: 4996)
        float delta = 0;
        if(lastTimeStamp)
        {
            delta = (*lastTimeStamp - timeStamp)*1000.0f;
            *lastTimeStamp = timeStamp;
        }

        int printed = snprintf(result, maxPrint-1, "%.5f %.1f ms %u:", timeStamp, delta, threadid);
        strncat(result, message, MAX_LOG_MESSAGE-printed-1);
#pragma warning(pop)
        return;
    }

	void printMessage(char* result, unsigned maxPrint, size_t threadid, float timeStamp, const char* message, const char* sarg, float* lastTimeStamp = NULL)
	{
#pragma warning(push)
#pragma warning(disable: 4996)
		float delta = 0;
		if(lastTimeStamp)
		{
			delta = (*lastTimeStamp - timeStamp)*1000.0f;
			*lastTimeStamp = timeStamp;
		}
		int printed = snprintf(result, maxPrint-1, "%.5f %.1f ms %x:", timeStamp, delta, unsigned(threadid));
		snprintf(result + printed, MAX_LOG_MESSAGE-printed-1, message, sarg);
#pragma warning(pop)
		return;
	}

    struct BinaryLogDumper
    {
		void* context;
        Detail::BinaryLogFunc callback;

        boost::mutex currentMutex;
        LogEntry* current;
        long counter;
        long history;

        boost::mutex writeQueueMutex;
        boost::condition_variable writeQueueHasData;
        std::deque<LogEntry*> writeQueue;

        boost::thread writeThread;

        BinaryLogDumper(void* context, Detail::BinaryLogFunc callback): context(context), callback(callback), current(0), counter(0), history(8192)
        {
			boost::thread t(boost::bind(&BinaryLogDumper::writeWorker, this));
            writeThread.swap(t);
        }

        static void append(std::vector<char>& s, const void* data, size_t size)
        {
            s.insert(s.end(), static_cast<const char*>(data), static_cast<const char*>(data) + size);
        }

        template <typename T> static void append(std::vector<char>& s, const T& v)
        {
            append(s, &v, sizeof(v));
        }

        void writeWorker()
        {
            std::set<const void*> strings;

            std::vector<char> data;

            while (true)
            {
                LogEntry* entry = NULL;

                {
                    boost::unique_lock<boost::mutex> lock(writeQueueMutex);

                    if (writeQueue.empty())
                        writeQueueHasData.wait(lock);

                    if (!writeQueue.empty())
                    {
                        entry = writeQueue.front();
                        writeQueue.pop_front();
                    }
                }

                if (entry)
                {
                    int header[2] = {};

                    data.resize(sizeof(header));

                    // append string data
                    for (int i = 0; i < history; ++i)
                    {
                        std::pair<std::set<const void*>::iterator, bool> p = strings.insert(entry[i].message);

                        if (p.second)
                        {
                            size_t length = strlen(entry[i].message);

                            header[0] += 1;

                            append(data, entry[i].message);
                            append(data, length);
                            append(data, entry[i].message, length);
                        }
                    }

                    // append entry data
                    header[1] = history;
                    append(data, entry, sizeof(entry[0]) * history);

                    // write!
                    memcpy(&data[0], header, sizeof(header));
                    callback(context, &data[0], data.size());

                    delete[] entry;
                }
            }
        }

        void addEntry(Channel level, const LogEntry& entry)
        {
            boost::unique_lock<boost::mutex> lock(currentMutex);
            
            if (counter == history)
            {
	            boost::unique_lock<boost::mutex> lock(writeQueueMutex);

                writeQueue.push_back(current);
                writeQueueHasData.notify_one();

                current = 0;
                counter = 0;
            }

            if (!current)
            {
                current = new LogEntry[history];
            }
            
            current[counter++] = entry;
        }
    };

    static BinaryLogDumper* gDumper;
    
	void FastLog(
		Channel level, 
		const char* message, 
		const void* arg0)
	{
		FastLog(level, message, arg0, 0, 0, 0, 0);
	}

	void FastLog(
		Channel level, 
		const char* message, 
		const void* arg0,
		const void* arg1,
		const void* arg2)
	{
		FastLog(level, message, arg0, arg1, arg2, 0, 0);
	}

    void FastLogFormatted(
        Channel level,
        const char* message, ... )
    {
        va_list pArgList;
        va_start( pArgList, message );
        char temp[MAX_LOG_MESSAGE];
#ifdef WIN32
        vsnprintf_s(temp,MAX_LOG_MESSAGE,message,pArgList);
#else
        vsnprintf(temp,MAX_LOG_MESSAGE,message,pArgList);
#endif
        va_end( pArgList );

        if (level > LOGCHANNELS)
        {
            if(logF)
            {
                char temp2[MAX_LOG_MESSAGE];
                memset(temp2, 0, sizeof(temp2));
                printMessageFormatted(temp2, MAX_LOG_MESSAGE, (unsigned)GetCurrentThreadId(), (float)timeF(), temp);
                logF(level - LOGCHANNELS, temp2);
                return;
            }
            else
                level = 1;
        }

        LogEntry entry;
        entry.message = temp;
        entry.timestamp = (float)timeF();
        entry.threadid = (unsigned)GetCurrentThreadId();

        rbx::atomic<unsigned int>& counter = g_LogCounters[level-1];

        unsigned int index = (--counter) % LOG_HISTORY;

        g_FastLog[level-1][index] = entry;

        if (gDumper) gDumper->addEntry(level, entry);
    }

    void FastLogFormattedNoAdorn(
        Channel level,
        const char* message, ... )
    {
        va_list pArgList;
        va_start( pArgList, message );
        char temp[MAX_LOG_MESSAGE];
#ifdef WIN32
        vsnprintf_s(temp,MAX_LOG_MESSAGE,message,pArgList);
#else
        vsnprintf(temp,MAX_LOG_MESSAGE,message,pArgList);
#endif
        va_end( pArgList );

        if (level > LOGCHANNELS)
        {
            if(logF)
            {
                logF(level - LOGCHANNELS, temp);
                return;
            }
            else
                level = 1;
        }
    }

	void FastLog(
		Channel level, 
		const char* message, 
		const void* arg0, 
		const void* arg1, 
		const void* arg2, 
		const void* arg3, 
		const void* arg4)
	{
		if (level > LOGCHANNELS)
		{
			if(logF)
			{
				char temp[MAX_LOG_MESSAGE];
				memset(temp, 0, sizeof(temp));
				printMessage(temp, MAX_LOG_MESSAGE, GetCurrentThreadId(), (float)timeF(), message, arg0, arg1, arg2, arg3, arg4);
				logF(level - LOGCHANNELS, temp);
				return;
			}
			else
				level = 1;
		}

		LogEntry entry;
		entry.message = message;
		entry.args.intArgs.arg0 = arg0;
		entry.args.intArgs.arg1 = arg1;
		entry.args.intArgs.arg2 = arg2;
		entry.args.intArgs.arg3 = arg3;
		entry.args.intArgs.arg4 = arg4;
		entry.timestamp = (float)timeF();
		entry.threadid = GetCurrentThreadId();

        rbx::atomic<unsigned int>& counter = g_LogCounters[level-1];

		unsigned int index = (--counter) % LOG_HISTORY;

		g_FastLog[level-1][index] = entry;

        if (gDumper) gDumper->addEntry(level, entry);
	}

	void Init(TimeFunc tF)
	{ 
		timeF = tF;
	}

	void SetExternalLogFunc(ExternalLogFunc lF)
	{
		logF = lF;
	}


	void FastLogF(Channel channel, const char* message, float arg1, float arg2, float arg3, float arg4)
	{
		FastLog(channel, message, (void*)FLOAT_MARKER, flf(arg1), flf(arg2), flf(arg3), flf(arg4));
	}


	void FastLogS(Channel level, const char* message, const char* sarg, int sargsize)
	{
		if (level > LOGCHANNELS)
		{
			if(logF)
			{
				char temp[MAX_LOG_MESSAGE];
				memset(temp, 0, sizeof(temp));
				printMessage(temp, MAX_LOG_MESSAGE, GetCurrentThreadId(), (float)timeF(), message, sarg);
				logF(level - LOGCHANNELS, temp);
				return;
			}
			else
				level = 1;
		}

        LogEntry entry;
		entry.message = message;

#pragma warning(push)
#pragma warning(disable: 4996)
		if(sarg != NULL)
			{
			const char* start = &sarg[ std::max(0, sargsize-(int)(sizeof(entry.args.sarg)-1)) ];
			strncpy(entry.args.sarg, start, sizeof(entry.args.sarg));
			entry.args.sarg[sizeof(entry.args.sarg)-1] = '\0';
		}
		else
			strncpy(entry.args.sarg, "NULL", sizeof(entry.args.sarg));
#pragma warning(pop)

		entry.timestamp = (float)timeF();
		entry.threadid = GetCurrentThreadId();

        rbx::atomic<unsigned int>& counter = g_LogCounters[level-1];

		unsigned int index = (--counter) % LOG_HISTORY;

		g_FastLog[level-1][index] = entry;

        if (gDumper) gDumper->addEntry(level, entry);
	}

	void FastLogS(Channel level, const char* message, const char* sarg)
    {
        FastLogS(level, message, sarg, sarg ? (int)strlen(sarg) : 0);
    }

	void FastLogS(Channel level, const char* message, const std::string& sarg)
	{
        FastLogS(level, message, sarg.c_str(), (int)sarg.size());
    }
	
	void WriteFastLogDump(const char *fileName, int numEntries)
	{
		std::ofstream logFileStream(fileName);
		if(logFileStream.fail())
			return;
		
		if (numEntries > LOG_HISTORY)
			numEntries = LOG_HISTORY;
		
		for (int ii = 0; ii < LOGCHANNELS; ++ii)
		{
			float lastTimeStamp = 0;
			logFileStream << "Level "<<ii<<"\n";
			
			if (!g_LogCounters[ii])
				continue;
			
			unsigned int currentIndex = g_LogCounters[ii] % LOG_HISTORY;
			
			for (int currentCount = 1; currentCount <= numEntries; ++currentCount)
			{
				if (currentIndex >= LOG_HISTORY)
					currentIndex = 0;
				
				LogEntry* entry=&(g_FastLog[ii][currentIndex++]);
				if (!entry || !entry->message)
					continue;

				char buffer[MAX_LOG_MESSAGE];
				if (strstr(entry->message, "%s"))
					printMessage(buffer, MAX_LOG_MESSAGE, entry->threadid, entry->timestamp, entry->message, entry->args.sarg, &lastTimeStamp);
				else 
					printMessage(buffer, MAX_LOG_MESSAGE, entry->threadid, entry->timestamp, entry->message, entry->args.intArgs.arg0, entry->args.intArgs.arg1, entry->args.intArgs.arg2, entry->args.intArgs.arg3, entry->args.intArgs.arg4, &lastTimeStamp);

				logFileStream << buffer <<"\n";
			}
			
			logFileStream.flush();
		}
		
		logFileStream.close();
	}

    unsigned short GetNumSynchronizedVariable()
    {
        unsigned short count = 0;
        for (Variables::iterator iter = gVariables->begin(); iter != gVariables->end(); iter++)
        {
            if ((iter->second->getType() & FASTVARTYPE_SYNC) > 0)
            {
                count++;
            }
        }
        return count;
    }

	void ResetSynchronizedVariablesState()
	{
		for (Variables::iterator iter = gVariables->begin(); iter != gVariables->end(); iter++)
		{
			if ((iter->second->getType() & FASTVARTYPE_SYNC) > 0)
			{
				iter->second->setIsSync(false);
			}
		}
	}

	int GetFastLogCounter(Channel channel)
	{
		if (channel > LOGCHANNELS)
			channel = 1;

		return g_LogCounters[channel-1];
	}

	template<class T> void RegisterVariable(const char* varName, T* value, bool** isSync, FastVarType fastVarType)
	{
		if(gVariables == NULL)
			gVariables = new Variables();

		if(gUnknownVariables == NULL)
			gUnknownVariables = new UnknownVariables();


		std::string name = varName;

		assert(gVariables->find(name) == gVariables->end());

        ValueGetSet<T>* varValue = new ValueGetSet<T>(value, fastVarType);
		gVariables->insert(Variables::value_type(name, varValue));
        if (isSync != NULL)
        {
            *isSync = varValue->getIsSyncAddr();
        }

		UnknownVariables::iterator itUnknown = gUnknownVariables->find(varName);
		if(itUnknown != gUnknownVariables->end())
		{
			Variables::iterator itVars = gVariables->find(name);
			itVars->second->set(itUnknown->second, fastVarType);

            gUnknownVariables->erase(itUnknown);
		}
	}

	int RegisterLogGroup(const char* groupName, Channel* groupVar, bool** isSync, FastVarType fastVarType)
	{
		RegisterVariable(groupName, groupVar, isSync, fastVarType);
		return 1;
	}

	int RegisterString(const char* groupName, std::string* groupVar, bool** isSync, FastVarType fastVarType)
	{
		RegisterVariable(groupName, groupVar, isSync, fastVarType);
		return 1;
	}

	int RegisterInt(const char* groupName, int* groupVar, bool** isSync, FastVarType fastVarType)
	{
		RegisterVariable(groupName, groupVar, isSync, fastVarType);
		return 1;
	}

	int RegisterFlag(const char* flagName, bool* flagVar, bool** isSync, FastVarType fastVarType)
	{
		RegisterVariable(flagName, flagVar, isSync, fastVarType);
		return 1;
	}

	static void visitVariable(Variables::value_type pair, VariableVisitor visitor, void* context)
	{
		visitor(pair.first.c_str(), pair.second->get(), context);
	}

	void ForEachVariable(VariableVisitor visitor, void* context, FastVarType flagType)
	{
        if (flagType == FASTVARTYPE_ANY)
        {
		    std::for_each(gVariables->begin(), gVariables->end(), boost::bind(visitVariable, _1, visitor, context));
        }
        else
        {
            for (Variables::iterator iter = gVariables->begin(); iter != gVariables->end(); iter++)
            {
                if ((iter->second->getType() & flagType) > 0)
                {
                    visitor(iter->first.c_str(), iter->second->get(), context);
                }
            }
        }
	}

    bool SetValue(const std::string& name, const std::string& value, FastVarType fastVarType, bool loadedFromServer)
	{
		if(!gVariables)
			return false;

		if(!gUnknownVariables)
			gUnknownVariables = new UnknownVariables();

		bool result = false;

		Variables::iterator it = gVariables->find(name);
		if(it != gVariables->end())
		{
			it->second->set(value, fastVarType);
            if (loadedFromServer)
            {
                it->second->setIsSync(true);
            }
			result = true;
		}
		else
		{
			(*gUnknownVariables)[name] = value;
		}

		FASTLOGS(FLog::FastLogValueChanged, "Setting variable %s", name);
		FASTLOGS(FLog::FastLogValueChanged, "...to value %s", value);
		return result;
	}

	bool GetValue(const std::string& name, std::string& value, bool alsoCheckUnknown)
	{
		Variables::iterator it = gVariables->find(name);
		if (it != gVariables->end())
		{
			value = it->second->get();
			return true;
		}
		else
		{
			if (alsoCheckUnknown)
			{
				UnknownVariables::const_iterator unknownIt = gUnknownVariables->find(name);
				if (unknownIt != gUnknownVariables->end())
				{
					value = unknownIt->second;
					return true;
				}
			}
			return false;
		}
	}

    bool SetValueFromServer(const std::string& name, const std::string& value)
    {
        return SetValue(name, value, FASTVARTYPE_SYNC, true);
    }

	double NowFast()
	{
		return timeF();
	}

    namespace Detail
    {
        void SetBinaryLog(void* context, BinaryLogFunc callback)
        {
            assert(!gDumper);
            
            BinaryLogDumper* dumper = new BinaryLogDumper(context, callback);
            gDumper = dumper;
        }
    }
}

#include "FastLogStream.h"
#include <stdio.h>

#ifdef __ANDROID__
#include <unistd.h>
#endif

#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

typedef SOCKET socket_t;

#define SOCKET_VALID(s) (s != INVALID_SOCKET)
#define SOCKET_CLOSE(s) closesocket(s)
#else
#include <sys/socket.h>
#include <arpa/inet.h>

typedef int socket_t;

#define SOCKET_VALID(s) (s >= 0)
#define SOCKET_CLOSE(s) close(s)

static int fopen_s(FILE** pFile, const char *filename, const char *mode) {
	(*pFile) = fopen(filename, mode);
	return (*pFile) != NULL ? 0 : -1;
}
#endif

namespace
{
	void LogStreamFileWrite(void* context, const void* data, size_t size)
	{
		fwrite(data, 1, size, static_cast<FILE*>(context));
	}
	
	void LogStreamSocketWrite(void* context, const void* data, size_t size)
	{
        // socket_t for windows and socklen_t for Apple
		send(static_cast<socket_t>(reinterpret_cast<long>(context)), static_cast<const char*>(data), (int)size, 0);
	}
}

namespace FLog
{
	bool SetLogStreamFile(const char* path)
	{
		FILE* file;
		
		if (fopen_s(&file, path, "wb") == 0)
		{
            printf("LogStream: Dumping to file %s\n", path);

			Detail::SetBinaryLog(file, LogStreamFileWrite);

            return true;
		}

        return false;
	}

	bool SetLogStreamNetworkClient(const char* address, unsigned int port)
	{
	#ifdef _WIN32
		WSADATA wsaData;
		WSAStartup(MAKEWORD(1, 1), &wsaData);
	#endif
	
		socket_t sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		
		if (SOCKET_VALID(sock))
		{
			sockaddr_in sin = {};
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = inet_addr(address);
			sin.sin_port = htons(port);
			
            printf("LogStream: Connecting to %s:%d\n", address, port);

			if (connect(sock, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) >= 0)
			{
                printf("LogStream: Dumping to socket\n");

				Detail::SetBinaryLog(reinterpret_cast<void*>(sock), LogStreamSocketWrite);

                return true;
			}

            SOCKET_CLOSE(sock);
		}

        return false;
	}

	bool SetLogStreamNetworkServer(unsigned int port)
    {
	#ifdef _WIN32
		WSADATA wsaData;
		WSAStartup(MAKEWORD(1, 1), &wsaData);
	#endif
	
		socket_t sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		
		if (SOCKET_VALID(sock))
		{
			sockaddr_in sin = {};
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			sin.sin_port = htons(port);

            if (bind(sock, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) >= 0)
            {
                if (listen(sock, 1) >= 0)
                {
                    printf("LogStream: Listening for incoming connection on port %d\n", port);

                    socket_t client = accept(sock, NULL, NULL);

                    if (SOCKET_VALID(client))
                    {
                        printf("LogStream: Dumping to socket\n");

                        Detail::SetBinaryLog(reinterpret_cast<void*>(client), LogStreamSocketWrite);

                        return true;
                    }
                }
            }

            SOCKET_CLOSE(sock);
		}

        return false;
    }
}

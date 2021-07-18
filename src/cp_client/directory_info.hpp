#pragma once

const int MAX_BUFFER = 4096;
const int s_notifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION;

typedef struct _DIRECTORY_INFO
{
    HANDLE      hDir;
    TCHAR       lpszDirName[MAX_PATH];
    CHAR        lpBuffer[MAX_BUFFER];
    DWORD       dwBufLength;
    OVERLAPPED  Overlapped;
} DIRECTORY_INFO, * PDIRECTORY_INFO, * LPDIRECTORY_INFO;
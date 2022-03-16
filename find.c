#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MIN_ALLOC 320

char* dirWildCard = "\\*";
int MAX_THREADS = 7;
typedef struct _SEARCH_INFO {
	char path[MAX_PATH];
	char* query;
} SEARCH_INFO;


void SearchFile(SEARCH_INFO* searchInfo){
	int lineCounter = 1;
	char* buffer = NULL;
	HANDLE hFile = CreateFileA(searchInfo->path,
		GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	char* query = searchInfo->query;
	int queryLen = strlen(query);
		
	if (hFile == INVALID_HANDLE_VALUE){
		goto epilog;
	}
	int fileSize = GetFileSize(hFile, NULL);
	buffer = VirtualAlloc(NULL, fileSize, MEM_COMMIT, PAGE_READWRITE);
	if (!ReadFile(hFile, buffer, fileSize, NULL, NULL)){
		goto epilog;
	}
	for (int i = 0; i < fileSize; i++){
		if (buffer[i] == '\n'){
			lineCounter++;
		}
		if (!memcmp(&buffer[i], query, queryLen)){
			printf("FILE: %s\nLINE: %d\n\n", 
			searchInfo->path, lineCounter);
		}
	}
	
	epilog:
	if (buffer){
		VirtualFree(buffer,0, MEM_RELEASE);
	}
	if (hFile != INVALID_HANDLE_VALUE){
		CloseHandle(hFile);
	}
	ExitThread(0);
	
}
int _min(int a, int b){
	if (a > b)
		return b;
	return a;
}

void ScanAllFiles(char* path, char* query){
	int count = 0;
	int thread_count;
	int dir_count;
	char buf[MAX_PATH];
	char dirs[8][MAX_PATH];
	HANDLE threads[MAX_THREADS];
	SEARCH_INFO searches[MAX_THREADS];
	WIN32_FIND_DATAA files[1024];
	HANDLE hFindFile = FindFirstFileA(path, &files[count++]);
	if (hFindFile == INVALID_HANDLE_VALUE){
		goto epilog;
	}
	do {
		if (count == 1024){
			exit(1);
		}
	} while (FindNextFileA(hFindFile, &files[count++]));	
	int i = 0;
	int j;
	while (count > 0){ // while(count) would loop infinitely 
		// unless you do count -= _min(MAX_THREADS, count) no point though 
		thread_count = 0;
		dir_count = 0;
		j = i + _min(count,MAX_THREADS);
		for (; i < j; i++){
			if (files[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				char* dirName = (char*)files[i].cFileName;
				if (!strcmp(dirName, "..") || !strcmp(dirName, ".")){
					continue;
				}
				strcpy(dirs[dir_count], dirName);
				strcat(dirs[dir_count], dirWildCard);
				dir_count++;
			}
			else {
				if (!strlen(files[i].cFileName)){
					continue;
				}
				strcpy(searches[i].path, files[i].cFileName);
				searches[i].query = query;
				threads[thread_count++] = CreateThread(NULL, 0, 
				(LPTHREAD_START_ROUTINE)SearchFile, &searches[i],0,NULL);
			}
		}
		i += MAX_THREADS;
		count -= MAX_THREADS;
		WaitForMultipleObjects(thread_count,
			threads, TRUE, INFINITE);
		// do it this way to make sure
		// no more than MAX_THREADS+1 execute at a time
		for (int k = 0; k < dir_count; k++){
			ScanAllFiles(dirs[k], query);
		}
	}
	
	epilog:
	if (hFindFile != INVALID_HANDLE_VALUE){
		FindClose(hFindFile);
	}
}
int getproccount(){
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
	/*
	int count = 0; // terribly inefficient way to count bits 
	// but it's only done once so it's OK
	DWORD_PTR processAffinity;
	DWORD_PTR systemAffinity;
	GetProcessAffinityMask((HANDLE)-1, &processAffinity, &systemAffinity);
	for (int i = 0; i < sizeof(DWORD_PTR) * 8; i++){
		if (processAffinity & 1 == 1)
			count++;
		processAffinity >> 1;
	}
	return count;*/
}


int main(int argc, char** argv) {
	if (argc != 2){
		printf("find_alt.exe <str>\n");
		return 0;
	}
	char buf[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buf);
	strcat(buf,dirWildCard);
	MAX_THREADS = getproccount()-1;
	ScanAllFiles(buf, argv[1]);
	return 0;
}

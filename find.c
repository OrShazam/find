#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MIN_ALLOC 320
#define MAX_THREADS 8

char* dirWildCard = "\\*";

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
			printf("FILE: %s\nLINE: %d\n", 
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
	char buf[MAX_PATH];
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
		j = i + _min(count,MAX_THREADS);
		for (; i < j; i++){
			if (files[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				char* dirName = (char*)files[i].cFileName;
				if (!strcmp(dirName, "..") || !strcmp(dirName, ".")){
					continue;
				}
				strcpy(buf, dirName);
				strcat(buf, dirWildCard);
				ScanAllFiles(buf, query);
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
	}
	
	epilog:
	if (hFindFile != INVALID_HANDLE_VALUE){
		FindClose(hFindFile);
	}
}


int main(int argc, char** argv) {
	if (argc != 2){
		printf("find_alt.exe <str>\n");
		return 0;
	}
	char buf[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buf);
	strcat(buf,dirWildCard);
	ScanAllFiles(buf, argv[1]);
	return 0;
}
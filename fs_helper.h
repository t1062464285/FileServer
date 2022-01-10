#ifndef _FS_HELPER_H
#define _FS_HELPER_H

#include "fs_client.h"
#include "fs_param.h"
#include "fs_server.h"
#include "servers.h"
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <queue>

//void init();
void readblock(char* msg_copy, int connectionfd, char* username, const char* pathname, char* block);
void writeblock(char* msg_copy, int connectionfd, char* username, const char* pathname, char* block, char* data);
bool create(char* msg_copy, int connectionfd, char* username, const char* pathname, char type);
bool delete_fd(char* msg_copy, int connectionfd, char* username, const char* pathname);

#endif

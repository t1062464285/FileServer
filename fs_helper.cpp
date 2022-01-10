#include "fs_helper.h"
#include "fs_client.h"
#include "fs_param.h"
#include "fs_server.h"
#include "servers.h"
#include <cassert>
#include <chrono>
#include <cstring>
#include <map>
#include <queue>
#include <string>
#include <thread>
#include <vector>
using namespace std;

map<int, std::mutex> mutex_map;

queue<uint32_t> available_blocks;
mutex available_blocks_mutex;
mutex cout_mutex;

/*
    pathname:"/a/b/c" -> parsed_path: [a, b, c]
    return 1 if seccess, 0 if fails
*/
bool parse_path(vector<string> &parsed_path, string pathname) {
    assert(parsed_path.size() == 0);

    if (pathname[0] != '/' || pathname[pathname.length() - 1] == '/')
        return false;

    string temp_str;
    bool first_char = true;
    for (unsigned int i = 0; i < pathname.length(); ++i) {
        if (pathname[i] != '/') {
            temp_str += pathname[i];
        } else {              // '/'
            if (first_char) { //first '/'
                first_char = false;
            } else {
                if (temp_str.length() == 0 || temp_str.length() > FS_MAXFILENAME) {
                    parsed_path.clear();
                    return false;
                }
                parsed_path.push_back(temp_str);
                temp_str.clear();
            }
        }
    }
    if (temp_str.length() == 0 || temp_str.length() > FS_MAXFILENAME) {
        return false;
    }

    parsed_path.push_back(temp_str);

    return true;
}

// check if the current user own this inode
bool is_owner(fs_inode &inode, string username) {
    return !strcmp(inode.owner, username.c_str());
}

/*  given a pathname and the current inode block, find the next inode block
    if success, return the inode block num
    if fail, return -1
*/
int find_inode_in_entries(fs_inode &inode, string curr_path, string username, fs_inode &found_inode) {
    for (unsigned int i = 0; i < inode.size; i++) {
        // read in the entries
        fs_direntry entries[FS_DIRENTRIES];
        disk_readblock(inode.blocks[i], &entries);

        for (int idx = 0; idx < int(FS_DIRENTRIES); idx++) {
            fs_direntry entry = entries[idx];
            if (entry.inode_block != 0) {
                if (strcmp(entry.name, curr_path.c_str()) == 0) {

                    fs_inode temp_inode;
                    disk_readblock(entry.inode_block, &temp_inode);

                    if (strcmp(username.c_str(), temp_inode.owner) == 0) {
                        found_inode = temp_inode;
                        return entry.inode_block;
                    }
                }
            }
        }
    }
    return -1;
}

/*  given a pathname and the current inode block, find the next inode block
    if success, return the inode block num
    if fail, return -1
*/
bool create_check_inode_exist(fs_inode &inode, string curr_path, string username) {
    for (unsigned int i = 0; i < inode.size; i++) {
        // read in the entries
        fs_direntry entries[FS_DIRENTRIES];
        disk_readblock(inode.blocks[i], &entries);

        for (int idx = 0; idx < int(FS_DIRENTRIES); idx++) {
            fs_direntry entry = entries[idx];
            if (entry.inode_block != 0) {
                if (strcmp(entry.name, curr_path.c_str()) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

/* Given a file path, return the block number of the inode(dir or file)
   If we can't find the path(owner validation fail, or the last block is not the file block), return -1
   Otherwise, return the block number
*/
int find_fd_block(vector<string> path_vec, string username, unique_lock<mutex> &new_mutex, fs_inode &found_node) {
    int curr_inode_block = 0;

    if (path_vec.size() == 0) {
        fs_inode inode;
        disk_readblock(curr_inode_block, &inode);
        found_node = inode;
        return curr_inode_block;

    } else {
        fs_inode inode;
        disk_readblock(curr_inode_block, &inode);

        while (path_vec.size() > 0) {

            // if the user is not the owner of this dir/file
            if (curr_inode_block != 0 && !is_owner(inode, username)) {
                return -1;
            }

            string curr_path = path_vec[0];

            int next_inode_block = find_inode_in_entries(inode, curr_path, username, found_node);
            if (next_inode_block == -1) {
                return -1;
            }

            path_vec.erase(path_vec.begin());
            curr_inode_block = next_inode_block;
            inode = found_node;

            unique_lock<mutex> old_mutex(mutex_map[curr_inode_block]); //old mutex lock the new inode, new mutex lock the old inode
            swap(new_mutex, old_mutex);                                // new mutex lock the new inode, old mutext lock the old inode
            // old mutex go out of scope and unlock the old inode
        }
        return curr_inode_block;
    }
}

void readblock(char *msg_copy, int connectionfd, char *username, const char *pathname, char *block) {
    // get the path to the parsed path vector
    vector<string> path_vec;
    string path_str(pathname);
    if (!parse_path(path_vec, path_str)) {
        return;
    }

    // first lock the root dir
    unique_lock<mutex> new_mutex(mutex_map[0]);
    fs_inode file_inode;
    int file_inode_block = find_fd_block(path_vec, username, new_mutex, file_inode);

    // if we can find the file inode
    if (file_inode_block != -1) {

        // disk_readblock(file_inode_block, &file_inode);
        if (file_inode.type != 'f') {
            return;
        }

        if (atoi(block) >= int(file_inode.size)) {
            return;
        } else {
            int data_block = file_inode.blocks[atoi(block)];
            char data[FS_BLOCKSIZE];
            disk_readblock(data_block, &data);

            char new_msg[strlen(msg_copy) + 1 + FS_BLOCKSIZE];

            for (unsigned int i = 0; i < strlen(msg_copy) + 1; i++) {
                new_msg[i] = msg_copy[i];
            }

            for (unsigned int i = 0; i < FS_BLOCKSIZE; i++) {
                new_msg[strlen(msg_copy) + 1 + i] = data[i];
            }

            // send the response message
            send(connectionfd, new_msg, strlen(msg_copy) + 1 + FS_BLOCKSIZE, MSG_NOSIGNAL);
        }

        // if we can't find the file inode
    } else {
        return;
    }
    // std::this_thread::sleep_for(10000ms);
}

/* if success: return the response msg(orginal msg + data)
   if fail: return
*/
void writeblock(char *msg_copy, int connectionfd, char *username, const char *pathname, char *block, char *data) {
    // get the path to the parsed path vector
    vector<string> path_vec;
    string path_str(pathname);
    if (!parse_path(path_vec, path_str)) {
        return;
    }

    // first lock the root dir
    unique_lock<mutex> new_mutex(mutex_map[0]);

    fs_inode file_inode;
    int file_inode_block = find_fd_block(path_vec, username, new_mutex, file_inode);

    // if we can find the file inode
    if (file_inode_block != -1) {

        // disk_readblock(file_inode_block, &file_inode);
        if (file_inode.type != 'f') {
            return;
        }

        int data_block;
        // if the given block don't exist
        if (atoi(block) == int(file_inode.size)) {
            // if (file_inode.blocks[atoi(block)] == 0) {
            // unique_lock<mutex> available_blocks_lock(available_blocks_mutex);
            if (atoi(block) < FS_BLOCKSIZE) {
                // allocate a new block to write
                available_blocks_mutex.lock();
                if (available_blocks.size() > 0) {
                    data_block = available_blocks.front();
                    available_blocks.pop();
                } else {
                    available_blocks_mutex.unlock();
                    return;
                }
                available_blocks_mutex.unlock();

                file_inode.blocks[atoi(block)] = data_block;
                file_inode.size++;
                disk_writeblock(data_block, data);
                disk_writeblock(file_inode_block, &file_inode);
                send(connectionfd, msg_copy, strlen(msg_copy) + 1, MSG_NOSIGNAL);
            } else {
                return;
            }

        } else if (atoi(block) < int(file_inode.size)) {
            data_block = file_inode.blocks[atoi(block)];
            disk_writeblock(data_block, data);

            // send the response message
            send(connectionfd, msg_copy, strlen(msg_copy) + 1, MSG_NOSIGNAL);
        } else {
            return;
        }

        // if we can't find the file inode
    } else {
        return;
    }
}

/* create */
void init_new_node(fs_inode &new_inode, char *username, char type) {
    new_inode.type = type;
    strcpy(new_inode.owner, username);
    new_inode.size = 0;
    for (unsigned int i = 0; i < FS_MAXFILEBLOCKS; ++i) {
        new_inode.blocks[i] = 0;
    }
}

bool create_in_new_entries(fs_inode &parent_inode, int parent_inode_block, fs_inode &new_inode, string last_fd_name, char *msg_copy, int connectionfd) {
    //initialize direntries
    fs_direntry entry;
    entry.inode_block = 0;
    string temp_string = "undefined";
    strcpy(entry.name, temp_string.c_str());
    fs_direntry entries[FS_DIRENTRIES];
    for (unsigned int i = 0; i < FS_DIRENTRIES; ++i) {
        entries[i] = entry;
    }

    //get new blk_numbers from avaible_blocks queue
    unique_lock<mutex> available_blocks_lock(available_blocks_mutex);
    if (available_blocks.size() < 2) {
        return false;
    }
    uint32_t inode_blk_num = available_blocks.front();
    available_blocks.pop();
    uint32_t entries_blk_num = available_blocks.front();
    available_blocks.pop();

    mutex_map[inode_blk_num];

    //always allocate the first entry in the new direntries
    entries[0].inode_block = inode_blk_num;
    strcpy(entries[0].name, last_fd_name.c_str());

    //update the parent inode
    parent_inode.blocks[parent_inode.size] = entries_blk_num;
    parent_inode.size += 1;

    //update parent inode, and create direntries and  new inode
    disk_writeblock(inode_blk_num, &new_inode);
    disk_writeblock(entries_blk_num, &entries);
    disk_writeblock(parent_inode_block, &parent_inode);

    send(connectionfd, msg_copy, strlen(msg_copy) + 1, MSG_NOSIGNAL);
    return true;
}

bool create(char *msg_copy, int connectionfd, char *username, const char *pathname, char type) {
    vector<string> path_vec;
    string path_str(pathname);
    if (!parse_path(path_vec, path_str)) {
        return false;
    }
    assert(path_vec.size() > 0);
    //first lock the root dir
    unique_lock<mutex> new_mutex(mutex_map[0]);

    //store the path/file name
    string last_fd_name = path_vec.back();

    path_vec.pop_back();
    fs_inode parent_inode;
    int parent_inode_block = find_fd_block(path_vec, username, new_mutex, parent_inode);

    if (parent_inode_block != -1) {
        if (parent_inode.type == 'f') {
            return false;
        }
        available_blocks_mutex.lock();
        if (available_blocks.empty()) {
            available_blocks_mutex.unlock();
            return false;
        }
        available_blocks_mutex.unlock();

        fs_direntry entries[FS_DIRENTRIES];
        bool inode_exist = false;

        bool exist_empty_slot = false;
        fs_direntry exist_empty_entries[FS_DIRENTRIES];
        uint32_t exist_empty_inode_number = parent_inode.size;
        unsigned int exist_empty_entry_idx = FS_DIRENTRIES;

        /*** Check if the inode exist, if exist, return false ***/
        for (unsigned int i = 0; i < parent_inode.size; i++) {
            // read in the entries
            disk_readblock(parent_inode.blocks[i], &entries);

            for (unsigned int idx = 0; idx < FS_DIRENTRIES; idx++) {
                fs_direntry entry = entries[idx];
                if (entry.inode_block != 0) {
                    if (strcmp(entry.name, last_fd_name.c_str()) == 0) {
                        inode_exist = true;
                        break;
                    }
                } else if (!exist_empty_slot) {
                    exist_empty_slot = true;
                    for (unsigned int j = 0; j < FS_DIRENTRIES; ++j) {
                        exist_empty_entries[j] = entries[j];
                    }
                    exist_empty_inode_number = i;
                    exist_empty_entry_idx = idx;
                }
            }
        }

        if (inode_exist) {
            return false;
        }

        /*** The inode don't exist, create inode ***/
        fs_inode new_inode;
        init_new_node(new_inode, username, type);

        // check if we can create the new inode in current entries
        if (exist_empty_slot) {
            assert(exist_empty_inode_number < parent_inode.size);
            assert(exist_empty_entry_idx < FS_DIRENTRIES);
            fs_direntry entry = exist_empty_entries[exist_empty_entry_idx];
            unique_lock<mutex> available_blocks_lock(available_blocks_mutex);
            if (available_blocks.empty()) {
                return false;
            }
            strcpy(entry.name, last_fd_name.c_str());

            uint32_t new_inode_block = available_blocks.front();
            available_blocks.pop();

            entry.inode_block = new_inode_block;
            exist_empty_entries[exist_empty_entry_idx] = entry;

            mutex_map[new_inode_block];

            //update direntry and create the new inode
            disk_writeblock(new_inode_block, &new_inode);
            disk_writeblock(parent_inode.blocks[exist_empty_inode_number], &exist_empty_entries);

            send(connectionfd, msg_copy, strlen(msg_copy) + 1, MSG_NOSIGNAL);
            return true;
        } else { // if there aren't any new entry, need to create a new set of entries
            if (parent_inode.size >= FS_MAXFILEBLOCKS) {
                return false;
            }
            return create_in_new_entries(parent_inode, parent_inode_block, new_inode, last_fd_name, msg_copy, connectionfd);
        }
    } else {
        return false;
    }
} //end of create

/* delete helper */
void remove_from_arr(fs_inode &inode, int idx) {
    for (unsigned int i = idx; i < inode.size - 1; i++) {
        inode.blocks[i] = inode.blocks[i + 1];
    }
}

bool empty_entry_set(fs_inode &inode, int idx) {
    fs_direntry entries[FS_DIRENTRIES];
    disk_readblock(inode.blocks[idx], &entries);

    for (int i = 0; i < int(FS_DIRENTRIES); i++) {
        if (entries[i].inode_block != 0) {
            return false;
        }
    }
    return true;
}

bool empty_dir(fs_inode &inode) {
    return inode.size == 0;
}

/* given an inode, loop through its sets of entries and delete the given path */
bool delete_from_entries_sets(fs_inode &inode, int inode_block, string curr_path, char *username, char *msg_copy, int connectionfd) {
    // loop over the set of entries
    for (unsigned int set_idx = 0; set_idx < inode.size; set_idx++) {
        // read in the entries
        fs_direntry entries[FS_DIRENTRIES];
        disk_readblock(inode.blocks[set_idx], &entries);

        for (int entry_idx = 0; entry_idx < int(FS_DIRENTRIES); entry_idx++) {
            // fs_direntry entry = entries[entry_idx];
            bool non_empty_entry = entries[entry_idx].inode_block != 0;
            // if our current entry points to the last path we need to find
            if (non_empty_entry && strcmp(entries[entry_idx].name, curr_path.c_str()) == 0) {
                unique_lock<mutex> next_node_mutex(mutex_map[entries[entry_idx].inode_block]);
                fs_inode next_inode;
                disk_readblock(entries[entry_idx].inode_block, &next_inode);
                //unique_lock<mutex> next_node_mutex(mutex_map[entry.inode_block]);

                if (is_owner(next_inode, username)) {

                    // if we are deleting a file
                    if (next_inode.type == 'f') {
                        fs_direntry entry = entries[entry_idx];

                        unique_lock<mutex> available_blocks_lock(available_blocks_mutex);
                        // 1. make all file data content block available
                        for (unsigned int j = 0; j < next_inode.size; j++) {
                            available_blocks.push(next_inode.blocks[j]);
                        }

                        // 2. remove the file node(next_inode) mutex from mutex map
                        auto itr = mutex_map.find(entry.inode_block);
                        mutex_map.erase(itr);

                        // 3. make the file inode block available
                        available_blocks.push(entry.inode_block);

                        // 4. write/remove the entries
                        int non_empty_entry_num = 0;
                        for (int i = 0; i < int(FS_DIRENTRIES); i++) {
                            if (entries[i].inode_block != 0) {
                                non_empty_entry_num++;
                            }
                        }

                        // if this set size is 1(nothing left after delete), remove it from inode block, don't write
                        if (non_empty_entry_num == 1) {
                            remove_from_arr(inode, set_idx);
                            available_blocks.push(inode.blocks[set_idx]);
                            inode.size--;
                            disk_writeblock(inode_block, &inode);

                            // if the set size greated than 1, remove the entry pointing to the inode from entries
                        } else {
                            entries[entry_idx].inode_block = 0;
                            disk_writeblock(inode.blocks[set_idx], &entries);
                        }

                        // 5. send response message
                        send(connectionfd, msg_copy, strlen(msg_copy) + 1, MSG_NOSIGNAL);
                        return true;

                        // if we are deleting a dir
                    } else {
                        if (!empty_dir(next_inode)) {
                            return false;

                        } else {
                            fs_direntry entry = entries[entry_idx];

                            unique_lock<mutex> available_blocks_lock(available_blocks_mutex);
                            // 1. make the dir inode(next_inode) block available
                            available_blocks.push(entry.inode_block);

                            // 2. remove the dir node(next_inode) mutex from mutex map
                            auto itr = mutex_map.find(entry.inode_block);
                            mutex_map.erase(itr);

                            // 3. write/remove the entries
                            int non_empty_entry_num = 0;
                            for (int i = 0; i < int(FS_DIRENTRIES); i++) {
                                if (entries[i].inode_block != 0) {
                                    non_empty_entry_num++;
                                }
                            }

                            // if this set size is 1(nothing left after delete), remove it from inode block, don't write
                            if (non_empty_entry_num == 1) {
                                remove_from_arr(inode, set_idx);
                                available_blocks.push(inode.blocks[set_idx]);
                                inode.size--;
                                disk_writeblock(inode_block, &inode);

                                // if the set size greated than 1, remove the entry pointing to the inode from entries
                            } else {
                                entries[entry_idx].inode_block = 0;
                                disk_writeblock(inode.blocks[set_idx], &entries);
                            }

                            // 5. send response message
                            send(connectionfd, msg_copy, strlen(msg_copy) + 1, MSG_NOSIGNAL);
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

/*success return 1, fail return 0*/
bool delete_fd(char *msg_copy, int connectionfd, char *username, const char *pathname) {
    vector<string> path_vec;
    string path_str(pathname);
    if (!parse_path(path_vec, path_str)) {
        return false;
    }

    // first lock the root dir
    unique_lock<mutex> new_mutex(mutex_map[0]);

    // find the parent node path:/a/b/c, find:/a/b
    string last_fd_name = path_vec.back();
    path_vec.pop_back();

    fs_inode parent_inode;
    int parent_inode_block = find_fd_block(path_vec, username, new_mutex, parent_inode);

    // new mutex lock the parent inode
    if (parent_inode_block != -1) {
        //delete the file/dir by going through sets of entries
        return delete_from_entries_sets(parent_inode, parent_inode_block, last_fd_name, username, msg_copy, connectionfd);
    } else {
        return false;
    }
}

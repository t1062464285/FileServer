#include <arpa/inet.h> // htons(), ntohs()
#include <cassert>
#include <netdb.h>      // gethostbyname(), struct hostent
#include <netinet/in.h> // struct sockaddr_in
#include <stdio.h>      // perror(), fprintf()
#include <stdlib.h>     // atoi()
#include <string.h>     // memcpy()
#include <sys/socket.h> // getsockname() socket(), bind(), listen(), accept(), send(), recv()
#include <thread>
#include <unistd.h> // stderr close()

//#include "helpers.h"		// make_server_sockaddr(), get_port_number()
#include "fs_helper.h"

static const size_t MAX_MESSAGE_SIZE = 256;

extern map<int, std::mutex> mutex_map;
extern queue<uint32_t> available_blocks;
extern mutex available_blocks_mutex;
extern mutex cout_mutex;

/**
 * Make a server sockaddr given a port.
 * Parameters:
 *		addr: 	The sockaddr to modify (this is a C-style function).
 *		port: 	The port on which to listen for incoming connections.
 * Returns:
 *		0 on success, -1 on failure.
 * Example:
 *		struct sockaddr_in server;
 *		int err = make_server_sockaddr(&server, 8888);
 */
int make_server_sockaddr(struct sockaddr_in *addr, int port) {
    // Step (1): specify socket family.
    // This is an internet socket.
    addr->sin_family = AF_INET;

    // Step (2): specify socket address (hostname).
    // The socket will be a server, so it will only be listening.
    // Let the OS map it to the correct address.
    addr->sin_addr.s_addr = INADDR_ANY;

    // Step (3): Set the port value.
    // If port is 0, the OS will choose the port for us.
    // Use htons to convert from local byte order to network byte order.
    addr->sin_port = htons(port);

    return 0;
}

/**
 * Make a client sockaddr given a remote hostname and port.
 * Parameters:
 *		addr: 		The sockaddr to modify (this is a C-style function).
 *		hostname: 	The hostname of the remote host to connect to.
 *		port: 		The port to use to connect to the remote hostname.
 * Returns:
 *		0 on success, -1 on failure.
 * Example:
 *		struct sockaddr_in client;
 *		int err = make_client_sockaddr(&client, "141.88.27.42", 8888);
 */
int make_client_sockaddr(struct sockaddr_in *addr, const char *hostname, int port) {
    // Step (1): specify socket family.
    // This is an internet socket.
    addr->sin_family = AF_INET;

    // Step (2): specify socket address (hostname).
    // The socket will be a client, so call this unix helper function
    // to convert a hostname string to a useable `hostent` struct.
    struct hostent *host = gethostbyname(hostname);
    if (host == nullptr) {
        fprintf(stderr, "%s: unknown host\n", hostname);
        return -1;
    }
    memcpy(&(addr->sin_addr), host->h_addr, host->h_length);

    // Step (3): Set the port value.
    // Use htons to convert from local byte order to network byte order.
    addr->sin_port = htons(port);

    return 0;
}

/**
 * Return the port number assigned to a socket.
 *
 * Parameters:
 * 		sockfd:	File descriptor of a socket
 *
 * Returns:
 *		The port number of the socket, or -1 on failure.
 */
int get_port_number(int sockfd) {
    struct sockaddr_in addr;
    socklen_t length = sizeof(addr);
    if (getsockname(sockfd, (sockaddr *)&addr, &length) == -1) {
        perror("Error getting port of socket");
        return -1;
    }
    // Use ntohs to convert from network byte order to host byte order.
    return ntohs(addr.sin_port);
}

void process_msg(char *msg, int connectionfd, char *left_msg, int left_msg_len) {
    // cout << endl
    //      << msg << endl;
    // get the message and determine the type
    char msg_copy[strlen(msg) + 1];
    strcpy(msg_copy, msg);

    char *msg_type;
    msg_type = strtok(msg, " ");

    // receive the msg based on the message type
    char rb_type[] = "FS_READBLOCK";
    char wb_type[] = "FS_WRITEBLOCK";
    char create_type[] = "FS_CREATE";
    char delete_type[] = "FS_DELETE";

    char *username = strtok(NULL, " ");
    char *pathname = strtok(NULL, " ");

    for (unsigned int i = 0; i < strlen(username); i++) {
        if (username[i] == (' ' || '\f' || '\n' || '\r' || '\t' || '\v')) {
            return;
        }
    }

    for (unsigned int i = 0; i < strlen(pathname); i++) {
        if (pathname[i] == (' ' || '\f' || '\n' || '\r' || '\t' || '\v')) {
            return;
        }
    }

    if ((strlen(username) > FS_MAXUSERNAME) ||
        (strlen(username) == 0)) {
        return;
    }

    if ((strlen(pathname) > FS_MAXPATHNAME) ||
        (strlen(pathname) == 0)) {
        return;
    }

    // if the client request is fs_readblock
    if (strcmp(rb_type, msg_type) == 0) {

        char *block = strtok(NULL, " ");
        if (atoi(block) >= int(FS_MAXFILEBLOCKS)) {
            return;
        }
        readblock(msg_copy, connectionfd, username, pathname, block);

        // if the client request is fs_writeblock
    } else if (strcmp(wb_type, msg_type) == 0) {

        char *block = strtok(NULL, " ");
        if (atoi(block) >= int(FS_MAXFILEBLOCKS)) {
            return;
        }

        // receive the data from the socket
        char data[FS_BLOCKSIZE - left_msg_len + 1];

        size_t bytes = 0;
        int n;
        n = recv(connectionfd, data, FS_BLOCKSIZE - left_msg_len, MSG_WAITALL);

        if (n == -1) {
            perror("Error reading stream message");
            return;
        }

        for (unsigned int i = 0; i < FS_BLOCKSIZE - left_msg_len; i++) {
            left_msg[left_msg_len + i] = data[i];
        }

        writeblock(msg_copy, connectionfd, username, pathname, block, left_msg);

        // if the client request is fs_create
    } else if (strcmp(create_type, msg_type) == 0) {
        char *type = strtok(NULL, " ");
        if (*type != 'f' && *type != 'd') {
            return;
        }

        create(msg_copy, connectionfd, username, pathname, *type);

        // if the client request is fs_delete
    } else if (strcmp(delete_type, msg_type) == 0) {
        delete_fd(msg_copy, connectionfd, username, pathname);
    } else {
        return;
    }
}

/* Given bytes and msg, check whether the null character exists */
bool check_null(char *msg, int bytes) {
    for (int i = 0; i < bytes; i++) {
        if (msg[i] == '\0') {
            return true;
        }
    }
    return false;
}

/**
 * Receives a string message from the client and prints it to stdout.
 *
 * Parameters:
 * 		connectionfd: 	File descriptor for a socket connection
 * 				(e.g. the one returned by accept())
 * Returns:
 *		0 on success, -1 on failure.
 */
int handle_connection(int connectionfd) {

    printf("New connection %d\n", connectionfd);

    // (1) Receive message from client.

    // init the message
    char msg[MAX_MESSAGE_SIZE + 1];
    memset(msg, 0, sizeof(msg));

    // TODO: handle msg without null character
    size_t bytes = 0;
    int n;
    bool recv_finished = false;
    int left_msg_len;
    char left_msg[FS_BLOCKSIZE];
    // stop receiving the message if we hit the null charater
    do {
        // n is the number of received bytes
        n = recv(connectionfd, msg + bytes, MAX_MESSAGE_SIZE - bytes, 0);
        if (n == -1) {
            perror("Error reading stream message");
            return -1;
        }
        bytes += n;

        // go through the current message and check if we have encountered the null character
        recv_finished = check_null(msg, bytes);
        if (recv_finished) {
            int msg_len = strlen(msg) + 1;
            left_msg_len = bytes - msg_len;
            //left msg len 224
            for (int i = 0; i < left_msg_len; i++) {
                left_msg[i] = msg[msg_len + i];
            }
        }

        // cout << recv_finished << endl;

    } while (!recv_finished && (bytes <= MAX_MESSAGE_SIZE));

    if (!recv_finished) {
        return -1;
    }

    // (2) Process the message
    // thread t(process_msg, msg, connectionfd, left_msg, left_msg_len);
    // t.join();
    // t.detach();

    process_msg(msg, connectionfd, left_msg, left_msg_len);

    // (4) Close connection
    close(connectionfd);

    return 0;
}

// Initialize the list of free disk blocks by reading the relevant data from the existing file system.
void init_file_system() {
    unique_lock<mutex> available_blocks_lock(available_blocks_mutex);
    fs_inode root;
    disk_readblock(0, &root);
    mutex_map[0];

    //if root is empty, insert everything and return
    if (root.size == 0) {
        for (uint32_t i = 1; i < FS_DISKSIZE; ++i) {
            available_blocks.push(i);
        }
        return;
    }

    //every inode will be stored in this queue
    queue<uint32_t> inode_queue;
    inode_queue.push(0);

    //bfs search
    while (!inode_queue.empty()) {
        //read the inode block from queue
        fs_inode curr_inode;
        if (inode_queue.front() != 0) {
            disk_readblock(inode_queue.front(), &curr_inode);
        } else {
            curr_inode.type = root.type;
            strcpy(curr_inode.owner, root.owner);
            curr_inode.size = root.size;
            for (unsigned int i = 0; i < FS_MAXFILEBLOCKS; ++i) {
                curr_inode.blocks[i] = root.blocks[i];
            }
        }

        inode_queue.pop();

        //iterate through the inode block
        for (uint32_t i = 0; i < curr_inode.size; ++i) {
            //insert the direntry/fileblock's block numbers to mutex map
            mutex temp_m1;
            mutex_map[curr_inode.blocks[i]];

            //if the inode is a directory, loop through the direntry
            if (curr_inode.type == 'd') {
                fs_direntry entries[FS_DIRENTRIES];
                disk_readblock(curr_inode.blocks[i], &entries);

                for (unsigned int idx = 0; idx < FS_DIRENTRIES; ++idx) {
                    fs_direntry entry = entries[idx];

                    /*if the entry is not empty,
					 *  push the inode to the queue and insert into the mutex map
					*/
                    if (entry.inode_block != 0) {
                        mutex_map[entry.inode_block];
                        inode_queue.push(entry.inode_block);
                    }
                }
            }
        }
    }

    //initialize the available_block based on the mutex_map
    for (uint32_t i = 1; i < FS_DISKSIZE; ++i) {
        if (mutex_map.find(i) == mutex_map.end())
            available_blocks.push(i);
    }
}

/**
 * Endlessly runs a server that listens for connections and serves
 * them _synchronously_.
 *
 * Parameters:
 *		port: 		The port on which to listen for incoming connections.
 *		queue_size: 	Size of the listen() queue
 * Returns:
 *		-1 on failure, does not return on success.
 */
int run_server(int port, int queue_size) {
    // initialize the file system
    init_file_system();
    // cout << "finish initializing the system" << endl;

    // (1) Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error opening stream socket");
        return -1;
    }

    // (2) Set the "reuse port" socket option
    int yesval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
        perror("Error setting socket options");
        return -1;
    }

    // (3) Create a sockaddr_in struct for the proper port and bind() to it.
    struct sockaddr_in addr;
    if (make_server_sockaddr(&addr, port) == -1) {
        return -1;
    }

    // (3b) Bind to the port.
    if (bind(sockfd, (sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Error binding stream socket");
        return -1;
    }

    // (3c) Detect which port was chosen.
    port = get_port_number(sockfd);

    // (4) Begin listening for incoming connections.
    listen(sockfd, queue_size);
    cout << "\n@@@ port " << port << endl;

    // (5) Serve incoming connections one by one forever.
    while (true) {
        int connectionfd = accept(sockfd, 0, 0);
        if (connectionfd == -1) {
            perror("Error accepting connection");
            return -1;
        }
        // TODO: check this part?

        // if (handle_connection(connectionfd) == -1) {
        //     return -1;
        // }
        thread t(handle_connection, connectionfd);
        t.detach();
    }
}

int main(int argc, const char **argv) {
    // Parse command line arguments
    int port = 0;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc == 1) {
        port = 0;
    } else {
        printf("Usage: ./server port_num\n");
        return 1;
    }

    if (run_server(port, 10) == -1) {
        return 1;
    }
    return 0;
}
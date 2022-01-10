/* Test write */

#include "fs_client.h"
#include <cstdlib>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    char *server;
    int server_port;
    unsigned int session, seq = 0;

    const char *data = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    const char *data1 = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";
    //char readdata[FS_BLOCKSIZE];

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    cout << "reaching here" << endl;
    fs_clientinit(server, server_port);
    cout << "clientinit finished" << endl;

    fs_create("user1", "/zeus", 'f');
    fs_writeblock("user1", "/zeus", 0, data);
    fs_writeblock("user1", "/zeus", 0, data1); // overwrite same thing
    fs_create("user1", "/paul", 'd');
    fs_create("user1", "/paul/klee", 'd');
    fs_create("user2", "/paul/klee/jean", 'd'); //no
    fs_create("user1", "/paul/klee/kappa", 'f');
    fs_writeblock("user1", "/paul/klee/kappa", 0, data1);
    fs_create("user1", "/paul/klee/jean/kappa", 'd'); //no
    fs_create("user1", "/jake", 'd');
    fs_create("user1", "/jake1", 'd');
    fs_create("user1", "/jake2", 'd');
    fs_create("user1", "/jake3", 'd');
    fs_create("user1", "/jake4", 'd');
    fs_create("user1", "/jake5", 'd');
    fs_create("user1", "/jake6", 'f');
    fs_writeblock("user1", "/jake6", 0, data);
    fs_create("user1", "/jake7", 'd');
    fs_create("user1", "/jake7/paul", 'f');
    fs_writeblock("user1", "/jake7/paul", 0, data1);
    fs_create("user1", "/paul/klee/jean/jarjar", 'd'); //no
    return 0;
}
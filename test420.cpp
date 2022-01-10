/* Test write error*/

#include "fs_client.h"
#include "fs_server.h"
#include <cstdlib>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])

{
    char *server;
    int server_port;
    unsigned int session, seq = 0;

    const char *data = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    cout << "reaching here" << endl;
    fs_clientinit(server, server_port);
    cout << "clientinit finished" << endl;

    // fs_create("user1", "/zeus", 'd');
    // fs_create("user1", "/zeus/file", 'f');

    // for (int i = 0; i < 200; i++) { //Overfill the file's allotted amount of space
    //     fs_writeblock("user1", "/zeus/file", i, data);
    // }

    cout << endl;
    cout << endl;
    fs_delete("user1", "/zeus/file");

    fs_writeblock("user1", "/zeus/file", 0, data); //no

    fs_create("user1", "/zeus/file", 'f');

    fs_writeblock("user1", "zeus/file", 0, data);   //no
    fs_writeblock("user1", "zeusfile", 0, data);    //no
    fs_writeblock("user1", "/zeus/file/", 0, data); //no

    fs_writeblock("user1", "/zeus/file", 1, data);      //Write to a block that is greater than 1 away from the current size of the file
    fs_writeblock("user1", "/zeus/file", 100000, data); //Write outside of the possible range of the file no

    return 0;
}
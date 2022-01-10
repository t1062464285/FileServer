/* Test for multi thread*/
// test: one thread create a
#include "fs_client.h"
#include "fs_helper.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace std;

void t1(char *server, int server_port) {
    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    char readdata[FS_BLOCKSIZE];
    int status;

    fs_clientinit(server, server_port);

    // success
    status = fs_create("user1", "/dir", 'd');
    cout << "expected = 0 status = " << status << endl;

    // success
    status = fs_create("user1", "/dir/file", 'f');
    cout << "expected = 0 status = " << status << endl;

    // success
    status = fs_writeblock("user1", "/dir/file", 0, writedata);
    cout << "expected = 0 status = " << status << endl;

    // success
    status = fs_readblock("user1", "/dir/file", 0, readdata);
    cout << "expected = 0 status = " << status << endl;

    //success
    status = fs_delete("user1", "/dir/file");
    cout << "expected = 0 status = " << status << endl;

    //fail, wrong username
    status = fs_delete("user1", "/dir");
    cout << "expected = -1 status = " << status << endl;

    //success
    status = fs_delete("user1", "/dir");
    cout << "expected = 0 status = " << status << endl;
}

void t2(char *server, int server_port) {
    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    char readdata[FS_BLOCKSIZE];
    int status;

    fs_clientinit(server, server_port);

    // success
    status = fs_create("user1", "/dir", 'd');
    cout << "expected = 0 status = " << status << endl;

    // success
    status = fs_create("user1", "/dir/file", 'f');
    cout << "expected = 0 status = " << status << endl;

    // success
    status = fs_writeblock("user1", "/dir/file", 0, writedata);
    cout << "expected = 0 status = " << status << endl;

    // success
    status = fs_readblock("user1", "/dir/file", 0, readdata);
    cout << "expected = 0 status = " << status << endl;

    //success
    status = fs_delete("user1", "/dir/file");
    cout << "expected = 0 status = " << status << endl;

    //fail, wrong username
    status = fs_delete("user1", "/dir");
    cout << "expected = -1 status = " << status << endl;

    //success
    status = fs_delete("user1", "/dir");
    cout << "expected = 0 status = " << status << endl;
}

int main(int argc, char *argv[]) {
    char *server;
    int server_port;

    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    char readdata[FS_BLOCKSIZE];
    int status;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    cout << "server: " << server << endl;
    server_port = atoi(argv[2]);
    cout << "server port: " << server_port << endl;

    thread first(t1, server, server_port);
    thread second(t2, server, server_port);

    cout << "Two thread happen concurrently" << endl;

    first.join();
    second.join();

    cout << "Both thread complete" << endl;
}
//test stress concurrency

#include "fs_client.h"
#include "fs_helper.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
#include <vector>

using namespace std;

char *server;
int server_port;
const char *writedata1 = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
const char *writedata2 = "In 1901 she founded the Tuesday Musical Club in her home, with her sister Jenny Sachs and several other women, to play chamber music together and promote cultural events.[3] She was also founding president of the San Antonio chapter of the National Council of Jewish Women, organizer and president of the San Antonio Symphony Society, president of the Texas Free Kindergarten Association, and chair of the Texas committee for the Panama-Pacific International Exposition. From 1911 to 1913, she was president of the Texas Federation of Women's Clubs.[4][5] She oversaw the opening of a night school for immigrant workers in the city in 1907, and sewing classes for young women.[1] She was active in the city's Battle of Flowers parades for several years";
const char *writedata3 = "Tommy Black (born Fredrik Halldin) is a record producer from Sweden. He has worked with Kendrick Lamar, Schoolboy Q, Dr. Dre, Big Pooh, Ab-Soul and Shade Sheist. He was one of the producers of the 5 Grammy awarded album To Pimp A Butterfly by American rapper Kendrick Lamar. In 2017, Tommy Black received the award of SKAP - The Swedish Society of Songwriters, Composers and Authors.[1]";
const char *writedata4 = "The 1902 Ibrox disaster was the collapse of a stand at Ibrox Park (now Ibrox Stadium) in Glasgow, Scotland, which led to the deaths of 25 and injuries to more than 500 supporters during an international association football match between Scotland and England on 5 April 1902. The stadium had completed construction three years before the incident and was hosting its first international fixture, with the crowd estimated to be over 68,000. The match was the first time that the ground had been used more than half full. During the first half of the match, a section of the newly built West Tribune Stand collapsed, dropping between 200 and 300 people to the concrete floor below. Two spectators were declared dead at the scene, and a further twenty-three died of injuries sustained in the incident soon after, the last victim dying three weeks later.";
const char *writedata5 = "Frühschoppen (pronounced [fʁyːʃɔpm], meaning: an alcoholic drink before midday in company) is the German and Austrian tradition of meeting up at a pub, inn or tavern in the late morning, usually on Sundays. The specific customs vary from region to region. Frühschoppen is often a kind of brunch, but does not necessarily involve food. Frühschoppen is also often held at fun fairs, the most famous being Oktoberfest or Cannstatter Volksfest.";

void thread1(int val) {

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 0; i < 1; i++) {
        string dir_name = "/t1dir_u1" + to_string(i);
        fs_create("user1", dir_name.c_str(), 'd');
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for (int j = 0; j < 2; j++) {
            string dir_name2 = dir_name + "/t2dir" + to_string(j);
            fs_create("user1", dir_name2.c_str(), 'd');
            std::this_thread::sleep_for(std::chrono::seconds(1));
            for (int k = 0; k < 3; k++) {
                string dir_name3 = dir_name2 + "/t3dir" + to_string(k);
                fs_create("user1", dir_name3.c_str(), 'd');
                std::this_thread::sleep_for(std::chrono::seconds(1));
                for (int l = 0; l < 20; l++) {
                    string dir_name4 = dir_name3 + "/t4file" + to_string(l);
                    fs_create("user1", dir_name4.c_str(), 'f');
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    for (int m = 0; m < 2; m++) {
                        fs_writeblock("user1", dir_name4.c_str(), m, writedata1);
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            }
        }
    }
}

void thread2(int val) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 0; i < 1; i++) {
        string dir_name = "/t1dir_u2" + to_string(i);
        //user1_directories.push_back(dir_name);
        fs_create("user2", dir_name.c_str(), 'd');
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for (int j = 0; j < 2; j++) {
            string dir_name2 = dir_name + "/t2dir" + to_string(j);
            fs_create("user2", dir_name2.c_str(), 'd');
            std::this_thread::sleep_for(std::chrono::seconds(1));
            for (int k = 0; k < 3; k++) {
                string dir_name3 = dir_name2 + "/t3dir" + to_string(k);
                fs_create("user2", dir_name3.c_str(), 'd');
                std::this_thread::sleep_for(std::chrono::seconds(1));
                for (int l = 0; l < 20; l++) {
                    string dir_name4 = dir_name3 + "/t4file" + to_string(l);
                    fs_create("user2", dir_name4.c_str(), 'f');
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    for (int m = 0; m < 2; m++) {
                        fs_writeblock("user2", dir_name4.c_str(), m, writedata1);
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            }
        }
    }
}

void thread4(int val) {
    char readdata[FS_BLOCKSIZE];

    std::this_thread::sleep_for(std::chrono::seconds(10));

    for (int i = 0; i < 1; i++) {
        string dir_name = "/t1dir_u2" + to_string(i);
        //user1_directories.push_back(dir_name);
        for (int j = 0; j < 2; j++) {
            string dir_name2 = dir_name + "/t2dir" + to_string(j);
            for (int k = 0; k < 3; k++) {
                string dir_name3 = dir_name2 + "/t3dir" + to_string(k);
                for (int l = 0; l < 20; l++) {
                    string dir_name4 = dir_name3 + "/t4file" + to_string(l);
                    for (int m = 0; m < 2; m++) {
                        fs_readblock("user2", dir_name4.c_str(), m, readdata);
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    fs_delete("user2", dir_name4.c_str());
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                fs_delete("user2", dir_name3.c_str());
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            fs_delete("user2", dir_name2.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        fs_delete("user2", dir_name.c_str());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void thread5(int val) {
    char readdata[FS_BLOCKSIZE];

    std::this_thread::sleep_for(std::chrono::seconds(10));

    for (int i = 0; i < 1; i++) {
        string dir_name = "/t1dir_u1" + to_string(i);
        //user1_directories.push_back(dir_name);
        for (int j = 0; j < 2; j++) {
            string dir_name2 = dir_name + "/t2dir" + to_string(j);
            for (int k = 0; k < 3; k++) {
                string dir_name3 = dir_name2 + "/t3dir" + to_string(k);
                for (int l = 0; l < 20; l++) {
                    string dir_name4 = dir_name3 + "/t4file" + to_string(l);
                    for (int m = 0; m < 2; m++) {
                        fs_readblock("user1", dir_name4.c_str(), m, readdata);
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    fs_delete("user1", dir_name4.c_str());
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                fs_delete("user1", dir_name3.c_str());
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            fs_delete("user1", dir_name2.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        fs_delete("user1", dir_name.c_str());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);
    cout << "clientinit finished" << endl;

    thread t1(thread1, 0);
    thread t2(thread2, 0);
    // thread t4(thread4, 0);
    // thread t5(thread5, 0);

    t1.join();
    t2.join();
    // t4.join();
    // t5.join();

    return 0;
}
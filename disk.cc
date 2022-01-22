// Project 2 - Disk Scheduler: Simulation of a concurrent disk scheduler
// @file disk.cc
// @author Kevin Li and Liam Juskevice

#include "thread.h"
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <tuple>
#define lock 0xFFFFFFFF
#define cv1 0xFFFFFFFE
#define cv2 0xFFFFFFFD
#define cv3 0xFFFFFFFC

using namespace std;

//global variables
map<int, vector<int>> diskInput;
vector<tuple<int, int>> diskQueue;
int max_queue_size;
int num_requesters;
char* requesterID;
vector<int> iterCounter;

//requester info struct
typedef struct {
    int id;
} requester_info; 


/*
 * Reads a file by white space and adds the contents as
 * a key, value pair in the map diskInput 
 * 
 * @param count: the file number
 *        filename: file name
 * @return void
 */
void read_file(int count, string filename) {
    vector<int> tracks;
    int trackNum;
    ifstream fileStream;
    fileStream.open(filename);
    string number;
    while (fileStream >> number) {
        trackNum = stoi(number);
        tracks.push_back(trackNum);
    }
    diskInput.insert(make_pair(count, tracks));    
}


/*
 * Requester function thread
 * @param arg: requester_info struct
 * @return void 
 */
void requester(void* arg) {
    requester_info* ri = (requester_info*) arg;
    if (thread_lock(lock)) {
        cout << "lock is not acquired requester" << endl; 
    }
    int threadSize = diskInput.at(ri->id).size();
    for (int i = 0; i < threadSize; i++) {
        int track_num; 
        while ((int) diskQueue.size() == max_queue_size) {
            if (thread_signal(lock, cv1)) {
                cout << "cv1 signal failed in while requester" << endl;
            }
            if (thread_wait(lock, cv2)) {
                cout << "cv2 wait failed in while requester" << endl;
            }
        }
        track_num = diskInput.at(ri->id)[i];
        auto trackInfo = make_tuple(ri->id, track_num);
        diskQueue.push_back(trackInfo);
        iterCounter.at(ri->id)++;
        cout << "requester " << to_string(ri->id) << " track " << track_num << endl;
        if (thread_signal(lock, cv1)) {
            cout << "cv1 signal failed in requester (not while)" << endl;
        }

        if (thread_wait(lock, ri->id)) {
            cout << "cv3 wait fialed in requester" << endl;
        }
    }

    if (thread_unlock(lock)){
        cout << "it's not released" << endl;
    }
}


/*
 * Servicer function thread
 * @param a: no meaning
 * @return void
 */
void servicer(void* a) {
    int curr_tracker = 0;
    if (thread_lock(lock)){
        cout << "lock acquire failed" << endl;
    }
    for (auto const& keys: diskInput) {
	int key = keys.first;
        requester_info *ri = new requester_info;
        ri -> id = key;
        string keystring = "requester " + to_string(key);
        requesterID = const_cast<char*>(keystring.c_str());
    	if (thread_create((thread_startfunc_t) requester, (void*) ri)) {
		cout << "thread_create failed\n";
		exit(1);
	}
    }
    while (num_requesters > 0) {
        while ((int) diskQueue.size() != max_queue_size &&  \
                                num_requesters != (int) diskQueue.size()) {
            if (thread_wait(lock, cv1)) {
                cout << "cv1 wait failed in servicer while" << endl;
            }
            if (thread_signal(lock, cv2)) {
                cout << "cv2 signal failed in servicer while" << endl;
            }
        }
        int lowestDiff = 1000;
        int lowestDiffTrack;
        int lowestDiffID;
        int posInQueue;
        for (int i = 0; i < (int) diskQueue.size(); i++) {
            int currDiff = abs(get<1>(diskQueue.at(i)) - curr_tracker);
            if (currDiff < lowestDiff) {
                lowestDiff = currDiff;
                lowestDiffID = get<0>(diskQueue.at(i));
                lowestDiffTrack = get<1>(diskQueue.at(i));
                posInQueue = i;
            }
        }
        curr_tracker = lowestDiffTrack;
        cout << "service requester " << lowestDiffID << " track " << lowestDiffTrack << endl;
        if (iterCounter.at(lowestDiffID) == (int) diskInput.at(lowestDiffID).size()) {
            num_requesters--;
        }
        diskQueue.erase(diskQueue.begin()+posInQueue);
        if (thread_signal(lock, lowestDiffID)) {
            cout << "cv3 signal failed outside of while servicer" << endl;
        }
    }
    if (thread_unlock(lock)) {
        cout << "lock not released in servicer" << endl;
    }
}


/*
 * Main function
 */
int main(int argc, char** argv) {
    int counter = 0;
    num_requesters = argc-2;
    max_queue_size = stoi(argv[1]);
    iterCounter.resize(num_requesters, 0);
    for (int i = 2; i < argc; i++) {
        read_file(counter, argv[i]);
        counter++;
    }
    if (thread_libinit((thread_startfunc_t) servicer, (void*) 12)) {
        cout << "thread_libinit failed\n";
        exit(1);
    }
    return 0;
}


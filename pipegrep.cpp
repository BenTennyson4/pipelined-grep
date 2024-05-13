#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <utility>
#include <atomic>
#include <chrono>

using namespace std;
using namespace std::chrono;

using Clock = high_resolution_clock;

// Alias for a time point
using TimePoint = time_point<Clock>;

double durationMs(TimePoint start, TimePoint end) {
    return duration_cast<milliseconds>(end - start).count();
}

const string DONE_TOKEN = "DONE";

// Structure to hold command line arguments
struct CommandLineArgs {
    int buffsize;
    int filesize;
    int uid;
    int gid;
    string searchString;
};

// Bounded buffer class
template<typename T>
class BoundedBuffer {
private:
    queue<T> buffer;
    const int capacity;
    mutex mtx;
    condition_variable not_full;
    condition_variable not_empty;

public:
    BoundedBuffer(int capacity) : capacity(capacity) {}

    void produce(T item) {
        unique_lock<mutex> lock(mtx);
        not_full.wait(lock, [this](){ return buffer.size() < capacity; });
        buffer.push(item);
        not_empty.notify_one();
    }

    T consume() {
        unique_lock<mutex> lock(mtx);
        not_empty.wait(lock, [this](){ return !buffer.empty(); });
        T item = buffer.front();
        buffer.pop();
        not_full.notify_one();
        return item;
    }
};

// Stage 1: Filename Acquisition
void stage1(const CommandLineArgs& args, BoundedBuffer<string>& buff1) {
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir(".")) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) { // regular file
                string filename = entry->d_name;
                buff1.produce(filename);
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
    }
    buff1.produce(DONE_TOKEN); // indicate completion
}

// Stage 2: File Filter
void stage2(const CommandLineArgs& args, BoundedBuffer<string>& buff1, BoundedBuffer<string>& buff2) {
    while (true) {
        string filename = buff1.consume();
        if (filename == DONE_TOKEN) {
            buff2.produce(DONE_TOKEN); // indicate completion
            break;
        }
        struct stat file_stat;
        if (stat(filename.c_str(), &file_stat) == 0) {
            if ((args.filesize == -1 || file_stat.st_size > args.filesize) &&
                (args.uid == -1 || file_stat.st_uid == args.uid) &&
                (args.gid == -1 || file_stat.st_gid == args.gid)) {
                buff2.produce(filename);
            }
        } else {
            perror("stat");
        }
    }
}

// Stage 3: Line Generation
void stage3(const CommandLineArgs& args, BoundedBuffer<string>& buff2, BoundedBuffer<pair<string, string>>& buff3) {
    while (true) {
        string filename = buff2.consume();
        if (filename == DONE_TOKEN) {
            buff3.produce(make_pair(DONE_TOKEN, "")); // indicate completion
            break;
        }
        ifstream file(filename);
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                buff3.produce(make_pair(filename, line)); // store filename along with the line
            }
            file.close();
        } else {
            perror("ifstream");
        }
    }
}

// Stage 4: Line Filter
void stage4(const CommandLineArgs& args, BoundedBuffer<pair<string, string>>& buff3, BoundedBuffer<string>& buff4, atomic<int>& matchCount) {
    while (true) {
        auto pair = buff3.consume();
        string filename = pair.first;
        string line = pair.second;
        if (filename == DONE_TOKEN) {
            buff4.produce(DONE_TOKEN); // indicate completion
            break;
        }
        if (line.find(args.searchString) != string::npos) {
            buff4.produce(filename + ": " + line); // output filename along with the line
            matchCount++;
        }
    }
}

// Stage 5: Output
void stage5(BoundedBuffer<string>& buff4) {
    while (true) {
        string line = buff4.consume();
        if (line == DONE_TOKEN) {
            break; // exit when done token is received
        }
        cout << line << endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0] << " <buffsize> <filesize> <uid> <gid> <string>" << endl;
        return 1;
    }

    CommandLineArgs args;
    args.buffsize = atoi(argv[1]);
    args.filesize = atoi(argv[2]);
    args.uid = atoi(argv[3]);
    args.gid = atoi(argv[4]);
    args.searchString = argv[5];

    BoundedBuffer<string> buff1(args.buffsize);
    BoundedBuffer<string> buff2(args.buffsize);
    BoundedBuffer<pair<string, string>> buff3(args.buffsize);
    BoundedBuffer<string> buff4(args.buffsize);
    atomic<int> matchCount(0); // atomic variable to store the count of matches found

    TimePoint start = Clock::now();

    thread t1(stage1, args, ref(buff1));
    thread t2(stage2, args, ref(buff1), ref(buff2));
    thread t3(stage3, args, ref(buff2), ref(buff3));
    thread t4(stage4, args, ref(buff3), ref(buff4), ref(matchCount));
    thread t5(stage5, ref(buff4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    TimePoint end = Clock::now();

    double elapsedMs = durationMs(start, end);
    cout << "Execution time: " << elapsedMs << " milliseconds" << endl;

    cout << "***** You found " << matchCount << " matches *****" << endl;

    return 0;
}

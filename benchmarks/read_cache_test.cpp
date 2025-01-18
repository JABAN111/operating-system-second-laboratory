#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "../lab2.h"

#define BLOCK_SIZE (8192)

using namespace std;

void io_thpt_read_cache_test(const string& filename, size_t num_blocks, int repetitions) {
    vector<char> buffer(BLOCK_SIZE);
    double total_bandwidth = 0;

    size_t cache_size = 10;
    cache_init(cache_size);
    for(int i = 0; i < repetitions; ++i) {
        int fd = lab2_open(filename.c_str());
        if(fd == -1) {
            cerr << "ERROR: Не удалось открыть файл" << filename << endl;
            exit(EXIT_FAILURE);
        }

        size_t bytesRead = 0;
        auto start = chrono::high_resolution_clock::now();
        for(size_t j = 0; j < num_blocks; j++) {
            ssize_t readSize = lab2_read(fd, buffer.data(), BLOCK_SIZE);
            if(readSize <= 0) {
                break;
            }
            bytesRead += readSize;
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> diff = end - start;
        double bandwidth = (bytesRead / 1024.0/1024.0) / diff.count();
        total_bandwidth += bandwidth;
        cout << "Прочитано " << bytesRead / 1024 << "Кб за " << diff.count() << " секунд " << endl;
        lab2_fsync(fd);
        lab2_close(fd);

    }
    cache_free();
    cout << "Средняя пропускная способность за " << repetitions << " повторений: " << total_bandwidth / repetitions << "MB/s" << endl;
}

int ma2n(int argc, char* argv[]) {
    if(argc != 4) {
        cerr << "ERROR: Неверно введённые параметры" << endl;
        return 1;
    }

    string filename = argv[1];
    size_t num_blocks = stoull(argv[2]);
    int repetitions = stoi(argv[3]);
    io_thpt_read_cache_test(filename, num_blocks, repetitions);
    return 0;
}

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#define BLOCK_SIZE (8192)

using namespace std;

void io_thpt_read_test(const string& filename, size_t num_blocks, int repetitions) {
    vector<char> buffer(BLOCK_SIZE);
    double total_bandwidth = 0;

    for (int i = 0; i < repetitions; ++i) {
        ifstream file(filename, ios::binary| ios::);
        if (!file) {
            cerr << "Error: Не удалось открыть файл " << filename << endl;
            exit(EXIT_FAILURE);
        }

        size_t bytesRead = 0;
        auto start = chrono::high_resolution_clock::now();

        for (size_t i = 0; i < num_blocks; i++) {
            file.read(buffer.data(), BLOCK_SIZE);
            if (file.gcount() == 0) break;
            bytesRead += file.gcount();
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> diff = end - start;
        double bandwidth = (bytesRead / 1024.0 / 1024.0) / diff.count();
        total_bandwidth += bandwidth;


        cout << "Прочитано " << bytesRead / 1024 << "Кб за " << diff.count() << " секунд" << endl;
        cout << "Пропускная способность: " << bandwidth << " MB/s" << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Error: Неверно введенный параметры" << argv[0] << endl;
        return 1;
    }

    string filename = argv[1];
    size_t num_blocks = stoull(argv[2]);
    int repetitions = stoi(argv[3]);
    io_thpt_read_test(filename, num_blocks, repetitions);
    return 0;
}

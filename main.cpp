#include <cassert>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/fcntl.h>

#include "lab2.h"

using namespace std;

void test_lru_eviction() {
    const char *filename = "../test.txt";
    cache_init(3);
    const pair<int, ino_t> fd_inot = lab2_open(filename);

    char data1[] = "1";
    char data2[] = "2";
    char data3[] = "3";
    char data4[] = "4";

    lab2_write(fd_inot, 0, data1, sizeof(data1));
    lab2_write(fd_inot, 1, data2, sizeof(data2));
    lab2_write(fd_inot, 2, data3, sizeof(data3));
    lab2_write(fd_inot, 3, data4, sizeof(data4));

    char readBuffer1[10] = {};
    ssize_t bytesRead1 = lab2_read(fd_inot, readBuffer1, sizeof(data1), 0);
    assert(bytesRead1 == -1);

    cout << "LRU вытеснение проверено" << endl;
}


int main() {
    cout << "---------------- Начало теста ------------------" << endl;
    test_lru_eviction();

    cout << "lru eviction test finished successfully" << endl;


    const char *filename = "../test.txt";

    cache_init(CACHE_SIZE);


    const pair<int, ino_t> fd_inot = lab2_open(filename);
    if (fd_inot.first == -1) {
        cerr << "ERROR: " << filename << " не найден или не может быть открыт" << endl;
        cache_free();
        return -1;
    }
    cout << "Файл открыт: fd = " << fd_inot.first << ", inode = " << fd_inot.second << endl;

    char writeBuffer[BUFFER_SIZE];
    memset(writeBuffer, '1', BUFFER_SIZE);

    off_t offset = lab2_lseek(fd_inot.first, 0, SEEK_CUR);
    cout << "offset in write: " << offset << endl;
    if (offset == -1) {
        cerr << "PANIC: lseek in write" << endl;
        lab2_close(fd_inot.first);
        cache_free();
        return -1;
    }

    const ssize_t bytesWritten = lab2_write(fd_inot, offset, writeBuffer, BUFFER_SIZE);
    if (bytesWritten == -1) {
        cerr << "ERROR: не получилось записать данные в файл" << endl;
        lab2_close(fd_inot.first);
        cache_free();
        return -1;
    }
    cout << "Записано " << bytesWritten << " байт в файл" << endl;

    if (lab2_lseek(fd_inot.first, 0, SEEK_SET) == -1) {
        cerr << "ERROR: проблемы при перемещении указателя" << endl;
        lab2_close(fd_inot.first);
        cache_free();
        return -1;
    }

    char readBuffer[BUFFER_SIZE] = {};
    off_t offsetForRead = lab2_lseek(fd_inot.first, 0, SEEK_CUR);
    ssize_t bytesRead = lab2_read(fd_inot, readBuffer, BUFFER_SIZE, offsetForRead);
    if (bytesRead == -1) {
        cerr << "ERROR: не получилось прочитать файл" << endl;
        lab2_close(fd_inot.first);
        cache_free();
        return -1;
    }
    cout << "Прочитано " << bytesRead << " байт" << endl;

    if (memcmp(writeBuffer, readBuffer, bytesRead) == 0) {
        cout << "Данные совпадают" << endl;
    } else {
        cerr << "ERROR: данные не совпадают" << endl;
        for (int i = 0; i < bytesRead; i++) {
            cout << "readBuffer[" << i << "]: " << readBuffer[i] << "\t";
            cout << "writeBuffer[" << i << "]: " << writeBuffer[i] << endl;
        }
        lab2_close(fd_inot.first);
        cache_free();
        return -1;
    }

    char write_data_2[BUFFER_SIZE];
    memset(write_data_2, '2', BUFFER_SIZE);
    if (lab2_write(fd_inot, offset, write_data_2, BUFFER_SIZE) != BUFFER_SIZE) {
        cerr << "ERROR: запись данных '2'" << endl;
        lab2_close(fd_inot.first);
        cache_free();
        return 1;
    }

    char write_data_3[BUFFER_SIZE];
    memset(write_data_3, '3', BUFFER_SIZE);
    if (lab2_write(fd_inot, offset, write_data_3, BUFFER_SIZE) != BUFFER_SIZE) {
        cerr << "ERROR: запись данных '3'" << endl;
        lab2_close(fd_inot.first);
        cache_free();
        return 1;
    }

    char write_data_4[BUFFER_SIZE];
    memset(write_data_4, '4', BUFFER_SIZE);
    if (lab2_write(fd_inot, offset, write_data_4, BUFFER_SIZE) != BUFFER_SIZE) {
        cerr << "ERROR: запись данных '4'" << endl;
        lab2_close(fd_inot.first);
        cache_free();
        return 1;
    }

    display_cache();

    if (lab2_fsync(fd_inot.first) == -1) {
        cerr << "ERROR: синхронизация файла" << endl;
    } else {
        cout << "Файл успешно синхронизирован" << endl;
    }

    if (lab2_close(fd_inot.first) == -1) {
        cerr << "ERROR: закрытие файла" << endl;
    } else {
        cout << "Файл успешно закрыт" << endl;
    }

    cache_free();

    cout << "---------------- Тест завершён ------------------" << endl;
    return 0;
}

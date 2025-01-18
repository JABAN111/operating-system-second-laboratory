#include <cstring>
#include <iostream>
#include <unistd.h>

#include "lab2.h"


using namespace std;

int main() {
    cout << "---------------- Начало теста ------------------" << endl;
    const auto filename = "../test.txt";
    cache_init(CACHE_SIZE);
    const int fd = lab2_open(filename);

    if (fd == -1) {
        cerr << "ERROR: " << filename << " не найден или е может быть открыт" << endl;
        cache_free();
        return -1;
    }

    char writeBuffer[BUFFER_SIZE];
    for (int i = 0; i < BUFFER_SIZE; i++) {
        // writeBuffer[i] = '0' + i;
        writeBuffer[i] = 4 + '0';
    }
    cout << "element 501 equal: " << writeBuffer[501] - '0' << endl;

    // memset(writeBuffer, '1', BUFFER_SIZE); // заполнили массив {1, 1, ..., 1}
    //
    const ssize_t bytesWritten = lab2_write(fd, writeBuffer, BUFFER_SIZE);
    if (bytesWritten == -1) {
        cerr << "ERROR: не получилось записать данные в файл" << endl;
        lab2_close(fd);
        cache_free();
        return -1;
    }

    cout << "Записано " << bytesWritten << " байт в файл" << endl;
    //
    if (lab2_lseek(fd, 0, SEEK_SET) == -1) {
        cerr << "ERROR: проблемы при перемещении указателя" << endl;
        lab2_close(fd);
        return -1;
    }

    char readBuffer[BUFFER_SIZE] = {};
    ssize_t bytesRead = lab2_read(fd, readBuffer, BUFFER_SIZE);

    if (bytesRead == -1) {
        cerr << "ERROR: не получилось прочитать файл" << endl;
        lab2_close(fd);
        cache_free();
        return -1;
    }

    cout << "Прочитано " << bytesRead << " байт" << endl;

    if (memcmp(writeBuffer, readBuffer, bytesRead) == 0) {
        cout << "Данные совпадают" << endl;
    } else {
        cout << "ERROR: данные не совпадают" << endl;
        cache_free();
    }

    char write_data_2[BUFFER_SIZE];
    memset(write_data_2, '2', BUFFER_SIZE);
    if (lab2_write(fd, write_data_2, BUFFER_SIZE) != BUFFER_SIZE) {
        cerr << "ERROR: записи" << endl;
        lab2_close(fd);
        cache_free();
        return 1;
    }

    char write_data_3[BUFFER_SIZE];
    memset(write_data_3, '3', BUFFER_SIZE);
    if (lab2_write(fd, write_data_3, BUFFER_SIZE) != BUFFER_SIZE) {
        cerr << "ERROR: записи" << endl;
        lab2_close(fd);
        cache_free();
        return 1;
    }
    char write_data_4[BUFFER_SIZE];
    memset(write_data_4, '4', BUFFER_SIZE);
    if (lab2_write(fd, write_data_4, BUFFER_SIZE) != BUFFER_SIZE) {
        cerr << "ERROR: записи" << endl;
        lab2_close(fd);
        cache_free();
        return 1;
    }

    display_cache();
    lab2_fsync(fd);

    if (lab2_close(fd) == -1) {
        cerr << "ERROR: закрытие файла" << endl;
    }
    cache_free();
    cout << "---------------- Всё тест завершён ------------------" << endl;
    return 0;
}

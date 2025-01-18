#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include "lab2.h"

using namespace std;

static Cache cache;

int cache_init(size_t size) {
    cache.size = size;
    return 0;
}

void cache_free() {

    for(auto &entry: cache.table) {
        if(entry.second && entry.second->data) {
            cout << "Освобождение блока со сдвигом = " << entry.second->offset << " и адресом данных" << static_cast<void*>(entry.second->data) << endl;
            free(entry.second->data);
            entry.second->data = nullptr;
            delete entry.second;
        }
    }

    cache.table.clear();
    cache.usage_list.clear();
    // cout << "Кэш очищен" << endl;
}

void display_cache() {
    cout << "Содержимое таблицы кэша: " << endl;
    for(const auto &entry : cache.table) {
        const CacheBlock* block = entry.second;
        cout << "Сдвиг: " <<block->offset
             << "Размер: " << block->size
             << "Адрес данных: " << static_cast<void*>(block->data) << endl;
    }
    cout << "Общее кол-во блоков в кэше: " << cache.table.size() << endl;
}


/**
 *
 */
void evict_block_lru() {
    if(cache.usage_list.empty()) {
        return;
    }
    off_t offset = cache.usage_list.front();// берем указатель на самый старый блок
    // cout << "Eviction block at " << offset << endl;
    free(cache.table[offset]->data);
    delete cache.table[offset];//Удаление данных блока из памяти
    cache.table.erase(offset);//Удаление записи из хэш-таблицы
    cache.usage_list.pop_front();//Удаление блока из списка LRU
}


/**
 * Функция обертка над обычным системным lseek,
 * который управляет позицией указателя при работе с файлами
 *
 * @param fd file descriptor
 * @param offset на сколько сместить
 * @param whence в случае данной лабораторной работы принимает только значения `SEEK_CUR` - переместиться на offset от текущей позиции и `SEEK_SET` - переместиться на позицию offset от начала файла.
 * @return
 */
off_t lab2_lseek(int fd, off_t offset, int whence) {
    off_t fixedOffset = lseek(fd, offset, whence);
    if(fixedOffset == -1) {
        cerr << "PANIC: lseek file!!!" << endl;
        return -1;
    }
    return fixedOffset;
}

int lab2_open(const char *path) {
    // Задает режим работы с файлом, чтобы работал наш собственный механизм кэша
    // В данном случае используется комбинация двух флагов:
    // O_RDWR          0x0002          /* open for reading and writing */
    // O_DIRECT direct io, обходя файловый кэш оси
    // constexpr int flags = O_RDWR | O_DIRECT;
    // int fd = open(path, flags);

    // Задает режим работы с файлом
    constexpr int flags = O_RDWR | O_DIRECT | O_CREAT;
    // Устанавливает права доступа для нового файла (например, rw-r--r--)
    // constexpr mode_t mode = 0644;

    int fd = open(path, flags);//, mode);



    if(fd == -1) {
        cerr << "PANIC: open file!!!" << endl;
    }

    cout << "Open success with descriptor: " << fd << endl;
    return fd;
}

int lab2_close(const int fd) {
    const int result = close(fd);
    if(result < 0) {
        cerr << "PANIC: close file!!!" << endl;
    }
    // cout << "Close success" << endl;
    return result;
}


/**
 * Выделяет память, выровненную по заданному объему памяти
 * Гарантирует, что адрес выделенного блока кратен значению alignment.
 *
 *  По факту является просто оберткой над стандартной `posix_memalign`
 *
 * @param alignment Задает требуемое выравнивание памяти
 * @param size Указывает размер выделяемой области памяти.
 *
 * @return Возвращает указатель на выделенную память
 */
void* aligned_alloc(size_t alignment, size_t size) {
    void* aligned_buf = nullptr;

    int ret = posix_memalign(&aligned_buf, alignment, size);

    if(ret != 0) {
        cerr << "PANIC: in align_alloc!!!" << endl;
        return nullptr;
    }

    // cout << "Aligned memory success, return: "  << aligned_buf << endl;
    return aligned_buf;
}

CacheBlock* find_cache_block(off_t offset) {
    auto iterator = cache.table.find(offset);
    if(iterator != cache.table.end()) {
        cache.usage_list.remove(offset);
        cache.usage_list.push_back(offset);
        // cout << "Find cache success";
        return iterator->second;
    }
    return nullptr;
}

ssize_t lab2_read(int fd, void* buf, size_t count) {
    off_t offset = lab2_lseek(fd, 0, SEEK_CUR);
    if(offset == -1) {
        cerr << "PANIC: lseek in rad!!!" << endl;
        return -1;
    }

    CacheBlock *block = find_cache_block(offset);


    //если нашли блок памяти, то записали туда всю информацию
    if(block && block->size >= count) {
        cout << "found block with size: " << block->size << endl;
        memcpy(buf, block->data, count);
        return count;
    }

    void *aligned_buf = aligned_alloc(512, count);
    if(!aligned_buf) {
        return -1;
    }

    ssize_t bytesRead = read(fd, aligned_buf, count);
    if(bytesRead == -1) {
        cerr << "PANIC: read file!!!" << endl;
        free(aligned_buf);
        return -1;
    }

    if(cache.table.size() >= cache.size) {
        evict_block_lru();
    }

    if(cache.table.find(offset) == cache.table.end()) {
        auto addBlock = new CacheBlock();
        addBlock->offset = offset;
        addBlock->size = bytesRead;
        addBlock->data = static_cast<char*>(aligned_buf);
        cache.table[offset] = addBlock;
        cache.usage_list.push_back(offset);
        cout << "Add cache success in read" << endl;
    }else {
        free(aligned_buf);
    }
    return bytesRead;
}

ssize_t lab2_write(int fd, const void* buf, size_t count) {
    off_t offset = lab2_lseek(fd, 0, SEEK_CUR);
    cout << "offset in write: " << offset << endl;

    if(offset == -1) {
        cerr << "PANIC: lseek in write" << endl;
        return -1;
    }

    void* aligned_buf = aligned_alloc(512, count);//выравниванием адрес на кратный 512

    if(!aligned_buf) {
        cerr << "PANIC: to allocate memory" << endl;
        return -1;
    }

    memcpy(aligned_buf, buf, count);//копирует данные из исходного в выровненный
    const ssize_t bytesWritten = write(fd, aligned_buf, count);
    if(bytesWritten < 0) {
        cerr << "PANIC: write in file failed" << endl;
        free(aligned_buf);
        return -1;
    }

    //проверка заполненности кэша
    if(cache.table.size() >= cache.size) {
        evict_block_lru();
    }


    auto iterator = cache.table.find(offset);

    if(iterator == cache.table.end()) {
        auto* addBlock = new CacheBlock();
        addBlock->offset = offset;
        addBlock->size = count;
        addBlock->data = static_cast<char*>(aligned_buf);
        cache.table[offset] = addBlock;
        cache.usage_list.push_back(offset);
    }else {
        free(aligned_buf);
    }

    return bytesWritten;
}

int lab2_fsync(int fd)
{
    for(auto& entry : cache.table) {
        CacheBlock* block = entry.second;
        ssize_t bytesWritten = pwrite(fd, block->data, block->size, block->offset);
        if(bytesWritten != static_cast<ssize_t>(block->size)) {
            cerr << "PANIC: fsync write!!!" << block->offset << endl;
            return -1;
        }
    }
    if(fsync(fd) == -1) {
        cerr << "PANIC: fsync!!!" << endl;
        return -1;
    }
    cout << "Данные успешно синхронизированы с диском" << endl;
    return 0;
}
#ifndef LAB2_H
#define LAB2_H

#include <list>
#include <unordered_map>
#include <sys/types.h>

#define BUFFER_SIZE (1024)
#define CACHE_SIZE (2)

struct CacheBlock {
    off_t offset;//Смещение блока в файле.
    size_t size;//размер блока
    char* data;//указатель на данные
};

struct Cache {
    size_t size; //Максимальное количество блоков в кэше.
    std::pmr::unordered_map<off_t, CacheBlock*> table;//Хранит таблицу кэшированных блоков, где ключ — смещение, а значение — структура CacheBlock.
    std::pmr::list<off_t> usage_list;//Двусвязный список, используемый для отслеживания порядка доступа к блокам (необходим для реализации LRU).
};

int cache_init(size_t size);
void cache_free();
void display_cache();

/**
 * Открытие файла по заданному пути файла,
 * доступного для чтения.
 * Процедура возвращает некоторый хэндл на файл. Пример:
 */
int lab2_open(const char* path);

int lab2_close(int fd);
ssize_t lab2_read(int fd, void* buf, size_t count);
ssize_t lab2_write(int fd, const void* buf, size_t count);
off_t lab2_lseek(int fd, off_t offset, int whence);
int lab2_fsync(int fd);

#endif // LAB2_H


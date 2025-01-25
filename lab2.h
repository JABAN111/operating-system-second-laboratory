#ifndef LAB2_H
#define LAB2_H

#include <list>
#include <unordered_map>
#include <sys/types.h>
#include <utility>

#define BUFFER_SIZE (1024)
#define CACHE_SIZE (2)

struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1);
    }
};



struct CacheBlock {
    ino_t inode;      // Идентификатор файла
    off_t offset;     // Смещение блока в файле
    size_t size;      // Размер блока
    char* data;       // Указатель на данные
};

struct Cache {
    size_t size;
    std::unordered_map<std::pair<ino_t, off_t>, CacheBlock, PairHash> table;
    std::list<std::pair<ino_t, off_t>> usage_list;
};

int cache_init(size_t size);
void cache_free();
void display_cache();
void evict_block_lru();

std::pair<int, ino_t> lab2_open(const char* path);
int lab2_close(int fd);
ssize_t lab2_read(std::pair<int, ino_t> fd_inot, void* buf, size_t count, off_t offset);
ssize_t lab2_write(std::pair<int, ino_t> fd_inot, off_t offset, const void* buf, size_t count);
off_t lab2_lseek(int fd, off_t offset, int whence);
int lab2_fsync(int fd);

#endif // LAB2_H
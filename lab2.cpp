#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <unordered_map>
#include <list>
#include <utility>
#include "lab2.h"
#include <algorithm>

using namespace std;

static Cache cache;

int cache_init(size_t size) {
    cache.size = size;
    return 0;
}

void cache_free() {
    cache.table.clear();
    cache.usage_list.clear();
}

void display_cache() {
    cout << "Содержимое кэша:\n";
    for (const auto &entry: cache.table) {
        const auto &block = entry.second;
        cout << "inode: " << block.inode
                << ", offset: " << block.offset
                << ", size: " << block.size << '\n';
    }
}

void evict_block_lru() {
    if (cache.usage_list.empty()) return;

    auto oldest = cache.usage_list.front();
    cache.usage_list.pop_front();

    auto it = cache.table.find(oldest);
    if (it != cache.table.end()) {
        auto block = it->second;
        free(block.data);
        cache.table.erase(it);
        cout << "Удален блок с inode: " << block.inode << ", offset: " << block.offset << endl;
    }
}

CacheBlock &find_cache_block(ino_t inode, off_t offset) {
    auto key = std::make_pair(inode, offset);

    auto it = cache.table.find(key);
    if (it != cache.table.end()) {
        cache.usage_list.remove(key);
        cache.usage_list.push_back(key);
        return it->second;
    }

    throw std::runtime_error("Cache block not found");
}


off_t lab2_lseek(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}

pair<int, ino_t> lab2_open(const char *path) {
    constexpr int flags = O_RDWR | O_DIRECT;
    int fd = open(path, flags);
    if (fd == -1) {
        cerr << "Ошибка открытия файла\n";
        return {-1, -1};
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        close(fd);
        cerr << "Ошибка получения inode\n";
        return {-1, -1};
    }

    return {fd, file_stat.st_ino};
}

int lab2_close(int fd) {
    return close(fd);
}

ssize_t lab2_read(pair<int, ino_t> fd_inot, void *buf, size_t count, off_t offset) {
    if (offset == -1) {
        cerr << "PANIC: lseek invalid" << endl;
        return -1;
    }

    try {
        CacheBlock &block = find_cache_block(fd_inot.second, offset);

        if (block.size >= count) {
            memcpy(buf, block.data, count);
            return count;
        }
    } catch (const std::runtime_error &e) {
        cerr << e.what() << endl;
    }

    void *aligned_buf = aligned_alloc(512, count);
    if (!aligned_buf) {
        return -1;
    }

    ssize_t bytesRead = read(fd_inot.first, aligned_buf, count);
    if (bytesRead == -1) {
        cerr << "PANIC: read file!!!" << endl;
        free(aligned_buf);
        return -1;
    }


    if (cache.table.size() >= cache.size) {
        evict_block_lru();
    }

    if (cache.table.find({fd_inot.second, offset}) == cache.table.end()) {
        CacheBlock addBlock{fd_inot.second, offset, static_cast<size_t>(bytesRead), static_cast<char *>(aligned_buf)};
        cache.table[{fd_inot.second, offset}] = addBlock;
        cache.usage_list.push_back({fd_inot.second, offset});
    } else {
        free(aligned_buf);
    }

    return bytesRead;
}


ssize_t lab2_write(pair<int, ino_t> fd_inot, off_t offset, const void *buf, size_t count) {
    void *aligned_buf = aligned_alloc(512, count);
    if (!aligned_buf) return -1;

    memcpy(aligned_buf, buf, count);
    ssize_t bytesWritten = pwrite(fd_inot.first, aligned_buf, count, offset);
    free(aligned_buf);

    if (bytesWritten == -1) return -1;

    if (cache.table.size() >= cache.size) evict_block_lru();

    auto key = make_pair(fd_inot.second, offset);

    if (cache.table.find(key) == cache.table.end()) {
        CacheBlock newBlock{fd_inot.second, offset, static_cast<size_t>(bytesWritten), nullptr};
        newBlock.data = static_cast<char *>(malloc(bytesWritten));
        if (!newBlock.data) {
            return -1;
        }
        memcpy(newBlock.data, buf, bytesWritten);
        cache.table[key] = newBlock;
        cache.usage_list.push_back(key);
    }

    return bytesWritten;
}

int lab2_fsync(int fd) {
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) return -1;

    for (auto it = cache.table.begin(); it != cache.table.end();) {
        auto &block = it->second;
        if (block.inode == file_stat.st_ino) {
            pwrite(fd, block.data, block.size, block.offset);
            it = cache.table.erase(it);
        } else {
            ++it;
        }
    }

    return fsync(fd);
}

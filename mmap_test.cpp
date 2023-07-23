#include <barrier>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <thread>
#include <functional>
#include <sys/mman.h>
#include <fcntl.h>
#include <x86intrin.h>

using namespace std;

#define FILE_NAME "/mnt/huge/hugepagefile%d"

#define PAGE_SIZE	(1 << 12)
#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)

constexpr auto ADDR = static_cast<void *>(0x0ULL);
constexpr auto PROT_RW = PROT_READ | PROT_WRITE;

constexpr auto MAP_FLAGS =
    MAP_HUGETLB | MAP_HUGE_1GB | MAP_PRIVATE | MAP_ANONYMOUS;

constexpr auto MMAP_SZ = 64ULL * 1024 * 1024 * 1024;

void *mmap_alloc(char *mmap_path, int fd) {

    auto addr = mmap(ADDR, MMAP_SZ, PROT_RW, MAP_FLAGS, fd,
        0);

    if (addr == MAP_FAILED) {
      perror("mmap");
      unlink(mmap_path);
      exit(1);
    }

    //madvise(addr, MMAP_SZ, MADV_POPULATE_WRITE);
    return addr;
}

void *regular_alloc(void) {
  return std::aligned_alloc(PAGE_SIZE, MMAP_SZ);
}

void thread_fn(int tid, std::barrier<std::function<void()>>* barrier) {
    int fd;
    char mmap_path[256] = {0};

    snprintf(mmap_path, sizeof(mmap_path), FILE_NAME, tid);

    fd = open(mmap_path, O_CREAT | O_RDWR, 0755);

    if (fd < 0) {
      perror("open");
      exit(1);
    }

    barrier->arrive_and_wait();

    auto start = __rdtsc();

    //auto addr = mmap_alloc(mmap_path, fd);
    auto addr = regular_alloc();

    auto diff = __rdtsc() - start;

    //std::memset(addr, 0, MMAP_SZ);

    printf("[%d] Cycles: %llu addr: %p\n", sched_getcpu(), diff, addr);
}

void sync_complete(void) {

}

void no_thread_alloc(void) {
    int fd;
    int tid = 1;
    char mmap_path[256] = {0};

    snprintf(mmap_path, sizeof(mmap_path), FILE_NAME, tid);

    fd = open(mmap_path, O_CREAT | O_RDWR, 0755);

    if (fd < 0) {
      perror("open");
      exit(1);
    }


    auto start = __rdtsc();

    char *addr = (char*) mmap_alloc(mmap_path, fd);
    //auto addr = regular_alloc();

    // auto ret = madvise(addr, MMAP_SZ, MADV_POPULATE_WRITE);
    //
    // if (ret) {
    //   perror("madvise");
    //   printf("errno : %d\n", errno);
    //   //exit(1);
    // }

    uint64_t j = 0;
    for (uint64_t i = 0; i < 64; i++) {
      addr[j] = 0xdd;
      j += (1<< 30);
    }

    auto diff = __rdtsc() - start;

    uint64_t sum = 0;


    printf("[%d] Cycles: %llu addr: %p sum %lu\n", sched_getcpu(), diff, addr, sum);
    exit(1);
}

int main() {
    std::function<void()> on_sync_complete = sync_complete;
    std::barrier barrier(64, on_sync_complete);
    std::vector<std::thread> threads;

    no_thread_alloc();

    for (int i = 0; i < 64; i++) {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(i, &cpuset);
      auto _thread = std::thread(thread_fn, i, &barrier);
      auto ret = pthread_setaffinity_np(_thread.native_handle(), sizeof(cpu_set_t), &cpuset);
      if (ret < 0) {
        perror("pthread_set_affinity");
      }
      threads.push_back(std::move(_thread));
    }

    for (auto& t : threads) {
     t.join();
    }

    return 0;
}

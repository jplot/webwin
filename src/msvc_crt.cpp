#include <cstdlib>

extern "C" void *_aligned_malloc(size_t size, size_t align) {
    void *p = nullptr; if (align < sizeof(void *)) align = sizeof(void *);
    if (posix_memalign(&p, align, size) != 0) return nullptr; return p;
}
extern "C" void _aligned_free(void *p) { free(p); }

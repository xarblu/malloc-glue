#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// _Nullable is a clang extension
#if !defined(__clang__)
#define _Nullable
#endif

/**
 * Override the malloc interface with mimalloc versions
 * if they would be the libc version without (hopefully)
 * breaking apps shipping their own malloc
 * 
 * we will override everything mimalloc would override:
 * https://github.com/microsoft/mimalloc/blob/main/include/mimalloc-override.h
 *
 * some caveats accoring to glibc:
 * https://www.gnu.org/software/libc/manual/html_node/Replacing-malloc.html
 */

/**
 * lookup table for resolved symbols
 */
typedef struct malloc_lut {
    // Standard C allocation
    void* (*malloc)(size_t);
    void* (*calloc)(size_t, size_t);
    void* (*realloc)(void *_Nullable, size_t);
    void (*free)(void *_Nullable);
    char* (*strdup)(const char*);
    char* (*strndup)(const char*, size_t);
    char* (*realpath)(const char*, char*);

    // Various Posix and Unix variants
    void* (*reallocf)(void*, size_t);
    size_t (*malloc_size)(void*);
    size_t (*malloc_usable_size)(void *_Nullable);
    size_t (*malloc_good_size)(size_t);
    void (*cfree)(void*);
    void* (*valloc)(size_t);
    void* (*pvalloc)(size_t);
    void* (*reallocarray)(void *_Nullable, size_t, size_t);
    int (*reallocarr)(void *_Nullable, size_t, size_t);
    void* (*memalign)(size_t, size_t);
    void* (*aligned_alloc)(size_t, size_t);
    int (*posix_memalign)(void**, size_t, size_t);
    int (*_posix_memalign)(void **, size_t, size_t);
} malloc_lut;

/**
 * Global instance of our LUT to actually
 * store the symbols
 */
static malloc_lut lut = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/**
 * Resolve a function named symbol
 * If RTLD_NEXT is from libc return custom
 * else return whatever it is
 */
void* resolve_func(const char* symbol) {
    // no libc -> bad
    void *libc_so = dlopen("libc.so.6", RTLD_NOW | RTLD_NOLOAD);
    if (!libc_so) {
        fprintf(stderr, "libc.so.6 not loaded\n");
        abort();
    }

    // no mimalloc -> bad
    void *libmimalloc_so = dlopen("libmimalloc.so", RTLD_NOW);
    if (!libmimalloc_so) {
        fprintf(stderr, "Failed to load libmimalloc.so\n");
        abort();
    }

    // address of the symbol in libc
    void* libc_sym = dlsym(libc_so, symbol);

    // address of the symbol in mimalloc
    void* libmimalloc_sym = dlsym(libmimalloc_so, symbol);

    // address of the next (in search order) symbol
    void* next_sym = dlsym(RTLD_NEXT, symbol);

    // if next is either libc default or mimalloc select it
    if (next_sym == libc_sym || next_sym == libmimalloc_sym) {
        return libmimalloc_sym;
    }

    // if not that means another library already claimed the symbol
    // use that version then
    return next_sym;
}

/**
 * Check if a symbol is defined (not NULL)
 * if it isn't abort
 */
void check_defined(void* symbol, const char* name) {
    if (symbol == NULL) {
        fprintf(stderr, "%s() is not defined\n", name);
        abort();
    }
}

/**
 * Initialisation to call on load.
 * This initially sets a default "real" malloc interface
 * for following dlopen() calls to work.
 * Once we have working malloc we load our custom malloc shared object
 * and replace all symbols with their equivalent version from it.
 */
__attribute__((constructor)) void init(void) {
    // temporarily set defaults for resolve_func() to work
    lut.malloc = dlsym(RTLD_NEXT, "malloc");
    lut.calloc = dlsym(RTLD_NEXT, "calloc");
    lut.realloc = dlsym(RTLD_NEXT, "realloc");
    lut.free = dlsym(RTLD_NEXT, "free");
    lut.strdup = dlsym(RTLD_NEXT, "strdup");
    lut.strndup = dlsym(RTLD_NEXT, "strndup");
    lut.realpath = dlsym(RTLD_NEXT, "realpath");
    lut.reallocf = dlsym(RTLD_NEXT, "reallocf");
    lut.malloc_size = dlsym(RTLD_NEXT, "malloc_size");
    lut.malloc_usable_size = dlsym(RTLD_NEXT, "malloc_usable_size");
    lut.malloc_good_size = dlsym(RTLD_NEXT, "malloc_good_size");
    lut.cfree = dlsym(RTLD_NEXT, "cfree");
    lut.valloc = dlsym(RTLD_NEXT, "valloc");
    lut.pvalloc = dlsym(RTLD_NEXT, "pvalloc");
    lut.reallocarray = dlsym(RTLD_NEXT, "reallocarray");
    lut.reallocarr = dlsym(RTLD_NEXT, "reallocarr");
    lut.memalign = dlsym(RTLD_NEXT, "memalign");
    lut.aligned_alloc = dlsym(RTLD_NEXT, "aligned_alloc");
    lut.posix_memalign = dlsym(RTLD_NEXT, "posix_memalign");
    lut._posix_memalign = dlsym(RTLD_NEXT, "_posix_memalign");

    // now resolve
    void* malloc = resolve_func("malloc");
    void* calloc = resolve_func("calloc");
    void* realloc = resolve_func("realloc");
    void* free = resolve_func("free");
    void* strdup = resolve_func("strdup");
    void* strndup = resolve_func("strndup");
    void* realpath = resolve_func("realpath");
    void* reallocf = resolve_func("reallocf");
    void* malloc_size = resolve_func("malloc_size");
    void* malloc_usable_size = resolve_func("malloc_usable_size");
    void* malloc_good_size = resolve_func("malloc_good_size");
    void* cfree = resolve_func("cfree");
    void* valloc = resolve_func("valloc");
    void* pvalloc = resolve_func("pvalloc");
    void* reallocarray = resolve_func("reallocarray");
    void* reallocarr = resolve_func("reallocarr");
    void* memalign = resolve_func("memalign");
    void* aligned_alloc = resolve_func("aligned_alloc");
    void* posix_memalign = resolve_func("posix_memalign");
    void* _posix_memalign = resolve_func("_posix_memalign");

    // finally bulk update
    lut.malloc = malloc;
    lut.calloc = calloc;
    lut.realloc = realloc;
    lut.free = free;
    lut.strdup = strdup;
    lut.strndup = strndup;
    lut.realpath = realpath;
    lut.reallocf = reallocf;
    lut.malloc_size = malloc_size;
    lut.malloc_usable_size = malloc_usable_size;
    lut.malloc_good_size = malloc_good_size;
    lut.cfree = cfree;
    lut.valloc = valloc;
    lut.pvalloc = pvalloc;
    lut.reallocarray = reallocarray;
    lut.reallocarr = reallocarr;
    lut.memalign = memalign;
    lut.aligned_alloc = aligned_alloc;
    lut.posix_memalign = posix_memalign;
    lut._posix_memalign = _posix_memalign;
}

// All the wrappers

// malloc(3)
void* malloc(size_t size) {
    check_defined(lut.malloc, "malloc");
    return lut.malloc(size);
}

// malloc(3)
void* calloc(size_t n, size_t size) {
    check_defined(lut.calloc, "calloc");
    return lut.calloc(n, size);
}

// malloc(3)
void* realloc(void *_Nullable ptr, size_t size) {
    check_defined(lut.realloc, "realloc");
    return lut.realloc(ptr, size);
}

// malloc(3)
void free(void *_Nullable ptr) {
    check_defined(lut.free, "free");
    lut.free(ptr);
}

// strdup(3)
char *strdup(const char *s) {
    check_defined(lut.strdup, "strdup");
    return lut.strdup(s);
}

// strdup(3)
char *strndup(const char *s, size_t n) {
    check_defined(lut.strndup, "strndup");
    return lut.strndup(s, n);
}

// realpath(3)
char *realpath(const char *path, char *resolved_path) {
    check_defined(lut.realpath, "realpath");
    return lut.realpath(path, resolved_path);
}

// reallocf(3bsd)
void *reallocf(void *ptr, size_t size) {
    check_defined(lut.reallocf, "reallocf");
    return lut.reallocf(ptr, size);
}

// malloc_usable_size(3)
size_t malloc_size(void *ptr) {
    check_defined(lut.malloc_size, "malloc_size");
    return lut.malloc_size(ptr);
}

// malloc_usable_size(3)
size_t malloc_usable_size(void *_Nullable ptr) {
    check_defined(lut.malloc_usable_size, "malloc_usable_size");
    return lut.malloc_usable_size(ptr);
}

// malloc_usable_size(3)
size_t malloc_good_size(size_t size) {
    check_defined(lut.malloc_good_size, "malloc_good_size");
    return lut.malloc_good_size(size);
}

// cfree(3)
void cfree(void *ptr) {
    check_defined(lut.cfree, "cfree");
    return lut.cfree(ptr);
}

// posix_memalign(3)
[[deprecated]] void *valloc(size_t size) {
    check_defined(lut.valloc, "valloc");
    return lut.valloc(size);
}

// posix_memalign(3)
[[deprecated]] void *pvalloc(size_t size) {
    check_defined(lut.pvalloc, "pvalloc");
    return lut.pvalloc(size);
}

// malloc(3)
void *reallocarray(void *_Nullable ptr, size_t n, size_t size) {
    check_defined(lut.reallocarray, "reallocarray");
    return lut.reallocarray(ptr, n, size);
}

// reallocarr(3)
int reallocarr(void *_Nullable ptr, size_t n, size_t size) {
    check_defined(lut.reallocarr, "reallocarr");
    return lut.reallocarr(ptr, n, size);
}

// posix_memalign(3)
[[deprecated]] void *memalign(size_t alignment, size_t size) {
    check_defined(lut.memalign, "memalign");
    return lut.memalign(alignment, size);
}

// posix_memalign(3)
void *aligned_alloc(size_t alignment, size_t size) {
    check_defined(lut.aligned_alloc, "aligned_alloc");
    return lut.aligned_alloc(alignment, size);
}

// posix_memalign(3)
int posix_memalign(void **memptr, size_t alignment, size_t size) {
    check_defined(lut.posix_memalign, "posix_memalign");
    return lut.posix_memalign(memptr, alignment, size);
}

// posix_memalign(3)
int _posix_memalign(void **memptr, size_t alignment, size_t size) {
    check_defined(lut._posix_memalign, "_posix_memalign");
    return lut._posix_memalign(memptr, alignment, size);
}

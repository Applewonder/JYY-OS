#include <os.h>
#include <syscall.h>

// #include "initcode.inc"

void uproc_init() {
    vme_init(pmm->alloc, pmm->free);
}

MODULE_DEF(uproc) = {
    .init = uproc_init,
};

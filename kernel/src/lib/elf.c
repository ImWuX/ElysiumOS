#include "elf.h"
#include <klibc/string.h>
#include <lib/kprint.h>
#include <lib/assert.h>
#include <memory/pmm.h>
#include <memory/hhdm.h>
#include <memory/heap.h>
#include <arch/vmm.h>

#define ID0 0x7F
#define ID1 'E'
#define ID2 'L'
#define ID3 'F'

// TODO: This are only correct for amd64
#define LITTLE_ENDIAN 1
#define CLASS64 2
#define MACHINE_386 0x3E

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7

#define PF_X 0x1 /* Execute */
#define PF_W 0x2 /* Write */
#define PF_R 0x4 /* Read */

static_assert(sizeof(unsigned char) == 1);

typedef uint64_t elf64_addr_t;
typedef uint64_t elf64_off_t;
typedef uint16_t elf64_half_t;
typedef uint32_t elf64_word_t;
typedef int32_t elf64_sword_t;
typedef uint64_t elf64_xword_t;
typedef int64_t elf64_sxword_t;

typedef struct {
    unsigned char magic[4];
    unsigned char class;
    unsigned char encoding;
    unsigned char file_version;
    unsigned char abi;
    unsigned char abi_version;
    unsigned char rsv0[6];
    unsigned char nident;
} __attribute__((packed)) elf_identifier_t;

typedef struct {
    elf_identifier_t ident;
    elf64_half_t type;
    elf64_half_t machine;
    elf64_word_t version;
    elf64_addr_t entry;
    elf64_off_t phoff; /* Program header offset */
    elf64_off_t shoff; /* Section header offset */
    elf64_word_t flags;
    elf64_half_t ehsize; /* ELF Header size */
    elf64_half_t phentsize; /* Program header entry size */
    elf64_half_t phnum; /* Program header count */
    elf64_half_t shentsize; /* Section header entry size */
    elf64_half_t shnum; /* Section header count */
    elf64_half_t shstrndx; /* Section name string table index */
} __attribute__((packed)) elf_header_t;

typedef struct {
    elf64_word_t type;
    elf64_word_t flags;
    elf64_off_t offset;
    elf64_addr_t vaddr; /* Virtual address */
    elf64_addr_t paddr; /* Physical address */
    elf64_xword_t filesz; /* File size */
    elf64_xword_t memsz; /* Memory size */
    elf64_xword_t align; /* Alignment */
} __attribute__((packed)) elf_phdr_t;

bool elf_load(vfs_node_t *node, vmm_address_space_t *as, char **interpreter, elf_auxv_t *auxv) {
    vfs_rw_t rw;
    size_t read_count;
    int r;

    // TODO: ENOEXEC is probably the errno we want to return

    #define FAIL_GOTO fail_free_none
    #define FAIL(MSG) { kprintf("ELF: WARNING: %s. Aborting.\n", MSG); goto FAIL_GOTO; }

    if(interpreter) *interpreter = 0;

    if(node->type != VFS_NODE_TYPE_FILE) FAIL("Tried loading a non-elf file");
    vfs_node_attr_t attributes;
    r = node->ops->attr(node, &attributes);
    if(r < 0) FAIL("Unable to retrieve file attributes");
    if(attributes.file_size < sizeof(elf_header_t)) FAIL("File does not contain an ELF header");

    elf_header_t *header = heap_alloc(sizeof(elf_header_t));
    #undef FAIL_GOTO
    #define FAIL_GOTO fail_free_header
    rw = (vfs_rw_t) { .rw = VFS_RW_READ, .size = sizeof(elf_header_t), .buffer = (void *) header };
    r = node->ops->rw(node, &rw, &read_count);
    if(r < 0 || read_count != sizeof(elf_header_t)) FAIL("Failed to read ELF header");

    if(header->ident.magic[0] != ID0 || header->ident.magic[1] != ID1 ||header->ident.magic[2] != ID2 || header->ident.magic[3] != ID3) FAIL("Invalid header identification");
    if(header->ident.class != CLASS64) FAIL("Only elf64 is supported currently");
    if(header->ident.encoding != LITTLE_ENDIAN) FAIL("Only little endian encoding is supported");
    if(header->version > 1) FAIL("Unsupported version");
#ifdef __ARCH_AMD64
    if(header->machine != MACHINE_386) FAIL("Only the i386:x86-64 instruction-set is supported");
#endif
    if(header->phentsize < sizeof(elf_phdr_t)) FAIL("Program headers are too small");

    elf_phdr_t *phdr = heap_alloc(header->phentsize);
    #undef FAIL_GOTO
    #define FAIL_GOTO fail_free_all
    for(int i = 0; i < header->phnum; i++) {
        rw = (vfs_rw_t) { .rw = VFS_RW_READ, .size = header->phentsize, .offset = header->phoff + (i * header->phentsize), .buffer = (void *) phdr };
        r = node->ops->rw(node, &rw, &read_count);
        if(r < 0 || read_count != header->phentsize) FAIL("Failed to read program header");

        switch(phdr->type) {
            case PT_NULL: break;
            case PT_LOAD:
                if(phdr->filesz > phdr->memsz) FAIL("Invalid program header (filesz > memsz)");

                uint64_t prot = VMM_PROT_USER;
                if(phdr->flags & PF_W) prot |= VMM_PROT_WRITE;
                if(phdr->flags & PF_X) prot |= VMM_PROT_EXEC;

                for(uintptr_t count = 0; count < phdr->memsz;) {
                    uintptr_t alignment_offset = (phdr->vaddr + count) & (ARCH_PAGE_SIZE - 1);
                    pmm_page_t *page = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO);

                    if(phdr->filesz > count) {
                        size_t read_sz = phdr->filesz - count;
                        if(read_sz > ARCH_PAGE_SIZE - alignment_offset) read_sz = ARCH_PAGE_SIZE - alignment_offset;
                        rw = (vfs_rw_t) { .rw = VFS_RW_READ, .size = read_sz, .offset = phdr->offset + count, .buffer = (void *) HHDM(page->paddr + alignment_offset) };
                        r = node->ops->rw(node, &rw, &read_count);
                        if(r < 0 || read_count != read_sz) FAIL("Failed to load program header");
                    }

                    arch_vmm_map(as, phdr->vaddr + count - alignment_offset, page->paddr, prot);
                    count += ARCH_PAGE_SIZE - alignment_offset;
                }
                break;
            case PT_INTERP:
                ASSERT(interpreter);
                char *interp = heap_alloc(phdr->filesz + 1);
                memset(interp, 0, phdr->filesz + 1);
                r = node->ops->rw(node, &(vfs_rw_t) { .rw = VFS_RW_READ, .buffer = interp, .offset = phdr->offset, .size = phdr->filesz}, &read_count);
                if(r < 0) FAIL("Failed to read interpreter path\n");
                *interpreter = interp;
                break;
            case PT_PHDR:
                auxv->phdr = phdr->vaddr;
                break;
            default:
                kprintf("WARNING: Ignoring program header %lu\n", phdr->type);
                break;
        }
    }

    auxv->entry = header->entry;
    auxv->phent = header->phentsize;
    auxv->phnum = header->phnum;

    heap_free(header);
    heap_free(phdr);
    return false;

    fail_free_all:
    if(interpreter && *interpreter) heap_free(*interpreter);
    heap_free(phdr);
    fail_free_header:
    heap_free(header);
    fail_free_none:
    return true;
    #undef FAIL_GOTO
    #undef FAIL
}
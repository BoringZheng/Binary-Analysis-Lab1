#include <bfd.h>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "loader.h"

static bool bfd_inited = false;
static void init_bfd() {
    if (!bfd_inited) {
        bfd_init();
        bfd_inited = true;
    }
}

static BinaryArch arch_from_bfd(enum bfd_architecture a, unsigned mach, unsigned &bits, std::string &astr) {
    switch (a) {
        case bfd_arch_i386:
            if (mach == bfd_mach_i386_i386) { bits = 32; astr = "x86"; return ARCH_X86; }
            if (mach == bfd_mach_x86_64)    { bits = 64; astr = "x86-64"; return ARCH_X86_64; }
            break;
        case bfd_arch_arm:     bits = 32; astr = "ARM";     return ARCH_ARM;
        case bfd_arch_aarch64: bits = 64; astr = "AArch64"; return ARCH_AARCH64;
        default: break;
    }
    bits = 0; astr = "unknown"; return ARCH_NONE;
}

static SectionType sectype_from_flags(flagword f) {
    if (f & SEC_CODE) return SEC_TYPE_CODE;
    if (f & SEC_DATA) return SEC_TYPE_DATA;
    return SEC_TYPE_NONE;
}

static SymbolBind bind_from_flags(flagword f) {
    if (f & BSF_GLOBAL) return SYM_BIND_GLOBAL;
    if (f & BSF_LOCAL)  return SYM_BIND_LOCAL;
    if (f & BSF_WEAK)   return SYM_BIND_WEAK;
    return SYM_BIND_NONE;
}

static BinaryType bin_type_from_bfd(bfd* abfd, std::string &tstr) {
    if (bfd_check_format(abfd, bfd_object)) {
        const char* name = bfd_get_target(abfd);
        if (name && std::string(name).find("elf") != std::string::npos) { tstr = "ELF"; return BIN_TYPE_ELF; }
        if (name && (std::string(name).find("pei") != std::string::npos ||
                     std::string(name).find("pe")  != std::string::npos)) { tstr = "PE";  return BIN_TYPE_PE; }
        tstr = "Object"; return BIN_TYPE_ELF;
    }
    tstr = "Unknown"; return BIN_TYPE_AUTO;
}

static int load_sections_bfd(bfd *abfd, Binary *bin) {
    for (asection *s = abfd->sections; s; s = s->next) {
        const char *name = bfd_section_name(s);
        if (!name) name = "<unnamed>";

        flagword flags = bfd_section_flags(s);

        Section sec;
        sec.binary = bin;
        sec.name   = name;
        sec.vma    = bfd_section_vma(s);
        sec.size   = bfd_section_size(s);
        sec.type   = sectype_from_flags(flags);
        sec.bytes  = nullptr;

        if (!(flags & SEC_HAS_CONTENTS)) {
            bin->sections.push_back(std::move(sec));
            continue;
        }

        if (sec.size > 0) {
            sec.bytes = new uint8_t[sec.size];
            if (!bfd_get_section_contents(abfd, s, sec.bytes, 0, sec.size)) {
                delete[] sec.bytes; sec.bytes = nullptr;
            }
        }
        bin->sections.push_back(std::move(sec));
    }
    return 0;
}

/* 使用 minisymbols 读符号表（兼容新老 binutils） */
static void load_symbols_bfd(bfd *abfd, Binary *bin) {
    asymbol *store = bfd_make_empty_symbol(abfd);

    auto read_and_consume = [&](bfd_boolean dyn) {
        void *minisyms = nullptr;
        unsigned int size = 0;
        long count = bfd_read_minisymbols(abfd, dyn, &minisyms, &size);
        if (count <= 0 || !minisyms || size == 0) {
            if (minisyms) free(minisyms);
            return;
        }

        // 遍历 minisymbol 表
        for (long i = 0; i < count; ++i) {
            const void *entry = static_cast<const void*>(
                static_cast<const bfd_byte*>(minisyms) + i * size);

            // ✅ 在你的环境里，这个函数名是 bfd_minisymbol_to_symbol
            asymbol *sym = bfd_minisymbol_to_symbol(abfd, dyn, entry, store);
            if (!sym || !sym->section) continue;

            flagword f = sym->flags;

            // 仅保留函数和数据对象
            SymbolType ty = SYM_TYPE_NONE;
            if (f & BSF_FUNCTION)      ty = SYM_TYPE_FUNC;
            else if (f & BSF_OBJECT)   ty = SYM_TYPE_OBJECT;
            else continue;

            Symbol s;
            s.binary = bin;
            s.name   = sym->name ? sym->name : "";
            s.addr   = bfd_asymbol_value(sym);
            s.size   = 0;
            s.type   = ty;
            s.bind   = (f & BSF_GLOBAL) ? SYM_BIND_GLOBAL :
                       (f & BSF_LOCAL)  ? SYM_BIND_LOCAL  :
                       (f & BSF_WEAK)   ? SYM_BIND_WEAK   : SYM_BIND_NONE;

            bin->symbols.push_back(std::move(s));
        }

        free(minisyms);
    };

    // 先读静态符号表，再读动态符号表
    read_and_consume(FALSE);
    read_and_consume(TRUE);
}


int load_binary(const char *fname, Binary *bin, BinaryType /*type*/) {
    init_bfd();

    bfd *abfd = bfd_openr(fname, nullptr);
    if (!abfd) {
        fprintf(stderr, "[bfd] open failed: %s\n", bfd_errmsg(bfd_get_error()));
        return -1;
    }

    // 有些环境对 matches 支持不稳定，分步更鲁棒：object -> core -> archive
    if (!bfd_check_format(abfd, bfd_object)) {
        // 记录第一次失败原因
        const char* e1 = bfd_errmsg(bfd_get_error());
        // 尝试 core 文件（避免把 core 当成 object 报错）
        if (bfd_check_format(abfd, bfd_core)) {
            fprintf(stderr, "[bfd] '%s' is a core file; not a loadable object.\n", fname);
            bfd_close(abfd);
            return -1;
        }
        // 尝试 archive（静态库 .a）
        if (bfd_check_format(abfd, bfd_archive)) {
            fprintf(stderr, "[bfd] '%s' is an archive; please pass a regular executable/object.\n", fname);
            bfd_close(abfd);
            return -1;
        }
        // 三者都不是：给出原始错误
        fprintf(stderr, "[bfd] not an object: %s (detail: %s)\n", fname, e1);
        bfd_close(abfd);
        return -1;
    }

    bin->filename = fname;
    bin->type = bin_type_from_bfd(abfd, bin->type_str);
    bin->entry = bfd_get_start_address(abfd);

    unsigned bits = 0; std::string astr;
    bin->arch = arch_from_bfd(bfd_get_arch(abfd), bfd_get_mach(abfd), bits, astr);
    bin->arch_str = astr; bin->bits = bits;

    if (load_sections_bfd(abfd, bin) < 0) {
        fprintf(stderr, "[bfd] load sections failed: %s\n", bfd_errmsg(bfd_get_error()));
        bfd_close(abfd);
        return -1;
    }
    load_symbols_bfd(abfd, bin);

    bfd_close(abfd);
    return 0;
}


void unload_binary(Binary *bin) {
    for (auto &s : bin->sections) { delete[] s.bytes; s.bytes = nullptr; }
    bin->sections.clear();
    bin->symbols.clear();
}

const Section* Binary::get_text_section() const {
    auto it = std::find_if(sections.begin(), sections.end(),
        [](const Section& s){ return s.type == SEC_TYPE_CODE; });
    return (it == sections.end()) ? nullptr : &(*it);
}

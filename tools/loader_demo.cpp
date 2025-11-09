#include "loader.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cinttypes>

static void usage(const char* prog) {
    printf("Usage:\n");
    printf("  %s <binary> [--dump-section <name>] [--print-data-symbols]\n", prog);
}

static void print_hex(const uint8_t* p, uint64_t n, uint64_t base_vma) {
    const uint64_t W = 16;
    for (uint64_t i = 0; i < n; i += W) {
        printf("%08" PRIx64 "  ", base_vma + i);
        uint64_t line = std::min<uint64_t>(W, n - i);
        for (uint64_t j = 0; j < line; ++j) printf("%02x ", p[i+j]);
        for (uint64_t j = line; j < W; ++j) printf("   ");
        printf(" |");
        for (uint64_t j = 0; j < line; ++j) {
            uint8_t c = p[i+j];
            putchar((c >= 32 && c < 127) ? c : '.');
        }
        printf("|\n");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    const char* binpath = argv[1];
    const char* dump_sec = nullptr;
    bool print_data_syms = false;

    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "--dump-section") && i+1 < argc) {
            dump_sec = argv[++i];
        } else if (!strcmp(argv[i], "--print-data-symbols")) {
            print_data_syms = true;
        }
    }

    Binary bin;
    if (load_binary(binpath, &bin, BIN_TYPE_AUTO) < 0) {
        fprintf(stderr, "Failed to load binary: %s\n", binpath);
        return 2;
    }

    printf("File   : %s\n", bin.filename.c_str());
    printf("Type   : %s\n", bin.type_str.c_str());
    printf("Arch   : %s, %u-bit\n", bin.arch_str.c_str(), bin.bits);
    printf("Entry  : 0x%016" PRIx64 "\n", bin.entry);

    printf("\n[Sections]\n");
    for (auto &s : bin.sections) {
        const char* t = (s.type==SEC_TYPE_CODE?"CODE":(s.type==SEC_TYPE_DATA?"DATA":"NONE"));
        printf("  %-18s vma=0x%016" PRIx64 " size=%-8" PRIu64 " type=%s\n",
               s.name.c_str(), s.vma, s.size, t);
    }

    printf("\n[Symbols]\n");
    for (auto &sym : bin.symbols) {
        const char* t =
            sym.type==SYM_TYPE_FUNC?"FUNC":
            sym.type==SYM_TYPE_OBJECT?"OBJECT":"OTHER";
        const char* b =
            sym.bind==SYM_BIND_GLOBAL?"GLOBAL":
            sym.bind==SYM_BIND_LOCAL?"LOCAL":
            sym.bind==SYM_BIND_WEAK?"WEAK":"";
        printf("  %-30s %-6s %-6s addr=0x%016" PRIx64 "\n",
               sym.name.c_str(), t, b, sym.addr);
    }

    if (dump_sec) {
        auto it = std::find_if(bin.sections.begin(), bin.sections.end(),
            [&](const Section& s){ return s.name == dump_sec; });
        if (it == bin.sections.end()) {
            fprintf(stderr, "\n[Dump] section '%s' not found.\n", dump_sec);
        } else if (!it->bytes || it->size == 0) {
            fprintf(stderr, "\n[Dump] section '%s' is empty or has no file contents.\n", dump_sec);
        } else {
            printf("\n[Dump %s]\n", dump_sec);
            print_hex(it->bytes, it->size, it->vma);
        }
    }

    if (print_data_syms) {
        printf("\n[Data Symbols]\n");
        for (auto &sym : bin.symbols) {
            if (sym.type == SYM_TYPE_OBJECT) {
                const char* b =
                    sym.bind==SYM_BIND_GLOBAL?"GLOBAL":
                    sym.bind==SYM_BIND_LOCAL?"LOCAL":
                    sym.bind==SYM_BIND_WEAK?"WEAK":"";
                printf("  %-30s %-6s addr=0x%016" PRIx64 "\n",
                       sym.name.c_str(), b, sym.addr);
            }
        }
    }

    unload_binary(&bin);
    return 0;
}
#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Binary;
struct Section;

enum BinaryType { BIN_TYPE_AUTO = 0, BIN_TYPE_ELF, BIN_TYPE_PE };
enum BinaryArch { ARCH_NONE = 0, ARCH_X86, ARCH_X86_64, ARCH_ARM, ARCH_AARCH64 };

enum SectionType { SEC_TYPE_NONE = 0, SEC_TYPE_CODE, SEC_TYPE_DATA };

enum SymbolType { SYM_TYPE_NONE = 0, SYM_TYPE_FUNC, SYM_TYPE_OBJECT };

enum SymbolBind { SYM_BIND_NONE = 0, SYM_BIND_LOCAL, SYM_BIND_GLOBAL, SYM_BIND_WEAK };

struct Section {
    Binary      *binary = nullptr;
    std::string  name;
    SectionType  type = SEC_TYPE_NONE;
    uint64_t     vma = 0;
    uint64_t     size = 0;
    uint8_t     *bytes = nullptr;
    bool contains(uint64_t addr) const { return (addr >= vma) && (addr - vma < size); }
};

struct Symbol {
    Binary      *binary = nullptr;
    std::string  name;
    SymbolType   type = SYM_TYPE_NONE;
    SymbolBind   bind = SYM_BIND_NONE;
    uint64_t     addr = 0;
    uint64_t     size = 0;
};

struct Binary {
    std::string              filename;
    BinaryType               type = BIN_TYPE_AUTO;
    std::string              type_str;
    BinaryArch               arch = ARCH_NONE;
    std::string              arch_str;
    unsigned                 bits = 0;
    uint64_t                 entry = 0;
    std::vector<Section>     sections;
    std::vector<Symbol>      symbols;
    const Section* get_text_section() const;
};

int  load_binary(const char *fname, Binary *bin, BinaryType type);
void unload_binary(Binary *bin);

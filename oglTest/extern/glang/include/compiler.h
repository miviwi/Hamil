#pragma once

#include <vm/vm.h>
#include <vm/assembler.h>
#include <debug/database.h>

#include <vector>

namespace glang {

assembler::CodeObject compile_string(const char *src, DebugDatabase::Ptr debug, bool output_assembly = false);
assembler::CodeObject compile_string(const char *src, bool output_assembly = false);

/*
  BINARY FORMAT:
    GLNG [4 byte size (little endian)] [program]
*/

std::vector<unsigned char> export_binary(assembler::CodeObject& co);
assembler::CodeObject import_binary(unsigned char *binary);

}
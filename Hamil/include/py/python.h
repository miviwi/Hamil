#pragma once

#include <common.h>

#include <Python.h>

#include <string>
#include <vector>

namespace py {

class Object;

void init();
void finalize();

// Do not call init() before or finalize() after this!
int main(int argc, char *argv[]);

void exec(const char *input);
Object eval(const char *input);

void run_script(const char *input);

Object load(const void *code, size_t sz);
Object load(const std::vector<char>& code);
Object deserialize(const void *code, size_t sz);
Object deserialize(const std::vector<char>& code);

std::vector<char> compile(const char *src, const char *filename = "");
std::vector<char> serialize(const char *src, const char *filename = "");

Object get_global(const char *name);
void set_global(const char *name, const Object& value);

}
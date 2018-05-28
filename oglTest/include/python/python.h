#pragma once

#include <common.h>

#include <Python.h>

#include <string>
#include <vector>

namespace python {

class Object;

void init();
void finalize();

void exec(const char *input);
Object eval(const char *input);

Object load(const void *code, size_t sz);
Object load(const std::vector<char>& code);

std::vector<char> compile(const char *src, const char *filename = nullptr);

Object get_global(const char *name);
void set_global(const char *name, const Object& value);

}
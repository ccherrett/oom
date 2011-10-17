//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2011 Filipe Coelho (falktx@gmail.com)
//=========================================================

#include "lib_functions.h"

void* lib_open(const char* filename)
{
#ifdef __WINDOWS__
        return LoadLibraryA(filename);
#else
        return dlopen(filename, RTLD_NOW);
#endif
}

int lib_close(void* lib)
{
#ifdef __WINDOWS__
        return FreeLibrary((HMODULE)lib);
#else
        return dlclose(lib);
#endif
}

void* lib_symbol(void* lib, const char* symbol)
{
#ifdef __WINDOWS__
    return (void*)GetProcAddress((HMODULE)lib, symbol);
#else
    return dlsym(lib, symbol);
#endif
}

const char* lib_error()
{
#ifdef __WINDOWS__
    return "Unknown error";
#else
    return dlerror();
#endif
}

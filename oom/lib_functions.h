//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2011 Filipe Coelho (falktx@gmail.com)
//=========================================================

#ifndef LIB_FUNCTIONS_H
#define LIB_FUNCTIONS_H

#if defined(__WIN32__) || defined(__WIN64__) || defined(__WINE__)
#include <windows.h>
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#else
#include <dlfcn.h>
#endif

void* lib_open(const char* filename);
int lib_close(void* lib);
void* lib_symbol(void* lib, const char* symbol);
const char* lib_error();

#endif // LIB_FUNCTIONS_H

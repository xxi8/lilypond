#include <fstream.h>
#include <signal.h>
#include <std/new.h>
#include <stdlib.h>
#include "debug.hh"
#include "dstream.hh"
#include "vector.hh"

Dstream monitor(&cout,".dstreamrc");
ostream * nulldev =0;

struct _Dinit {
    _Dinit() { nulldev = new ofstream("/dev/null");}
    ~_Dinit() { delete nulldev; }
} dinit;



/*
  want to do a stacktrace .
  */
void
mynewhandler()
{
    assert(false);
}

void
float_handler(int)
{
    cerr << "Floating point exception .. \n"<< flush;
    assert(false);
}

void
debug_init()
{
#ifndef NDEBUG
    set_new_handler(&mynewhandler);
#endif
    set_matrix_debug(monitor);
    signal(SIGFPE, float_handler);
}   

bool check_debug=false;

void
set_debug(bool b)
{
    check_debug =b;
}



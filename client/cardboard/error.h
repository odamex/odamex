#ifndef GERROR_H
#define GERROR_H

#include <string>
#include <fstream>
#include <stdarg.h>
using namespace std;


class basicError
{
   public:
   // Throw functions ----------------------------------------------------------------------
   // These functions will create a new basicError object, set the error string according to the
   // parameters given, and throw an exception with that object passed as a basicError pointer.
   static void Throw(const string str);
   static void Throw(const char *fmt, ...);

   string str;
};


class fatalError
{
   public:
   // Throw functions ----------------------------------------------------------------------
   // These functions will create a new fatalError object, set the error string according 
   // to the parameters given, and throw an exception with that object passed as a fatalError 
   // pointer.
   static void Throw(const string str);
   static void Throw(const char *fmt, ...);

   string str;
};


class logStream
{
   public:
   // Log functions ------------------------------------------------------------------------
   // enable must be called before any logging will be recorded. If filename is blank
   // the file will not be opened. If you pass false to print and "" to filename it will have
   // the same effect as calling disable. 
   static void enable(const string filename, bool append, bool print);
   // These functions will write the given messages to the current logging stream. By
   // default, these functions will call printf with the error string.
   static void log(const string str);
   static void log(const char *fmt, ...);
   // Stops all logging.
   static void disable();

   protected:
   // protected member variables -----------------------------------------------------------
   static fstream file;
   static string  logname;
   static bool    printlogs;
};


#endif
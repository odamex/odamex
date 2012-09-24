#ifndef __WIN32TIME__
#define __WIN32TIME__

#ifdef WIN32

char *
strptime (const char *buf, const char *fmt, struct tm *timeptr);
time_t
timegm (struct tm *tm);

#endif

#endif

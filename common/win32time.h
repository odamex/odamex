#ifndef __WIN32TIME__
#define __WIN32TIME__

#ifdef WIN32

#if (defined _MSC_VER)
#define strncasecmp _strnicmp
#endif

char *
strptime (const char *buf, const char *fmt, struct tm *timeptr);
time_t
timegm (struct tm *tm);

#endif

#endif

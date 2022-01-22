//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <signal.h> // For raise.

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#undef  MIN
#undef  MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define C_STRING(x) (x.toLocal8Bit().constData())
#define CB_BASENAME(x) C_STRING(cb_emu_8080::BaseName(x))

#define CB_CALLOC(Pointer,Cast,Num,Size)                        \
    {                                                           \
    Pointer = (Cast) calloc(Num,Size);                          \
    if (!Pointer)                                               \
        {                                                       \
        if (QThread::currentThread() == qApp->thread())         \
            {                                                   \
            printf("(%s)(%-20s,%5d): ",                         \
                   "AppThread",                                 \
                   CB_BASENAME(__FILE__),                       \
                   __LINE__);                                   \
            }                                                   \
        else                                                    \
            {                                                   \
            printf("(%p)(%-20s,%5d): ",                         \
                   QThread::currentThread(),                    \
                   CB_BASENAME(__FILE__),                       \
                   __LINE__);                                   \
            }                                                   \
        printf("%-20s  ","Memory allocation error.");           \
        printf("\n");                                           \
        fflush(stdout);                                         \
        ::raise(SIGABRT);                                       \
        }                                                       \
    }


#define CB_FREE(x)  \
    {               \
    if (x) free(x); \
    x = nullptr;    \
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8

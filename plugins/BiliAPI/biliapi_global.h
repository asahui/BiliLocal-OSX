#ifndef BILIAPI_GLOBAL_H
#define BILIAPI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(BILIAPI_LIBRARY)
#  define BILIAPISHARED_EXPORT Q_DECL_EXPORT
#else
#  define BILIAPISHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // BILIAPI_GLOBAL_H

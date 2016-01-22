#ifndef STAYAWAKEPOI_GLOBAL_H
#define STAYAWAKEPOI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(STAYAWAKEPOI_LIBRARY)
#  define POI_EXPORT Q_DECL_EXPORT
#else
#  define POI_EXPORT Q_DECL_IMPORT
#endif

#endif // STAYAWAKEPOI_GLOBAL_H

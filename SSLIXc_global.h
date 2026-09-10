#ifndef SSLIXC_GLOBAL_H
#define SSLIXC_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(SSLIXC_LIBRARY)
#define SSLIXC_EXPORT Q_DECL_EXPORT
#else
#define SSLIXC_EXPORT Q_DECL_IMPORT
#endif

#endif // SSLIXC_GLOBAL_H

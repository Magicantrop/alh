/**
 * @file bool.h
 * @brief Boolean type compatibility header
 * 
 * This header provides backward compatibility for projects that don't
 * have a native boolean type. It defines BOOL as an integer and provides
 * TRUE/FALSE constants.
 */

#ifndef BOOL
/**
 * @def BOOL
 * @brief Boolean type definition (int) for compatibility
 * 
 * Defines BOOL as an integer type for environments that don't
 * have a native boolean type. This allows code to use BOOL consistently
 * across different platforms and compilers.
 */
typedef int  BOOL;
#endif

#ifndef TRUE
/**
 * @def TRUE
 * @brief True value constant (1)
 * 
 * Defines TRUE as 1 for boolean operations when using the BOOL type.
 */
#define TRUE (1)
#endif

#ifndef FALSE
/**
 * @def FALSE
 * @brief False value constant (0)
 * 
 * Defines FALSE as 0 for boolean operations when using the BOOL type.
 */
#define FALSE (0)
#endif
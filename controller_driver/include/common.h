#ifndef INCLUDE_COMMON_H_
#define INCLUDE_COMMON_H_

#include <hal.h>
#include <ch.h>

/*** Configuration ***/

/* Program routine selection */
/*
 * Bad idea to make it <enum> as we use #if (preprocessor) to
 * check what routine to use
 * <enum> is compilation type, not preprocessor
 * This must be #define
 */
#define     PROGRAM_ROUTINE_MASTER                  0
#define     PROGRAM_ROUTINE_TEST_BREAK_SENSOR       1

#define     MAIN_PROGRAM_ROUTINE				    PROGRAM_ROUTINE_MASTER

/*** Prototypes ***/

#endif /* INCLUDE_COMMON_H_ */

#ifndef TESTS_TESTS_H_
#define TESTS_TESTS_H_

#include <common.h>
/**************************/
/*** Brake Sensor tests ***/
/**************************/

/*  If defined - simulation is used to generated signals for sensor */
/*
 *  Hardware connection for simulation
 *  PA4 (DAC) <-> PA7               | Direct connection acts strangely, but it wokrs
 *  Direct control of PA0
 */

#define TEST_BRAKE_SENSOR_SIMULATED

/*
 * @brief   Routine of brake sensor testing
 * @note    The routine has internal infinite loop
 * @note    SD7 is used for testing (PE7, PE8)
 */
void testBrakeSensorRoutine( void );

/********************************/
/*** Brake Unit Control tests ***/
/********************************/

/**
 * @brief   Routine of brake unit control system
 * @note    The routine has internal infinite loop
 * @note    SD7 is used for testing (PE7, PE8)
 */
void testBrakeUnitCSRoutine( void );

/***********************************/
/*** Wheel position sensor tests ***/
/***********************************/

/*
 *  Hardware connection for simulation
 *  PF13 <-> PF14 (pulses)
 */

#define TEST_WHEEL_POS_SENSOR_SIMULATED
/*
 * @brief   Routine of wheel position sensor testing
 * @note    The routine has internal infinite loop
 * @note    SD7 is used for testing (PE7, PE8)
 */
void testWheelPosSensorRoutine( void );

/**************************/
/*** Clutch lever tests ***/
/**************************/

/**
 * @brief   Routine of clutch level of quadrocycle
 * @note    The routine has internal infinite loop
 * @note    Test uses LEDs to check pressing
 */
void testClutchLeverRoutine( void );

/****************************/
/*** Driver Control tests ***/
/****************************/

/*
 * @brief   Routine of low level driver control testing
 * @note    The routine has internal infinite loop
 */
void testDriverControlRoutine( void );

/**
 * @brief   Routine of low level driver control testing
 * @note    The routine has internal infinite loop
 * @note    Extended behavior that interacts with user through Serial
 */
void testDriverControlRoutineExt1( void );

/*
 * @brief   Routine of steering sensors testing
 * @note    The routine has internal infinite loop
 * @note    SD7 is used for testing (PE7, PE8)
 */
void testSteerSensorsWorking( void );


/*************************/
/*** Tests application ***/
/*************************/

/**
 * @brief   Routines of tests
 */
static inline void testsRoutines( void )
{
#if (MAIN_PROGRAM_ROUTINE == PROGRAM_ROUTINE_TEST_BRAKE_SENSOR)

    testBrakeSensorRoutine();

#elif (MAIN_PROGRAM_ROUTINE == PROGRAM_ROUTINE_TEST_BRAKE_UNIT_CS)

    testBrakeUnitCSRoutine();

#elif (MAIN_PROGRAM_ROUTINE == PROGRAM_ROUTINE_TEST_WHEEL_POS_SENSOR)

    testWheelPosSensorRoutine();

#elif (MAIN_PROGRAM_ROUTINE == PROGRAM_ROUTINE_TEST_CLUTCH_LEVER)

    testClutchLeverRoutine();

#elif (MAIN_PROGRAM_ROUTINE == PROGRAM_ROUTINE_TEST_LL_DRIVER)

    testDriverControlRoutine();

#elif (MAIN_PROGRAM_ROUTINE == PROGRAM_ROUTINE_TEST_LL_DRIVER_EXT1)

    testDriverControlRoutineExt1();

#endif
}


#endif /* TESTS_TESTS_H_ */

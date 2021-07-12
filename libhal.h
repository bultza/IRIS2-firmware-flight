/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */


#ifndef __LIBHAL_H__
#define __LIBHAL_H__


//******************************************************************************
//* PUBLIC MACRO FUNCTION DEFINITIONS :                                        *
//******************************************************************************
#define HWREG32(x) (*((volatile uint32_t *)((uint16_t)x)))
#define HWREG16(x) (*((volatile uint16_t *)((uint16_t)x)))
#define HWREG8(x)  (*((volatile uint8_t *)((uint16_t)x)))



#endif  // __LIBHAL_H__

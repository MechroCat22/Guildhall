/************************************************************************/
/* File: GameCommon.hpp
/* Author: Andrew Chase
/* Date: March 24th, 2018
/* Description: Is included in all game .cpp files, to have access to all
				common game elements (renderers, input systems, etc)
/************************************************************************/
#pragma once
#include "Engine/Core/Utility/ErrorWarningAssert.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"


//-----Macros-----
// Macro to make TODO's and UNIMPLEMENTED reminders appear in build output
#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "
#define PRAGMA(p)  __pragma( p )
#define NOTE( x )  //PRAGMA( message(x) )
#define FILE_LINE  NOTE( __FILE__LINE__ )

#define TODO( x )  NOTE( __FILE__LINE__"\n"           \
        " --------------------------------------------------------------------------------------\n" \
        "|  TODO :   " ##x "\n" \
        " --------------------------------------------------------------------------------------\n" )

#define UNIMPLEMENTED()  QUOTE(__FILE__) " (" QUOTE(__LINE__) ")" ; ERROR_AND_DIE("Function unimplemented!") 
#define UNUSED(x) (void)(x);
//-----------------------------------------------------------------------------------------------
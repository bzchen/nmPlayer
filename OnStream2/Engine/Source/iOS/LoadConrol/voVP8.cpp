    /************************************************************************
    *                                                                      *
    *        VisualOn, Inc. Confidential and Proprietary, 2003 -           *
    *                                                                      *
    ************************************************************************/
/*******************************************************************************
    File:        load.cpp
 
    Contains:    sub load control cpp file
 
    Written by:  Jeff
 
    Change History (most recent first):
    2012-8-9        Jeff            Create file
 
 *******************************************************************************/

#include "voLoadLibControl.h"

#include "voVP8.h"

void* voGetVP8DecAPIAdapter()
{
    return (void *)voGetVP8DecAPI;
}

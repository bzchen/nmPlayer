/************************************************************************
VisualOn Proprietary
Copyright (c) 2012, VisualOn Incorporated. All Rights Reserved

VisualOn, Inc., 4675 Stevens Creek Blvd, Santa Clara, CA 95051, USA

All data and information contained in or disclosed by this document are
confidential and proprietary information of VisualOn, and all rights
therein are expressly reserved. By accepting this material, the
recipient agrees that this material and the information contained
therein are held in confidence and in trust. The material may only be
used and/or disclosed as authorized in a license agreement controlling
such use and disclosure.
************************************************************************/
#ifndef __voPackUV_H__
#define __voPackUV_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void voPackUV(unsigned char* dstUV, unsigned char* srcU, unsigned char* srcV, int strideU, int strideV, int rows, int width, int stridedest);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif // _CCRRRENDER_H_

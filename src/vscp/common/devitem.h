// DeviceList.h: interface for the CDeviceList class.
//
// This file is part of the VSCP (http://www.vscp.org) 
//
// The MIT License (MIT)
// 
// Copyright (c) 2000-2018 Ake Hedman, Grodans Paradis AB <info@grodansparadis.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#if !defined(_DEVICELIST_H__0ED35EA7_E9E1_41CD_8A98_5EB3369B3194__INCLUDED_)
#define _DEVICELIST_H__0ED35EA7_E9E1_41CD_8A98_5EB3369B3194__INCLUDED_

#ifdef WIN32
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#endif

#include "vscp.h"

/*!
  Structure for registry and config file
  interaction with device parameters
*/


typedef struct _devItem {
    int id;
    int regid;
    std::string strName;        // Name of device
    std::string strPath;        // Path to device
    std::string strParameters;  // Configuration parameters for device
    std::string strPrefix;      // Level II/III driver prefix for device flags
    uint32_t flags;             // Bit flags for device 
    uint32_t filter;            // Incoming filter for device driver
    uint32_t mask;              // Incoming mask for device driver
    std::string strUsername;    // Level II/III username for device driver
    std::string strPassword;    // Level II/III password for device driver
    short port;                 // Level II/III port for device driver
} devItem;

typedef  devItem  *PDEVITEM;

#endif

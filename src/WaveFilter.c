/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave filtering                                                            */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "WaveFilter.h"
/**************************************************************
WinFilter version 0.8
http://www.winfilter.20m.com
akundert@hotmail.com

Filter type: Band Pass
Filter model: Chebyshev
Filter order: 5
Sampling Frequency: 44 KHz
Fc1 and Fc2 Frequencies: 1.000000 KHz and 3.000000 KHz
Pass band Ripple: 1.000000 dB
Coefficents Quantization: 16-bit

Z domain Zeros
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000

Z domain Poles
z = 0.963273 + j -0.168261
z = 0.963273 + j 0.168261
z = 0.930858 + j -0.231892
z = 0.930858 + j 0.231892
z = 0.899223 + j -0.326391
z = 0.899223 + j 0.326391
z = 0.983451 + j -0.141538
z = 0.983451 + j 0.141538
z = 0.894087 + j -0.404846
z = 0.894087 + j 0.404846
***************************************************************/
#define Ntap 64

#define DCgain 262144

INT32 WFProcessSample(INT32 in_new_sample)
{
	static INT16 FIRCoef[Ntap] = { 
         4354,
         3860,
         2932,
         1679,
          277,
        -1056,
        -2106,
        -2700,
        -2742,
        -2249,
        -1354,
         -296,
          612,
         1039,
          696,
         -600,
        -2875,
        -5968,
        -9529,
        -13051,
        -15939,
        -17592,
        -17507,
        -15366,
        -11115,
        -5000,
         2440,
        10428,
        18039,
        24337,
        28511,
        30054,
        28511,
        24337,
        18039,
        10428,
         2440,
        -5000,
        -11115,
        -15366,
        -17507,
        -17592,
        -15939,
        -13051,
        -9529,
        -5968,
        -2875,
         -600,
          696,
         1039,
          612,
         -296,
        -1354,
        -2249,
        -2742,
        -2700,
        -2106,
        -1056,
          277,
         1679,
         2932,
         3860,
         4354,
         4383
    };

		static INT32 x[Ntap]; //input samples
    INT64 y=0;            //output sample
    int n;

    //shift the old samples
    for(n=Ntap-1; n>0; n--)
       x[n] = x[n-1];

    //Calculate the new output
    x[0] = in_new_sample;
    for(n=0; n<Ntap; n++)
        y += FIRCoef[n] * x[n];
    
    return (INT32)(y / DCgain);
}

#if 0
/**************************************************************
WinFilter version 0.8
http://www.winfilter.20m.com
akundert@hotmail.com

Filter type: Band Pass
Filter model: Chebyshev
Filter order: 6
Sampling Frequency: 44 KHz
Fc1 and Fc2 Frequencies: 1.000000 KHz and 3.000000 KHz
Pass band Ripple: 1.000000 dB
Coefficents Quantization: 16-bit

Z domain Zeros
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000

Z domain Poles
z = 0.950370 + j -0.201596
z = 0.950370 + j 0.201596
z = 0.972695 + j -0.162186
z = 0.972695 + j 0.162186
z = 0.923319 + j -0.272417
z = 0.923319 + j 0.272417
z = 0.901335 + j -0.350820
z = 0.901335 + j 0.350820
z = 0.985084 + j -0.140579
z = 0.985084 + j 0.140579
z = 0.898873 + j -0.407877
z = 0.898873 + j 0.407877
***************************************************************/
#define Ntap 128

#define DCgain 131072

INT16 BandpassFilter(INT16 in_new_sample)
{
    INT16 FIRCoef[Ntap] = { 
         -118,
          -67,
          -71,
         -147,
         -295,
         -503,
         -743,
         -975,
        -1157,
        -1249,
        -1224,
        -1071,
         -802,
         -449,
          -60,
          307,
          600,
          774,
          809,
          710,
          509,
          262,
           38,
          -92,
          -72,
          127,
          505,
         1020,
         1599,
         2146,
         2561,
         2755,
         2671,
         2295,
         1666,
          866,
           18,
         -742,
        -1290,
        -1532,
        -1433,
        -1027,
         -417,
          235,
          738,
          897,
          559,
         -358,
        -1839,
        -3758,
        -5886,
        -7915,
        -9500,
        -10311,
        -10086,
        -8679,
        -6098,
        -2518,
         1735,
         6218,
        10426,
        13863,
        16116,
        16904,
        16116,
        13863,
        10426,
         6218,
         1735,
        -2518,
        -6098,
        -8679,
        -10086,
        -10311,
        -9500,
        -7915,
        -5886,
        -3758,
        -1839,
         -358,
          559,
          897,
          738,
          235,
         -417,
        -1027,
        -1433,
        -1532,
        -1290,
         -742,
           18,
          866,
         1666,
         2295,
         2671,
         2755,
         2561,
         2146,
         1599,
         1020,
          505,
          127,
          -72,
          -92,
           38,
          262,
          509,
          710,
          809,
          774,
          600,
          307,
          -60,
         -449,
         -802,
        -1071,
        -1224,
        -1249,
        -1157,
         -975,
         -743,
         -503,
         -295,
         -147,
          -71,
          -67,
         -118,
         -199
    };

    static INT16 x[Ntap]; //input samples
    __int64 y=0;            //output sample
    int n;

    //shift the old samples
    for(n=Ntap-1; n>0; n--)
       x[n] = x[n-1];

    //Calculate the new output
    x[0] = in_new_sample;
    for(n=0; n<Ntap; n++)
        y += FIRCoef[n] * x[n];
    
    return (INT16)(y / DCgain);
}

#endif

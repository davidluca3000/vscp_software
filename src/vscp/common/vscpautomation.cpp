// vscpautomation.cpp
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version
// 2 of the License, or (at your option) any later version.
// 
// This file is part of the VSCP (http://www.vscp.org) 
//
// Copyright (C) 2000-2014 
// Ake Hedman, Paradise of the Frog, <akhe@paradiseofthefrog.com>
// 
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this file see the file COPYING.  If not, write to
// the Free Software Foundation, 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#ifdef __GNUG__
    //#pragma implementation
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


#include "wx/defs.h"
#include "wx/app.h"
#include <wx/datetime.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>
#include <wx/stdpaths.h>

#include <wx/listimpl.cpp>

#include "vscp.h"
#include "vscphelper.h"


#include "vscpautomation.h"


//-----------------------------------------------------------------------------
//                   Helpers for sunrise/sunset calculations
//-----------------------------------------------------------------------------

// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001

static double pi = 3.14159;
static double degs;
static double rads;

static double L, g, daylen;
static double SunDia = 0.53; // Sunradius degrees

static double AirRefr = 34.0 / 60.0; // athmospheric refraction degrees //

//-----------------------------------------------------------------------------
//                       End of sunset/sunrise functions
//-----------------------------------------------------------------------------



///////////////////////////////////////////////////////////////////////////////
// Constructor
//

CVSCPAutomation::CVSCPAutomation( void )
{
    m_zone = 0;
    m_subzone = 0;

    // Take me the freedom to use my own place as reference
    m_longitude = 61.7441833;
    m_latitude = 15.1604167;
    m_timezone = 1;

    m_bSegmentControllerHeartbeat = true;
    m_intervalSegmentControllerHeartbeat = 60;

    m_bHeartBeatEvent = true;
    m_intervalHeartBeat = 60;

    // Again Europe and Sweden as a reference
    m_daylightsavingtimeStart.ParseDateTime(_("2014-03-30 02:00:00"));
    m_daylightsavingtimeEnd.ParseDateTime(_("2014-10-26 02:00:00"));

    m_bSunRiseEvent = true;
    m_bSunRiseTwilightEvent = true;
    m_bSunSetEvent = true;
    m_bSunSetTwilightEvent = true;

    // Set to some early date 
    m_civilTwilightSunriseTime_sent.ResetTime();
    m_civilTwilightSunriseTime_sent.SetYear( 0 );
    m_SunriseTime_sent.ResetTime();
    m_SunriseTime_sent.SetYear( 0 );
    m_SunsetTime_sent.ResetTime();
    m_SunsetTime_sent.SetYear( 0 );
    m_civilTwilightSunsetTime_sent.ResetTime();
    m_civilTwilightSunsetTime_sent.SetYear( 0 );
    m_noonTime_sent.ResetTime();
    m_noonTime_sent.SetYear( 10 );

    m_lastCalculations = wxDateTime::Now();

}

///////////////////////////////////////////////////////////////////////////////
// Destructor
//

CVSCPAutomation::~CVSCPAutomation( void )
{

}


///////////////////////////////////////////////////////////////////////////////
// FNday
//
// Get the days to J2000
// h is UT in decimal hours
// FNday only works between 1901 to 2099 - see Meeus chapter 7
//

double CVSCPAutomation::FNday(int y, int m, int d, float h)
{
	long int luku = -7 * (y + (m + 9) / 12) / 4 + 275 * m / 9 + d;
	// type casting necessary on PC DOS and TClite to avoid overflow
	luku += (long int) y * 367;
	return(double) luku - 730531.5 + h / 24.0;
};


///////////////////////////////////////////////////////////////////////////////
// FNrange
//
// the function below returns an angle in the range
// 0 to 2*pi
//

double CVSCPAutomation::FNrange(double x)
{
	double b = 0.5 * x / pi;
	double a = 2.0 * pi * (b - (long) (b));
	if (a < 0) a = 2.0 * pi + a;
	return a;
};

///////////////////////////////////////////////////////////////////////////////
// f0
//
// Calculating the hourangle
//

double CVSCPAutomation::f0(double lat, double declin)
{
	double fo, dfo;
	// Correction: different sign at S HS
	dfo = rads * (0.5 * SunDia + AirRefr);
	if (lat < 0.0) dfo = -dfo;
	fo = tan(declin + dfo) * tan(lat * rads);
	if (fo > 0.99999) fo = 1.0; // to avoid overflow //
	fo = asin(fo) + pi / 2.0;
	return fo;
};

///////////////////////////////////////////////////////////////////////////////
// f1
//
// Calculating the hourangle for twilight times
//

double CVSCPAutomation::f1(double lat, double declin)
{
	double fi, df1;
	// Correction: different sign at S HS
	df1 = rads * 6.0;
	if (lat < 0.0) df1 = -df1;
	fi = tan(declin + df1) * tan(lat * rads);
	if (fi > 0.99999) fi = 1.0; // to avoid overflow //
	fi = asin(fi) + pi / 2.0;
	return fi;
};


///////////////////////////////////////////////////////////////////////////////
// FNsun
//
//   Find the ecliptic longitude of the Sun

double CVSCPAutomation::FNsun(double d)
{

	//   mean longitude of the Sun
	L = FNrange(280.461 * rads + .9856474 * rads * d);

	//   mean anomaly of the Sun
	g = FNrange(357.528 * rads + .9856003 * rads * d);

	//   Ecliptic longitude of the Sun
	return FNrange(L + 1.915 * rads * sin(g) + .02 * rads * sin(2 * g));
};


///////////////////////////////////////////////////////////////////////////////
// convert2HourMinute
//
// Display decimal hours in hours and minutes
//

void CVSCPAutomation::convert2HourMinute( double floatTime, int *pHours, int *pMinutes )
{
	*pHours = (int) floatTime;
	*pMinutes = (floatTime - (double) *pHours)*60;
};


///////////////////////////////////////////////////////////////////////////////
// calcSun
//

void CVSCPAutomation::calcSun( void )
{
    double year, month, day, hour; 
	double d, lambda;
	double obliq, alpha, delta, LL, equation, ha, hb, twx;
	double twilightSunraise, maxAltitude, noonTime, sunsetTime, sunriseTime, twilightSunset;
	time_t sekunnit;
	struct tm *p;

	degs = 180.0 / pi;
	rads = pi / 180.0;

	//  get the date and time from the user
	// read system date and extract the year

	// First get time 
	time( &sekunnit );

	// Next get localtime 
	p = localtime(&sekunnit);

	year = p->tm_year;
	year += 1900;
	month = p->tm_mon + 1;

	day = p->tm_mday;

	hour = 12;

	d = FNday(year, month, day, hour);

	//   Use FNsun to find the ecliptic longitude of the
	//   Sun
	lambda = FNsun(d);

	//   Obliquity of the ecliptic
	obliq = 23.439 * rads - .0000004 * rads * d;

	//   Find the RA and DEC of the Sun
	alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
	delta = asin(sin(obliq) * sin(lambda));

	// Find the Equation of Time
	// in minutes
	// Correction suggested by David Smith
	LL = L - alpha;
	if (L < pi) LL += 2.0 * pi;
	equation = 1440.0 * (1.0 - LL / pi / 2.0);
	ha = f0(m_latitude, delta);
	hb = f1(m_latitude, delta);
	twx = hb - ha;          // length of twilight in radians
	twx = 12.0 * twx / pi;  // length of twilight in hours

	// Conversion of angle to hours and minutes
	daylen = degs * ha / 7.5;
	if (daylen < 0.0001) {
		daylen = 0.0;
	}

	// arctic winter     //
	sunriseTime = 12.0 - 12.0 * ha / pi + m_timezone - m_longitude / 15.0 + equation / 60.0;
	sunsetTime = 12.0 + 12.0 * ha / pi + m_timezone - m_longitude / 15.0 + equation / 60.0;
	noonTime = sunriseTime + 12.0 * ha / pi;
	maxAltitude = 90.0 + delta * degs - m_latitude;
	// Correction for S HS suggested by David Smith
	// to express altitude as degrees from the N horizon
	if (m_latitude < delta * degs) maxAltitude = 180.0 - maxAltitude;

	twilightSunraise = sunriseTime - twx; // morning twilight begin
	twilightSunset = sunsetTime + twx; // evening twilight end

	if (sunriseTime > 24.0) sunriseTime -= 24.0;
	if (sunsetTime > 24.0) sunsetTime -= 24.0;

    m_declination = delta * degs;
    m_daylength = daylen;
    m_SunMaxAltitude = maxAltitude;

    int intHour, intMinute;

    // Civil Twilight Sunrise
    convert2HourMinute( twilightSunraise, &intHour, &intMinute );
    m_civilTwilightSunriseTime = wxDateTime::Now();
    m_civilTwilightSunriseTime.ResetTime();     // Set to midnight
    m_civilTwilightSunriseTime.SetHour( intHour );
    m_civilTwilightSunriseTime.SetMinute( intMinute ); 

    // Sunrise
    convert2HourMinute( sunriseTime, &intHour, &intMinute );
    m_SunriseTime = wxDateTime::Now();
    m_SunriseTime.ResetTime();     // Set to midnight
    m_SunriseTime.SetHour( intHour );
    m_SunriseTime.SetMinute( intMinute ); 

    // Sunset
    convert2HourMinute( sunsetTime, &intHour, &intMinute );
    m_SunsetTime = wxDateTime::Now();
    m_SunsetTime.ResetTime();     // Set to midnight
    m_SunsetTime.SetHour( intHour );
    m_SunsetTime.SetMinute( intMinute ); 

    // Civil Twilight Sunset
    convert2HourMinute( twilightSunset, &intHour, &intMinute );
    m_civilTwilightSunsetTime = wxDateTime::Now();
    m_civilTwilightSunsetTime.ResetTime();     // Set to midnight
    m_civilTwilightSunsetTime.SetHour( intHour );
    m_civilTwilightSunsetTime.SetMinute( intMinute );

    // NoonTime
    convert2HourMinute( noonTime, &intHour, &intMinute );
    m_noonTime = wxDateTime::Now();
    m_noonTime.ResetTime();     // Set to midnight
    m_noonTime.SetHour( intHour );
    m_noonTime.SetMinute( intMinute );
}


///////////////////////////////////////////////////////////////////////////////
// doWork
//

void CVSCPAutomation::doWork( void )
{
    wxTimeSpan hourSpan( 1 );  // one hour span

    // Every hour calculate Sunrise/sunset parameters
    // Events are just sent once per 24h/period
    if ( wxDateTime::Now() > m_lastCalculations ) {
        m_lastCalculations.Add( hourSpan ); // nest check is one hour in the future
        calcSun();      
    }


}
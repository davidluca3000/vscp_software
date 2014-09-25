// tables.cpp
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version
// 2 of the License, or (at your option) any later version.
// 
// This file is part of the VSCP (http://www.vscp.org) 
// Copyright (C) 2000-2014 
// Ake Hedman, Grodans Paradis AB, <akhe@grodansparadis.com>
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
//


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

#include <sys/stat.h>

#include "wx/defs.h"
#include "wx/app.h"
#include <wx/datetime.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <wx/listimpl.cpp>

#ifndef WIN32
#include <errno.h>
#endif

#include "vscp.h"
#include "vscphelper.h"
#include "variablecodes.h"
#include "tables.h"


// This list holds the tables for vscpd
WX_DEFINE_LIST( listVSCPTables );


///////////////////////////////////////////////////////////////////////////////
// Constructor
//

// Constructor for nmormal table
CVSCPTable::CVSCPTable( const char *path, int type, uint32_t size )
{
	if ( NULL != path ) m_path = wxString::FromAscii( path );
    
	m_number_of_records = 0;
	m_timestamp_first = 0;
	m_timestamp_last = 0;

	m_vscpFileHead.type = type;
	
	memset( &m_vscpFileHead, 0, sizeof( m_vscpFileHead ) );
	strcpy( m_vscpFileHead.nameTable, "Table" );
	strcpy( m_vscpFileHead.nameXLabel, "X Label" );
	strcpy( m_vscpFileHead.nameYLabel, "Y Label" );

	// Next pos to write at for a new static file
	m_vscpFileHead.posStaticRead = sizeof( m_vscpFileHead );
	m_vscpFileHead.posStaticWrite = sizeof( m_vscpFileHead );
	m_vscpFileHead.staticSize = size;

	m_ft = NULL;

	init();
}


///////////////////////////////////////////////////////////////////////////////
// Destructor
//

CVSCPTable::~CVSCPTable( void )
{
	if ( NULL != m_ft )  fclose( m_ft );
}


///////////////////////////////////////////////////////////////////////////////
// init
//

int CVSCPTable::init() 
{
	int rv;
	struct _vscpFileRecord record;

	// Open/create main file
	if ( fileExists( m_path.mbc_str() ) ) {
		m_ft = fopen( m_path.mbc_str(), "r+b") ;	// binary Read Write
		if ( NULL == m_ft ) return errno;			// Failed to open file
	}
	else {
		// Create file
		m_ft = fopen( m_path.mbc_str(), "w+b") ;	// binary Read Write		
		if ( NULL == m_ft ) return errno;			// Failed to create file
		
		if ( VSCP_TABLE_NORMAL == m_vscpFileHead.type ) {
			m_vscpFileHead.id[0] = 0x55;
			m_vscpFileHead.id[0] = 0xAA;
		}
		else {
			m_vscpFileHead.id[0] = 0xAA;
			m_vscpFileHead.id[0] = 0x55;
		}
		
		// Write header
		if ( sizeof( m_vscpFileHead ) != 
			fwrite( &m_vscpFileHead , 1, sizeof(m_vscpFileHead), m_ft ) ) return errno;

		// If we have a static table we initiate it
		if ( VSCP_TABLE_STATIC == m_vscpFileHead.type ) {
			_vscpFileRecord rec;
			memset( &rec, 0, sizeof(_vscpFileRecord) ); 
			fwrite( &rec, m_vscpFileHead.staticSize, sizeof(_vscpFileRecord), m_ft );
		}
	}

	rv = readMainHeader();

	m_number_of_records = ( fdGetFileSize( m_path.mbc_str() ) - sizeof( m_vscpFileHead ))/sizeof(_vscpFileRecord);
	if ( m_number_of_records ) {

		size_t rpos;
		
		// Go to last pos
		fseek( m_ft, sizeof( m_vscpFileHead ) + (m_number_of_records-1) * sizeof(_vscpFileRecord) , SEEK_SET );							
		rpos = fread( &record, 1, sizeof(record), m_ft );
		m_timestamp_last = record.timestamp;

		// Go to first pos
		fseek( m_ft, sizeof( m_vscpFileHead ), SEEK_SET );							
		rpos = fread( &record, 1, sizeof(record), m_ft );
		m_timestamp_first = record.timestamp;
	}

	return rv; 
}

///////////////////////////////////////////////////////////////////////////////
// setTableInfo
//

int CVSCPTable::setTableInfo( const char *path,
									uint8_t type,
									const char *tableName, 
									const char *tableDescription,
									const char *xAxisLabel, 
									const char *yAxisLabel,
									uint32_t size,
									uint16_t vscp_class, 
									uint16_t vscp_type,
									uint8_t vscp_unit )
{
	int rv;

	m_path = wxString::FromAscii( path );
	m_vscpFileHead.type = type;
	strncpy( m_vscpFileHead.nameTable, tableName, sizeof( m_vscpFileHead.nameTable ) );
	strncpy( m_vscpFileHead.descriptionTable, tableDescription, sizeof( m_vscpFileHead.descriptionTable ) );
	strncpy( m_vscpFileHead.nameXLabel, xAxisLabel, sizeof( m_vscpFileHead.nameXLabel ) );
	strncpy( m_vscpFileHead.nameYLabel, yAxisLabel, sizeof( m_vscpFileHead.nameYLabel) );
	m_vscpFileHead.staticSize = size;
	m_vscpFileHead.vscp_class = vscp_class;
	m_vscpFileHead.vscp_type = vscp_type;
	m_vscpFileHead.vscp_unit = vscp_unit;

	// Write header
	rv = writeMainHeader();		

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
// fileExists
//

int CVSCPTable::fileExists( const char *path )
{
	struct stat buffer;
	return ( stat(path, &buffer) == 0);
}

///////////////////////////////////////////////////////////////////////////////
// fdGetFileSize
//

long CVSCPTable::fdGetFileSize( const char *path )
{
	struct stat st;
	stat( path, &st);
	return st.st_size;
}

///////////////////////////////////////////////////////////////////////////////
// readMainHeader
//

int CVSCPTable::readMainHeader( void )
{
	int rv;
	
	// File must be open
	if ( NULL == m_ft ) return -1;

	rv = fseek( m_ft, 0, SEEK_SET );									// Go to beginning of file
	if ( rv ) return errno;
	rv = fread( &m_vscpFileHead, 1, sizeof(m_vscpFileHead), m_ft );		// Read structure
	if ( rv ) return errno;

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
// writeMainHeader
//

int CVSCPTable::writeMainHeader( void )
{
	int rv;
	
	// File must be open
	if ( NULL == m_ft ) return -1;

	rv = fseek( m_ft, 0, SEEK_SET );									// Go to beginning of file
	if ( rv ) return errno;
	rv = fwrite( &m_vscpFileHead, 1, sizeof(m_vscpFileHead), m_ft );	// Read structure
	if ( rv ) return errno;

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
// logData
//

int CVSCPTable::logData( time_t timestamp, double measurement )
{
	int rv;
	struct _vscpFileRecord record;

	// File must be open
	if ( NULL == m_ft ) return -1;
	
	record.timestamp = timestamp;
	record.measurement = measurement;

	if ( VSCP_TABLE_NORMAL == m_vscpFileHead.type ) {

		// Go to end of main file
		rv = fseek( m_ft, 0, SEEK_END );
		if ( rv ) return errno;

		long pos =  ftell( m_ft );

		// Write record
		if ( sizeof( record ) != 
			fwrite( &record , 1, sizeof(record), m_ft ) ) return errno;

		m_number_of_records++;	// Another record
		if ( 0 == m_timestamp_first ) m_timestamp_first = timestamp; // If first 
		m_timestamp_last = timestamp;

	}
	else if ( VSCP_TABLE_STATIC == m_vscpFileHead.type ) {
		
		fseek( m_ft, m_vscpFileHead.posStaticWrite, SEEK_SET );
		fwrite( &record, 1, sizeof(record), m_ft );
		// Check if we have gone past the end
		if ( ftell( m_ft ) > (long)( m_vscpFileHead.staticSize * sizeof( _vscpFileRecord ) + sizeof( _vscpFileHead ) ) ) {
			m_vscpFileHead.posStaticWrite = sizeof( _vscpFileHead );
		}

	}

	return rv;
}

///////////////////////////////////////////////////////////////////////////////
// GetRangeOfData
//

long CVSCPTable::GetRangeOfData( time_t from, time_t to, void *buf, uint16_t size )
{
	long  returnCount = 0;
	bool bFound = false;
	struct _vscpFileRecord record;
	long startSearchPos;
	long startRecord = 0;
	long endRecord = m_number_of_records-1;
	long midRecord;
	struct _vscpFileRecord *p = (struct _vscpFileRecord *)buf;

	// File must be open
	if ( NULL == m_ft ) return -1;
	
	// Just work for normal files
	if ( VSCP_TABLE_NORMAL != m_vscpFileHead.type ) return 0;

	// Set size to filesize if no buffer is supplied.
	if ( 0 == buf ) size = m_number_of_records * sizeof( struct _vscpFileRecord ) + sizeof(_vscpFileHead);

	// If there is nothing to do - do nothing
	if ( to < from ) return 0;

	if ( m_timestamp_first >= from ) {
		// Set initial searchpos to start.
		startSearchPos = sizeof(_vscpFileHead);
	}
	else {
		while ( endRecord >= startRecord ) {

			midRecord = startRecord + (endRecord - startRecord) / 2;

			// Seek the pos
			startSearchPos = sizeof(_vscpFileHead) + midRecord*sizeof(_vscpFileRecord);
			fseek( m_ft, startSearchPos, SEEK_SET );

			// read record
			if ( sizeof(_vscpFileRecord) != fread( &record, 1, sizeof(_vscpFileRecord), m_ft ) ) {
				// Set initial searchpos to start.
				startSearchPos = sizeof(_vscpFileHead);
				break;
			}

			if ( record.timestamp == from ) {
				bFound = true;
				break;
			}
			// determine which subarray to search
			else if (record.timestamp < from ) {
				// change min index to search upper subarray
				startRecord = midRecord + 1;
			}
			else {        
				// change max index to search lower subarray
				endRecord = midRecord - 1;
			}
		} // while

		if ( bFound ) {
			startSearchPos = sizeof(_vscpFileHead) + midRecord*sizeof(_vscpFileRecord);
		}
		else {
			if ( midRecord ) midRecord--;	// Start search befor current mid
			startSearchPos = sizeof(_vscpFileHead) + midRecord*sizeof(_vscpFileRecord);
		}
	}

	// Position at search pos
	fseek( m_ft, startSearchPos, SEEK_SET );

	while ( size > sizeof(_vscpFileRecord) && !feof(m_ft) ) {
		
		// read record
		if ( sizeof(_vscpFileRecord) != fread( &record, 1, sizeof(_vscpFileRecord), m_ft ) ) {
				break;
		}

		// Break if we are ready
		if ( record.timestamp > to ) break;

		if ( record.timestamp >= from ) {
			if ( NULL != buf ) {
				memcpy( p++, &record, sizeof(_vscpFileRecord) );
				size -= sizeof(_vscpFileRecord);
			}
		
			returnCount++;
		}

	}

	return returnCount;
}


///////////////////////////////////////////////////////////////////////////////
// GetStaticData
//

long CVSCPTable::GetStaticData( void *buf, uint16_t size )
{
	// File must be open
	if ( NULL == m_ft ) return -1;
	
	// Just work for static files
	if ( VSCP_TABLE_STATIC != m_vscpFileHead.type ) return 0;

	// The size of he buf must fit the data
	if ( ( m_vscpFileHead.staticSize * sizeof( _vscpFileRecord ) ) > size ) return 0;

	// read records
	if ( sizeof(_vscpFileRecord) != fread( buf, m_vscpFileHead.staticSize, sizeof(_vscpFileRecord), m_ft ) ) {
		return 0;
	}

	return m_vscpFileHead.staticSize;
}


///////////////////////////////////////////////////////////////////////////////
// GetStaticData
//

long CVSCPTable::GetStaticRequiredBuffSize( void )
{
	return ( m_vscpFileHead.staticSize * sizeof( _vscpFileRecord ) );
}

///////////////////////////////////////////////////////////////////////////////
// getInfo
//

int CVSCPTable::getInfo( struct _vscptableInfo *pInfo, time_t from, time_t to )
{
	bool bFirst =true;
	_vscpFileRecord record;

	if ( NULL == pInfo ) return 0;

	memset( pInfo, 0, sizeof( struct _vscptableInfo ) ); 

	fseek( m_ft, 0, SEEK_SET ); // Go to beginning of file

	while ( !m_ft ) {

		// read record
		if ( sizeof(_vscpFileRecord) != fread( &record, 1, sizeof(_vscpFileRecord), m_ft ) ) {
			return 0;
		}

		if ( bFirst ) {
				// Store reference values on first run.
				pInfo->minValue = record.measurement;
				pInfo->maxValue = record.measurement;
				pInfo->minTime = record.timestamp;
				pInfo->maxTime = record.timestamp;
				bFirst = false;
		}

		if ( 0 == (from+to) || ( (record.timestamp >= from ) && ( record.timestamp <= to ) ) ) {
				pInfo->nRecords++;
				if ( record.measurement < pInfo->minValue ) pInfo->minValue = record.measurement;
				if ( record.measurement > pInfo->maxValue ) pInfo->maxValue = record.measurement;
				if ( record.timestamp < pInfo->minTime ) pInfo->minTime = record.timestamp;
				if ( record.timestamp > pInfo->maxTime ) pInfo->maxTime = record.timestamp;
				pInfo->meanValue = (record.measurement + pInfo->meanValue)/2;
		}

	}

	return pInfo->nRecords;
}
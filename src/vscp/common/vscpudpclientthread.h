// udpclientthread.h
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version
// 2 of the License, or (at your option) any later version.
// 
// This file is part of the VSCP (http://www.vscp.org) 
//
// Copyright (C) 2000-2017 
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


#if !defined(UDPCLIENTTHREAD_H__INCLUDED_)
#define UDPCLIENTTHREAD_H__INCLUDED_


#include "wx/thread.h"
#include "wx/socket.h"

#include "userlist.h"
#include "controlobject.h"

// Client structure (Will receive events from VSCP server)
typedef struct {
    bool                m_bEnable;          // Enable the client
    wxString            m_remoteAddress;    // Address for remote client "UDP://19.168.1.99:1234
    vscpEventFilter     m_filter;           // Filter for outgoing events to this UDP client
    uint8_t             m_nEncryption;      // Encryption algorithm to use for this client
    bool                m_bSetBroadcast;    // Set broadcast flag  MG_F_ENABLE_BROADCAST
    uint8_t             m_index;            // Rolling send index (only low tree bits used)
} udpRemoteClientInfo;

WX_DECLARE_LIST( udpRemoteClientInfo, udpRemoteClientList );

typedef struct  {       
    bool                m_bEnable;          // Enable UDP
    bool                m_bAllowUnsecure;   // Allow un encrypted datagrams
    bool                m_bAck;             // ACK received datagram
    wxString            m_user;             // User account to use for UDP
    wxString            m_password;         // Password for user account
    wxString            m_interface;        // Interface to bind to "UDP://33333"
    cguid               m_guid;             // GUID to use for server client object
    vscpEventFilter     m_filter;           // Filter for incoming events to this UDP client
    udpRemoteClientList m_remotes;          // List containing remote receiving clients
} udpServerInfo;    
    

typedef struct {
    uint8_t             m_index;            // Frame index
    vscpEvent           *m_pEvent;          // Event to send
    uint32_t            m_lastSent;         // Last sent
    uint8_t             m_nRetry;           // Retry counter
} eventResendItem;

WX_DECLARE_LIST( eventResendItem, udpResendItemList );


/*!
    This class implement the listen thread for
    the vscpd connections on the UDP interface
*/

class VSCPUDPClientThread : public wxThread
{

public:
    
    /// Constructor
    VSCPUDPClientThread();

    /// Destructor
    ~VSCPUDPClientThread();

    /*!
        Thread code entry point
    */
    virtual void *Entry();

    /*!
     *  Connection handler
     * 
     * @param nc Mongoose connection handle
     * @param ev Mongoose event.
     * @param p Pointer to user data.
    */
    static void ev_handler( struct mg_connection *nc, int ev, void *p );

    
    /*!
     * Receive UDP frame
     * 
     * @param nc Mongoose connection handle
     * @param pClientItem Client item for this user. Normally "USP"
     * @param pRxFilter Pointer to receive filter. Can be NULL to accept 
     *          all events.
     * @param index Running packet index.-
     * @return True on success, false on failure.
     */
    static bool receiveFrame( struct mg_connection *nc, 
                                            CClientItem *pClientItem,
                                            vscpEventFilter *pRxFilter );
    
    /*!
     *  Send ACk reply
     * 
     *  @pram nc Pointer to connection
     *  @param pkttype Packet type
     *  @param index Running index 0-7
     */
    static bool replyAckFrame( struct mg_connection *nc, 
                                    uint8_t pkttype,
                                    uint8_t index );
    
    /*!
     *  Send NACk reply
     * 
     *  @pram nc Pointer to connection
     *  @param pkttype Packet type
     *  @param index Running index 0-7
     */
    static bool replyNackFrame( struct mg_connection *nc, 
                                    uint8_t pkttype,
                                    uint8_t index );
    
    /*!
     * Send frame to all clients
     * 
     * @param pmgt Pointer to communication structure
     * @param pClientItem Pointer to client
     * @return True on success false on failure
     */
    bool sendFrame( struct mg_mgr *pmgr, 
                        CClientItem *pClientItem );

    
    /*! 
        called when the thread exits - whether it terminates normally or is
        stopped with Delete() (but not when it is Kill()ed!)
    */
    virtual void OnExit();


// --- Member variables ---

    /*!
        Termination control
    */
    bool m_bQuit;

private:

    // UDP Client item
    CClientItem *m_pClientItem;
    

};




/*!
    This class implement a worker thread that
    logs UDP received data.
*/

class UDPWorkerThread : public wxThread
{

public:
    
    /// Constructor
    UDPWorkerThread();

    /// Destructor
    ~UDPWorkerThread();

    /*!
        Thread code entry point
    */
    virtual void *Entry();

    /*! 
        called when the thread exits - whether it terminates normally or is
        stopped with Delete() (but not when it is Kill()ed!)
    */
    virtual void OnExit();


// --- Member variables ---

    /*!
        Termination control
    */
    bool m_bQuit;

    

};




#endif





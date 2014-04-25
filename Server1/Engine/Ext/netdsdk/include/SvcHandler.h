//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-03-28 10:40
 */
//========================================================================

#ifndef _SVCHANDLER_H_
#define _SVCHANDLER_H_
#include "Pre.h"

#include "Reactor.h"
#include "SockStream.h"
#include "EventHandler.h"

/**
 * @class SvcHandler
 *
 * @brief 
 */
class SvcHandler : public EventHandler
{
public:
    /**
     * The <r> reactor is passed to EventHandler
     */
    SvcHandler (Reactor *r = Reactor::instance ());

    //
    virtual ~SvcHandler ();

    // Activate the client handler, this is typically 
    // called by Acceptor or Connector
    // The <arg> default is its generator which is 
    // Acceptor or Connector
    virtual int open (void *arg = 0);

    /**
     * Object terminate hook -- application-specific cleanup
     * code goes here. Default action is call handle_close
     * This method maybe called by Acceptor when accept an 
     * connection failed
     */
    virtual int close ();

    /**
     * Perform termination activites on the SVC_HANDLER.
     * The default behavior is to close down the <peer_>
     * (to avoid descriptor leaks) and to <destroy> this 
     * object (to avoid memory leaks)! If you don't want
     * this behavior make sure you override this method.
     */
    virtual int handle_close (NDK_HANDLE = NDK_INVALID_HANDLE,
	    ReactorMask = EventHandler::ALL_EVENTS_MASK);

    // Returns the underlying SockStream.  Used by Accpeotr
    // or Connector.
    SockStream &peer () const;

    // Get the underlying handle associated with the <peer_>.
    virtual NDK_HANDLE handle () const;

    // Set the underlying handle associated with the <peer_>.
    virtual void handle (NDK_HANDLE);
    /**
     * Call this to free up dynamically allocated <SvcHandler>
     * (otherwise you will get memory leaks). In general, you 
     * should call this method rather than <delete> since 
     * this method knows whether or not the object was allocated
     * dynamically, and can act accordingly (i.e., deleting it if 
     * it was allocated dynamically).
     */
    virtual void destroy (void);

    // Close down the descriptor and unregister from the Reactor
    void shutdown (void);
protected:
    // Maintain connection with client
    SockStream peer_;

    // Keeps track of whether we are in the process of closing 
    // (required to avoid circular calls to <handle_close>).
    int closed_;
private:
};

#include "Post.h"
#endif


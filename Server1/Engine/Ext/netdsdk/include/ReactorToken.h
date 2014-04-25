//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-02-15 15:41
 */
//========================================================================

#ifndef _REACTORGUARD_H_
#define _REACTORGUARD_H_
#include "Pre.h"

#include "Trace.h"
#include "Debug.h"
#include "Token.h"

class ReactorImpl;
/**
 * @class ReactorToken
 *
 * @brief 
 */
class ReactorToken : public Token
{
public:
    ReactorToken (ReactorImpl *r,
	    int s_queue = Token::FIFO);
    virtual ~ReactorToken ();

    // Get the reacotr implementation
    ReactorImpl *reactor (void);

    // Set the reacotr implementation
    void reactor (ReactorImpl *);   

    // 
    virtual void sleep_hook (void);
private:
    // Reactor implement
    ReactorImpl *reactor_;
};

#include "ReactorToken.inl"
#include "Post.h"
#endif


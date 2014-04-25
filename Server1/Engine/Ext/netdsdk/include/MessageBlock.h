//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-08 11:54
 */
//========================================================================

#ifndef _MESSAGEBLOCK_H_
#define _MESSAGEBLOCK_H_
#include "Pre.h"

#include <malloc.h>

/**
 * @class MessageBlock
 *
 * @brief Message payload.
 */
class MessageBlock
{
public:
    enum
    {
	/// = Data and proto
	// Undifferentiated data message
	MB_DATA     = 0x01,

	/// = Control messages
	// Restart transmission after stop
	MB_START    = 0x80,

	// Stop transmission immediately
	MB_STOP     = 0x81,

	// Fatal error used to set u.u_error
	MB_ERROR    = 0x82,

	/// = Message class masks
	// Normal priority message mask
	MB_NORMAL   = 0x00,

	// High priority control message mask
	MB_PRIORITY = 0x80,

	// User-defined message mask
	MB_USER     = 0x200,
	MB_USER1    = 0x201,
	MB_USER2    = 0x202
    };
public:
    MessageBlock (const size_t size);
    MessageBlock (const char *pdata, const size_t size);
    ~MessageBlock ();

    // get message data pointer
    char * base (void);

    // get message data len
    size_t size (void);

    // get messageblock type
    size_t data_type (void);

    // set messageblock type
    void data_type (size_t type);

    // pointer to the MessageBlock directly ahead in the MessageQueue.
    // get link to next mb,
    MessageBlock *next (void) const;

    // set link to next mb,
    void next (MessageBlock *mb);

    // pointer to the MessageBlock directly behind in the MessageQueue.
    // get link to prev mb,
    MessageBlock *prev (void) const;

    // set link to prev mb,
    void prev (MessageBlock *mb);

    // release message data which alloc by MessageBlock 
    void release (void);

    // 
    void clean (void);
protected:
private:
    // point to beginning of message payload.
    char *	    mb_base_;
    int             release_base_;
    size_t	    mb_size_;
    size_t	    mb_type_;
    MessageBlock *  prev_;
    MessageBlock *  next_;
};

#include "MessageBlock.inl"
#include "Post.h"
#endif


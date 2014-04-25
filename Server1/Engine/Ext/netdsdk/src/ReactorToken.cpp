#include "ReactorToken.h"
#include "ReactorImpl.h"

void ReactorToken::sleep_hook ()
{
    TRACE ("ReactorToken");
    if (this->reactor_->notify () != 0)
	NDK_DBG ("ReactorToken::sleep_hook failed");
}

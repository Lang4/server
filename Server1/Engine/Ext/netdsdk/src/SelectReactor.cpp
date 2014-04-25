#include "SelectReactor.h"
#include "TimerQueue.h"
#include "Guard_T.h"
#include "Reactor.h"
#include "Common.h"
#include "Debug.h"
#include "Trace.h"
#include "NDK.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

SelectReactor::SelectReactor ()
{

}
SelectReactor::~SelectReactor ()
{

}

thread_t SelectReactor::owner ()
{
    Guard_T<SelectReactorToken> g (this->token_);
    return this->owner_;
}
void SelectReactor::owner (thread_t thr_id)
{
    Guard_T<SelectReactorToken> g (this->token_);
    this->owner_ = thr_id;
}



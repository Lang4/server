#pragma once

namespace mynet{
	class EventBase;
	/**
	 * Target	 */
	class Target{
	public:
		virtual unsigned long getHandle() = 0;
		virtual unsigned long getPeerHandle() {return 0;}
		EventBase *inEvt;
		EventBase *outEvt;
		Target()
		{
			inEvt = outEvt = NULL;
		}
		virtual void destroy()
		{
			if (inEvt)
				delete inEvt;
			if (outEvt)
				delete outEvt;
			inEvt = NULL;
			outEvt = NULL;
		}
		virtual void doSend(EventBase *evt,bool over = true){};
	};
};
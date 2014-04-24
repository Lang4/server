#pragma once
#include "Entry.h"
#include <set>
namespace mymap{
	class Block{
	public:
		void addEntry(Entry * entry);
		void removeEntry(Entry *entry);
		void execAll(stExecEntry *exec);
		bool hadEntry(Entry *entry);
		unsigned short getPositionX();
		unsigned short getPositionY();
	private:
		std::set<Entry*> _entries;
		typedef std::set<Entry*>::iterator ENTRIES_ITER;
	};
	class stExecBlock{
	public:
		virtual void exec(Block *block) = 0;
	};
};
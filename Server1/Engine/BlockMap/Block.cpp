#include "Entry.h"
#include "Block.h"

namespace mymap{
	
	void Block::addEntry(Entry * entry)
	{
		_entries.insert(entry);
	}
	void Block::removeEntry(Entry *entry)
	{
		ENTRIES_ITER iter = _entries.find(entry);
		if (iter != _entries.end())
		{
			 _entries.erase(iter);
		}
	}
	void Block::execAll(stExecEntry *exec)
	{
		for (ENTRIES_ITER iter = _entries.begin();iter != _entries.end();++iter)
		{
			if (*iter)
				exec->exec(*iter);
		}
	}
	bool Block::hadEntry(Entry *entry)
	{
		ENTRIES_ITER iter = _entries.find(entry);
		if (iter != _entries.end())
		{
			return true;
		}
		return false;
	}
};
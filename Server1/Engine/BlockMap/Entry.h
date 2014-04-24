#pragma once
namespace mymap{
	class Block;
	class Entry{
	public:
		virtual void otherEnter(Entry*other) = 0;
		virtual void otherLeave(Entry *other) = 0;
		virtual void setPosition(unsigned short x,unsigned short y) = 0;
		Block *block;
	};
	class Position{
	public:
		unsigned short x;
		unsigned short y;
		Position()
		{
			x = y = 0;
		}
	};
	class stExecEntry{
	public:
		virtual void exec(Entry *entry) = 0;
	};

};
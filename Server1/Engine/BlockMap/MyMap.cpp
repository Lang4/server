#include "Entry.h"
#include "MyMap.h"

namespace mymap{
	void ViewCell::setTag(char tag,void *content)
	{
		_tag = tag;_content = content;
	}
	bool ViewCell::checkTag(char tag)
	{
		return tag == _tag;
	}
	void ViewCell::addTag(char tag,void *content)
	{
		_tag += tag;
		_content = content;
	}
	/**
	 * 执行为tag的节点
	 */
	void ViewRect::exec(char tag,stExecBlock *exec)
	{
		for (CELLS_ITER iter = _cells.begin(); iter != _cells.end();++iter)
		{
			for (CELL_ITER pos = iter->begin(); pos != iter->end();++pos)
			{
				if (pos->checkTag(tag))
				{
					exec->exec(pos->get<Block>());
				}
			}
		}
	}
	/**
	 * 设置大小
	 */
	void ViewRect::setSize(unsigned char width,unsigned char height)
	{
		_cells.resize(width*2 -1);
		for (CELLS_ITER iter = _cells.begin(); iter != _cells.end();++iter)
		{
			iter->resize(height * 2 -1);
		}
		this->_viewWidth = width;
		this->_viewHeight = height;
	}
	
	/**
	 * 叠加位置的标志
	 */
	bool ViewRect::addTag(unsigned short x,unsigned short y,char tag,void *entry)
	{
		unsigned short centerx = _cells.size() /2 - _viewWidth/2;
		unsigned short ox = x - center.x + centerx;
		for (unsigned short startx = ox;ox < ox + _viewWidth;startx++)
		{
			unsigned short oy = _cells[startx].size() /2 - _viewHeight/2;
			for (unsigned short starty = oy; starty < oy + _viewHeight;starty++)
			{
				_cells[startx][starty].addTag(tag,entry);
			}
		}
		return true;
	}
	/**
	 * 清除Tag
	 */
	void ViewRect::clear()
	{
		for (CELLS_ITER iter = _cells.begin(); iter != _cells.end();++iter)
		{
			for (CELL_ITER pos = iter->begin(); pos != iter->end();++pos)
			{
				pos->setTag(ViewCell::_NULL,NULL);
			}
		}
	}

	/**
	 * 设置中心点 并重新设置位置
	 */
	void ViewRect::setCenter(unsigned short x,unsigned short y,char tag,void *entry)
	{
		clear();
		center.x = x;
		center.y = y;
		unsigned short ox = _cells.size() /2 - _viewWidth/2;
		for (unsigned short startx = ox; startx < ox + _viewWidth;startx++)
		{
			unsigned short oy = _cells[startx].size() /2 - _viewHeight/2;
			for (unsigned short starty = oy; starty < oy + _viewHeight;starty++)
			{
				_cells[startx][starty].setTag(tag,entry);
			}
		}
	}
	struct stEnterCallback:stExecEntry{
		void exec(Entry *entry)
		{
			if (entry != owner)
				entry->otherEnter(owner);
		}
		Entry *owner;
		stEnterCallback()
		{
			owner = NULL;
		}
	};
	struct stLeaveCallback:stExecEntry{
		void exec(Entry *entry)
		{
			if (entry != owner)
				entry->otherEnter(owner);
		}
		Entry *owner;
		stLeaveCallback()
		{
			owner = NULL;
		}
	};
	struct stDelCallback:stExecBlock{
	public:
		void exec(Block *block)
		{
			block->execAll(&callback);
		}
		stDelCallback(Entry *o)
		{
			callback.owner = o;
		}
		stLeaveCallback callback;
	};
	struct stAddCallback:stExecBlock{
	public:
		void exec(Block *block)
		{
			block->execAll(&callback);
		}
		stAddCallback(Entry *o)
		{
			callback.owner = o;
		}
		stEnterCallback callback;
	};
	void MyMap::updateEntry(Entry *entry,unsigned short x,unsigned short y)
	{
		Block *block = _getBlockByEntryPosition(x,y);
		stAddCallback enter(entry);
		stDelCallback leave(entry);
		if (entry->block)
		{
			_view.setCenter(entry->block->getPositionX(),entry->block->getPositionY(),ViewCell::_OLD,entry->block);
			entry->setPosition(x,y);
		}
		if (block != entry->block) // 新的块更新
		{
			entry->block->removeEntry(entry);
			entry->block = block;
			entry->block->addEntry(entry);
			if (false == _view.addTag(entry->block->getPositionX(),entry->block->getPositionY(),ViewCell::_NEW,block)) // 不在该块内
			{
				_view.exec(ViewCell::_OLD,&leave);

				// 新的视图
				_view.setCenter(entry->block->getPositionX(),entry->block->getPositionY(),ViewCell::_NEW,block);

				_view.exec(ViewCell::_NEW,&enter);
			}
			else // 在块内
			{
				_view.exec(ViewCell::_OLD,&leave);
				_view.exec(ViewCell::_NEW,&enter);
			}
		}
	}

	void MyMap::addEntry(Entry *entry,unsigned short x,unsigned short y)
	{
		if (entry->block) return;
		Block *block = _getBlockByEntryPosition(x,y);
		if (block)
		{
			stEnterCallback callback;
			callback.owner = entry;
			entry->block = block;
			entry->block->addEntry(entry);
			entry->block->execAll(&callback);
		}
	}
	void MyMap::removeEntry(Entry *entry)
	{
		stLeaveCallback callback;
		callback.owner = entry;
		if (entry && entry->block)
		{
			entry->block->removeEntry(entry);
			entry->block->execAll(&callback);
			entry->block = NULL;
		}
	}
	unsigned int MyMap::_getBlockIDByEntryPosition(unsigned short x,unsigned short y)
	{
		return ( x / _blockWidth ) * (_width / _blockWidth ) + ( y / _blockWidth );
	}
	Block * MyMap::_getBlockByEntryPosition(unsigned short x,unsigned short y)
	{
		unsigned int index = _getBlockIDByEntryPosition(x,y);
		if (index < _blocks.size())
		{
			return _blocks.at(index);
		}
		return NULL;
	}
};
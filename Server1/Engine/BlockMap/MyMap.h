#pragma once
#include "Entry.h"
#include "Block.h"
#include <vector>
namespace mymap{
	class ViewCell{
	public:
		void setTag(char tag,void *content);
		bool checkTag(char tag);
		void addTag(char tag,void *content);
		ViewCell()
		{
			_tag = 0;
			_content = 0;
		}
		enum{
			_NULL = 0,
			_OLD = 1,
			_NEW = 2,
		};
		template<typename T>
		T * get()
		{
			return static_cast<T*>(_content);
		}
	private:
		char _tag;
		void* _content;
	};
	class ViewRect{
	public:
		/**
		 * 执行为tag的节点
		 */
		void exec(char tag,stExecBlock *exec);
		/**
		 * 设置大小
		 */
		void setSize(unsigned char width,unsigned char height);
		
		/**
		 * 叠加位置的标志
		 */
		bool addTag(unsigned short x,unsigned short y,char tag,void *entry);
		/**
		 * 清除Tag
		 */
		void clear();

		/**
		 * 设置中心点 并重新设置位置
		 */
		void setCenter(unsigned short x,unsigned short y,char tag,void *entry);
	private:
		std::vector<std::vector<ViewCell> > _cells;
		typedef std::vector<std::vector<ViewCell> >::iterator CELLS_ITER;
		typedef std::vector<ViewCell>::iterator CELL_ITER;
		Position center;
		unsigned short _viewWidth;
		unsigned short _viewHeight;
	};
	class MyMap{
	public:
		void updateEntry(Entry *entry,unsigned short x,unsigned short y);
		void addEntry(Entry *entry,unsigned short x,unsigned short y);
		void removeEntry(Entry *entry);
	private:
		ViewRect _view;
		std::vector<Block*> _blocks;
		typedef std::vector<Block*>::iterator BLOCKS_ITER;
		unsigned short _width;
		unsigned short _height;
		unsigned short _blockWidth;
		unsigned short _blockHeight;
		unsigned int _getBlockIDByEntryPosition(unsigned short x,unsigned short y);
		Block * _getBlockByEntryPosition(unsigned short x,unsigned short y);
	};
};
#pragma once
#include <vector>
#include <list>
#include <map>
#include <set>
/**
 * 模板元真的狠强大 我只是个未入门的 就简单实用并做了协议的参数化
 */
namespace msg{
	typedef unsigned short BodySize_t;
	struct Stream{
		virtual bool append(const void *content,unsigned int size) = 0;
		virtual bool pick(void *content,unsigned int size) = 0;
		virtual void reset() = 0;
		virtual ~Stream(){}
	};
	struct BinaryStream:public Stream{
		virtual void set(void *content,int size)
		{
			_offset = 0;
			if (!size) return;
			contents.resize(size);
			memcpy(&contents[_offset],content,size);
		}
		BinaryStream(){
			_offset = 0;
			_size = 0;
		}
		virtual ~BinaryStream(){}
		virtual bool append(const void *content,unsigned int size)
		{
			if (size == 0) return false;
			if (contents.size() - _offset < size)
			{
				contents.resize(_offset + size);
			}
			memcpy(&contents[_offset],content,size);
			_offset += size;
			_size = _offset;
			return true;
		}
		virtual bool pick(void *content,unsigned int size)
		{
			if (contents.size() - _offset < size)
			{
				return false;
			}
			memcpy(content,&contents[_offset],size);
			_offset += size;
			return true;
		}
		virtual void reset(){_offset = 0;}
		std::vector<unsigned char> contents;
		virtual unsigned int size(){return _size;}
		virtual void * content(){return &contents[0];}
	private:
		int _offset;
		int _size;
	};
	const int GET = 0;
	const int ADD = 1;
	const int CLEAR = 2;
#define MESSA__DEC_OBJECT_RECORD(container,insert_func)\
	template<typename T>\
	void addRecord(typename container<T> &objects,Stream *ss)\
	{\
		int size = objects.size();\
		ss->append(&size,sizeof(int));\
		for (typename container<T>::iterator iter = objects.begin(); iter != objects.end();++iter)\
		{\
			iter->__to_msg__record__(ADD,ss);\
		}\
	}\
	template<typename T>\
	void getRecord(typename container<T> &objects,Stream *ss)\
	{\
		int size;\
		if (ss->pick(&size,sizeof(int)))\
		{\
			while ( size-- > 0)\
			{\
				T value;\
				value.__to_msg__record__(GET,ss);\
				objects.insert_func(value);\
			}\
		}	\
	}\
	template<typename T>\
	void clearRecord(typename container<T> &objects)\
	{\
		objects.clear();\
	}\

#define MESSA__DEC_TYPE_RECORD(container,insert_func,type)\
	void addRecord(container<type> &objects,Stream *ss)\
	{\
		int size = objects.size();\
		ss->append(&size,sizeof(int));\
		for (container<type>::iterator iter = objects.begin(); iter != objects.end();++iter)\
		{\
			type value = *iter;\
			addRecord(value,ss);\
		}\
	}\
	void getRecord(container<type> &objects,Stream *ss)\
	{\
		int size;\
		if (ss->pick(&size,sizeof(int)))\
		{\
			while ( size-- > 0)\
			{\
				type value;\
				getRecord(value,ss);\
				objects.insert_func(value);\
			}\
		}	\
	}\
	void clearRecord(container<type> &objects)\
	{\
		objects.clear();\
	}
#define MESSA__DEC_TYPE(__type__)\
	void addRecord(__type__& value,Stream *ss) {ss->append(&value,sizeof(__type__));}\
	void getRecord(__type__ &value,Stream *ss) {ss->pick(&value,sizeof(__type__));}\
	void clearRecord(__type__ &value) {value =0;}
	class Object{
	public:
		MESSA__DEC_OBJECT_RECORD(std::vector,push_back);
		MESSA__DEC_OBJECT_RECORD(std::list,push_back);
		MESSA__DEC_OBJECT_RECORD(std::set,insert);
		MESSA__DEC_TYPE_RECORD(std::vector,push_back,int);
		MESSA__DEC_TYPE_RECORD(std::list,push_back,int);
		MESSA__DEC_TYPE_RECORD(std::set,insert,int);
		//MESSA__DEC_TYPE_RECORD(std::vector,push_back,char);
		MESSA__DEC_TYPE(int);
		MESSA__DEC_TYPE(float);
		MESSA__DEC_TYPE(char);
		MESSA__DEC_TYPE(unsigned int);
		MESSA__DEC_TYPE(unsigned char);
		template <class T>
		void addRecord(T& value,Stream *ss)
		{
			value.__to_msg__record__(ADD,ss);
		}
		template<class T>
		void getRecord(T &value,Stream *ss)
		{
			value.__to_msg__record__(GET,ss);
		}

		template<class T>
		void clearRecord(T &value)
		{
		}

		void addRecord(std::string &value,Stream* ss)
		{
			BodySize_t size = value.size();
			ss->append(&size,sizeof(BodySize_t));
			ss->append((void*)value.c_str(),size);
		}
		void addRecord(const void *value,int size,Stream *ss)
		{
			ss->append((void*)value,size);
		}
		void getRecord(void *value,int size,Stream *ss)
		{
			ss->pick(value,size);
		}
		
		void clearRecord(void * value,int size)
		{
			memset(value,0,size);
		}
		void addRecord(const std::basic_string<char> &value,Stream *ss)
		{
			BodySize_t size = value.size();
			ss->append(&size,sizeof(BodySize_t));
			ss->append((void*)value.c_str(),size);
		}
		void getRecord(std::basic_string<char> &value,Stream *ss)
		{
			BodySize_t size = 0;
			if (ss->pick(&size,sizeof(BodySize_t)))
			{
				if (size > 0)
				{
					char *str = new char [size +1];
					memset(str,0,size+1);
					if (ss->pick(str,size))
						value = str;
					delete[] str;
				}
			}
		}
		void addRecord(std::vector<char> &value,Stream *ss)
		{
			BodySize_t size = value.size();
			ss->append(&size,sizeof(BodySize_t));
			ss->append((void*)&value[0],value.size());
		}
		void getRecord(std::vector<char> &value,Stream *ss)
		{
			BodySize_t size = 0;
			if (ss->pick(&size,sizeof(BodySize_t)))
			{
				if (size > 0)
				{
					value.resize(size);
					ss->pick(&value[0],size);
				}
			}
		}

		void clearRecord(std::string &value)
		{
			value = "";
		}
		void addRecord(std::vector<std::string> &value,Stream *ss)
		{
			BodySize_t size = value.size();
			ss->append(&size,sizeof(BodySize_t));
			for (std::vector<std::string>::iterator iter = value.begin(); iter != value.end();++iter)
			{
				addRecord(*iter,ss);
			}
		}
		void getRecord(std::vector<std::string> &value,Stream *ss)
		{
			BodySize_t size = 0;
			if (ss->pick(&size,sizeof(BodySize_t)))
			{
				while (size -- >0)
				{
					std::string str;
					getRecord(str,ss);
					value.push_back(str);
				}
			}	
		}
		void clearRecord(std::vector<std::string> &value)
		{
			value.clear();
		}

		template<class KEY,class VALUE>
		void addRecord(typename std::map<KEY,VALUE> &value,Stream *ss)
		{
			BodySize_t size = value.size();
			ss->append(&size,sizeof(BodySize_t));
			for (typename std::map<KEY,VALUE>::iterator iter = value.begin(); iter != value.end();++iter)
			{
				addRecord(iter->first,ss);
				addRecord(iter->second,ss);
			}
		}
		template<class KEY,class VALUE>
		void getRecord(typename std::map<KEY,VALUE> &values,Stream *ss)
		{
			BodySize_t size = 0;
			if (ss->pick(&size,sizeof(BodySize_t)))
			{
				while (size -- >0)
				{
					KEY key;
					VALUE value;
					getRecord(key,ss);
					getRecord(value,ss);	
					values[key] = value;
				}
			}	
		}
		template<class KEY,class VALUE>
		void clearRecord(typename std::map<KEY,VALUE> &values)
		{
			values.clear();
		}

	public:
		template<typename STREAM>
		STREAM toStream()
		{
			STREAM ss;
			__to_msg__record__(ADD,ss);
			ss.reset();
			return ss;
		}
		void fromStream(void * content,int size)
		{
			if (size == 0) return;
			BinaryStream ss;
			ss.set(content,size);
			fromStream(ss);
		}
		template<typename STREAM>
		void fromStream(STREAM& ss)
		{
			ss.reset();
			__to_msg__record__(GET,&ss);
			ss.reset();
		}
		void clearContent()
		{
		
		}
		virtual ~Object(){}
	public:
		virtual void __to_msg__record__(unsigned char tag,Stream* ss)
		{
		};
	};
	template<typename A>
	struct is_ref { static const bool value = false; };
	
	template<typename A>
	struct is_ref<A&> { static const bool value = true; };

	template<typename A>
	struct ischars { static const bool value = false; };
	template<>
	struct ischars<const char*> { static const bool value = true; };

	template<typename A>
	struct charstostring{
		typedef A type;
	};
	template<>
	struct charstostring<const char*> { typedef std::string type; };

	template<typename A>
	struct ispointer { static const bool value = false; };
	template<typename A>
	struct ispointer<A*> { static const bool value = true; };

	template<typename A>
	struct istype{static const bool value = false;};
	template<>
	struct istype<int>{static const bool value = true;};
	template<>
	struct istype<float>{static const bool value = true;};
	template<>
	struct istype<long>{static const bool value = true;};
	template<>
	struct istype<double>{static const bool value = true;};
	template<>
	struct istype<unsigned int>{static const bool value = true;};
	template<>
	struct istype<unsigned long>{static const bool value = true;};
	template<>
	struct istype<char>{static const bool value = true;};
	template<>
	struct istype<short>{static const bool value = true;};
	template<>
	struct istype<unsigned char>{static const bool value = true;};
	template<>
	struct istype<unsigned short>{static const bool value = true;};

	template<typename A>
	struct isObject{
		static const bool value = false;
	};
	template<>
	struct isObject<Object>{
		static const bool value = true;
	};
	
	template<typename A>
	struct isstring{
		static const bool value = false;
	};
	template<>
	struct isstring<std::string>{
		static const bool value = true;
	};

	template<bool C, typename A, typename B> struct if_ {};
	template<typename A, typename B>		struct if_<true, A, B> { typedef A type; };
	template<typename A, typename B>		struct if_<false, A, B> { typedef B type; };

	template<typename A>
	struct remove_const { typedef  A type; };
	template<typename A>
	struct remove_const<const A> { typedef  A type; };

	template<typename A>
	struct remove_ref { typedef  A type; };
	template<typename A>
	struct remove_ref< A & > { typedef  A type; };
	
	template<typename A>
	struct remove_pointer { typedef A type; };
	template<typename A>
	struct remove_pointer< A * > { typedef  A type; };

	template<typename TYPE>
	struct base_type{
		static bool toStream(TYPE &type,Stream *ss)
		{
			ss->append(&type,sizeof(TYPE));
			return true;
		}
		static bool parseStream(TYPE &type,Stream *ss)
		{
			ss->pick(&type,sizeof(TYPE));
			return true;
		}
	};
	template<typename PACK>
	struct pack_type{
		static bool toStream(PACK* & type,Stream *ss)
		{
			ss->append(type,sizeof(PACK));
			return true;
		}
		static bool parseStream(PACK* & type,Stream *ss)
		{
			ss->pick(type,sizeof(PACK));
			return true;
		}
	};
	template<typename OBJECT>
	struct object_type{
		static bool toStream(OBJECT &object,Stream *ss)
		{
			object.__to_msg__record__(ADD,ss);
			return true;
		}
		static bool parseStream(OBJECT &type,Stream *ss)
		{
			type.__to_msg__record__(GET,ss);
			return true;
		}
	};
	template<typename OBJECT>
	struct string_type{
		static bool toStream(OBJECT &value,Stream *ss)
		{
			BodySize_t size = value.size();
			ss->append(&size,sizeof(BodySize_t));
			ss->append((void*)value.c_str(),size);
			return true;
		}
		static bool parseStream(OBJECT &value,Stream *ss)
		{
			BodySize_t size = 0;
			if (ss->pick(&size,sizeof(BodySize_t)))
			{
				if (size > 0)
				{
					char *str = new char [size +1];
					memset(str,0,size+1);
					if (ss->pick(str,size))
						value = str;
					delete[] str;
				}
				return true;
			}
			return false;
		}
	};
	template<typename OBJECT>
	bool toStream(OBJECT &object,Stream *ss)
	{
		if_<isstring<OBJECT>::value,
			string_type<OBJECT>,
			typename if_<istype<OBJECT>::value,
			base_type<OBJECT>,
			typename if_<isObject<OBJECT>::value,
				object_type<OBJECT>,
					typename if_<ispointer<OBJECT>::value,
					pack_type<typename remove_pointer<OBJECT>::type>,
					base_type<OBJECT> > ::type
				>::type 
			>::type>::type::toStream(object,ss);
		return true;
	}
	template<typename OBJECT>
	bool parseStream(OBJECT &object,Stream *ss)
	{
		if_<isstring<OBJECT>::value,
			string_type<OBJECT>,
			typename if_<istype<OBJECT>::value,
			base_type<OBJECT>,
			typename if_<isObject<OBJECT>::value,
				object_type<OBJECT>,
					typename if_<ispointer<OBJECT>::value,
					pack_type<typename remove_pointer<OBJECT>::type>,base_type<OBJECT> > ::type
				>::type 
			>::type>::type::parseStream(object,ss);
		return true;
	}

	/**
	 * 可使用模板进行内存使用上的优化
	 */
	struct function{
		virtual void call(const char *content,unsigned int len){};
		virtual void call(void *object,const char *content,unsigned int len){};
		virtual ~function(){}
		function()
		{
		}
	};

#define PASRE_ARG(T,obj) \
	typename remove_pointer<typename remove_const<typename remove_ref<typename charstostring<T>::type>::type>::type >::type obj;	\
	parseStream(obj,&ss);

#define BEGIN_PARSE \
	BinaryStream ss;\
	ss.set((void*)content,len);\

#define GF_CALL (*func)
#define OF_CALL (((T*)object)->*func)

#define BEGIN_STREAM \
	BinaryStream ss;\

#define TO_STREAM(arg1) \
	toStream(arg1,&ss);

#define END_STREAM\
	std::string temp;\
	if (ss.size()){\
	temp.resize(ss.size());\
	memcpy(&temp[0],ss.content(),ss.size());}\
	return temp;
	
	template<typename RVAL,typename T1=void,typename T2=void,typename T3=void,typename T4=void,typename T5=void,typename T6=void>
	struct message:public function{
		typedef RVAL (*FUNC)(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5,T6 arg6);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			PASRE_ARG(T4,t4);
			PASRE_ARG(T5,t5);
			PASRE_ARG(T6,t6);

			GF_CALL(t1,t2,t3,t4,t5,t6);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5,T6 arg6)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);
			TO_STREAM(arg4);
			TO_STREAM(arg5);
			TO_STREAM(arg6);
			END_STREAM
		}
	};
	template<typename RVAL,typename T,typename T1=void,typename T2=void,typename T3=void,typename T4=void,typename T5=void,typename T6=void>
	struct omessage:public function{
		typedef RVAL (T::*FUNC)(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5,T6 arg6);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(void *object,const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			PASRE_ARG(T4,t4);
			PASRE_ARG(T5,t5);
			PASRE_ARG(T6,t6);
			OF_CALL (t1,t2,t3,t4,t5,t6);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5,T6 arg6)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);
			TO_STREAM(arg4);
			TO_STREAM(arg5);
			TO_STREAM(arg6);
			END_STREAM
		}
	};

	template<typename RVAL,typename T1,typename T2,typename T3,typename T4,typename T5>
	struct message<RVAL,T1,T2,T3,T4,T5>:public function{
		typedef RVAL (*FUNC)(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			PASRE_ARG(T4,t4);
			PASRE_ARG(T5,t5);

			GF_CALL(t1,t2,t3,t4,t5);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);
			TO_STREAM(arg4);
			TO_STREAM(arg5);
			END_STREAM
		}
	};

	template<typename RVAL,typename T,typename T1,typename T2,typename T3,typename T4,typename T5>
	struct omessage<RVAL,T,T1,T2,T3,T4,T5>:public function{
		typedef RVAL (T::*FUNC)(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(void *object,const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			PASRE_ARG(T4,t4);
			PASRE_ARG(T5,t5);
			OF_CALL (t1,t2,t3,t4,t5);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);
			TO_STREAM(arg4);
			TO_STREAM(arg5);
			END_STREAM
		}
	};

	template<typename RVAL,typename T1,typename T2,typename T3,typename T4>
	struct message<RVAL,T1,T2,T3,T4>:public function{
		typedef RVAL (*FUNC)(T1 arg1,T2 arg2,T3 arg3,T4 arg4);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			PASRE_ARG(T4,t4);
			GF_CALL(t1,t2,t3,t4);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3,T4 arg4)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);
			TO_STREAM(arg4);

			END_STREAM
		}
	};

	template<typename RVAL,typename T,typename T1,typename T2,typename T3,typename T4>
	struct omessage<RVAL,T,T1,T2,T3,T4>:public function{
		typedef RVAL (T::*FUNC)(T1 arg1,T2 arg2,T3 arg3,T4 arg4);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(void *object,const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			PASRE_ARG(T4,t4);
			OF_CALL (t1,t2,t3,t4);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3,T4 arg4)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);
			TO_STREAM(arg4);
			END_STREAM
		}
	};
	
	template<typename RVAL,typename T1,typename T2,typename T3>
	struct message<RVAL,T1,T2,T3>:public function{
		typedef RVAL (*FUNC)(T1 arg1,T2 arg2,T3 arg3);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			GF_CALL(t1,t2,t3);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);

			END_STREAM
		}
	};

	template<typename RVAL,typename T,typename T1,typename T2,typename T3>
	struct omessage<RVAL,T,T1,T2,T3>:public function{
		typedef RVAL (T::*FUNC)(T1 arg1,T2 arg2,T3 arg3);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(void *object,const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			PASRE_ARG(T3,t3);
			OF_CALL (t1,t2,t3);
		}
		std::string build(T1 arg1,T2 arg2,T3 arg3)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			TO_STREAM(arg3);
			END_STREAM
		}
	};

	template<typename RVAL,typename T1,typename T2>
	struct message<RVAL,T1,T2>:public function{
		typedef RVAL (*FUNC)(T1 arg1,T2 arg2);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			GF_CALL(t1,t2);
		}
		std::string build(T1 arg1,T2 arg2)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			END_STREAM
		}
	};

	template<typename RVAL,typename T,typename T1,typename T2>
	struct omessage<RVAL,T,T1,T2>:public function{
		typedef RVAL (T::*FUNC)(T1 arg1,T2 arg2);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(void *object,const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			PASRE_ARG(T2,t2);
			OF_CALL (t1,t2);
		}
		std::string build(T1 arg1,T2 arg2)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			TO_STREAM(arg2);
			END_STREAM
		}
	};

	template<typename RVAL,typename T1>
	struct message<RVAL,T1>:public function{
		typedef RVAL (*FUNC)(T1 arg1);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			GF_CALL(t1);
		}
		std::string build(T1 arg1)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			END_STREAM
		}
	};
	template<typename RVAL,typename T,typename T1>
	struct omessage<RVAL,T,T1>:public function{
		typedef RVAL (T::*FUNC)(T1 arg1);
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(void *object,const char *content,unsigned int len)
		{
			BEGIN_PARSE
			PASRE_ARG(T1,t1);
			OF_CALL(t1);
		}
		std::string build(T1 arg1)
		{
			BEGIN_STREAM
			TO_STREAM(arg1);
			END_STREAM
		}
	};
	
	template<typename RVAL>
	struct message<RVAL>:public function{
		typedef RVAL (*FUNC)();
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(const char *content,unsigned int len)
		{
			BEGIN_PARSE
			GF_CALL();
		}
		std::string build()
		{
			BEGIN_STREAM
			END_STREAM
		}
	};
	template<typename RVAL,typename T>
	struct omessage<RVAL,T>:public function{
		typedef RVAL (T::*FUNC)();
		FUNC func;
		void bind(FUNC func)
		{
			this->func = func;
		}
		void call(void *object,const char *content,unsigned int len)
		{
			BEGIN_PARSE
			OF_CALL();
		}
		std::string build()
		{
			BEGIN_STREAM
			END_STREAM
		}
	};
	
	template<typename RVAL>
	function* bind(RVAL (*func)())
	{
		message<RVAL> *msg = new message<RVAL>();
		msg->bind(func);
		return msg;
	}
	template<typename RVAL,typename T>
	function* bind(RVAL (T::*func)())
	{
		omessage<RVAL,T> *mes = new omessage<RVAL,T>();
		mes->bind(func);
		return mes;
	}
	
	// 一个参数
	template<typename RVAL,typename T1>
	function* bind(RVAL (*func)(T1 arg1))
	{
		message<RVAL,typename charstostring<T1>::type> *msg = new message<RVAL,typename charstostring<T1>::type>();
		msg->bind(func);
		return msg;
	}
	template<typename RVAL,typename T,typename T1>
	function* bind(RVAL (T::*func)(T1 arg1))
	{
		omessage<RVAL,T,T1> *mes = new omessage<RVAL,T,T1>();
		mes->bind(func);
		return mes;
	}

	// 两个参数
	template<typename RVAL,typename T1,typename T2>
	function* bind(RVAL (*func)(T1 arg1,T2 arg2))
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type> *msg = new message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type>();
		msg->bind(func);
		return msg;
	}
	template<typename RVAL,typename T,typename T1,typename T2>
	function* bind(RVAL (T::*func)(T1 arg1,T2 arg2))
	{
		omessage<RVAL,T,T1,T2> *message = new omessage<RVAL,T,T1,T2>();
		message->bind(func);
		return message;
	}

	// 三个参数
	template<typename RVAL,typename T1,typename T2,typename T3>
	function* bind( RVAL (*func)(T1 arg1,T2 arg2,T3 arg3))
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type> *msg =
			new message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type>();
		msg->bind(func);
		return msg;
	}
	template<typename RVAL,typename T,typename T1,typename T2,typename T3>
	function* bind( RVAL (T::*func)(T1 arg1,T2 arg2,T3 arg3))
	{
		omessage<RVAL,T,T1,T2,T3> *message = new omessage<RVAL,T,T1,T2,T3>();
		message->bind(func);
		return message;
	}

	// 四个参数
	template<typename RVAL,typename T1,typename T2,typename T3,typename T4>
	function* bind( RVAL (*func)(T1 arg1,T2 arg2,T3 arg3,T4 arg4))
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type,typename charstostring<T4>::type> *msg =
			new message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type,typename charstostring<T4>::type>();
		msg->bind(func);
		return msg;
	}
	template<typename RVAL,typename T,typename T1,typename T2,typename T3,typename T4>
	function* bind(RVAL (T::*func)(T1 arg1,T2 arg2,T3 arg3,T4 arg4))
	{
		omessage<RVAL,T,T1,T2,T3,T4> *message = new omessage<RVAL,T,T1,T2,T3,T4>();
		message->bind(func);
		return message;
	}

	// 五个参数
	template<typename RVAL,typename T1,typename T2,typename T3,typename T4,typename T5>
	function* bind(RVAL (*func)(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5))
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type,typename charstostring<T4>::type,typename charstostring<T5>::type> *msg =
			new message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type,typename charstostring<T4>::type,typename charstostring<T5>::type>();
		msg->bind(func);
		return msg;
	}
	template<typename RVAL,typename T,typename T1,typename T2,typename T3,typename T4,typename T5>
	function* bind( RVAL (T::*func)(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5))
	{
		omessage<RVAL,T,T1,T2,T3,T4,T5> *message = new omessage<RVAL,T,T1,T2,T3,T4,T5>();
		message->bind(func);	
		return msg;
	}
	template<typename RVAL>
	std::string build()
	{
		message<RVAL> msg;
		return msg.build();
	}
	
	template<typename RVAL,typename T1>
	std::string build(T1 t1)
	{
		message<RVAL,typename charstostring<T1>::type> msg;
		return msg.build(t1);
	}
	template<typename RVAL,typename T1,typename T2>
	std::string build(T1 t1,T2 t2)
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type> msg;
		return msg.build(t1,t2);
	}
	template<typename RVAL,typename T1,typename T2,typename T3>
	std::string build(T1 t1,T2 t2,T3 t3)
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type> msg;
		return msg.build(t1,t2,t3);
	}
	template<typename RVAL,typename T1,typename T2,typename T3,typename T4>
	std::string build(T1 t1,T2 t2,T3 t3,T4 t4)
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type,typename charstostring<T4>::type> msg;
		return msg.build(t1,t2,t3,t4);
	}
	template<typename RVAL,typename T1,typename T2,typename T3,typename T4,typename T5>
	std::string build(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5)
	{
		message<RVAL,typename charstostring<T1>::type,typename charstostring<T2>::type,typename charstostring<T3>::type,typename charstostring<T4>::type,typename charstostring<T5>::type> msg;
		return msg.build(t1,t2,t3,t4,t5);
	}

	template<typename RVAL>
	void call(function* func)
	{
		std::string info = build<RVAL>();
		func->call(info.c_str(),info.size());
	}
	template<typename RVAL,typename T1>
	void call(function* func,T1 t1)
	{
		std::string info = build<RVAL>(t1);
		func->call(info.c_str(),info.size());
	}
	template<typename RVAL,typename T1,typename T2>
	void call(function* func,T1 t1,T2 t2)
	{
		std::string info = build<RVAL>(t1,t2);
		func->call(info.c_str(),info.size());
	}
	template<typename RVAL,typename T1,typename T2,typename T3>
	void call(function* func,T1 t1,T2 t2,T3 t3)
	{
		std::string info = build<RVAL>(t1,t2,t3);
		func->call(info.c_str(),info.size());
	}
	template<typename RVAL,typename T1,typename T2,typename T3,typename T4>
	void call(function* func,T1 t1,T2 t2,T3 t3,T4 t4)
	{
		std::string info = build<RVAL>(t1,t2,t3,t4);
		func->call(info.c_str(),info.size());
	}
	template<typename RVAL,typename T1,typename T2,typename T3,typename T4,typename T5>
	void call(function* func,T1 t1,T2 t2,T3 t3,T4 t4,T5 t5)
	{
		std::string info = build<RVAL>(t1,t2,t3,t4,t5);
		func->call(info.c_str(),info.size());
	}


	template<typename RVAL,typename OBJECT>
	void call(OBJECT *o,function* func)
	{
		std::string info = build<RVAL>();
		func->call(o,info.c_str(),info.size());
	}
	template<typename RVAL,typename OBJECT,typename T1>
	void call(OBJECT *o,function* func,T1 t1)
	{
		std::string info = build<RVAL>(t1);
		func->call(o,info.c_str(),info.size());
	}
	template<typename RVAL,typename OBJECT,typename T1,typename T2>
	void call(OBJECT *o,function* func,T1 t1,T2 t2)
	{
		std::string info = build<RVAL>(t1,t2);
		func->call(o,info.c_str(),info.size());
	}
	template<typename RVAL,typename OBJECT,typename T1,typename T2,typename T3>
	void call(OBJECT *o,function* func,T1 t1,T2 t2,T3 t3)
	{
		std::string info = build<RVAL>(t1,t2,t3);
		func->call(o,info.c_str(),info.size());
	}
	template<typename RVAL,typename OBJECT,typename T1,typename T2,typename T3,typename T4>
	void call(OBJECT *o,function* func,T1 t1,T2 t2,T3 t3,T4 t4)
	{
		std::string info = build<RVAL>(t1,t2,t3,t4);
		func->call(o,info.c_str(),info.size());
	}
	template<typename RVAL,typename OBJECT,typename T1,typename T2,typename T3,typename T4,typename T5>
	void call(OBJECT *o,function* func,T1 t1,T2 t2,T3 t3,T4 t4,T5 t5)
	{
		std::string info = build<RVAL>(t1,t2,t3,t4,t5);
		func->call(o,info.c_str(),info.size());
	}
}

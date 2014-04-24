#include "cJson.h"
#include "stdio.h"
#include "stdlib.h"
#include <string>
class JSONObject{
public:
	JSONObject(const char *text)
	{
		root =cJSON_Parse(text);
		clearTag = true;
	}
	JSONObject()
	{
		clearTag = true;
		root=cJSON_CreateObject();
	}
	JSONObject(cJSON *temp):root(temp)
	{
		clearTag = false;
	}
	void add(const char *name,JSONObject *object)
	{
		object->clearTag = false;
		cJSON_AddItemToObject(root, name,object->root);
	}
	void add(const char *name,const char *value)
	{
		cJSON_AddStringToObject(root,name,value);
	}
	void add(const char *name,int value)
	{
		cJSON_AddNumberToObject(root,name,value);
	}
	~JSONObject()
	{
		if (clearTag)
		cJSON_Delete(root);
	}
	std::string get()
	{
		char *out=cJSON_Print(root);	
		std::string tempstr = out;
		free(out);
		return tempstr;
	}
	std::string get(const char *name)
	{
		cJSON * temp = cJSON_GetObjectItem(root,name);
		if (temp)
		{
			char *out=cJSON_Print(temp);	
			std::string tempstr = out;
			free(out);
			return tempstr;
		}
		return "";
	}
	JSONObject getChild(const char *name)
	{
		JSONObject object(cJSON_GetObjectItem(root,name));
		return object;
	}
private:
	bool clearTag;
	cJSON *root;
};
int main()
{
cJSON *root,*fmt,*img,*thm,*fld;char *out;int i;	/* declare a few. */

/* Here we construct some JSON standards, from the JSON site. */

/* Our "Video" datatype: */
root=cJSON_CreateObject();	
cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
cJSON_AddItemToObject(root, "format", fmt=cJSON_CreateObject());
cJSON_AddStringToObject(fmt,"type",		"rect");
cJSON_AddNumberToObject(fmt,"width",		1920);
cJSON_AddNumberToObject(fmt,"height",		1080);
cJSON_AddFalseToObject (fmt,"interlace");
cJSON_AddNumberToObject(fmt,"frame rate",	24);

out=cJSON_Print(root);	
cJSON_Delete(root);
printf("%s\n",out);
free(out);	
printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx1\n");
	JSONObject object;
	object.add("name","Jack (\"Bee\") Nimble");

	JSONObject format;
	format.add("type",1000);
	object.add("format",&format);

	printf("%s",object.get().c_str());

printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx2\n");
	printf("name:%s",object.get("name").c_str());
	format = object.getChild("format");
	printf("type:%s",format.get("type").c_str()); // 获取子值
}

/**
 * for lua
 * json = createJson("") 
 * type = json:getInt("type") // 获取数字	
 * json.add("name",base:getValue(""); // 构建json
 *
 * theNet:send(1,josn.str()); // 发送到网络
 **/
/**
 * C++ 的快捷构建 将直接转为json 的字符串 或者C++的数据结构[C++ 系列]
 * struct Object:public JSONObject{
 *		std::string str()
 *		{
 *			
 *		}
 *		void parse(const  char *str)
 *		{
 *		
 * 		}
 *		int a;
 *		std::vector<int> as;
 *		std::map<int,int> amaps; 
 * }
 * // 在脚本中不得不手工解析
 */
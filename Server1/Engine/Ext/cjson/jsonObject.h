#include "cJSON.h"
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
	bool isValid(){return root;}
	bool clearTag;
	cJSON *root;
};


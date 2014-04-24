#include "Code.h"

#include "Lang.h"

Code * Code::clone()
{
	Code * code = new Code();
	*code = *this;
	return code;
}
bool Code::execFile(const char *fileName,Args *args)
{
	CodeNode *code = readFromFile(fileName);
	if (code)
	{
		args->codeEnv = this;
		code->exec(args);
		delete code;
		destroy();
		return true;
	}
	destroy();
	return false;
}

int Code::exec(Args *args)
{
	args->codeEnv = this;
	if (code)
		return code->exec(args);
	return CodeState::NO;
}

void Code::destroy()
{
	for (ACTIONS_ITER iter = actions.begin(); iter != actions.end();++iter)
	{
		if (iter->second) delete iter->second;
	}
	actions.clear();
	for (ACTIONS_ITER iter = innerVars.begin(); iter != innerVars.end();++iter)
	{
		if (iter->second) delete iter->second;
	}
	innerVars.clear();
	for (CONDITIONS_ITER iter = conditions.begin();iter != conditions.end();++iter)
	{
		if (iter->second) delete iter->second;
	}
	conditions.clear();

}


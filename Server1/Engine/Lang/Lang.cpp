#include "Lang.h"

int Args::exec()
{
	if (!nowNode) return CodeState::NO; 
	return nowNode->execChild(this);
}


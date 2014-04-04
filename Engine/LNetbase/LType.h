/**
 * \file
 * \version  $Id: LType.h  $
 * \author  
 * \date 
 * \brief 定义基本类型
 *
 * 
 */
#pragma once

#include <time.h>
#include <strings.h>
#include <string.h>
#include <set>
#include "EncDec.h"
#include <stdint.h>

//#define _DEBUG_LOG

//#ifdef _DEBUG_LOG
#define SAFE_DELETE(x) { if (x) { delete (x); (x) = NULL; } }
#define SAFE_DELETE_VEC(x) { if (x) { delete [] (x); (x) = NULL; } }
#define NEW_CHECK(x) { if (x) { }}
//#else
//#define SAFE_DELETE(x) { if (x) { delete (x); (x) = NULL; } }
//#define SAFE_DELETE_VEC(x) { if (x) { delete [] (x); (x) = NULL; } }
//#define NEW_CHECK(x) {} 
//#endif

/**
 * \brief 单字节无符号整数
 *
 */
typedef uint8_t BYTE;

/**
 * \brief 双字节无符号整数
 *
 */
typedef uint16_t WORD;

/**
 * \brief 双字节符号整数
 *
 */
typedef int16_t SWORD;

/**
 * \brief 四字节无符号整数
 *
 */
typedef uint32_t DWORD;

/**
 * \brief 四字节符号整数
 *
 */
typedef int32_t SDWORD;

/**
 * \brief 八字节无符号整数
 *
 */
typedef uint64_t QWORD;

/**
 * \brief 八字节符号整数
 *
 */
typedef int64_t SQWORD;

enum {
	state_none		=	0,							/**< 空的状态 */
	state_maintain	=	1 << 0,						/**< 维护中，暂时不允许建立新的连接 */
};

#pragma pack()


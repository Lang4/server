/**
 * \file
 * \version  $Id: LType.h  $
 * \author  
 * \date 
 * \brief �����������
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
 * \brief ���ֽ��޷�������
 *
 */
typedef uint8_t BYTE;

/**
 * \brief ˫�ֽ��޷�������
 *
 */
typedef uint16_t WORD;

/**
 * \brief ˫�ֽڷ�������
 *
 */
typedef int16_t SWORD;

/**
 * \brief ���ֽ��޷�������
 *
 */
typedef uint32_t DWORD;

/**
 * \brief ���ֽڷ�������
 *
 */
typedef int32_t SDWORD;

/**
 * \brief ���ֽ��޷�������
 *
 */
typedef uint64_t QWORD;

/**
 * \brief ���ֽڷ�������
 *
 */
typedef int64_t SQWORD;

enum {
	state_none		=	0,							/**< �յ�״̬ */
	state_maintain	=	1 << 0,						/**< ά���У���ʱ���������µ����� */
};

#pragma pack()


#pragma once
#include <assert.h>
#include <string.h>

/******************************************************************************
 FixArray
******************************************************************************/

#define ct_FixArray(type, capacity) \
	struct { \
		size_t n; \
		type a[capacity]; \
	}

#define ct_fixArrayInit(array) \
	((array)->n = 0)

#define ct_fixArrayPushBack(array, instance) \
	do { \
		assert((array)->n <= sizeof((array)->a)/sizeof((array)->a[0])); \
		memcpy(&(array)->a[(array)->n], instance, sizeof((array)->a[0])); \
		(array)->n++; \
	} while(0)

#define ct_fixArrayPushBackRaw(array) \
	do { \
		assert((array)->n <= sizeof((array)->a)/sizeof((array)->a[0])); \
		(array)->n++; \
	} while(0);

#define ct_fixArrayPopBack(array) \
	do { \
		assert((array)->n > 0); \
		(array)->n--; \
	} while(0)

#define ct_fixArrayUnorderedErase(array, iterator) \
	do { \
		size_t index = ((iterator) - (array)->a)/sizeof((array)->a[0]); \
		assert(index < (array)->n); \
		(array)->n--; \
		memcpy((array)->a[index], (array)->a[(array)->n], sizeof((array)->a[0])); \
	} while(0)

#define ct_fixArrayLast(array) \
	(&((array)->a[(array)->n - 1]))

#define ct_fixArrayForEach(array, iterator) \
	for((iterator) = (array)->a; (iterator) != &(array)->a[(array)->n]; ++(iterator))

#define ct_fixArraySize(array) \
	((array)->n)


/******************************************************************************
 FixStack
******************************************************************************/

#define ct_FixStack(type, capacity)		ct_FixArray(type, capacity)
#define ct_fixStackInit(stack)			ct_fixArrayInit(stack)
#define ct_fixStackPush(stack, instance)	ct_fixArrayPushBack(stack, instance)
#define ct_fixStackPop(stack)			ct_fixArrayPopBack(stack)
#define ct_fixStackTop(stack)			(&((stack)->a[(stack)->n - 1]))


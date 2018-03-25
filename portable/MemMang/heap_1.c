/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */


/*
 * The simplest possible implementation of pvPortMalloc().  Note that this
 * implementation does NOT allow allocated memory to be freed again.
 *
 * See heap_2.c, heap_3.c and heap_4.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif
/*
Heap1
��������Щһ�������������ź����Ͷ��о���Ҳ����ɾ����Ӧ��
ʵ���ϴ������FreeRTOSӦ�ö��������ġ�
���п�ȷ����(ִ�������ѵ�ʱ����������һ����)�����Ҳ��ᵼ���ڴ���Ƭ��
xPortGetFreeHeapSize�����õ�ʣ�������
����ʵ�ֺ��ڴ������̶��ǳ��򵥣��ڴ��Ǵ�һ����̬�����з��䵽��
Ҳ�����ʺ�����Щ����Ҫ��̬�ڴ�����Ӧ��
*/
/* A few bytes might be lost to byte aligning the heap start address. */
#define configADJUSTED_HEAP_SIZE	( configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT )

/* Allocate the memory for the heap. */
/* Allocate the memory for the heap. */
#if( configAPPLICATION_ALLOCATED_HEAP == 1 )
	/* The application writer has already defined the array used for the RTOS
	heap - probably so it can be placed in a special segment or address. */
	extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
	static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */
//����Ҫ RAM ��ʱ��ʹ�һ��������ucHeap(�ڴ��)�з�һС�����������(�ڴ��)������ΪconfigTOTAL_HEAP_SIZE
/* Index into the ucHeap array. */
static size_t xNextFreeByte = ( size_t ) 0;

/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
void *pvReturn = NULL;
static uint8_t *pucAlignedHeap = NULL;

	/* Ensure that blocks are always aligned to the required number of bytes. */
	#if( portBYTE_ALIGNMENT != 1 )//�Ƿ���Ҫ�ڴ����
	{
		if( xWantedSize & portBYTE_ALIGNMENT_MASK )//�ж���Ҫ�Ĵ�С�ǲ���8�ֽڵı���
		{
			/* Byte alignment required. */
			//���ǵĻ�,������Ҫ�����ڴ�Ĵ�СΪ8�ı���
			xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
		}
	}
	#endif

	vTaskSuspendAll();//�����ڴ�ʱ��Ҫ����,�ص�������
	{
		if( pucAlignedHeap == NULL )
		{
			//��Ϊ�����������ucHeap�ĵ�ַ��һ����8�ֽڶ����,����Ҫ������8�ֽڶ���Ĵ���
			/* Ensure the heap starts on a correctly aligned boundary. */
			pucAlignedHeap = ( uint8_t * ) ( ( ( portPOINTER_SIZE_TYPE ) &ucHeap[ portBYTE_ALIGNMENT ] ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );
		}

		/* Check there is enough room left for the allocation. */
		//����ǲ������㹻���ڴ����
		//�ѷ�����ڴ��ַxNextFreeByte<��Ҫ���ڴ��ַ<�����ڴ��ַ
		if( ( ( xNextFreeByte + xWantedSize ) < configADJUSTED_HEAP_SIZE ) &&
			( ( xNextFreeByte + xWantedSize ) > xNextFreeByte )	)/* Check for overflow. */
		{
			/* Return the next free byte then increment the index past this
			block. */
			//����ɹ��Ļ�,���뵽���ڴ��׵�ַ���Ƹ�����ֵ����
			pvReturn = pucAlignedHeap + xNextFreeByte;
			xNextFreeByte += xWantedSize;
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
	( void ) xTaskResumeAll();//�ָ�������

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )//�ڴ����ʧ�ܹ��Ӻ���
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
	}
	#endif

	return pvReturn;//�������뵽�ĵ�ַ
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
	//��û���ͷŵĹ���
	/* Memory cannot be freed using this scheme.  See heap_2.c, heap_3.c and
	heap_4.c for alternative implementations, and the memory management pages of
	http://www.FreeRTOS.org for more information. */
	( void ) pv;

	/* Force an assert as it is invalid to call this function. */
	configASSERT( pv == NULL );
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* Only required when static memory is not cleared. */
	xNextFreeByte = ( size_t ) 0;
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return ( configADJUSTED_HEAP_SIZE - xNextFreeByte );
}




/*-----------------------------------------------------------------------------
   Copyright (c) 2000  Microsoft Corporation

Module:
  exts.c

Sample old windbg style interface using extension 
  ----------------------------------------------------------------------------
  �� �����Ͽ� ����.

  �������Ͽ��� ���Ͽ���Ʈ�� �ش��ϴ� ������ ������ �ִ� �ͽ��ټ� �ۼ�!
  
  ����� ������ ���Ϸ� ���µǾ� ������ ������ �����ϴ� ���ϸ� ������.
  ���������� �̹��� ������ �о�� �ϴµ� ���� �������� ��. -_-;;;

  Written by GreeMate 2008/01/22 
------------------------------------------------------------------------------*/

#include "myext.h"

//
// ntddk.h �� include ���� �ʱ� ������ �ʿ��� Ÿ�Ը� ������ nttype.h �� �����.
// �⺻������ dll �̱� ������ windows.h �� include �ϴµ� ntddk.h �� �г��� ���� ����.
// ���� ����ִ°� �������Ƿ� nttype.h �� ����� ������.
//
#include "nttype.h"

//
// C ��Ÿ�� ���� �Լ��� ����ϱ� ���Ͽ� stdio.h �� include �Ѵ�.
//
#include <stdio.h>


//
// ���� ������
//
#define SIZE_1MB	0x100000		// ���� VACB ���� ���� 1MB
#define SIZE_32MB	0x2000000		// ���� VACB ���� ���� 32MB

#define SIZE_256KB	0x40000			// VACB �� Cache View ũ��
#define SIZE_4KB	0x1000			// x86 PAGE ũ��
#define SIZE_PAGE	SIZE_4KB		// x86 PAGE ũ��

#define PAGE_NUM	(SIZE_256KB / SIZE_PAGE)	// Cache View �ϳ��� ���Ե� ������ ����

#define MAX_VACB	128				// ���� VACB ������ �ִ� ����


//
// ���� ����
//

// 0 ���� �ʱ�ȭ�� 4KB ¥�� ���۸� �غ��Ѵ�.
char	g_ZeroData[SIZE_PAGE] = {0,};



//
// �Լ� ����
//

void FillZeroData(FILE *fp)
/**
	�ϳ��� Cache View ������ ���Ͽ� 0 ���� ä���.

	@param[in]	fp		����������

	@retval     ����

	@remarks

	@see	
*/
{
	ULONG i;
	
#if DBG
    dprintf("FillZeroData 256KB\n");
#else
	dprintf("x");		// �����͸� ������ ���ߴٴ� ǥ�ø� �Ѵ�.
#endif

	//
	// PAGE_NUM ��ŭ ������ ���鼭 1 �������� 0 ���� ä���.
	//
	for (i = 0; i < PAGE_NUM; i++)
	{
		if (fwrite( g_ZeroData, 1, SIZE_PAGE, fp ) != SIZE_PAGE)
		{
			dprintf( "FillZeroData-fwrite Error\n" );
			return; 	
		}	
	}
}

void CopyVacbData(PVACB pVacb, FILE *fp)
/**
	VACB �� ����Ű�� Cache View 256KB �� ���Ͽ� ����.

	@param[in]	pVacb	VACB �ּ�
	@param[in]	fp		����������

	@retval     ����

	@remarks

	@see	
*/
{
	VACB 	stVacb;
	ULONG	cb;
	PCHAR	pData;
	ULONG 	i, j;
	char	buf[SIZE_PAGE];
	
	//
	// pVacb �ּ��� ������ stVacb �� �о�´�.
	//
	if (ReadMemory( (ULONG64)pVacb, &stVacb, sizeof(VACB), &cb ) == 0)
	{
		// �����ϸ� �� VACB �� ����Ű�� ������ ��� 0 ���� ���Ͽ� ���� �����Ѵ�.
#if DBG
	    dprintf("CopyVacbData-pVacb reading error.\n");
#endif
		FillZeroData( fp );
	    return;
	}
		
	//
	// PAGE_NUM ��ŭ ������ ���鼭 Cache View 256KB �� ���Ͽ� ����.
	//
	for (i = 0; i < PAGE_NUM; i++)
	{
		// PAGE ũ�⸸ŭ ������Ű�鼭 ������ ������ �ּҸ� ���Ѵ�.
		pData = (PCHAR)((ULONG64)stVacb.BaseAddress + (i * SIZE_PAGE));

		// �ּҿ��� �����͸� PAGE ������ �д´�.
		if (ReadMemory( (ULONG64)pData, buf, SIZE_PAGE, &cb ) != 0)
		{
#if DBG
			for (j = 0; j < cb; j=j+16)
			{
				dprintf( "%08x  %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\n", 
						pData + j,
						buf[j], buf[j+1], buf[j+2], buf[j+3], buf[j+4], buf[j+5], buf[j+6], buf[j+7], buf[j+8], buf[j+9], buf[j+10], buf[j+11], buf[j+12], buf[j+13], buf[j+14], buf[j+15] );
			}
#endif
			// �����ϸ� ���Ͽ� ����.
			if (fwrite( buf, 1, SIZE_PAGE, fp ) != SIZE_PAGE)
			{
				dprintf( "CopyVacbData-fwrite Error\n" );
				return; 	
			}
		}
		else
		{
#if DBG
			dprintf( "%08x  4KB Data Page Out\n", pData );
#endif
			// �����ϸ� 0 ���� ä���� 4KB �����͸� ���Ͽ� ����.
			if (fwrite( g_ZeroData, 1, SIZE_PAGE, fp ) != SIZE_PAGE)
			{
				dprintf( "CopyVacbData-fwrite ZeroData Error\n" );
				return; 	
			}	
		}
	}
}

void ReadFileContent(SHARED_CACHE_MAP stSharedCcMap, ULONG ulFileSize, FILE *fp)
/**
	SHARED_CACHE_MAP �� �̿��Ͽ� ���� �����͸� �а� ���Ͽ� ����.

	@param[in]	stSharedCcMap	SHARED_CACHE_MAP ����ü
	@param[in]	ulFileSize		����ũ��
	@param[in]	fp				����������

	@retval     ����

	@remarks

	@see	
*/
{
	PVACB	apVacb[MAX_VACB];
	PVACB	pVacb;
	ULONG	ulMaxIndex, ulIndex;
	ULONG	cb;

	ulMaxIndex = (ULONG)(ulFileSize / SIZE_256KB);
	ulMaxIndex++;

#if DBG	
	dprintf( "VACBs Num : %d\n", ulMaxIndex );
#endif

	// ����ũ�Ⱑ 1MB ���� ũ�� pSharedCcMap->InitialVacbs �� ������� �ʰ� pSharedCcMap->Vacbs �� ����Ѵ�.
	if (ulFileSize > SIZE_1MB)
	{
		// Vacbs �ּҿ� ����� VACB ������ ����Ʈ�� �д´�.
		if (ReadMemory( (ULONG64)stSharedCcMap.Vacbs, &apVacb, (sizeof(VACB)*ulMaxIndex), &cb ) == 0)
		{
#if DBG
		    dprintf("pSharedCcMap->Vacbs reading error.\n");
#endif
		    return;
		}
		else
		{
			// aVacb[] ä��� ����
		}
	}
	else
	{
		// 1MB���� ������ 256KB ũ�� 4������ ������ Vacb �ּ� ��� 
		apVacb[0] = stSharedCcMap.InitialVacbs[0];
		apVacb[1] = stSharedCcMap.InitialVacbs[1];
		apVacb[2] = stSharedCcMap.InitialVacbs[2];
		apVacb[3] = stSharedCcMap.InitialVacbs[3];
	}

	// VACB �ϳ��� �ش��ϴ� 256KB �� �����͸� �����鼭 ��� VACB �� �д´�.
	for (ulIndex = 0; ulIndex < ulMaxIndex; ulIndex++)
	{
#if !DBG
		// Release ���忡�� ���������� ǥ���Ѵ�.
		dprintf(".");
#endif		

		pVacb = apVacb[ulIndex];

		if (pVacb == NULL)
		{
			// pVacb �� NULL �̸� �޸𸮿� �������� �ʴ� ���̹Ƿ� �׳� 0 ���� ä���.
			FillZeroData( fp );	
		}
		else
		{
			// �����Ͱ� �����ϸ� Cache view �� �����ϴ� ������ ���Ͽ� ����.
			CopyVacbData( pVacb, fp );
		}
	}
}


void ExtractFile( ULONG64 pAddress, PCSTR pFileName )
/**
	���Ͽ���Ʈ �ּҷκ��� ���� �����͸� �о ���Ϸ� �����Ѵ�.

	@param[in]	pAddress	���Ͽ���Ʈ �ּ�
	@param[in]	pFileName	�����̸�

	@retval     ����

	@remarks

	@see	
*/
{
	FILE_OBJECT				stFileObj;
	SECTION_OBJECT_POINTERS	stSecObjPtr;
	SHARED_CACHE_MAP		stSharedCcMap;
	ULONG					ulFileSize;
	ULONG					cb;
	FILE*					fp;
	
	// �Է� �Ķ���͸� Ȯ���Ѵ�.
	if (pAddress == (ULONG64)NULL || pFileName == NULL)
	{
	    dprintf("Parameter is NULL! Check your parameters\n");
	    return;
	}

	// ���Ͽ���Ʈ �޸� ������ �о���δ�.
	if (ReadMemory( pAddress, &stFileObj, sizeof(FILE_OBJECT), &cb ) == 0)
	{
	    dprintf("FILE_OBJECT reading error.\n");
	    return;
	}
	
	// ���� ����Ʈ Ÿ���� Ȯ���Ѵ�.
	if (stFileObj.Type != 5)
	{
	    dprintf("%x is not file object.\n");
	    return;
	}
	
	// SectionObjectPointer �� ��ȿ���� Ȯ���Ѵ�.
	if (stFileObj.SectionObjectPointer == NULL)
	{
	    dprintf("[%x]->SectionObjectPointer == NULL.\n", pAddress);
	    return;
	}
	
	// SectionObject �ּ��� ������ �о���δ�.
	if (ReadMemory( (ULONG64)stFileObj.SectionObjectPointer, &stSecObjPtr, sizeof(SECTION_OBJECT_POINTERS), &cb ) == 0)
	{
	    dprintf("SECTION_OBJECT_POINTER reading error.\n");
	    return;
	}
	
	// SharedCacheMap �� ��ȿ���� Ȯ���Ѵ�.
	if (stSecObjPtr.SharedCacheMap == NULL)
	{
	    dprintf("[%x]->SectionObjectPointer->SharedCacheMap == NULL.\n", pAddress);
	    return;
	}
	
	// SharedCacheMap �� �о���δ�.
	if (ReadMemory( (ULONG64)stSecObjPtr.SharedCacheMap, &stSharedCcMap, sizeof(SHARED_CACHE_MAP), &cb ) == 0)
	{
	    dprintf("SHARED_CACHE_MAP reading error.\n");
	    return;
	}
	
	// SharedCacheMap ������ ����Ѵ�.
	dprintf("SHARED_CACHE_MAP\n");
	dprintf("================\n");
	dprintf("FileSize        : %ld\n", stSharedCcMap.FileSize);
	dprintf("FileSize(Hex)   : 0x%x\n", stSharedCcMap.FileSize);
	dprintf("InitialVacbs[0] : 0x%x\n", stSharedCcMap.InitialVacbs[0]);
	dprintf("InitialVacbs[1] : 0x%x\n", stSharedCcMap.InitialVacbs[1]);
	dprintf("InitialVacbs[2] : 0x%x\n", stSharedCcMap.InitialVacbs[2]);
	dprintf("InitialVacbs[3] : 0x%x\n", stSharedCcMap.InitialVacbs[3]);
	dprintf("Vacbs           : 0x%x\n", stSharedCcMap.Vacbs);
	dprintf("FileObject      : 0x%x\n", stSharedCcMap.FileObject);
	
	// ����ũ�Ⱑ 32MB �� �Ѵ��� Ȯ���Ѵ�. 
	if (stSharedCcMap.FileSize.HighPart != 0)
	{
	    dprintf("File size over 32MB is NOT supported.\n");
	    return;
	}
	
	ulFileSize = stSharedCcMap.FileSize.LowPart;  
              
	// 32MB �� �Ѵ� ������ ���� Vacb ������ �ؾ��ϴµ� �������Ƿ� �������� �ʴ´�.
	if (ulFileSize > SIZE_32MB)
	{
	    dprintf("File size over 32MB is NOT supported.\n");
	    return;
	}
	
	// ��� ������ �����.
	fp = fopen( pFileName, "wb" );
	if (fp == NULL)
	{
	    dprintf("Creating %s failed.\n", pFileName);
	    return;
	}
		
	// VACB �� ã�Ƽ� ���� �����͸� �а� ���Ͽ� ����.
	ReadFileContent( stSharedCcMap, ulFileSize, fp );
	
	// ������ �ݰ� �����Ѵ�.
	fclose( fp );
}


PCSTR SkipSpace(PCSTR pString)
/**
	���鹮�ڸ� �˻��ؼ� ���鹮�ڰ� �ƴ� ��ġ�� �����Ѵ�.

	@param[in]	pString		���鹮�ڷ� �����ϴ� ���ڿ�

	@retval     �����Ͱ�	���鹮�ڰ� �ƴ� ������ ������
				NULL		���鹮�ڰ� �ƴ� ���ڸ� ã�� �� ����

	@remarks

	@see	
*/
{
	if (pString == NULL)
		return NULL;
		
	while( *pString != '\0' )
	{
		if (*pString != ' ')
		{
			return pString;
		}

		pString++;
	} 	
	
	return NULL;
}


//
// Extension to extract file from shared section
//  
//    !ef <fileobject address> <filename>
//
DECLARE_API( ef )
{
    ULONG cb;
    ULONG64 Address = 0;
    PCSTR pFileName = NULL;
    

	// args ���ڿ����� ù��° �Ķ���͸� ���ڷ� �����´�.
    if (GetExpressionEx(args, &Address, &args) == FALSE) 
    {
        dprintf("Usage:   !ef <fileobject address> <filename>\n");
        return;
    }
    
    // ���ŵ� args ���ڿ����� �ι�° �Ķ���Ͱ� �����ϴ��� Ȯ���Ѵ�.
    if (args[0] == 0)
    {
        dprintf("Usage:   !ef <fileobject address> <filename>\n");
        return;
    }
    	
	// args ���� ���ڿ� �������� �Լ��� ���� �� ������ ã���� ���ߴ�. -_-
	// GetExpressionEx() �� �����ϰ� ���� args �� <fileobject address> �ٷ� ���� ������ ����Ű�µ�
	// ������ ��ŵ�ϰ� <filename> �� ��� ���� SkipSpace() �Լ��� ����Ѵ�.
	pFileName = SkipSpace( args );
    
    dprintf("Creating \"%s\" with file object(%x)\n", pFileName, Address);
    
	// ���� �����͸� �����Ͽ� ������ �����.
    ExtractFile( Address, pFileName );
    
    dprintf("Done!\n");
}


/*
  A built-in help for the extension dll
*/

DECLARE_API ( help ) 
{
    dprintf("Help for extension dll myext.dll\n"
            "   help - Shows this help\n"
            "   ef <fileobject address> <filename> - It extract file from shared section\n"
            );
}

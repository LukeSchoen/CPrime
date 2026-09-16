#ifndef _CPRIME_CRTDBG_H
#define _CPRIME_CRTDBG_H

#include <stddef.h>

typedef struct _CrtMemState
{
  void* pBlockHeader;
  size_t lCounts[5];
  size_t lSizes[5];
  size_t lHighWaterCount;
  size_t lTotalCount;
} _CrtMemState;

#define _CRTDBG_REPORT_FLAG (-1)
#define _CRTDBG_LEAK_CHECK_DF 0x20

int _CrtSetDbgFlag(int flag);
int _CrtDumpMemoryLeaks(void);
void _CrtMemCheckpoint(_CrtMemState* state);
int _CrtMemDifference(_CrtMemState* difference,
                      const _CrtMemState* old_state,
                      const _CrtMemState* new_state);
void _CrtMemDumpStatistics(const _CrtMemState* state);

#endif

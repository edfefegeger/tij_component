#ifndef __IMEMORYMANAGER_H__
#define __IMEMORYMANAGER_H__

class IMemoryManager
{
public:
    virtual bool ADDIN_API AllocMemory(void** pMemory, unsigned long ulCountByte) = 0;
    virtual void ADDIN_API FreeMemory(void** pMemory) = 0;
};

#endif

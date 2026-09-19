#ifndef __IMEMORYMANAGER_H__
#define __IMEMORYMANAGER_H__

/*
 * Интерфейс менеджера памяти платформы, через который компонента выделяет
 * строки, возвращаемые во встроенный язык.
 *
 * ВАЖНО: структура взята из официального IMemoryManager.h комплекта
 * "Технология создания внешних компонент", и порядок методов здесь критичен.
 *
 * Виртуальный деструктор объявлен первым и занимает нулевую ячейку таблицы
 * виртуальных методов. Без него вызовы AllocMemory и FreeMemory уходят на
 * чужие ячейки: платформа выделяет память не там, где ожидает, строка с именем
 * объекта не создается, и встроенный язык сообщает "Тип не определен".
 *
 * Менять порядок объявлений нельзя, удалять деструктор нельзя.
 */

class IMemoryManager
{
public:
    virtual ~IMemoryManager() {}

    virtual bool ADDIN_API AllocMemory(void** pMemory, unsigned long ulCountByte) = 0;
    virtual void ADDIN_API FreeMemory(void** pMemory) = 0;
};

#endif /* __IMEMORYMANAGER_H__ */

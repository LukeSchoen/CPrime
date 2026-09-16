#include <stddef.h>

namespace compatibility
{
class VirtualPoolBase
{
public:
    virtual size_t ItemSize() const = 0;
    virtual void* Alloc() = 0;
    virtual void Free(void*) = 0;
    virtual void SetTracked() = 0;
};

template<size_t ItemSizeValue>
class VirtualPool : public VirtualPoolBase
{
public:
    VirtualPool() {}

    virtual size_t ItemSize() const override
    {
        return ItemSizeValue;
    }

    virtual void* Alloc() override { return storage; }
    virtual void Free(void*) override {}
    void SetTracked() override {}

private:
    char storage[ItemSizeValue];
};

struct PoolItem
{
    char bytes[120];
};

class PoolOwner
{
public:
    PoolOwner() : pool() {}
    PoolItem* AllocateItem();

private:
    template<class ItemType, size_t PoolSize>
    ItemType* Create(VirtualPool<PoolSize>& source);

    char padding[64];
    VirtualPool<sizeof(PoolItem)> pool;
};

template<class ItemType, size_t PoolSize>
ItemType* PoolOwner::Create(VirtualPool<PoolSize>& source)
{
    return (ItemType*)source.Alloc();
}

inline PoolItem* PoolOwner::AllocateItem()
{
    return Create<PoolItem>(pool);
}
}

int main()
{
    compatibility::PoolOwner owner;
    compatibility::PoolItem* item = owner.AllocateItem();
    if (!item)
        return 1;
    item->bytes[0] = 42;
    return item->bytes[0] == 42 ? 0 : 2;
}

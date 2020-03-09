
#ifndef REFCOUNT_H
#define REFCOUNT_H

class RefCount
{
private:
    int refcount;

public:
    RefCount() : refcount(1) {}
    virtual ~RefCount(){}

    inline void AddRef()
    {
        refcount++;
    }

    inline void Release()
    {
        refcount--;
        if (refcount==0)
            delete this;
    }
};

#endif

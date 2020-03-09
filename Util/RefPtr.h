
#ifndef REFPTR_H
#define REFPTR_H

template <class T>
class RefPtr
{
private:
    T* data;
public:

    RefPtr() : data(0) {}

    RefPtr(T* p) : data(p)
    {
        if (data)
            data->AddRef();
    }

    RefPtr(const RefPtr& rhs)
    {
        data = rhs.data;

        if (data)
            data->AddRef();
    }

    ~RefPtr()
    {
        if (data)
            data->Release();
    }

    inline RefPtr& operator = (T* rhs)
    {
        if (data)
            data->Release();

        data = rhs;

        if (data)
            data->AddRef();

        return *this;
    }

    inline RefPtr& operator = (const RefPtr& rhs)
    {
        if (data)
            data->Release();

        data = rhs.data;

        if (data)
            data->AddRef();

        return *this;
    }

    inline T* operator -> ()
    {
        return data;
    }

    inline operator T* ()
    {
        return data;
    }

    inline operator bool()
    {
        return data!=0;
    }

    inline bool operator !()
    {
        return data==0;
    }

    // for std::map

    inline bool operator < (const RefPtr& rhs) const
    {
        return data<rhs.data;
    }
};

#endif

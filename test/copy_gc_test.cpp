
#include <iostream>
#include <cstdlib>

#include "../include/gc/gc.hpp"
#include <vector>

using namespace copy;

class String : public GCObject
{
public:
	~String() override
	{
		std::cout << "~String" << std::endl;
	}
protected:
	GCObject* Copy(GC* gc,char*& space) override;
};

class Vector : public GCObject
{
public:
	explicit Vector(size_t n) : GCObject() , objects_(n,nullptr){ }
	Vector(Vector&& rhs) noexcept : GCObject(rhs) { objects_ = std::move(rhs.objects_); }

	~Vector() override
	{
		std::cout << "~Vector" << std::endl;
	}

	void PushBack(GCObject*& object)
	{
		objects_.push_back(&object);
	}

	

	void Clear()
	{
		objects_.clear();
	}

protected:
	GCObject* Copy(GC* gc,char*& space) override
	{
		new(space) Vector(std::move(*this));
		Vector* forwarding = reinterpret_cast<Vector*>(space);
		space += sizeof(Vector);

		for (size_t i = 0; i < forwarding->objects_.size(); ++i)
		{
			if(forwarding->objects_[i])
			{
				*forwarding->objects_[i] = gc->Copy(*forwarding->objects_[i]);
			}
		}

		return forwarding;
	}
private:
	std::vector<GCObject**> objects_;
};

int main()
{
	// todo，这里没法很好的获得根的指针,很难修改根的指向
	GC gc;
	GCObject* root = gc.New<Vector>(10);
	gc.PushRoot(&root);
	gc.New<Vector>(20);
	gc.New<Vector>(20);
	GCObject* p = gc.New<Vector>(20);
	dynamic_cast<Vector*>(root)->PushBack(p);
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	system("pause");
	return 0;
}

#include <iostream>
#include <cstdlib>

#include "../include/gc/gc.hpp"
#include <vector>

using namespace mark_sweep;

class String : public GCObject
{
public:
	explicit String(GC* gc)
		: GCObject(gc)
	{
		
	}

	~String() override
	{
		std::cout << "~String" << std::endl;
	}
protected:
	void Mark(const GC* gc) override
	{
		mark_ = true;
	}
};

class Vector : public GCObject
{
public:
	explicit Vector(GC* gc)
		: GCObject(gc)
	{
		
	}

	~Vector() override
	{
		std::cout << "~Vector" << std::endl;
	}

	void PushBack(GCObject* object)
	{
		objects_.push_back(object);
	}
protected:
	void Mark(const GC* gc) override
	{
		mark_ = true;
		for(auto object : objects_)
		{
			gc->Mark(object);
		}
	}
private:
	std::vector<GCObject*> objects_;
};

int main()
{
	
	{
		std::cout << "-------Test1-------" << std::endl;
		GC* gc = new GC();
		Vector* root = new Vector(gc);
		String* s = new String(gc);
		root->PushBack(s);
		Vector* v1 = new Vector(gc);
		root->PushBack(v1);
		Vector* v2 = new Vector(gc);
		v1->PushBack(v2);
		v2->PushBack(v1);
		gc->Mark(root);
		gc->Collect();
	}

	{
		std::cout << "-------Test2-------" << std::endl;
		GC* gc = new GC();
		Vector* root = new Vector(gc);
		String* s = new String(gc);
		root->PushBack(s);
		Vector* v1 = new Vector(gc);
		root->PushBack(v1);
		Vector* v2 = new Vector(gc);
		v1->PushBack(v2);
		v2->PushBack(v1);
		gc->Mark(root);
		gc->Collect();
		gc->Collect();
	}

	{
		std::cout << "-------Test3-------" << std::endl;
		GC* gc = new GC;
		Vector* root = new Vector(gc);
		String* s = new String(gc);
		root->PushBack(s);
		Vector* v1 = new Vector(gc);
		root->PushBack(v1);
		Vector* v2 = new Vector(gc);
		v1->PushBack(v2);
		v2->PushBack(v1);
		gc->Collect();
		std::cout << "\n" << std::endl;
		gc->Collect();
	}

	{
		std::cout << "-------Test4-------" << std::endl;
		GC* gc = new GC();
		Vector* root = new Vector(gc);
		String* s = new String(gc);
		root->PushBack(s);
		Vector* v1 = new Vector(gc);
		root->PushBack(v1);
		Vector* v2 = new Vector(gc);
		v1->PushBack(v2);
		v2->PushBack(v1);
	}

	{
		std::cout << "-------Test5-------" << std::endl;
		GC* gc = new GC();
		Vector* root = new Vector(gc);
		String* s = new String(gc);
		root->PushBack(s);
		Vector* v1 = new Vector(gc);
		root->PushBack(v1);
		Vector* v2 = new Vector(gc);
		v2->PushBack(v1);
		gc->Mark(root);
		gc->Collect();
	}

	{
		std::cout << "-------Test6-------" << std::endl;
		GC* gc = new GC();
		Vector* root = new Vector(gc);
		String* s = new String(gc);
		Vector* v1 = new Vector(gc);
		root->PushBack(v1);
		Vector* v2 = new Vector(gc);
		v2->PushBack(v1);
		gc->Mark(root);
		gc->Collect();
	}


	system("pause");
	return 0;
}
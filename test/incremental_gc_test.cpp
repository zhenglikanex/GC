
#include <iostream>
#include <cstdlib>
#include <typeinfo>
#include "../include/gc/gc.hpp"
#include <vector>
#include <string>
#include <string_view>

using namespace incremental;

class MyClass : public GCObject
{
public:
	MyClass() : GCObject(),field1_(nullptr), field2_(nullptr), field3_(nullptr) {}
	MyClass(std::string_view name) : MyClass() { name_ = name; }

	~MyClass() override
	{
		std::cout << name_ + "~MyClass" << std::endl;
	}

	FieldIterator BeginField() override
	{
		return FieldIterator(&field1_);
	}

	FieldIterator EndField() override
	{
		return FieldIterator((&field3_) + 1);
	}

	void set_field1(GC* gc, GCObject* field) { gc->WriteBarrier(field1_, field); }
	void set_field2(GC* gc, GCObject* field) { gc->WriteBarrier(field2_, field); }
	void set_field3(GC* gc, GCObject* field) { gc->WriteBarrier(field3_, field); }

	GCObject* field1() const { return field1_; }
	GCObject* field2() const { return field2_; }
	GCObject* field3() const { return field3_; }

private:
	std::string name_;
	GCObject* field1_;
	GCObject* field2_;
	GCObject* field3_;
};

class String : public GCObject
{
public:
	String()
		: GCObject()
	{
		
	}

	String(std::string_view name)
		: String()
	{
		name_ = name;
	}

	~String() override
	{
		std::cout << name_ + "~String" << std::endl;
	}

	std::string name_;
};

class Vector : public GCObject
{
public:
	Vector()
		: GCObject()
	{
		
	}

	Vector(std::string_view name)
		: Vector()
	{
		name_ = name;
	}

	~Vector() override
	{
		std::cout << name_ + "~Vector" << std::endl;
	}

	void PushBack(GCObject* object)
	{
		objects_.push_back(object);
	}

	FieldIterator BeginField() override
	{
		return FieldIterator(objects_.data());
	}

	FieldIterator EndField() override
	{
		return FieldIterator(objects_.data() + objects_.size());
	}
private:
	std::vector<GCObject*> objects_;
	std::string name_;
};

int main()
{
	GC gc;
	gc.set_mark_step(1);
	gc.set_sweep_step(1);
	MyClass* my_class = gc.New<MyClass>();
	//gc.AddRoot(my_class);
	my_class->set_field1(&gc,gc.New<String>());
	my_class->set_field2(&gc,gc.New<Vector>());
	my_class->set_field3(&gc,gc.New<Vector>());

	Vector* vec = gc.New<Vector>();
	vec->PushBack(nullptr);
	vec->PushBack(nullptr);
	vec->PushBack(nullptr);
	vec->PushBack(nullptr);

	gc.Collect();
	gc.New<Vector>();
	gc.Collect();
	gc.New<Vector>();
	gc.Collect();

	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();
	gc.Collect();

	{
		std::cout << "-------Test1-------" << std::endl;
		GC gc;
		gc.set_mark_step(2);
		gc.set_sweep_step(2);

		MyClass* my_class = gc.New<MyClass>("my_class1");
		my_class->set_field1(&gc,gc.New<String>("field1"));
		my_class->set_field2(&gc, gc.New<Vector>("field2"));
		my_class->set_field3(&gc, gc.New<MyClass>("field3"));
		((MyClass*)my_class->field1())->set_field1(&gc, gc.New<String>("field1_field1"));

		gc.AddRoot(my_class);

		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		//清理完了

		//标记root
		gc.Collect();

		// 标记my_class变黑field3变黑
		gc.Collect();

		// 把白色的放进黑色的my_class中,测试WriteBarrier
		my_class->set_field3(&gc,static_cast<MyClass*>(my_class->field1())->field1());

		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();
		gc.Collect();

		std::cout << "-------Test1-------" << std::endl;
	}

	{
		std::cout << "-------Test2-------" << std::endl;
		
	}

	{
		std::cout << "-------Test3-------" << std::endl;
		
	}

	{
		std::cout << "-------Test4-------" << std::endl;
	}

	{
		std::cout << "-------Test5-------" << std::endl;
		
	}

	{
		std::cout << "-------Test6-------" << std::endl;

	}


	system("pause");
	return 0;
}
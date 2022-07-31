#pragma once

#include <list>
#include <vector>
#include <stack>

namespace mark_sweep
{
	class GCObject
	{
		friend class GC;
	public:
		GCObject(GC* gc);
		virtual ~GCObject() = 0 {  }
	protected:
		virtual void Mark(const GC* gc) = 0;
		bool IsMark() const { return mark_; }
		void Reset() { mark_ = false; }

		int mark_;
	};

	class GC
	{
	public:
		~GC();

		void Mark(GCObject* root) const noexcept;
		void Collect();

		void AddObject(GCObject* object);
	private:
		std::list<GCObject*> objects_;
	};

	inline GCObject::GCObject(GC* gc)
		: mark_(false)
	{
		gc->AddObject(this);
	}

	inline GC::~GC()
	{
		for (auto object : objects_)
		{
			delete object;
		}
	}

	inline void GC::Mark(GCObject* object) const noexcept
	{
		if (object && !object->IsMark())
		{
			object->Mark(this);
		}
	}

	inline void GC::Collect()
	{
		auto it = objects_.begin();
		while (it != objects_.end())
		{
			GCObject* object = *it;
			if (!object->IsMark())
			{
				delete object;
				it = objects_.erase(it);
			}
			else
			{
				object->Reset();
				++it;
			}
		}
	}

	inline void GC::AddObject(GCObject* object)
	{
		objects_.push_back(object);
	}
}

namespace copy
{
	class GCObject
	{
		friend class GC;
	public:
		GCObject();
		GCObject(const GCObject& rhs) noexcept;
		virtual ~GCObject() = 0 {  }

		uint32_t instance_size() const noexcept { return instance_size_; }
	protected:
		virtual GCObject* Copy(GC* gc,char*& space) = 0;

		void set_tag(bool tag) noexcept { tag_ = tag; }
		bool tag() const noexcept { return tag_; }
		void set_instance_size(uint32_t size) noexcept { instance_size_ = size; }

		bool tag_;
		uint32_t instance_size_;
	};

	inline GCObject::GCObject()
		: tag_(false)
		, instance_size_(0)
	{
	}

	inline GCObject::GCObject(const GCObject& rhs) noexcept
		: tag_(false)
		, instance_size_(rhs.instance_size_)
	{
		
	}

	class GC
	{
	public:
		static constexpr size_t kMinObjectSize = 16;

		GC();
		~GC();

		template <class T,std::enable_if_t<std::is_base_of_v<GCObject,T>,size_t> N = 
			sizeof(T) < kMinObjectSize ? kMinObjectSize : sizeof(T),class ...Args>
		T* New(Args&& ... args);

		void Collect();
		GCObject* Copy(GCObject* object);
		void PushRoot(GCObject** root);
	private:
		void AllocSpace(size_t size);

		char* from_space_;
		char* to_space_;
		char* free_;
		size_t space_size_;
		std::vector<GCObject**> roots_;
	};

	template <class T,std::enable_if_t<std::is_base_of_v<GCObject, T>, size_t> N,class ...Args>
	T* GC::New(Args&& ... args)
	{
		if (free_ + N > from_space_ + space_size_)
		{
			Collect();

			while (free_ + N > from_space_ + space_size_)
			{
				AllocSpace(space_size_ * 2);
			}
		}

		T* obj = reinterpret_cast<T*>(free_);
		new (obj) T(std::forward<Args>(args)...);
		obj->set_instance_size(N);
		free_ += N;

		return obj;
	}

	inline GC::GC()
		: from_space_(nullptr)
		, to_space_(nullptr)
		, free_(nullptr)
	{
		AllocSpace(65536);
	}

	inline GC::~GC()
	{

	}

	inline void GC::Collect()
	{
		char* end = free_;
		free_ = to_space_;

		for(auto root : roots_)
		{
			*root = Copy(*root);
		}

		char* p = from_space_;
		while (p < end)
		{
			GCObject* obj = reinterpret_cast<GCObject*>(p);
			if (!obj->tag())
			{
				obj->~GCObject();
			}
			p += obj->instance_size();
		}

		std::swap(from_space_, to_space_);
	}

	inline GCObject* GC::Copy(GCObject* object)
	{
		GCObject* forwarding = reinterpret_cast<GCObject*>(reinterpret_cast<char*>(object) + sizeof(intptr_t));
		if(!object->tag())
		{
			forwarding = object->Copy(this, free_);
			object->set_tag(true);
			return forwarding;
		}

		return forwarding;
	}

	inline void GC::PushRoot(GCObject** root)
	{
		roots_.push_back(root);
	}

	inline void GC::AllocSpace(size_t size)
	{
		space_size_ = size;

		if(to_space_)
		{
			free(to_space_);
		}

		to_space_ = static_cast<char*>(malloc(size));
		if(to_space_ == nullptr)
		{
			throw std::bad_alloc();
		}

		if (from_space_)
		{
			Collect();

			// from_space已经被交换到to_space
			free(to_space_);

			to_space_ = static_cast<char*>(malloc(size));
			if (to_space_ == nullptr)
			{
				throw std::bad_alloc();
			}
		}
		else
		{
			from_space_ = static_cast<char*>(malloc(size));
			free_ = from_space_;
			if (from_space_ == nullptr)
			{
				throw std::bad_alloc();
			}
		}
	}
}

namespace incremental
{
	class GCObject
	{
	public:
		class FieldIterator
		{
		public:
			using ValueType = GCObject*;

			FieldIterator() : pointer_(nullptr) {}

			explicit  FieldIterator(const ValueType* pointer) : pointer_(pointer) {}

			bool operator==(const FieldIterator& rhs) const noexcept { return pointer_ == rhs.pointer_; }
			bool operator!=(const FieldIterator& rhs) const noexcept { return !(*this == rhs); }

			ValueType operator*() const noexcept { return *pointer_; }

			FieldIterator operator++(int) { return FieldIterator(pointer_++); }
			FieldIterator operator++() { return FieldIterator(++pointer_); }
		private:
			const ValueType* pointer_;
		};

		GCObject();
		virtual ~GCObject() = 0 {  }

		void Mark() noexcept { mark_ = true; }
		bool IsMark() const noexcept { return mark_; }
		void UnMark() noexcept { mark_ = false; }

		virtual FieldIterator BeginField() { return FieldIterator(nullptr); }
		virtual FieldIterator EndField() { return FieldIterator(nullptr); }

		uint32_t instance_size() const noexcept { return instance_size_; }
		void set_instance_size(uint32_t instance_size) noexcept
		{
			instance_size_ = instance_size;
		}
	private:
		uint32_t mark_;
		uint32_t instance_size_;
	};

	inline GCObject::GCObject()
		: mark_(false)
		, instance_size_(0)
	{

	}

	class GC
	{
	public:
		enum class Phase
		{
			kScan,
			kMark,
			kSweep
		};

		GC();
		~GC();

		template<class T,std::enable_if_t<std::is_base_of_v<GCObject,T>,size_t> N = sizeof(T),class ... Args>
		T* New(Args&& ... args);

		void Collect();

		//测试接口
		void AddRoot(GCObject* root);

		//写入屏障,防止gc暂停后将白色对象添加黑色对象中,导致错误的销毁
		void WriteBarrier(GCObject*& field, GCObject* obj) noexcept;

		void set_bytes_threshold(uint64_t bytes_threshold) noexcept { bytes_threshold_ = bytes_threshold; }
		void set_mark_step(uint32_t mark_step) noexcept { mark_step_ = mark_step; }
		void set_sweep_step(uint32_t sweep_step) noexcept { sweep_step_ = sweep_step; }
	private:
		//判断是否到内存阈值，开始启动GC
		//因为不是一次做完清理工作
		//所以需要在内存快满前开始执行
		void TryCollect();

		void Mark(GCObject* object);
		void Scan();
		void Mark();
		void Sweep();

		std::vector<GCObject*> roots_;		
		std::vector<GCObject*> gray_objects_;			//灰色对象,代表子节点还未被标记的对象
		std::list<GCObject*> objects_;
		std::list<GCObject*>::iterator cur_sweep_it_;  //记录当前清理到的节点
		Phase phase_;
		bool sweeping_;
		size_t managed_bytes_;
		size_t bytes_threshold_;
		uint32_t mark_step_;
		uint32_t sweep_step_;
	};


	template <class T, std::enable_if_t<std::is_base_of_v<GCObject, T>, size_t> N, class ... Args>
	T* GC::New(Args&& ... args)
	{
		TryCollect();

		T* obj = static_cast<T*>(malloc(N));
		new (obj)T(std::forward<Args>(args)...);
		obj->set_instance_size(N);

		//如果在gc到一半，标记新创建的对象,防止刚创建出来就被销毁
		if(phase_ != Phase::kScan)
		{
			Mark(obj);
		}

		//加入GC管理
		objects_.emplace_back(obj);
		managed_bytes_ += N;

		return obj;
	}

	inline GC::GC()
		: phase_(Phase::kScan)
		, sweeping_(false)
		, managed_bytes_(0)
		, bytes_threshold_(128 * 1024 * 1024)
		, mark_step_(std::max(bytes_threshold_ / sizeof(void*) / 4,1ull))
		, sweep_step_(mark_step_)
	{
	}

	inline GC::~GC()
	{
		for (auto object : objects_)
		{
			delete object;
		}
	}

	inline void GC::Collect()
	{
		switch (phase_)
		{
		case Phase::kScan:
			Scan();
			break;
		case Phase::kMark:
			Mark();
			break;
		case Phase::kSweep:
			Sweep();
			break;
		default: 
			break;
		}
	}

	inline void GC::AddRoot(GCObject* root)
	{
		roots_.emplace_back(root);
	}

	inline void GC::WriteBarrier(GCObject*& field, GCObject* obj) noexcept
	{
		if (obj && !obj->IsMark())
		{
			Mark(obj);
		}
		field = obj;
	}

	inline void GC::TryCollect()
	{
		if(managed_bytes_ > bytes_threshold_)
		{
			Collect();
		}
	}

	inline void GC::Mark(GCObject* object)
	{
		if (!object->IsMark())
		{
			object->Mark();
			gray_objects_.push_back(object);
		}
	}

	inline void GC::Scan()
	{
		for(auto object : roots_)
		{
			Mark(object);
		}

		phase_ = Phase::kMark;
	}

	inline void GC::Mark()
	{
		for (uint32_t i = 0; i < mark_step_; ++i)
		{
			if (!gray_objects_.empty())
			{
				//子对象标记位为灰色后,这个对象就标记为黑色了
				GCObject* black_object = gray_objects_.back();
				gray_objects_.pop_back();

				for (auto it = black_object->BeginField();
					it != black_object->EndField(); ++it)
				{
					if(*it)
					{
						Mark(*it);
					}
				}
			}
			else
			{
				//todo看看有没有新添加的根,看实际接入需求
				break;
			}
		}

		phase_ = Phase::kSweep;
	}

	inline void GC::Sweep()
	{
		//初始化成清理中
		if(!sweeping_)
		{
			cur_sweep_it_ = objects_.begin();
			sweeping_ = true;
		}

		uint32_t count = 0;
		while (cur_sweep_it_ != objects_.end())
		{
			GCObject* obj = *cur_sweep_it_;
			if (!obj->IsMark())
			{
				managed_bytes_ -= obj->instance_size();
				delete obj;
				cur_sweep_it_ = objects_.erase(cur_sweep_it_);
			}
			else
			{
				obj->UnMark();
				++cur_sweep_it_;
			}

			++count;

			if (count >= sweep_step_)
			{
				return;
			}
		}

		phase_ = Phase::kScan;
		sweeping_ = false;
		gray_objects_.clear();
	}
}

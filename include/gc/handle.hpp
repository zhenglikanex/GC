#pragma once

template<class T>
class Handle
{
public:
	Handle()
		: current_(nullptr)
	{
		
	}

	Handle(const Handle& rhs)
	{

	}

	explicit Handle(T* pointer)
	{

	}

	Handle(Handle&& rhs) noexcept
	{

	}

	Handle& operator=(const Handle&)
	{
	}

	Handle& operator=(Handle&&) noexcept
	{
	}

	~Handle() noexcept
	{
	}

	T& operator*() noexcept
	{
		return *current_;
	}

	T* operator->() noexcept
	{
		return current_;
	}
private:
	T* current_;
};
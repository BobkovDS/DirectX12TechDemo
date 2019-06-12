#pragma once
class ExecuterBase
{
public:
	virtual void execute() = 0;
	virtual void execute(int p1) = 0;
	virtual ~ExecuterBase() {}
};

template<class ClassName>
class ExecuterVoidVoid : public ExecuterBase // executer with void excute(UINT) function
{
	using ptrVoidFnRA = void(ClassName::*)();
	ptrVoidFnRA ptr;
	ClassName* parent;
public:
	ExecuterVoidVoid(ClassName* iparent, ptrVoidFnRA iptr) : parent(iparent), ptr(iptr)
	{
		assert(parent);
		assert(ptr);
	}

	void execute()
	{
		(parent->*ptr)();
	}
	void execute(int p1)
	{
		execute();
	}
};

template<class ClassName>
class ExecuterVoidInt: public ExecuterBase // executer with void excute(UINT) function
{
	using ptrVoidFnRA = void(ClassName::*)(int p1);
	ptrVoidFnRA ptr;
	ClassName* parent;
public:
	ExecuterVoidInt(ClassName* iparent, ptrVoidFnRA iptr) : parent(iparent), ptr(iptr)
	{
		assert(parent);
		assert(ptr);
	}
	void execute()
	{
		execute(0);
	}

	void execute(int p1)
	{
		(parent->*ptr)(p1);
	}
};
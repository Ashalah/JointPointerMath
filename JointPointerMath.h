/*
The MIT License (MIT)

Copyright (c) 2014 Andy Robbins

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
This is a tiny header-only C++ library for automating pointer maths when making joint allocations.
It was inspired by Jonathan Blow's talk about future gamedev programming languages.


Example usage:
==============

	vec3* Vertices;
	unsigned short* Indices;

	size_t TotalSize;
	void* Buffer = JointPointerAllocate(&TotalSize, malloc,
	{
		JointPointer(&Vertices, sizeof(vec3) * 4),
		JointPointer(&Indices, sizeof(unsigned short) * 6)
	});

	// Vertices and Indices now point to sections of Buffer.
	// ....

	free(Buffer);


Documentation:
==============


void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t Size), int Num, JointPointer_t* Elems);
void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t Size, size_t Alignment), int Num, JointPointer_t* Elems);

	JointPointerAllocate will calculate the total size required for the buffer, call the given allocator function,
	perform the pointer maths and write all the given pointers, then returns a pointer to the allocated
	buffer and optionally writes the size of the buffer to OutSize. (OutSize may be null)

	Note that if you need special alignment (ex 16bytes), you should use the version of JointPointerAllocate
	that takes an allocator function which can actually do an aligned allocation.
	Otherwise, the first element will not be properly aligned.
	If an aligned allocator is provided, the alignment of the new buffer is the alignment of the first element.

	NOTE, on Windows _aligned_malloc takes the size argument first, whereas on Linux,
	memalign takes the alignment argument first. Hopefully you'll read this and not waste your time.


template<int Num> void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size), JointPointer_t (&Arr)[Num]);
template<int Num> void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size, size_t alignment), JointPointer_t (&Arr)[Num]);

	Helper functions for JointPointerAllocate. Automatically detects array size to avoid copy-paste errors.


void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size), std::initializer_list<JointPointer_t> ini);
void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size, size_t alignment), std::initializer_list<JointPointer_t> ini);

	Helper functions for JointPointerAllocate. Used in the example.


template<typename T> JointPointer_t JointPointer(T** Ptr, size_t Sz, size_t Align);
template<typename T> JointPointer_t JointPointer(T** Ptr, size_t Sz);

	Helper functions to initialize a JointPointer_t.
	It avoids having to manually cast Ptr down to void**
	The second version determines alignment via std::alignment_of<T>

	An optional third argument with the alignment can be passed.
	This is useful for example when you need 16byte alignment for SIMD operations.
	If an alignment isn't specified, it is determined using std::alignment_of and the type of the pointer passed.
	Note that if the pointer is void*, alignment cannot be determined and you will get a compile error.


size_t JointPointerTotalSize(int Num, JointPointer_t* Elems);
template<int Num> size_t JointPointerTotalSize(JointPointer_t (&Arr)[Num]);

	If you just need to find out the required buffer size without allocating, use JointPointerTotalSize.
	Calculates and returns the total size of the buffer required, and write the offset to each JointPointer_t element.


Version history:
================


September 24, 2014 - first draft.
*/

#ifndef JOINT_POINTER_MATH_H
#define JOINT_POINTER_MATH_H

#define JOINTPOINTERMATH_ASSERT(x) assert(x)
//#define JOINTPOINTERMATH_ASSERT(x) do {} while(0)

#include <initializer_list>
#include <memory>
#include <assert.h>

struct JointPointer_t
{
public:
	void** Pointer;
	size_t Size;
	size_t Alignment;
	size_t Offset;

	JointPointer_t(){}

	JointPointer_t(void** P, size_t Sz, size_t Align)
	{
		Pointer = P;
		Size = Sz;
		Alignment = Align;
		Offset = 0;
	}
};

template<typename T> JointPointer_t JointPointer(T** Ptr, size_t Sz, size_t Align)
{
	JOINTPOINTERMATH_ASSERT(Ptr != nullptr);
	return JointPointer_t((void**) Ptr, Sz, Align);
}

template<typename T> JointPointer_t JointPointer(T** Ptr, size_t Sz)
{
	JOINTPOINTERMATH_ASSERT(Ptr != nullptr);
	return JointPointer_t((void**) Ptr, Sz, std::alignment_of<T>::value);
}

inline size_t JointPointerTotalSize(int Num, JointPointer_t* Elems)
{
	JOINTPOINTERMATH_ASSERT(Num > 0);
	JOINTPOINTERMATH_ASSERT(Elems != nullptr);
	void* Ptr = 0;
	for (int i=0; i <Num; i++)
	{
		size_t Ignore = std::numeric_limits<std::size_t>::max();
		Ptr = std::align(Elems[i].Alignment, Elems[i].Size, Ptr, Ignore);
		Elems[i].Offset = (size_t)Ptr;
		Ptr = ((char*)Ptr) + Elems[i].Size;
	}
	return (size_t)Ptr;
}

template<int Num> size_t JointPointerTotalSize(JointPointer_t (&Arr)[Num])
{
	JOINTPOINTERMATH_ASSERT(Num > 0);
	return JointPointerTotalSize(Num, Arr);
}

inline void JointPointerWrite(void* Memory, int Num, JointPointer_t* Elems)
{
	JOINTPOINTERMATH_ASSERT(Num > 0);
	JOINTPOINTERMATH_ASSERT(Elems != nullptr);
	for (int i=0; i <Num; i++)
	{
		*Elems[i].Pointer = ((char*)Memory) + Elems[i].Offset;
	}
}

inline void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t Size), int Num, JointPointer_t* Elems)
{
	JOINTPOINTERMATH_ASSERT(Num > 0);
	JOINTPOINTERMATH_ASSERT(Elems != nullptr);
	size_t TotalSize = JointPointerTotalSize(Num, Elems);
	if (OutSize != nullptr)
	{
		*OutSize = TotalSize;
	}
	void* Memory = Alloc(TotalSize);
	JointPointerWrite(Memory, Num, Elems);
	return Memory;
}

inline void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t Size, size_t Alignment), int Num, JointPointer_t* Elems)
{
	JOINTPOINTERMATH_ASSERT(Num > 0);
	JOINTPOINTERMATH_ASSERT(Elems != nullptr);
	size_t TotalSize = JointPointerTotalSize(Num, Elems);
	if (OutSize != nullptr)
	{
		*OutSize = TotalSize;
	}
	void* Memory = Alloc(Elems[0].Alignment, TotalSize);
	JointPointerWrite(Memory, Num, Elems);
	return Memory;
}

template<int Num> void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size), JointPointer_t (&Arr)[Num])
{
	return JointPointerAllocate(OutSize, Alloc, Num, Arr);
}

template<int Num> void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size, size_t alignment), JointPointer_t (&Arr)[Num])
{
	return JointPointerAllocate(OutSize, Alloc, Num, Arr);
}

inline void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size), std::initializer_list<JointPointer_t> ini)
{
	JOINTPOINTERMATH_ASSERT(ini.size() > 0);

	// Determine total size.
	void* Ptr = 0;
	for (const JointPointer_t& JP : ini)
	{
		size_t Ignore = std::numeric_limits<std::size_t>::max();
		Ptr = std::align(JP.Alignment, JP.Size, Ptr, Ignore);
		Ptr = ((char*)Ptr) + JP.Size;
	}
	size_t TotalSize = (size_t)Ptr;
	if (OutSize != nullptr)
	{
		*OutSize = TotalSize;
	}

	void* Memory = Alloc(TotalSize);

	// Write pointers.
	Ptr = 0;
	for (const JointPointer_t& JP : ini)
	{
		size_t Ignore = std::numeric_limits<std::size_t>::max();
		Ptr = std::align(JP.Alignment, JP.Size, Ptr, Ignore);
		*JP.Pointer = (char*)Memory + (size_t)Ptr;
		Ptr = ((char*)Ptr) + JP.Size;
	}
	return nullptr;
}

inline void* JointPointerAllocate(size_t* OutSize, void* (*Alloc)(size_t size, size_t alignment), std::initializer_list<JointPointer_t> ini)
{
	JOINTPOINTERMATH_ASSERT(ini.size() > 0);

	// Determine total size.
	void* Ptr = 0;
	for (const JointPointer_t& JP : ini)
	{
		size_t Ignore = std::numeric_limits<std::size_t>::max();
		Ptr = std::align(JP.Alignment, JP.Size, Ptr, Ignore);
		Ptr = ((char*)Ptr) + JP.Size;
	}
	size_t TotalSize = (size_t)Ptr;
	if (OutSize != nullptr)
	{
		*OutSize = TotalSize;
	}

	void* Memory = Alloc(TotalSize, ini.begin()->Alignment);

	// Write pointers.
	Ptr = 0;
	for (const JointPointer_t& JP : ini)
	{
		size_t Ignore = std::numeric_limits<std::size_t>::max();
		Ptr = std::align(JP.Alignment, JP.Size, Ptr, Ignore);
		*JP.Pointer = (char*)Memory + (size_t)Ptr;
		Ptr = ((char*)Ptr) + JP.Size;
	}
	return nullptr;
}

#endif

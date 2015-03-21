JointPointerMath
================

This is a tiny public domain header-only C++ library for automating pointer maths when making joint allocations.
It was inspired by Jonathan Blow's talk about future gamedev programming languages.

Documentation is inside the header itself.


Example usage:
--------------

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

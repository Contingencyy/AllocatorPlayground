#include <cstdint>
#include <stdio.h>
#include <malloc.h>
#include <assert.h>

// TODO: Remove, just for std::cin.get()
#include <iostream>

// -------------------------------------------------------------------------
// Some useful defines

#define ALIGN_POW2(x, align) ((intptr_t)(x) + ((align) - 1) & (-(intptr_t)(align)))
#define ALIGN_DOWN_POW2(x, align) ((intptr_t)(x) & (-(intptr_t)(align)))

#define BYTES(x) x
#define KILOBYTES(x) x << 10
#define MEGABYTES(x) x << 20
#define GIGABYTES(x) x << 30

// -------------------------------------------------------------------------
// Linear allocator

namespace LinearAllocator
{

	uint8_t* buffer_ptr_base;
	uint8_t* buffer_ptr_end;
	uint8_t* buffer_ptr_at;

	void Initialize(size_t initial_byte_size)
	{
		// TODO: Replace with virtual malloc so that we can reserve a large chunk
		// of memory without actually committing it until we need it
		buffer_ptr_base = (uint8_t*)malloc(initial_byte_size);
		buffer_ptr_end = buffer_ptr_base + initial_byte_size;
		buffer_ptr_at = buffer_ptr_base;
	}

	size_t GetAlignedByteSizeLeft(size_t align)
	{
		size_t byte_size_left = buffer_ptr_end - buffer_ptr_at;
		size_t aligned_byte_size_left = ALIGN_DOWN_POW2(byte_size_left, align);
		return aligned_byte_size_left;
	}

	void* Allocate(size_t num_bytes, size_t align)
	{
		void* alloc = nullptr;

		if (GetAlignedByteSizeLeft(align) >= num_bytes)
		{
			alloc = buffer_ptr_at;
			memset(alloc, 0, num_bytes);
			buffer_ptr_at += num_bytes;
		}
		else
		{
			// TODO: Handle this more gracefully, reserve and commit some more memory
			// but for now this is fine to test with
			//assert(false && "We have run out of memory!");
		}

		return alloc;
	}

}

#define LINEAR_ALLOC_(num_bytes, align) LinearAllocator::Allocate(num_bytes, align)
#define LINEAR_ALLOC_STRUCT_(type) (type*)LINEAR_ALLOC_(sizeof(type), alignof(type))
#define LINEAR_ALLOC_ARRAY_(type, count) (type*)LINEAR_ALLOC_(count * sizeof(type), alignof(type))

// -------------------------------------------------------------------------
// TODO: Implement scope allocator that deallocates if going out of scope

namespace ScopeAllocator
{
}

// -------------------------------------------------------------------------
// TODO: Implement arena allocator which allows linear allocation but in arenas

namespace ArenaAllocator
{
}

struct NonTrivialStruct
{
	int integer;
	const char* string;
};

// -------------------------------------------------------------------------
// Main

int main()
{
	// -------------------------------------------------------------------------
	// Allocator playground to test different implementations of memory allocators
	
	// LINEAR ALLOCATOR

	LinearAllocator::Initialize(BYTES(24));

	int* allocated_int = LINEAR_ALLOC_STRUCT_(int);
	printf("%i\n", *allocated_int);

	float* allocated_float = LINEAR_ALLOC_STRUCT_(float);
	printf("%f\n", *allocated_float);

	NonTrivialStruct* allocated_non_trivial_struct = LINEAR_ALLOC_STRUCT_(NonTrivialStruct);
	allocated_non_trivial_struct->integer = 1234;
	allocated_non_trivial_struct->string = "TestString";
	printf("%i, %s\n", allocated_non_trivial_struct->integer, allocated_non_trivial_struct->string);

	std::cin.get();
}

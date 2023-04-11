#include <cstdint>
#include <stdio.h>
#include <malloc.h>
#include <assert.h>

#include <Windows.h>
#include <memoryapi.h>

// TODO: Remove, just for std::cin.get()
#include <iostream>

// -------------------------------------------------------------------------
// Some useful defines

#define ALIGN_POW2(x, align) ((intptr_t)(x) + ((align) - 1) & (-(intptr_t)(align)))
#define ALIGN_DOWN_POW2(x, align) ((intptr_t)(x) & (-(intptr_t)(align)))

// -------------------------------------------------------------------------
// Virtual memory helpers

namespace VirtualMemory
{

	void* Reserve(size_t reserve_byte_size)
	{
		// This reserves a range of the process's virtual address space, but does not allocate actual
		// physical memory to back these addresses
		void* reserve = VirtualAlloc(NULL, reserve_byte_size, MEM_RESERVE, PAGE_NOACCESS);
		assert(reserve && "Failed to reserve virtual memory");
		return reserve;
	}

	bool Commit(void* address, size_t num_bytes)
	{
		// This allocates memory for the specified reserved memory location
		// The physical memory pages will not be allocated unless the virtual addresses are actually accessed
		// The memory will always be initialized to 0
		void* committed = VirtualAlloc(address, num_bytes, MEM_COMMIT, PAGE_READWRITE);
		assert(committed && "Failed to commit virtual memory");
		return committed;
	}

	void Decommit(void* address, size_t num_bytes)
	{
		// This decommits all memory pages that contain one or more bytes in the range of lpAddress - (lpAddress + dwSize)
		// If the address is a base address returned by VirtualAlloc and dwSize is 0, it will decommit an entire rection that
		// was previously allocated with VirtualAlloc (it will still remain reserved).
		int status = VirtualFree(address, num_bytes, MEM_DECOMMIT);
		assert(status > 0 && "Failed to decommit virtual memory");
	}

	void Release(void* address)
	{
		// dwSize needs to be 0 if the dwFreeType is MEM_RELEASE
		// This will free the entire region that was reserved in the initial VirtualAlloc call
		int status = VirtualFree(address, 0, MEM_RELEASE);
		assert(status > 0 && "Failed to release virtual memory");
	}

}

// -------------------------------------------------------------------------
// Linear allocator

// 4 GB
#define LINEAR_ALLOCATOR_DEFAULT_RESERVE_SIZE (4ull << 30)
// 4 KB
#define LINEAR_ALLOCATOR_COMMIT_CHUNK_SIZE (4ull << 10)

namespace LinearAllocator
{

	uint8_t* buffer_ptr_base;
	uint8_t* buffer_ptr_at;
	uint8_t* buffer_ptr_end;
	uint8_t* buffer_ptr_committed;

	size_t GetAlignedByteSizeLeft(size_t align)
	{
		size_t byte_size_left = buffer_ptr_end - buffer_ptr_at;
		size_t aligned_byte_size_left = ALIGN_DOWN_POW2(byte_size_left, align);
		return aligned_byte_size_left;
	}

	void SetAtPointer(uint8_t* new_at_ptr)
	{
		if (new_at_ptr >= buffer_ptr_base && new_at_ptr < buffer_ptr_end)
		{
			if (new_at_ptr > buffer_ptr_committed)
			{
				size_t commit_chunk_size = ALIGN_POW2(new_at_ptr - buffer_ptr_committed, LINEAR_ALLOCATOR_COMMIT_CHUNK_SIZE);
				VirtualMemory::Commit(buffer_ptr_committed, commit_chunk_size);

				buffer_ptr_committed += commit_chunk_size;

				printf("Committed %llu bytes of memory\n", commit_chunk_size);
			}

			buffer_ptr_at = new_at_ptr;
		}
	}

	void* Allocate(size_t num_bytes, size_t align)
	{
		printf("Trying to allocate %llu bytes\n", num_bytes);

		if (!buffer_ptr_base)
		{
			buffer_ptr_base = (uint8_t*)VirtualMemory::Reserve(LINEAR_ALLOCATOR_DEFAULT_RESERVE_SIZE);
			buffer_ptr_at = buffer_ptr_base;
			buffer_ptr_end = buffer_ptr_base + LINEAR_ALLOCATOR_DEFAULT_RESERVE_SIZE;
			buffer_ptr_committed = buffer_ptr_base;

			printf("Reserved %llu bytes of memory\n", LINEAR_ALLOCATOR_DEFAULT_RESERVE_SIZE);
		}

		uint8_t* alloc = nullptr;

		if (GetAlignedByteSizeLeft(align) >= num_bytes)
		{
			alloc = (uint8_t*)ALIGN_POW2(buffer_ptr_at, align);
			SetAtPointer(alloc + num_bytes);
			memset(alloc, 0, num_bytes);
		}

		printf("Allocated %llu bytes\n", num_bytes);

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

	printf("Integer - 4 bytes\n");
	int* allocated_int = LINEAR_ALLOC_STRUCT_(int);

	printf("Float - 4 bytes\n");
	float* allocated_float = LINEAR_ALLOC_STRUCT_(float);

	printf("NonTrivialStruct - 16 bytes\n");
	NonTrivialStruct* allocated_non_trivial_struct = LINEAR_ALLOC_STRUCT_(NonTrivialStruct);
	allocated_non_trivial_struct->integer = 1234;
	allocated_non_trivial_struct->string = "TestString";

	printf("NonTrivialStruct (1000) - 16000 bytes\n");
	NonTrivialStruct* allocated_nts_array = LINEAR_ALLOC_ARRAY_(NonTrivialStruct, 1000);
	for (size_t i = 0; i < 1000; ++i)
	{
		allocated_nts_array[i].integer = i;
		allocated_nts_array[i].string = "TestStringArray";
	}

	printf("uint64_t (10000) - 80000 bytes\n");
	uint64_t* allocated_uint64_array = LINEAR_ALLOC_ARRAY_(uint64_t, 10000);
	for (size_t i = 0; i < 10000; ++i)
	{
		allocated_uint64_array[i] = i;
	}

	std::cin.get();
}

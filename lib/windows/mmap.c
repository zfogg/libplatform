/**
 * @file platform/windows/mmap.c
 * @brief Windows implementation of memory-mapped files
 * @ingroup platform
 */

#ifdef _WIN32

#include <libplatform/mmap.h>
#include <ascii-chat/debug/named.h>
#include <ascii-chat/log/log.h>

#include <windows.h>

void platform_mmap_init(platform_mmap_t *mapping) {
  if (!mapping) {
    return;
  }
  mapping->addr = NULL;
  mapping->size = 0;
  mapping->file_handle = INVALID_HANDLE_VALUE;
  mapping->mapping_handle = NULL;
}

int platform_mmap_open(const char *name, const char *path, size_t size, platform_mmap_t *out) {
  if (!name || !path || !out) {
    return -1;
  }

  if (size == 0) {
    return -1;
  }

  // Open or create the file
  HANDLE file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, NULL);

  if (file == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    return -1;
  }

  // Get current file size
  LARGE_INTEGER file_size;
  if (!GetFileSizeEx(file, &file_size)) {
    DWORD error = GetLastError();
    CloseHandle(file);
    return -1;
  }

  // Resize file if needed
  if ((size_t)file_size.QuadPart < size) {
    LARGE_INTEGER new_size;
    new_size.QuadPart = (LONGLONG)size;
    if (!SetFilePointerEx(file, new_size, NULL, FILE_BEGIN) || !SetEndOfFile(file)) {
      DWORD error = GetLastError();
      CloseHandle(file);
      return -1;
    }
} else if ((size_t)file_size.QuadPart > size) {
    // File is larger than requested - use existing size
    size = (size_t)file_size.QuadPart;
}

  // Create file mapping
  DWORD size_high = (DWORD)((size >> 32) & 0xFFFFFFFF);
  DWORD size_low = (DWORD)(size & 0xFFFFFFFF);

  HANDLE mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, size_high, size_low, NULL);

  if (!mapping) {
    DWORD error = GetLastError();
    CloseHandle(file);
    return -1;
  }

  // Map view of file
  void *addr = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, size);

  if (!addr) {
    DWORD error = GetLastError();
    CloseHandle(mapping);
    CloseHandle(file);
    return -1;
  }

  out->addr = addr;
  out->size = size;
  out->file_handle = file;
  out->mapping_handle = mapping;
return 0;
}

void platform_mmap_close(platform_mmap_t *mapping) {
  if (!mapping) {
    return;
  }

  if (mapping->addr) {
    if (!UnmapViewOfFile(mapping->addr)) {
}
    mapping->addr = NULL;
  }

  if (mapping->mapping_handle) {
    CloseHandle(mapping->mapping_handle);
    mapping->mapping_handle = NULL;
  }

  if (mapping->file_handle != INVALID_HANDLE_VALUE) {
    CloseHandle(mapping->file_handle);
    mapping->file_handle = INVALID_HANDLE_VALUE;
  }

  mapping->size = 0;
}

void platform_mmap_sync(platform_mmap_t *mapping, bool async) {
  if (!mapping || !mapping->addr) {
    return;
  }

  // Note: FlushViewOfFile is always async; it just initiates the flush.
  // To wait for completion, you'd need FlushFileBuffers on the file handle.
  if (!FlushViewOfFile(mapping->addr, mapping->size)) {
}

  // If sync requested, also flush file buffers to ensure data hits disk
  if (!async && mapping->file_handle != INVALID_HANDLE_VALUE) {
    if (!FlushFileBuffers(mapping->file_handle)) {
}
  }
}

bool platform_mmap_is_valid(const platform_mmap_t *mapping) {
  return mapping && mapping->addr && mapping->file_handle != INVALID_HANDLE_VALUE && mapping->mapping_handle;
}

#endif

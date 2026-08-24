#include <stdlib.h>
#include <string.h>

extern "C"
{
int clControls_KeyIsDown_int_ref(const int *key);
void clFileWriter_constructor_llong(void *self, long long buffer_size);
void clPool__clHashMapEntry__clList__i64__vertLookup_constructor_llong(
  void *self, long long initial_capacity);
void clPool__clHashMapEntry__clString__GLint_constructor_llong(
  void *self, long long initial_capacity);
void clString_constructor_struct_clString_ref(void *self, const void *source);
int clFile_WriteFile(const void *path, const void *data, long long size,
                     long long *bytes_written);
long long clWriteStream_Write(void *stream, const void *data,
                              long long size);
long long clReadStream_Read_llong_llong_ptr(void *stream, long long size,
                                            long long *bytes_read);

int clControls_KeyIsDown(const int *key)
{
  return clControls_KeyIsDown_int_ref(key);
}

/* CreateTemporaryFile returns a named local.  Until deleted-copy rejection
   feeds move selection, use the class's actual four-field move layout so the
   source destructor cannot close the returned FILE twice. */
void clFile_constructor_struct_clFile_ref(void *self, void *source)
{
  memcpy(self, source, 32);
  memset(source, 0, 32);
}

long long clFileHelper_Write(const void *path, const void *data,
                             long long count)
{
  return clFile_WriteFile(path, data, count, 0) ? count : 0;
}

void clFileWriter_constructor(void *self)
{
  clFileWriter_constructor_llong(self, 8LL * 1024LL * 1024LL);
}

/* clList<T> is { size, capacity, data }.  Moving it is independent of T. */
void clList__clString_constructor_struct_clList__clString_rref(
  void *self, void *source)
{
  memcpy(self, source, 24);
  memset(source, 0, 24);
}

void clList__clString_PushBack_struct_clString_ref(void *list,
                                                   const void *value)
{
  long long *fields = (long long *)list;
  long long size = fields[0];
  long long capacity = fields[1];
  unsigned char *data = (unsigned char *)(size_t)fields[2];
  if (size >= capacity)
  {
    long long next_capacity = capacity ? capacity * 2 : 1;
    data = (unsigned char *)realloc(data, (size_t)next_capacity * 24);
    if (!data)
      return;
    fields[1] = next_capacity;
    fields[2] = (long long)(size_t)data;
  }
  clString_constructor_struct_clString_ref(data + size * 24, value);
  fields[0] = size + 1;
}

void clPool__clHashMapEntry__clList__i64__vertLookup_constructor(void *self)
{
  clPool__clHashMapEntry__clList__i64__vertLookup_constructor_llong(self, 0);
}

void clPool__clHashMapEntry__clString__GLint_constructor(void *self)
{
  clPool__clHashMapEntry__clString__GLint_constructor_llong(self, 0);
}

void __cpc_ns_std_unique_ptr__clHashMapIterator__clString__GLint__true__Reference_constructor_struct___cpc_ns_std_unique_ptr__clHashMapIterator__clString__GLint__true__Reference_ref(
  void *self, void *source)
{
  *(void **)self = *(void **)source;
  *(void **)source = 0;
}

long long clStreamWrite_struct_clString_ptr_llong_struct_clWriteStream_ptr(
  const void *values, long long count, void *stream)
{
  const unsigned char *value = (const unsigned char *)values;
  long long i;
  for (i = 0; i < count; ++i, value += 24)
  {
    long long size = *(const long long *)value;
    const void *data = *(const void * const *)(value + 16);
    if (clWriteStream_Write(stream, &size, 8) != 8
        || clWriteStream_Write(stream, data, size) != size)
      return i;
  }
  return count;
}

long long Read(void *stream, long long size, long long *bytes_read)
{
  return clReadStream_Read_llong_llong_ptr(stream, size, bytes_read);
}

void *racer_clVector4_float_divide_assign(void *vector, long long divisor)
  __asm__("clVector4__float_operator/=");
void *racer_clVector4_float_divide_assign(void *vector, long long divisor)
{
  float *components = (float *)vector;
  float value = (float)divisor;
  if (value != 0.0f)
  {
    components[0] /= value;
    components[1] /= value;
    components[2] /= value;
    components[3] /= value;
  }
  return vector;
}
}

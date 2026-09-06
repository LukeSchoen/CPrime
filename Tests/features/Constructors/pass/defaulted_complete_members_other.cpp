#include "defaulted_complete_members.h"
#include <new>
int construct_complete_image_elsewhere() {
  union Storage { char bytes[sizeof(CompleteImage)]; long long alignment; } storage;
  for (unsigned i=0; i<sizeof(storage); ++i) storage.bytes[i]=85;
  CompleteImage* image = new(storage.bytes) CompleteImage;
  int result = image->member.pointer != nullptr || image->size != 7
    || image->elements[1][1].pointer != nullptr || member_live != 5;
  image->~CompleteImage();
  return result;
}

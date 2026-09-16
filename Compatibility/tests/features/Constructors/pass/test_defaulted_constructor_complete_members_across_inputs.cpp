// EXPECT_SOURCES: ["defaulted_complete_members_other.cpp"]
#include "defaulted_complete_members.h"
#include <new>
int member_constructions;
int member_live;
int member_fail_at;
int main() {
  union Storage { char bytes[sizeof(CompleteImage)]; long long alignment; } storage;
  for (unsigned i=0; i<sizeof(storage); ++i) storage.bytes[i]=85;
  CompleteImage* image = new(storage.bytes) CompleteImage;
  if (member_constructions != 5 || image->member.pointer != nullptr || image->size != 7
      || image->elements[1][1].pointer != nullptr || member_live != 5)
    return 1;
  image->~CompleteImage();
  if (member_live || construct_complete_image_elsewhere()) return 2;
  if (member_constructions != 10 || member_live) return 3;
  member_fail_at = 13;
  try { new(storage.bytes) CompleteImage; return 4; }
  catch (int value) { if (value != 9) return 5; }
  return member_live != 0;
}

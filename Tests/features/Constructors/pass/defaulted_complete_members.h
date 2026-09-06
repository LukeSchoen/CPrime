#ifndef TEST_DEFAULTED_COMPLETE_MEMBERS_H
#define TEST_DEFAULTED_COMPLETE_MEMBERS_H
extern int member_constructions;
extern int member_live;
extern int member_fail_at;
struct InitializedMember {
  int *pointer;
  InitializedMember() : pointer(nullptr) {
    if (++member_constructions == member_fail_at) throw 9;
    ++member_live;
  }
  ~InitializedMember() { --member_live; }
};
template<class T> struct CompleteMembers {
  CompleteMembers() = default;
  T member;
  T elements[2][2];
  int size = 7;
};
struct CompleteImage : CompleteMembers<InitializedMember> {
  CompleteImage() = default;
};
int construct_complete_image_elsewhere();
#endif

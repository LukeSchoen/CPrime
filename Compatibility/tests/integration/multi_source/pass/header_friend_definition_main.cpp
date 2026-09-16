#include "header_friend_definition.h"

int ReadFriendBoxFromA(int value);

int main()
{
  FriendBox box{17};
  return ReadFriendBoxFromA(25) == 25 && ReadFriendBox(&box) == 17 ? 0 : 1;
}

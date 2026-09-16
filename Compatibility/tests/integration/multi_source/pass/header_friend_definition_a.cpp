#include "header_friend_definition.h"

int ReadFriendBoxFromA(int value)
{
  FriendBox box{value};
  return ReadFriendBox(&box);
}

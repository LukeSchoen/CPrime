#pragma once

struct FriendBox
{
  int value;

  friend int ReadFriendBox(FriendBox *box)
  {
    return box->value;
  }
};

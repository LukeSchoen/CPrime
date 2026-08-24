struct StaticState
{
  static bool enabled;

  static void Enable()
  {
    enabled = true;
  }

  static void Disable();
};

bool StaticState::enabled = false;

void StaticState::Disable()
{
  enabled = false;
}

int main()
{
  StaticState::Enable();
  if (!StaticState::enabled)
    return 1;
  StaticState::Disable();
  return StaticState::enabled ? 2 : 0;
}

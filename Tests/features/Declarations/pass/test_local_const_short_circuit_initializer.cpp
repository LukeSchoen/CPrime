static int calls;

static bool TrueCall()
{
  ++calls;
  return true;
}

static bool FalseCall()
{
  ++calls;
  return false;
}

int main()
{
  bool condition = false;
  calls = 0;
  const bool disjunction = condition || TrueCall();
  if (!disjunction || calls != 1) return 1;

  condition = true;
  calls = 0;
  const bool short_circuit = condition || TrueCall();
  if (!short_circuit || calls != 0) return 2;

  condition = false;
  calls = 0;
  const bool conjunction = condition && TrueCall();
  if (conjunction || calls != 0) return 3;

  condition = false;
  calls = 0;
  const int integral = condition || TrueCall();
  if (integral != 1 || calls != 1) return 4;

  const bool frozen = true;
  calls = 0;
  const bool from_const = frozen || TrueCall();
  if (!from_const || calls != 0) return 5;

  return 0;
}

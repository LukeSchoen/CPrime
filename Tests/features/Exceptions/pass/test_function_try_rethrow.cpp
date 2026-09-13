// EXPECT_EXIT: 0
int handled;

void raise() try {
    throw 17;
} catch (int value) {
    handled = value;
    throw;
}

int main()
{
    try { raise(); return 1; }
    catch (int value) { return handled == 17 && value == 17 ? 0 : 2; }
}

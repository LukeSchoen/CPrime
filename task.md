add tests to confirm / repro then fix / add support for the following:

- Destructor unwind on break, switch, and mixed nested loop exits.
- Temporary lifetime/destructor order for expression temporaries, operator-chain temporaries, and arguments.
- Copy/move edge cases: implicit copy constructor behavior, move assignment, self-assignment, copy elision-style returns.
- const/non-const receiver preference.
- Constructor member initializer lists.
- nested template instantiation stress
- Possible Operator overload gaps: unary operators, comparison operators, [] const overloads, assignment variants, chained expressions.

# Agent Instructions

When working on the cpc compiler, stay with the task for as long as useful
progress is possible. Prefer sustained, careful investigation and implementation
over stopping early. Keep working through diagnosis, reproduction, fixing, and
verification unless the task is complete or there is a concrete blocker that
requires user input.

Follow this workflow:

1. Check whether each reported symptom is a real issue.
2. Add tests that reproduce the limitation or bug.
3. If the issue reproduces, fix it.
4. Verify the fix against the tests.
5. Continue into related failing cases when they are part of the same compiler
   behavior, and keep iterating until the issue is genuinely handled.
6. Avoid unnecessary churn, but do the hard work needed to reach a complete,
   tested result.

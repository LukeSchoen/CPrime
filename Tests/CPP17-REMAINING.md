# C++17 remaining scope

Working queue only: open gaps and the rules for closing them. Completed work is
code, retained cases are the record of what passes, and git is the record of
what changed. Root `cpc.exe` is the only compiler, one process at a time.

The completion plan below was re-checked against root `cpc.exe` on 2026-09-16
and is the current work order. Re-verify it before trusting an aged queue.

## Test loop

```
Tests\test.exe -All -Tier fast        the loop: the open gaps below, ~0.2s
Tests\test.exe -Suite features/X      one suite, every retained case
Tests\test.exe -Regression            publication gate
scripts\build.exe                     self-host, validate, publish cpc.exe
```

Do not run the pedantic tier. It is close to banned: run it only as the last
and only step of an important confirmation, and avoid it if at all possible.
The affected suite plus `-Regression` is the broad check.

`Tests/tiers.json` lists the fast subset, which is the open work list; every
other retained internal case belongs to the pedantic tier, so a case leaves the
loop by not being listed.
Short self-contained pass cases compile and run in combined units at suite
scale; a combined unit that fails is recompiled and rerun case by case, and
`-GroupSize 1` disables combining.

## The gate is red on purpose

`features/Cpp17Gaps` carries one case per open gap, so the fast tier is red by
exactly the number of open cases. That count is the work list, and it falls as
gaps close.

The open-gap cases are the whole of `Tests/tiers.json`'s fast list, so the
routine loop reports the work list instead of hiding it until the pedantic run,
and a green fast run means the queue is empty. Fast is ~0.2s; pedantic carries
every representative case. A fixed case leaves `fast` by not being listed, and
returns only if the routine loop needs that behavior covered on every pass.

Each case names its facility, so a fix starts by running it:

```
Tests\test.exe -Suite features/Cpp17Gaps
```

## Completion plan

The current fast queue is 21 cases: 20 in `features/Cpp17Gaps` and one compiler
crash. Work in dependency order, retaining one case per gap and deleting its
scratch reproducer as soon as the durable case passes.

| Order | Cases | Required mechanism |
| --- | --- | --- |
| 1 | `features/CompilerCrash/pass/test_selection_initializer_binding.cpp` | Fix the frontend/lowering crash for a structured binding in a selection-statement initializer: `if (auto [a,b] = ...; ...)`. |
| 2 | `features/Cpp17Gaps/pass/test_type_traits_core.cpp` | Add the missing core traits: `is_convertible`, `is_pointer`, `remove_pointer`, `remove_cv_t`, `common_type`, and the missing `_v` templates. |
| 3 | `features/Cpp17Gaps/pass/test_auto_braced_deduction.cpp`, `test_deprecated_enumerator.cpp`, `test_lambda_constant_expression.cpp`, `test_using_declaration_pack_expansion.cpp` | Finish the remaining C++17 language semantics: braced `auto`, enumerator attributes, implicit `constexpr` lambda calls, and using-declaration pack expansion. |
| 4 | `features/Cpp17Gaps/pass/test_aligned_new.cpp`, `test_cstring_std_names.cpp`, `test_initializer_list_pair.cpp` | Complete core runtime surfaces: aligned allocation, standard C names, and `initializer_list<pair<...>>`. |
| 5 | `features/Cpp17Gaps/pass/test_algorithm_cpp17_additions.cpp`, `test_make_from_tuple.cpp`, `test_optional_relational_operators.cpp`, `test_std_ctad_deduction_guides.cpp`, `test_string_view_search.cpp` | Add the missing algorithms, tuple helper, optional comparisons, deduction guides, and string-view search/compare operations. |
| 6 | `features/Cpp17Gaps/pass/test_charconv_float.cpp`, `test_chrono_duration_rounding.cpp` | Implement floating-point `to_chars`/`from_chars` and duration `floor`/`ceil`/`round` plus floating-duration conversion. |
| 7 | `features/Cpp17Gaps/pass/test_map_node_handle.cpp`, `test_polymorphic_allocator.cpp`, `test_shared_ptr_array.cpp`, `test_variant_visit_and_monostate.cpp` | Finish node handles, PMR allocation, `shared_ptr<T[]>`, and variant visitation/traits/comparisons. |
| 8 | `features/Cpp17Gaps/pass/test_filesystem_path_operations.cpp` | Implement filesystem path decomposition, joining, and comparison. |
| 9 | all of the above | Run each selected case, the affected suite, `Tests\test.exe -All -Tier fast`, then `scripts\build.exe` for self-host, packaging, and the regression gate. |

External CPC bug reports that are not part of the C++17 queue are recorded in
`KNOWN-ISSUES.md`.

## Where the cases live

- `features/Abi/pass` - Microsoft x64 layout, nullptr and record-return facts
- `features/Cpp17Gaps/pass` - minimal standalone reproducers for the open gaps;
  every case is a valid program that must compile, link and exit 0
- `features/Declarations/pass`, `features/Statements/pass`,
  `features/Templates/pass`, `features/Includes/pass` - structured bindings,
  selection initializers, variadic class and pack semantics, runtime header and
  container behavior
- `features/StdConcurrency/pass` - mutex, lock and thread behavior

## Rules

- Reproduce first, then repair the shared mechanism, then retain one case. A
  failed case stays in `pass/`, never relabelled as an expected failure.
- Keep tests minimal, deterministic, and fast; reuse an existing case when it
  already proves the behavior.
- Put one-off reproducers in `build/` and delete them once the durable case
  exists.
- Run the exact selected case, then the affected suite, then the fast tier at a
  boundary. Publication does not need the pedantic tier: `scripts\build.exe`
  runs `-Regression`, which is the gate.

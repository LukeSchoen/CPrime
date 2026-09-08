param(
    [ValidateSet('replay', 'lookup', 'substitution', 'members', 'overloads')][string]$Subsystem = 'members',
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = ''
)
$ErrorActionPreference = 'Stop'
# Keep gates small; add the reproducer and its semantic opposite when fixing bugs.
$cases = @{
    overloads = @('test_braced_argument_reference_overload.cpp',
        'test_derived_memberwise_move_assignment.cpp',
        'test_template_braced_reference_overload.cpp',
        'test_braced_reference_unrelated_types_ambiguous.cpp',
        'test_braced_reference_conflicting_preferences.cpp',
        'test_braced_argument_constructor_viability.cpp',
        'test_braced_argument_requires_viable_constructor.cpp',
        'test_enum_integral_promotion_ranking.cpp')
    replay = @('test_member_template_token_payloads.cpp', 'test_template_prepass_replay_edges.cpp',
        'test_nested_function_template_inline_body_survives_argument_replay.cpp')
    lookup = @('test_template_class_replay_lookup_gates.cpp', 'test_friend_template_namespace_redeclaration.cpp',
        'test_friend_function_template_adl.cpp', 'test_friend_function_template_qualified_lookup.cpp')
    substitution = @('test_substitution_declaration_scope_and_recovery.cpp',
        'test_dependent_signature_substitution.cpp', 'test_return_substitution_candidate_eligibility.cpp',
        'test_substitution_function_body_is_hard_error.cpp')
    members = @('test_out_of_class_static_member_template.cpp', 'test_static_member_template_out_of_class_deduction.cpp',
        'test_member_template_does_not_capture_ordinary_definition.cpp',
        'test_template_out_of_class_member_template_definition.cpp', 'test_member_deduction_excludes_old_specializations.cpp',
        'test_reject_duplicate_member_specialization.cpp')
}
$suite = if ($Subsystem -eq 'overloads') { 'features/OperatorOverloads' } else { 'features/Templates' }
& (Join-Path $PSScriptRoot 'run.ps1') -Suite $suite -Select $cases[$Subsystem] -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
exit $LASTEXITCODE

"""Exercise the adapter's classification without invoking a compiler."""
import unittest
from run import assess, directives


class Directives(unittest.TestCase):
    def test_runtime(self):
        self.assertEqual(assess('// { dg-do run }\n// { dg-options "-O2" }'),
                         ('run', ['-O2'], [], False))

    def test_nested_selector_is_not_silently_ignored(self):
        source = '// { dg-do run { target { ! c++98_only } } }'
        self.assertEqual(directives(source), [('dg-do', 'run { target { ! c++98_only } }')])
        self.assertTrue(assess(source)[2])

    def test_diagnostics_are_never_just_expected_nonzero(self):
        result = assess('// { dg-error "expected \\"thing\\" [}]" }')
        self.assertTrue(result[3])
        self.assertTrue(result[2])

    def test_unsupported_options_are_visible(self):
        result = assess('// { dg-options "-std=c++20 -O2 -fconcepts" }')
        self.assertEqual(result[1], ['-O2'])
        self.assertIn('unverified option: -std=c++20', result[2])
        self.assertIn('unverified option: -fconcepts', result[2])

    def test_additional_options(self):
        result = assess('// { dg-options "-O0" } { dg-additional-options "-g" }')
        self.assertEqual(result[1], ['-O0', '-g'])

    def test_assembly_expectations_are_not_passes(self):
        self.assertTrue(assess('// { dg-final { scan-assembler "foo" } }')[2])

    def test_malformed(self):
        self.assertEqual(directives('// { dg-do run')[0][0], 'unclosed-directive')


if __name__ == '__main__':
    unittest.main()

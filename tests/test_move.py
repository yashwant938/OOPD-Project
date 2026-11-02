import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


def snapshot(root):
    return {
        str(path.relative_to(root)): ('directory' if path.is_dir() else hashlib.sha256(path.read_bytes()).hexdigest())
        for path in root.rglob('*') if not path.is_symlink()
    }


class MoveCases:
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='move-lab-test-')
        self.root = Path(self.temp.name).resolve()
        self.source = self.root / 'source'
        self.destination = self.root / 'destination'
        self.source.mkdir()

    def tearDown(self):
        self.temp.cleanup()

    def move(self, source=None, destination=None):
        source = source or self.source
        destination = destination or self.destination
        # Each mutating test is confined to its newly created temporary tree.
        self.assertTrue(source.resolve().is_relative_to(self.root))
        self.assertTrue(destination.resolve().is_relative_to(self.root))
        return subprocess.run([str(self.binary), 'mv', str(source), str(destination)], capture_output=True, text=True, timeout=10)

    def seed(self):
        (self.source / 'visible.txt').write_text('visible data')
        (self.source / '.hidden.txt').write_text('hidden data')
        (self.source / '.hidden-directory').mkdir()
        (self.source / '.hidden-directory' / 'nested.txt').write_text('nested hidden data')
        (self.source / 'nested').mkdir()
        (self.source / 'nested' / '.nested-hidden.txt').write_text('nested dotfile')
        (self.source / 'empty').mkdir()
        (self.source / '.empty-hidden').mkdir()

    def test_hidden_files_directories_and_empty_entries_are_preserved(self):
        self.seed()
        expected = snapshot(self.source)
        result = self.move()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.source.exists())
        self.assertEqual(snapshot(self.destination), expected)

    def test_empty_source_creates_an_empty_destination(self):
        result = self.move()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.source.exists())
        self.assertTrue(self.destination.is_dir())
        self.assertEqual(list(self.destination.iterdir()), [])

    def test_failed_nested_copy_preserves_the_entire_source(self):
        self.seed()
        expected = snapshot(self.source)
        self.destination.mkdir()
        conflict = self.destination / '.hidden-directory'
        conflict.write_text('existing destination data')
        result = self.move()
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn('moved successfully', result.stdout)
        self.assertEqual(snapshot(self.source), expected)
        self.assertEqual(conflict.read_text(), 'existing destination data')

    def test_equal_or_nested_destination_is_rejected_before_mutation(self):
        self.seed()
        expected = snapshot(self.source)
        for target in (self.source, self.source / 'nested' / 'destination'):
            with self.subTest(target=target):
                result = self.move(destination=target)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn('moved successfully', result.stdout)
                self.assertEqual(snapshot(self.source), expected)

    def test_regular_file_move_still_works(self):
        source = self.source / 'file.txt'
        source.write_text('file data')
        self.destination.mkdir()
        result = self.move(source=source)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(source.exists())
        self.assertEqual((self.destination / 'file.txt').read_text(), 'file data')

    def test_unsupported_symlink_does_not_remove_source_data(self):
        self.seed()
        expected = snapshot(self.source)
        outside = self.root / 'outside.txt'
        outside.write_text('outside synthetic data')
        link = self.source / 'link'
        try:
            link.symlink_to(outside)
        except OSError:
            self.skipTest('symlink creation is unavailable')
        result = self.move()
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(link.is_symlink())
        self.assertEqual(snapshot(self.source), expected)
        self.assertEqual(outside.read_text(), 'outside synthetic data')

    def test_missing_source_and_missing_arguments_return_failure(self):
        result = self.move(source=self.root / 'missing')
        self.assertNotEqual(result.returncode, 0)
        result = subprocess.run([str(self.binary), 'mv'], capture_output=True, text=True, timeout=5)
        self.assertNotEqual(result.returncode, 0)


class SequentialMoveTests(MoveCases, unittest.TestCase):
    binary = Path(os.environ.get('MOVE_Q1_BINARY', './Q1')).resolve()


class ThreadedMoveTests(MoveCases, unittest.TestCase):
    binary = Path(os.environ.get('MOVE_Q3_BINARY', './Q3')).resolve()


if __name__ == '__main__':
    unittest.main()

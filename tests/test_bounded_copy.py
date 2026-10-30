import os
from pathlib import Path
import subprocess
import tempfile
import unittest


BINARY = Path(os.environ.get("COPY_LAB_BINARY", "./bounded_copy")).resolve()


class CopyLabTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="copy-lab-test-")
        self.root = Path(self.tmp.name)
        self.src = self.root / "source"
        (self.src / "nested" / "deep").mkdir(parents=True)
        (self.src / "hello.txt").write_text("hello", encoding="utf-8")
        (self.src / "nested" / "deep" / "data.bin").write_bytes(bytes(range(256)))

    def tearDown(self):
        self.tmp.cleanup()

    def run_copy(self, dest, workers):
        return subprocess.run([str(BINARY), str(self.src), str(dest), str(workers)], capture_output=True, text=True)

    def test_sequential_and_pool_copy_the_same_tree(self):
        for workers in (0, 1, 4):
            dest = self.root / f"destination-{workers}"
            result = self.run_copy(dest, workers)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((dest / "hello.txt").read_text(), "hello")
            self.assertEqual((dest / "nested" / "deep" / "data.bin").read_bytes(), bytes(range(256)))

    def test_existing_or_nested_destination_is_rejected(self):
        self.assertNotEqual(self.run_copy(self.src, 4).returncode, 0)
        self.assertNotEqual(self.run_copy(self.src / "nested-output", 4).returncode, 0)
        self.assertEqual((self.src / "hello.txt").read_text(), "hello")

    def test_invalid_worker_count_is_rejected(self):
        for count in (-1, 33, "4oops"):
            self.assertNotEqual(self.run_copy(self.root / "bad", count).returncode, 0)

    def test_symlinks_are_rejected(self):
        try:
            (self.src / "link").symlink_to(self.src / "hello.txt")
        except OSError:
            self.skipTest("symlink creation unavailable")
        result = self.run_copy(self.root / "link-output", 2)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Symlinks", result.stderr)


if __name__ == "__main__":
    unittest.main()

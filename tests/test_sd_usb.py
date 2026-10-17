import base64
import importlib.util
import tempfile
import unittest
import zlib
from pathlib import Path

spec = importlib.util.spec_from_file_location("sd_copy", Path(__file__).parents[1] / "tools/copy_sd_usb.py")
copy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(copy)
spec2 = importlib.util.spec_from_file_location("usgs_tiles", Path(__file__).parents[1] / "tools/download_usgs_tiles.py")
tiles = importlib.util.module_from_spec(spec2)
spec2.loader.exec_module(tiles)

class FakeExport:
    def __init__(self, data, bad_chunk_once=False, bad_final=False):
        self.data, self.lines = data, []
        self.bad_chunk_once, self.bad_final = bad_chunk_once, bad_final

    def write(self, raw):
        words = raw.decode().split()
        verb, token = words[:2]
        if verb == "sd-open":
            result = f"OPEN {len(self.data)}"
        elif verb == "sd-close":
            result = "CLOSED"
        elif verb == "sd-read":
            offset = int(words[2])
            if offset == len(self.data):
                result = "EOF " + ("deadbeef" if self.bad_final else f"{zlib.crc32(self.data):08x}")
            else:
                block = self.data[offset:offset+24576]
                crc = "deadbeef" if self.bad_chunk_once else f"{zlib.crc32(block):08x}"
                self.bad_chunk_once = False
                result = f"DATA {offset} {crc} {base64.b64encode(block).decode()}"
        else:
            raise AssertionError(verb)
        response = f"[SDUSB] {token} {result}\n".encode()
        self.lines.extend([b"[SD] logging\n", response[:19], response[19:]])
        return len(raw)

    def flush(self):
        pass

    def read_until(self, _):
        return self.lines.pop(0) if self.lines else b""

class SdCopyTests(unittest.TestCase):
    def setUp(self):
        build = Path(__file__).parents[1] / "build/test-sd-copy"
        build.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=build)
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_copy_retries_bad_chunk_and_checks_destination(self):
        data = bytes(range(256))*512
        target = self.root / "s3-lora/test.bin"
        record = copy.SdPort(FakeExport(data, bad_chunk_once=True)).copy_file("/s3-lora/test.bin", target)
        self.assertEqual(target.read_bytes(), data)
        self.assertEqual(record["crc32"], f"{zlib.crc32(data):08x}")

    def test_empty_file(self):
        target = self.root / "empty"
        copy.SdPort(FakeExport(b"")).copy_file("/empty", target)
        self.assertEqual(target.read_bytes(), b"")

    def test_source_crc_failure_preserves_existing_destination(self):
        target = self.root / "old"
        target.write_bytes(b"keep")
        with self.assertRaisesRegex(copy.CopyError, "complete source file checksum"):
            copy.SdPort(FakeExport(b"new", bad_final=True)).copy_file("/old", target)
        self.assertEqual(target.read_bytes(), b"keep")

    def test_different_destination_is_backed_up(self):
        target = self.root / "old"
        target.write_bytes(b"keep")
        copy.SdPort(FakeExport(b"new")).copy_file("/old", target)
        self.assertEqual(target.read_bytes(), b"new")
        self.assertEqual((self.root / "old.before-usb-copy").read_bytes(), b"keep")

    def test_path_traversal_rejected(self):
        for source in ["relative/file", "/../escape", "/s3-lora/../../escape", "/C:/file", "/s3-lora\\bad"]:
            with self.assertRaises(copy.CopyError):
                copy.destination_path(self.root, source)
        self.assertEqual(copy.destination_path(self.root, "/s3-lora/a"), self.root / "s3-lora/a")

    def test_rgb565_primary_colors_match_device_byte_order(self):
        image = tiles.Image.new("RGB", (3, 1))
        image.putdata([(255,0,0), (0,255,0), (0,0,255)])
        self.assertEqual(tiles.rgb565_bytes(image), b"\x00\xf8\xe0\x07\x1f\x00")

if __name__ == "__main__":
    unittest.main()

import base64
import importlib.util
import tempfile
import unittest
import zlib
from pathlib import Path

spec = importlib.util.spec_from_file_location("usb_tiles", Path(__file__).parents[1] / "tools/upload_tiles_usb.py")
usb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(usb)


class FakePort:
    def __init__(self, *, skip=False, bad_ack=False, bad_crc=False):
        self.lines = []
        self.commands = []
        self.data = bytearray()
        self.skip, self.bad_ack, self.bad_crc = skip, bad_ack, bad_crc

    def write(self, data):
        command = data.decode().strip()
        self.commands.append(command)
        parts = command.split()
        verb = parts[0]
        token = parts[1] if len(parts) > 1 else "00000000"
        if verb == "tile-hello":
            result = "HELLO protocol=1 bytes=131072 chunk=3072"
        elif verb == "tile-abort":
            result = "ABORTED"
        elif verb == "tile-begin":
            self.expected_crc = parts[5]
            result = "SKIP " + parts[5] if self.skip else "READY"
        elif verb == "tile-data":
            assert int(parts[2]) == len(self.data)
            self.data.extend(base64.b64decode(parts[3], validate=True))
            result = f"ACK {len(self.data) + int(self.bad_ack)}"
        elif verb == "tile-end":
            assert f"{zlib.crc32(self.data):08x}" == self.expected_crc
            result = "DONE " + ("deadbeef" if self.bad_crc else self.expected_crc)
        else:
            raise AssertionError(command)
        # Interleaved diagnostics and an unrelated session must not count as acknowledgements.
        unrelated = f"[TILEUSB] {int(token, 16) ^ 0x80000000:08x} ACK 1\n".encode()
        self.lines.extend([b"[MEM] psram=6000KB\n", unrelated])
        reply = f"[TILEUSB] {token} {result}\n".encode()
        self.lines.extend([reply[:13], reply[13:]])  # A readline timeout can split a reply.
        return len(data)

    def flush(self):
        pass

    def readline(self):
        return self.lines.pop(0) if self.lines else b""


class UsbTileTests(unittest.TestCase):
    def setUp(self):
        temp_root = Path(__file__).parents[1] / "build/test-usb-tiles"
        temp_root.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=temp_root)
        self.root = Path(self.temp.name)
        self.tile = self.root / "14/4000/5000.rgb565"
        self.tile.parent.mkdir(parents=True)
        self.data = bytes(range(256)) * 512
        self.tile.write_bytes(self.data)

    def tearDown(self):
        self.temp.cleanup()

    def test_roundtrip_with_interleaved_and_partial_replies(self):
        port = FakePort()
        client = usb.TilePort(port, timeout=1)
        client.connect()
        self.assertEqual(client.upload(self.tile, (14, 4000, 5000)), "uploaded")
        self.assertEqual(port.data, self.data)
        self.assertEqual(len([c for c in port.commands if c.startswith("tile-data ")]), 43)

    def test_resume_skips_without_sending_data(self):
        port = FakePort(skip=True)
        self.assertEqual(usb.TilePort(port).upload(self.tile, (14, 4000, 5000)), "skipped")
        self.assertEqual(port.data, b"")

    def test_bad_ack_aborts_without_commit(self):
        port = FakePort(bad_ack=True)
        with self.assertRaisesRegex(usb.UploadError, "offset mismatch"):
            usb.TilePort(port).upload(self.tile, (14, 4000, 5000))
        self.assertTrue(port.commands[-1].startswith("tile-abort "))
        self.assertFalse(any(c.startswith("tile-end ") for c in port.commands))

    def test_bad_final_crc_rejected(self):
        with self.assertRaisesRegex(usb.UploadError, "final checksum mismatch"):
            usb.TilePort(FakePort(bad_crc=True)).upload(self.tile, (14, 4000, 5000))

    def test_reject_invalid_paths_and_sizes_before_usb(self):
        for name in ["15/0/0.rgb565", "10/1024/0.rgb565", "10/-1/0.rgb565", "../../0.rgb565", "14/2/3.png"]:
            with self.assertRaises(usb.UploadError):
                usb.tile_coordinates(self.root / name, self.root)
        self.tile.write_bytes(b"bad")
        port = FakePort()
        with self.assertRaisesRegex(usb.UploadError, "wrong tile size"):
            usb.TilePort(port).upload(self.tile, (14, 4000, 5000))
        self.assertEqual(port.commands, [])


if __name__ == "__main__":
    unittest.main()

"""Execute current PAYLOAD formatter paths on the host with transport stubs.

Only Arduino strings, output, input field formatting and encryption are stubbed;
logger and formatter control flow is extracted unchanged from production source.
This does not validate the crypto algorithm or replace a firmware build.
"""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

STUBS = r'''
#include <cassert>
#include <string>
#include <vector>
using __FlashStringHelper = char;
#define F(text) text
class String : public std::string {
public:
  using std::string::string;
  String() = default;
  String(const std::string& value) : std::string(value) {}
  String(size_t value) : std::string(std::to_string(value)) {}
  bool isEmpty() const { return empty(); }
  bool startsWith(const char* prefix) const { return rfind(prefix, 0) == 0; }
  friend String operator+(String left, const String& right) { left += right; return left; }
};
struct JsonDocument {};
struct SerialSink {
  std::vector<String> lines;
  void println(const String& line) { lines.push_back(line); }
} Serial;
void debug_outln_error(const char*) {}
void debug_outln_verbose(const String&, const String&) {}
void debug_outln_verbose(const String&) {}
bool encrypt_enabled = false;
bool fields_ok = true;
int append_calls = 0;
String plain = "t:24.5";
String wire = "t:e.CIPHERTEXT";
String encrypted_record = "e.RECORD";
bool anyMetricEncryptionEnabled() { return encrypt_enabled; }
String valueCryptoEncryptValue(const String&) { return encrypted_record; }
bool appendRobonomicsFields(JsonDocument&, String& out, bool per_field, bool* encrypted = nullptr) {
  ++append_calls;
  if (!fields_ok) return false;
  if (encrypted) *encrypted = encrypt_enabled;
  out = per_field && encrypt_enabled ? wire : plain;
  return true;
}
'''

CASES = r'''
void expectLog(const char* channel, const char* encoding, bool encrypted, size_t length) {
#ifdef ALTRUIST_BUILD_DEBUG
  assert(Serial.lines.size() == 1);
  String expected = String("[PAYLOAD] channel=") + channel + " encoding=" + encoding
      + " encrypted=" + (encrypted ? "1" : "0") + " payload_len=" + String(length);
  assert(Serial.lines[0] == expected);
#else
  assert(Serial.lines.empty());
#endif
  Serial.lines.clear();
}
int main() {
  JsonDocument data;
  String out;
  for (bool encryption : {false, true}) {
    encrypt_enabled = encryption;
    append_calls = 0;
    assert(formatRobonomicsString(data, out, F("sensors-connectivity")));
    assert(out == (encryption ? wire : plain));
    assert(append_calls == 1); // No second plaintext render solely for logging.
    expectLog("sensors-connectivity", encryption ? "mixed" : "plain", encryption, out.length());
  }
  fields_ok = false;
  assert(!formatRobonomicsString(data, out, F("sensors-connectivity")));
  assert(Serial.lines.empty());
  assert(formatRobonomicsDatalogString(data, out) == DATALOG_FORMAT_ENCRYPTION_FAILED);
  assert(out.empty() && Serial.lines.empty());
  fields_ok = true;
  plain = "";
  assert(formatRobonomicsDatalogString(data, out) == DATALOG_FORMAT_PAYLOAD_EMPTY);
  assert(out.empty());
  expectLog("datalog", "plain", false, 0);
  plain = "t:24.5";
  encrypt_enabled = false;
  assert(formatRobonomicsDatalogString(data, out) == DATALOG_FORMAT_PLAIN);
  assert(out == plain);
  expectLog("datalog", "plain", false, out.length());
  plain = String(DATALOG_CHAIN_SAFE_BYTES + 1, 'x');
  assert(formatRobonomicsDatalogString(data, out) == DATALOG_FORMAT_PAYLOAD_TOO_LARGE);
  assert(out.empty());
  expectLog("datalog", "plain", false, plain.length());
  plain = "t:24.5";
  encrypt_enabled = true;
  assert(formatRobonomicsDatalogString(data, out) == DATALOG_FORMAT_CPS);
  assert(out == encrypted_record);
  expectLog("datalog", "cps", true, out.length());
  encrypted_record = String("e.") + String(DATALOG_CHAIN_SAFE_BYTES, 'x');
  assert(formatRobonomicsDatalogString(data, out) == DATALOG_FORMAT_PAYLOAD_TOO_LARGE);
  assert(out.empty());
  expectLog("datalog", "cps", true, encrypted_record.length());
  encrypted_record = "invalid";
  assert(formatRobonomicsDatalogString(data, out) == DATALOG_FORMAT_ENCRYPTION_FAILED);
  assert(out.empty());
  expectLog("datalog", "cps", true, 0);
}
'''


class PayloadLogging(unittest.TestCase):
    def test_metadata_only_logs_preserve_formatter_results(self):
        source = (ROOT / "apis/helpers/message_formatter.cpp").read_text()
        header = (ROOT / "apis/helpers/message_formatter.h").read_text()
        crypto = (ROOT / "apis/helpers/value_crypto.h").read_text()
        functions = source[source.index("static void logPayload("):source.index("void addTimeAndSign(")]
        limits = "\n".join(re.findall(r"^static constexpr size_t DATALOG_CHAIN_.*;$", source, re.M))
        statuses = re.search(r"enum DatalogFormatStatus\s*\{.*?\};", header, re.S).group()
        prefix = re.search(r"^constexpr const char \*VALUE_CRYPTO_CPS_PREFIX.*;$", crypto, re.M).group()
        program = STUBS + limits + statuses + prefix + functions + CASES
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)
            cpp = path / "payload.cpp"
            cpp.write_text(program)
            for debug in (False, True):
                with self.subTest(debug=debug):
                    executable = path / ("debug" if debug else "release")
                    subprocess.run(["g++", "-std=c++17", *(["-DALTRUIST_BUILD_DEBUG"] if debug else []),
                                    str(cpp), "-o", str(executable)], check=True, capture_output=True, text=True)
                    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()

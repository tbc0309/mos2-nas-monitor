const fs = require('fs'), vm = require('vm'), assert = require('assert');
const crcSource = fs.readFileSync('web/management.js', 'utf8').split('\n').find(s => s.startsWith('function firmwareCRC('));
vm.runInThisContext(crcSource + '\n' + fs.readFileSync('web/firmware-bundle.js', 'utf8') + '\nglobalThis.bundle = FirmwareBundle;');
const raw = new Uint8Array(5000).fill(255); raw[0] = 0xe9;
assert.strictEqual(bundle.program(raw), raw);
const full = new Uint8Array(0x3fa000).fill(255);
full.set(raw);full.set(Buffer.from('MOS2ALL1'), 0x100000);
const header = new DataView(full.buffer, 0x100000, 24);
header.setUint32(8, raw.length, true);
header.setUint32(12, parseInt(firmwareCRC(raw), 16), true);
header.setUint32(16, parseInt(firmwareCRC(full.slice(0x200000)), 16), true);
header.setUint32(20, 1, true);
assert.deepStrictEqual(bundle.program(full), raw);
for (const offset of [0, 50, 0x100000, 0x100008, 0x200001, 0x3f9fff]) {
  const corrupt = full.slice(); corrupt[offset] ^= 1;
  assert.throws(() => bundle.program(corrupt));
}
assert.throws(() => bundle.program(full.slice(0, -1)));
console.log('PASS: unified image extraction, program/resource corruption and truncation');

function firmwareCRC(bytes) {
  let crc = 0xffffffff;
  for (const b of bytes) {
    crc ^= b;
    for (let i = 0; i < 8; i++) crc = (crc >>> 1) ^ (crc & 1 ? 0xedb88320 : 0);
  }
  return ((crc ^ 0xffffffff) >>> 0).toString(16).padStart(8, "0");
}
// The install image is also the update container. Only its verified application
// is sent to OTA; the installed compatible resources and user data stay in place.
const FirmwareBundle = {
  program(bytes) {
    if (bytes[0] !== 0xe9) throw new Error("invalid");
    if (bytes.length <= 0x100000) return bytes; // Existing program-only uploads.
    if (bytes.length !== 0x3fa000) throw new Error("invalid");
    const offset = 0x100000;
    if (String.fromCharCode(...bytes.slice(offset, offset + 8)) !== "MOS2ALL1")
      throw new Error("invalid");
    const view = new DataView(bytes.buffer, bytes.byteOffset + offset, 24);
    if (view.getUint32(20, true) !== 1) throw new Error("invalid");
    const length = view.getUint32(8, true);
    if (length < 4096 || length > offset) throw new Error("size");
    const crc = (part) => parseInt(firmwareCRC(part), 16);
    const program = bytes.slice(0, length);
    if (
      crc(program) !== view.getUint32(12, true) ||
      crc(bytes.slice(0x200000)) !== view.getUint32(16, true)
    )
      throw new Error("invalid");
    return program;
  },
};

// The page's file index for fs_threads: one file of 80 blocks and 1000 bytes, byte p = (p * 31 + p / 251) & 255.
Module['KBFS'] = {
  ready: true,
  bytes: 80 * 262144 + 1000,
  open: () => 1,
  size() { return this.bytes; },
  pread(id, offset, len) {
    const out = new Uint8Array(Math.max(0, Math.min(offset + len, this.bytes) - offset));
    for (let i = 0; i < out.length; i++) out[i] = ((offset + i) * 31 + Math.floor((offset + i) / 251)) & 255;
    return out;
  },
  close() {},
};

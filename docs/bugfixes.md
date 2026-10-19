# Preview maintenance fixes

These changes were tested on the host and cross-compiled for PS5. They do not
establish console execution or implement the missing Xbox USB/emulator backend.

- Configuration saves now write and close a temporary file before replacing
  the old one. Failed writes/renames preserve the previous configuration.
- Configuration loading is bounded, rejects embedded NULs, overflowing numbers,
  incomplete fields and trailing junk, and leaves the current settings intact
  on failure.
- Framebuffer submission now includes the CPU store fence used by the SDL PS5
  reference implementation for write-combined video memory. Empty flip events
  are rejected, and display failures return a failure status instead of success.
- The installer rejects noncanonical manifest paths and command delimiters,
  validates manifest structure, treats an explicitly supplied port 0 as invalid,
  and reports unexpected connection closure without an uncaught traceback.

Regression tests cover damaged configurations, blocked temporary files,
failed renames and malformed installer manifests. Existing local FTP install,
update, backup and rollback tests continue to pass. See home-screen-install.md
for the still-required first console test.

# Third-party notices

## libusb

- Source: https://github.com/libusb/libusb
- Revision: `v1.0.30` (`87a55632db62c9bdc58cd31d3ccfa673f1bb017f`)
- License: GNU Lesser General Public License v2.1 or later
- Integration: built as a separate shared library and packaged in the AAR

The complete license text is available in `third_party/libusb/COPYING`.

## stlink

- Source: https://github.com/stlink-org/stlink
- Revision: `6a1d36530b7ec0004498fb80c947c46e6c537134`
- License: BSD 3-Clause
- Integration: required library sources are compiled into the project native library; Android
  adapter files remain separate project sources

The complete license text is available in `third_party/stlink/LICENSE.md`.

These dependencies are pinned as Git submodules. Update a revision only with an Android build,
license review, native contract tests, and physical-hardware revalidation.

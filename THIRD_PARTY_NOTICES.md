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

## rtl-sdr

- Source: https://gitea.osmocom.org/sdr/rtl-sdr
- Revision: `v2.0.3` (`797f8143266d983c56d8f35d2d442527529dd8a5`)
- License: GNU General Public License v2.0 or later
- Integration: the `RtlSdrForAndroid` example compiles the R82xx tuner implementation into its
  native example library; it is not linked into or packaged with the publishable Android AAR

The complete license text is available in
`rtlSdrForAndroid/src/main/cpp/third_party/rtl-sdr/COPYING`.

These dependencies are pinned as Git submodules. Update a revision only with an Android build,
license review and relevant hardware revalidation. Changes affecting the publishable transport
library additionally require its native contract tests.

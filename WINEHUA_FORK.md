# WineHua DXVK fork

This directory is the editable WineHua fork of DXVK 1.10.3.

- Branch: `winehua/dxvk-legacy-1.10.3`
- Base tag: `v1.10.3`
- Build outputs: `build/dxvk/legacy/x64/bin` and `build/dxvk/legacy/x86/bin`
- Incremental build: `make dxvk NATIVE_ARCH=arm64-v8a GUEST_ARCH=x86_64`

The WineHua compatibility path is opt-in through
`WINEHUA_DXVK_RELAXED_FEATURES=1`. It permits D3D11 device creation when the
current Venus driver lacks BC formats or transform feedback, but it does not
emulate those resources. The product selector must keep this profile separate
from a fully qualified DXVK profile until format and stream-output support are
implemented.

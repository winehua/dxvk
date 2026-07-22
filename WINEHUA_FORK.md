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

Bool sampled-descriptor specialization is selected automatically from the
Vulkan adapter name for the current `Venus`/`Maleoon` path. The old
`DXVK_WINEHUA_FREEZE_BOOL_SPEC=1` setting remains a debug force-on override;
`0` disables it, and `WINEHUA_DXVK_QUIRKS=venus-bool-spec` can opt in a new
Venus adapter before its name is added to the built-in policy.

Maleoon's fragment compiler requires a padded vec4 coordinate for non-array
Cube `OpImageSampleDref*`; the otherwise equivalent minimal vec3 form produces
an all-one result. The fork enables this automatically when the Vulkan adapter
name contains `Maleoon`. `WINEHUA_DXVK_PAD_CUBE_DREF_COORD=0/1` is the explicit
debug override, and `WINEHUA_DXVK_QUIRKS=maleoon-cube-dref` can force it for a
new adapter identity. Other GPUs retain the standard minimal coordinate path.

Maleoon also hangs the Host Venus ring when a shader executes native
CubeArray shadow/Dref instructions. The fork analyzes DXBC before declaration,
marks only CubeArray resources used by comparison sampling, binds the existing
2D-array alternate view, and converts (direction, cubeIndex) to
(uv, cubeIndex * 6 + face) in the shader. The operation remains a native
2D-array OpImageSampleDref, preserving comparison-before-filtering behavior.
The policy is automatic for Maleoon;
WINEHUA_DXVK_EMULATE_CUBE_ARRAY_DREF=0/1 is the debug override, and
WINEHUA_DXVK_QUIRKS=maleoon-cube-array-dref can force it for another adapter.
Native CubeArray sampling remains the default for all other GPUs.

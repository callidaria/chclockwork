## Changelog

### v0.0.3 (DEV)

- single & multi-texture skyboxing
- hdr float texture support
- FIXED: pipelines conform their render stage, this result in a single pass for n registered pipelines


### v0.0.2 (UNX)

- porting render features to vulkan
- performance improvements
- architectural improvements
- FIXED: texture atlas segmentation


### v0.0.1 (WIN)

- initial basic feature engine rip
- animation load and interpolation
- interpolation between unrelated animations
- single joint manipulation through independent animation updates
- wheel system update registration simplicity improvements
- custom shadow passes
- improved build setup
- improved time conversion syntax
- FIXED: interpolation jump when repeating looped animation
- FIXED: issues with delta time accuracy
- FIXED: dT blowout when device hits low frequencies

# Textures


§1 Choosing Storetype
There are two different texture store types available. Singular texture (GPUTexture) and a texture atlas (GPUPixelBuffer). \
Whenever a single draw call has a predictable amount of textures, a GPUTexture should be used.
An example for this is the draw call to a mesh, using a fixed amount of material channels (albedo, normals, materials & emission). \
Should a drawcall be utilizing a dynamically determined amount of different textures, then the GPUPixelBuffer may be used.
The engine provides a precedent for this utilization, through the combination of all UI sprite and animation elements into one draw call.
This call therefore also needs a predictable amount of textures to be read from (no sparseless is allowed for the default use case, to
provide support for older systems). All sprite and animation textures are stored into the texture atlas of the GPUPixelBuffer and indexed
by texture coordinate. \

TODO better: there will be two available, once this part is optimized accordingly \


$2 Modi of the Texture System
Due to big differences in driver- & hardware-side modernization there must be two different texture handling systems,
depending on the device capabilities. Such is the curse of the PC developer. \
1) The modern way of handling textures is probably through sparse binding, although not available on all devices. \
2) Binding pre-updated descriptor sets as materials, for each draw call (OpenGL-esque) \

TODO: sparse binding is not implemented yet, everything is managed by the fallback system for now.

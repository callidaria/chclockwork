# Upload & Processing of Static Geometry


TODO old brainstorming session, many of those ideas are not up-to-date any more!


§1 Buffers \
.1 creating a buffer should be done by generating a buffer, uploading to it and having a few options,
when and if to include index buffers might be offered here, but that really just happens at bind \

whether a buffer is staged, mapped and unmapped is a hugely important distinction.
there is a difference between vertex buffers, only in the case of upload additional index information \

vertex buffers (as they are) only hold geometrical information, fed to the shaders. this is their only purpose.
the vertex buffer should store it's offset, so that authoritative callers can utilize them when uploading \

the default case for upload is always through a staging buffer, this seems to be the fastest way.
this includes all vbos, those that stay the same throughout and also dynamic instance data buffers \

vertex buffer memory is static and can be freed immediately, while instance memory needs to stay active.
this suggests, that the staging/upload and the unmap/free call are to be separated, depending on usage. \
?? is this actually true

also there must be a feature to directly update data in-memory \
copy must be part of an update function, that can be called once in case of vb or per-frame in case of ib \
The specified struct is the VertexBuffer in the memory component.

?? maybe it should be possible to store multiple geometric buffers into a single vbo
?? research if streaming into a single global vbo for all equi-material wg is an efficient alternative


§2 Vertex Arrays \
to bind and upload the buffers they will be called by bind vertex/index buffer commands descriptively
the vertex array should not be a distict structure or class, it only acts as an analogue to ogl
during the bind process, the cmd_buffer in framebuffer will be used to store the command until execution
this raises the important question if the framebuffer as such should take care of buffer upload


§3 Static Pipelines \
the pipeline should work with the buffers and uniforms, but it should NOT be defined by them.
the pipeline setup will consist of only direct decisions and automatically analyzed shader states
an example of the latter is already implemented in the ogl version:
the pipeline will read the defined needs by the shader and adopt the desired upload structure
it is then the duty of the buffers to provide the requested data, they have no influence over the demands


§4 Framebuffer Rendertarget \
the rendertarget is supposed to be !the! command authority. all draw commands belong to a target
a rendertarget should also stand on it's own feet and should not obey any other authority, besides mutex
this should suggest that the framebuffer rendertarget and the blitter are to be unified into one
for each target there is a respective command buffer, as there should be when flipping asynchronously
but right now blitter and framebuffer are in a dire war over resource authority, that should be sorted


§5 Blitter \
the blitter should only receive what is designated as the final result in form of a rendertarget
first it has to create the frame, setup swapchain et cetera and then it should draw the target
this is the full extent of the blitters purpose


§6 Hardware Interface \
the hardware interface is supposed to be handling all frees and mundane creations
creations that require a complex info struct setup are not to be misinterpreted as mundane
the hi is supposed to automatically scan for relevant hardware and test it for support upon inclusion
also the hi should globally store the gpu, so that it is accessible to all following components

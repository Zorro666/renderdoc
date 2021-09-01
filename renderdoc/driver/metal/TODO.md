## TODO
- Need to implement : ResourceManager::ResourceTypeRelease

- Need to capture buffer data in setVertexData
- Do I need to implement and call : ResourceManager::AddWrapper
- Should add specific serialisation for WrappedTypes instead of hard coded serialise by RefId
-- MTLRenderCommandEncoder
-- MTLCommandBuffer
-- MTLFunction
- Need to implement replay
-- MTLCommandBuffer presentDrawable
- Need to serialise presentDrawable with more details

- Need to investigate how to associate the window/drawable with the device
-- Wrap MTLDrawable properly

- Need to hook into Metal present : no MTL command except this
	[commandBuffer presentDrawable:drawable];
	[commandBuffer commit];

- Add ser.SetActionChunk() to Metal captures
- Choose correct FrameRefType properly for Metal captures (right now defaulting to eFrameRef_Read)
- Add .Important() to Metal serialisation

## Questions 
- Does MTLRenderPassDescriptor::renderPassDescriptor static method need hooking?

## Currently Hooked APIs
id_MTLLibrary = MTLDevice::newDefaultLibrary()
* id MTLLibrary
* uint32_t bytesCount
* void* pData

id_MTLBuffer = MTLDevice::newBufferWithBytes(const void *pointer, NSUInteger length, MTLResourceOptions options);
* id MTLBuffer
* NSUInteger length
* MTLResourceOptions options

id_MTLRenderPipelineState = MTLDevice::newRenderPipelineStateWithDescriptor(MTLRenderPipelineDescriptor *descriptor, NSError **error);
* id RenderPipelineState
* MTLRenderPipelineDescriptor descriptor

id_MTLCommandQueue = MTLDevice::newCommandQueue();
* id MTLCommandQueue

id_MTLTexture = MTLDevice::newTextureWithDescriptor(MTLTextureDescriptor *descriptor, IOSurfaceRef iosurface, NSUInteger plane);
* id MTLTexture
* MTLTextureDescriptor descriptor
* uint64_t iosurface
* NSUInteger plane

id_MTLTexture MTLDevice::newTextureWithDescriptor(MTLTextureDescriptor *descriptor);
* id MTLTexture
* MTLTextureDescriptor descriptor
* uint64_t iosurface = 0x0
* NSUInteger plane = ~0

// This is the only way to create a MTLRenderCommandEncoder
// This should be serialisation of MTLRenderCommandEncoder not hardcoded
id_MTLRenderCommandEncoder = MTLCommandBuffer::renderCommandEncoderWithDescriptor(MTLRenderPassDescriptor *);
* id MTLCommandBuffer
* id MTLRenderCommandEncoder
* MTLRenderPassDescriptor descriptor

MTLCommandBuffer::presentDrawable(id_MTLDrawable drawable);
* id MTLCommandBuffer

MTLCommandBuffer::commit();
* id MTLCommandBuffer

// This is the only way to create a MTLCommandBuffer
// This should be serialisation of MTLCommandBuffer not hardcoded
id_MTLCommandBuffer = MTLCommandQueue::commandBuffer();
* id MTLCommandQueue
* id MTLCommandBuffer

id_MTLFunction = MTLLibrary::newFunctionWithName(NSString *functionName);
* id MTLLibrary
* id MTLFunction
* NSString functionName

MTLRenderCommandEncoder::setRenderPipelineState(id_MTLRenderPipelineState pipelineState);
* id MTLRenderCommandEncoder
* MTLRenderPipelineState pipelineState

MTLRenderCommandEncoder::setVertexBuffer(id_MTLBuffer buffer, NSUInteger offset, NSUInteger index);
* id MTLRenderCommandEncoder
* id Buffer
* NSUInteger offset
* NSUInteger index

MTLRenderCommandEncoder::drawPrimitives(MTLPrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount);
* id MTLRenderCommandEncoder
* MTLPrimitiveType primitiveType
* NSUInteger vertexStart
* NSUInteger vertexCount
* NSUInteger instanceCount

MTLRenderCommandEncoder::endEncoding();
* id MTLRenderCommandEncoder

## Questions to discuss with Baldur

## Future
- Find all the APIs which are be used to create MTLLibrary : identify what needs hooking
- Change the property implementations to be APIs on the C++ object and obj-c is trampoline to C++
- ObjCWrappedMTLBuffer.remoteStorageBuffer should return an obj-c wrapped buffer
- Need to investigate these types for wrapping
	- MTLResource
	- MTLCommandEncoder
	- From MTLCommandBuffer
		- MTLCommandBufferHandler
		- MTLDrawable
		- CFTimeInterval
		- MTLBlitCommandEncoder
		- MTLComputeCommandEncoder
		- MTLComputePassDescriptor
		- MTLBlitPassDescriptor
		- MTLDispatchType
		- MTLEvent
		- MTLParallelRenderCommandEncoder
		- MTLResourceStateCommandEncoder
		- MTLAccelerationStructureCommandEncoder
		- MTLResourceStatePassDescriptor
	- From MTLRenderCommandEncoder
		- MTLSamplerState
		- NSRange
		- MTLViewport
		- MTLVertexAmplificationViewMapping
		- MTLCullMode
		- MTLDepthClipMode
		- MTLScissorRect
		- MTLTriangleFillMode
		- MTLDepthStencilState
		- MTLVisibilityResultMode
		- MTLIndexType
		- MTLRenderStages
		- MTLResource
		- MTLResourceUsage
		- MTLHeap
		- MTLIndirectCommandBuffer
		- MTLBarrierScope
		- MTLCounterSampleBuffer

## Notes

## Metal CommandBuffers
- The command buffer can only be committed for execution on the command queue that created it. All command buffers sent to a single command queue are guaranteed to execute in the order in which the command buffers were enqueued.
- At any given time, only a single encoder may be active for a particular command buffer. You create an encoder, use it to add commands to the buffer, and then end the encoding process. After you finish with an encoder, you can create another encoder and continue to add commands to the same buffer. When you are ready to execute the set of encoded commands, call the command buffer’s commit method to schedule the buffer for execution.
- After a command buffer has been committed for execution, the only valid operations on the command buffer are to wait for it to be scheduled or completed (using synchronous calls or handler blocks) and to check the status of the command buffer execution. When used, scheduled and completed handlers are blocks that are invoked in execution order. These handlers should perform quickly; if expensive or blocking work needs to be scheduled, defer that work to another thread.
- enqueue : Enqueueing a command buffer reserves a place for the command buffer on the command queue without committing the command buffer for execution. When this command buffer is later committed, it keeps its position in the queue. You enqueue command buffers so that you can create multiple command buffers with a fixed order of execution without encoding the command buffers serially. You can use other threads to encode commands into the command buffers and those threads can complete in any order.  You can call the enqueue method before, during, or after encoding of commands. You can only enqueue a command buffer once.  The enqueue method doesn’t make the command buffer eligible for execution. To execute the command buffer, call the commit method.
	- Can have multiple command buffers being filled in at once in any order via enqueue
	- enqueue : reserves place  in the command queue : like a JumpReturn sub-list
	- Only one active command encoder per command buffer

### Metal CommandEncoders
- After the command buffer creates this render command encoder, you can’t create other command encoders for this command buffer until you call the endEncoding method of the MTLRenderCommandEncoder object.
- After endEncoding is called, the command encoder has no further use. You cannot encode any other commands with this encoder.
- Command encoder objects are lightweight objects that you re-create every time you need to send commands to the GPU.
- While a command encoder is active, it has the exclusive right to append commands to its command buffer. Once you finish encoding commands, call the endEncoding method to finish encoding the commands. To write further commands into the same command buffer, create a new command encoder.
- The rendering commands encoded by MTLRenderCommandEncoder objects are executed in the order in which the MTLRenderCommandEncoder objects are created, not in the order they are ended.
- After the command buffer creates this render command encoder, you can’t create other command encoders for this command buffer until you call the endEncoding method of the MTLRenderCommandEncoder object.

## Capture Chunks
So there are three main places to put chunks, broadly speaking
- Creation type chunks
- Initial contents chunks
- Frame chunks

Creation chunks are the ones that initially set up the objects and only need to happen once these go into the resource record
if there are any dependencies (A is created from B) records can have a loose hierarchy to ensure that A is pulled in whenever we need B, we don't have to worry about tracking that
Chunks also have an atomic ID (64-bits) which ensures they're always kept ordered

Frame referenced chunks : tracking this - while actively recording a frame we need to ensure that we track all objects that are referenced, conservatively, using MarkResourceFrameReferenced. This then gets used by the resource manager to collate and pull in all the relevant resource records

MarkResourceFrameReferenced also takes a reference type - read, write, and a few other combinations. This is used later and initially you can just stick to read or write probably (or readbeforewrite in the case of buffers/images which are bound in a way that can be read and/or written). You can add more annotation later if needed, it is relevant for optimising the replay but basic read/write is necessary for proper functioning

Frame chunks are the ones that form the actual captured frame. 
It starts with SystemChunk::CaptureScope then SystemChunk::CaptureBegin and ends with SystemChunk::CaptureEnd

In between on a simple immediate API like D3D11 or GL we just record each immediate command into the frame capture record.
The frame capture record must not be marked referenced, instead we manually insert its records in at the frame capture part

For command buffer based APIs. You'll record each recorded command buffer to its own record and then any queue work like submissions or anything else you will record to the frame capture record. When it goes to collate it together, you'll insert the chunks for any submitted command buffers and all the queue work. This may not be necessary for metal but sometimes there can be non-queue/command work which has to be ordered correctly with command recording. E.g. updating descriptors or whatever else. So the frame capture ends up having the command recording and everything else intermixed - the commands chunks aren't all inserted all at once up front

For command buffers what I have done before is had a two stage process effectively. When recording a command buffer it's inserting chunks into one record, and when the command buffer is ended/finished/finalised it copies all those chunks (and any other associated data) into a 'baked' command buffer. This is useful in case the application records a command buffer multiple times a frame because you'd end up re-writing the same chunks over again, you need to hold them somewhere

The records have a simple refcounting so you can release/addref a baked command buffer record to keep it around until the capture is complete and then it will delete itself when you are done with it if it's no longer accessible

The initial contents are the most 'special' part of it all.
These don't correspond to any explicit function call.
Any object which is mutable (hopefully that is only textures and buffers) needs to have initial contents which snapshot its state.
The initial contents contain all the data needed to reset that resource to the state it was at immediately when the capture started.
Usually that just means the actual contents of the resource - the data inside. That's how we handle the case where a captured frame reads from some buffer at the start, and then writes over it later, because we need to be able to reset back to where we were at the start of the frame again the next time around.
What we do is when the capture starts the resource manager takes care of calling Prepare_InitialState() on resources that have been marked dirty (you will need to handle this, marking dirty any resource that could be GPU modified. These days if it's only textures/buffers I tend to mark them dirty as soon as they are created). That function then should take a copy of any data it needs but doesn't serialise anything yet
Later at the end of the frame while writing the capture, the resource manager will call Serialise_InitialState() for any resources which are dirty and actually referenced. That's the chance to write out a chunk containing the data then finally on replay any resources which are written in the frame (remember those reference types) but don't have initial states, maybe because they were created late or maybe because they were written for the first time in the captured frame, those will go through Create_InitialState() which mostly just lets you set up some tag so you can e.g. clear them or something

The main loop on replay after that is to apply the initial states with Apply_InitialState() to get back to a known point, then run 0-N commands of the replay

There's more details to that but hopefully that covers at least where the chunks go with a bit of context

Global resources like the MTLDevice, it's kind of up to you. You could make them an explicit parent of every resource if you wanted and they'd be pulled in that way, the dependency chain is as long as you want it to be - the only thing to note is I don't have a way of removing parents

Personally for objects like that, I just explicitly have the start of frame capture function mark them referenced, it seems easier that way. But most objects can be pulled in either because they're explicitly mentioned in the capture or because they are one-step indirected. E.g. on Vulkan I think the only AddParents I do are things like buffers/images have the memory bound as a parent. Or pipelines have the shader object as a parent

# Metal MTLDevice.h documentation errors

Should this be @method ?
	/*!
	 @property supportsVertexAmplificationCount:
	 @abstract Query device for vertex amplification support.
	 @param count The amplification count to check
	 @return BOOL value. If YES, the device supports vertex amplification with the given count. If NO, the device does not.
	 */
	- (BOOL)supportsVertexAmplificationCount:(NSUInteger)count API_AVAILABLE(macos(10.15.4), ios(13.0), macCatalyst(13.4));

No Docs?
	@property (readonly) NSUInteger maxBufferLength API_AVAILABLE(macos(10.14), ios(12.0));

Why normal comments and not the /*! .... !*/ form?
	/*
	 @method newDefaultLibraryWithBundle:error:
	 @abstract Returns the default library for a given bundle.
	 @return A pointer to the library, nil if an error occurs.
	*/
	- (nullable id <MTLLibrary>)newDefaultLibraryWithBundle:(NSBundle *)bundle error:(__autoreleasing NSError **)error API_AVAILABLE(macos(10.12), ios(10.0));

# Notes
- RenderDoc Metal started on 14th July 2021

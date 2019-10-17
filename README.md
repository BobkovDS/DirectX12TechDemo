# DirectX12TechDemo

**Goal**: 

Technical demo to show my DirectX 12 basic/junior-level programming skills

**Description:**

Hello, my name is Dmitriy and i want to be Engine/Rendering Engineer. This Project is to put together and upgrade my C++ and DirectX 12 programming skills.
This project looks like some kind of 3D Engine, with possability to load a scene from several FBX files and render it using some render techniques. 
But in the same time, it does not claim to be 'real' 3D Engine. The goal was to create a solid 3D application with attention to using next topics:
* DXGI
  * Factory, Device, Adapter
  * SwapChain
  
* The main parts of DirectX 12 application    
  * Command Queue and Command List
  * Pipeline State Object
  * Rootsignature (defined in Shaders files) 
  * Descriptions and Description Heaps
  * Resources and Heaps (CommitedResources in Upload and Default Heaps)
  * CPU/GPU synchronization with Fences
  
* Mathematic / Algorithmic part
  * 3D Mathematic (Matrix, Vectors, Coordinate transformation)
  * Bounding Objects mathematic
  * View Frustum Culling
  * Octree
  * Skeleton animation
  * Reading FBX file (using FBX SDK)
 
 * Some Rendering Techniques
   * Lighting model: Lambert's cosine law + Fresnel equations (Schlick approximation)
   * Screen Space Ambient Occlusion
   * Dynamic Shadows (Sun light with Orthogonal projection)
   * Normal Mapping
   * Water simulation on Computer Shaders

**Screenshots (3840x2160):**
![Alt text](/TechDemo/Screenshots/TechDemo_1.jpg?raw=true "Screenshot 1")
![Alt text](/TechDemo/Screenshots/TechDemo_3.jpg?raw=true "Screenshot 2")
![Alt text](/TechDemo/Screenshots/TechDemo_4.jpg?raw=true "Screenshot 3")

**Application architecture:**
![Alt text](/TechDemo/ClassDiagram.png?raw=true "ClassDiagram")
* Application part
1. Canvas class creates a basic Window application
2. BasicDXGI extents Canvas by inheriting to add a support of DXGI to application
3. TechDemo class extents BasicDXGI by inheriting to add application logic.
Inheriting is used to allow child-class, using virtual methods get access to main loop in Canvas class.

* Resource part
1. __*FrameResourceManager*__ allows application to store GPU and CPU resources requied for each frame:
   - Command allocator to store recorded Command list for frame
   - Constant buffers with data which should be updated for every frame
2. __*ResourceManager*__ allows to store GPU data for Scene data: Meshes, Textures (DDS format) and Materials data
* Scene part
1. __*FBXFileLoader*__ using FBX SDK loads FBX file to application, moving data from it to:
   - __*ObjectManager*__ - stores all objects in Scene, using Object layers (Opaque, Transparent, Sky etc)
   - __*SkeletonManager*__ - stores Skeleton data
2. __*Scene*__ class add some more abstraction to __*ObjectManager*__ idea: representing the scene, it allows to some Object Layers be turned on/off for drawing, it is a source of data for Renders. Using View Frustum culling and Octree it creates list of visible Object's Instances for each frame

* Render part
  - Renders, using __*Scene*__ object, implement some render technique (__*SSAORender*__, __*ShadowRender*__ etc) or final scene drawing (__*FinalRender*__). Each Render has its own set of Pipeline State Objects (PSOLayers) for each Objects Layer (Opaques objects, Transparent objects, Opaque Skinned Objects etc). But because PSOLayer for some Render, has the same RootSignature for all Objects Layers, it is no need to set all RootSignature arguments every time when we draw a Objects Layer, changing a PSO for this.

**Summary:**
 Please feel free to contact me if you want to get more information. Also please find a video with TechDemo application demonstration https://youtu.be/uSJOSiK7oxk
 
 Thank you.

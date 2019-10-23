# DirectX12TechDemo

**Please be sure to have a deal with RELEASE branch**

**Please find a video with TechDemo application demonstration https://youtu.be/uSJOSiK7oxk**

**Goal**: 

A Technical demo to show my DirectX 12 basic/junior-level programming skills

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
  * Bounding Objects mathematic (Books[4]. Chapter 8)
  * View Frustum Culling
  * Octree
  * Skeleton animation
  * Reading FBX file (using FBX SDK)
 
 * Some Rendering Techniques
   * Lighting model: Lambert's cosine law + Fresnel equations (Schlick approximation)
   * Screen Space Ambient Occlusion
   * Dynamic Shadows (Sun light with Orthogonal projection)
   * Normal Mapping
   * Water simulation on Computer Shaders (Books[4]. Chapter 15)
   
**Screenshots**
Please find Screenshots in Release branch.

**Application architecture:**
Please find Class Diagram in Release branch.

* Application part
1. __*Canvas*__ class creates a basic windowed application
2. __*BasicDXGI*__ extents __*Canvas*__ by inheriting to add a support of DXGI to application
3. __*TechDemo*__ class extents __*BasicDXGI*__ by inheriting to add application logic.
Inheriting is used to allow child-class, using virtual methods get access to main loop in Canvas class.

* Resource part
1. __*FrameResourceManager*__ allows application to store GPU and CPU resources requied for each frame:
   - Command allocator to store recorded Command list for frame
   - Constant buffers with data which should be updated for every frame
2. __*ResourceManager*__ allows to store GPU data for Scene data: Meshes, Textures (DDS format) and Materials data
* Scene part
1. __*FBXFileLoader*__ using FBX SDK loads a FBX file to the application, moving data from it to:
   - __*ObjectManager*__ - stores all objects in a Scene, using Object layers (Opaque, Transparent, Sky etc)
   - __*SkeletonManager*__ - stores Skeleton data
2. __*Scene*__ class adds some more abstraction to __*ObjectManager*__ idea: representing the scene, it allows to some Object Layers to be turned on/off for drawing, it is a source of data for Renders. Using View Frustum culling and Octree it creates list of visible Object's Instances for each frame

* Render part
  - Renders, using __*Scene*__ object, implement some render technique (__*SSAORender*__, __*ShadowRender*__ etc) or final scene drawing (__*FinalRender*__). Each Render has its own set of Pipeline State Objects (PSOLayers) for each Objects Layer (Opaques objects, Transparent objects, Opaque Skinned Objects etc). But because PSOLayer for some Render has the same RootSignature for all Objects Layers, it is no need to set all RootSignature arguments every time when we draw a Objects Layer, changing a PSO for this.

**Application Control:**
   - Camera rotation: a mouse
   - Camera moving: W/S/A/D
  - Camera animation on/off: C
  - Light animation on/off: L
  - SSAO turn on/off: NumPad7
  - Shadows turn on/off: NumPad8
  - Normal mapping turn on/off: NumPad6
  - Water reflection turn on/off: NumPad4
  - Water simulation turn on/off: NumPad9
  - Water drop add: NumPad5
  - Which texture to show:
    - Final Image: 1
    - SSAO View-space Normal Map (if SSAO is turned on): 2
    - SSAO AO Map (if SSAO is turned on): 3
    - SSAO AO blured Map (if SSAO is turned on): 4
    - Shadow map (if Shadows are turned on): 5
    
 **Books:**
 1. "Thinking in C++. Volume One: Introduction to Standard C++. Second Edition". Bruce Eckel.
 2. "Thinking in C++. Volume Two: Practical Programming. Second Edition". Bruce Eckel, Chuck Allision.
 3. "Introduction to 3D Game Programming with DirectX 12". Frank D. Luna.
 4. "Mathematics for 3D Game Programming and Computer Graphics. Third Edition". Eric Lengyel.
 
**Contacts:**
 Please feel free to contact me (bobkovds285@gmail.com) if you want to get more information.
 
 **Thank you.**

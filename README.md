# DirectX12TechDemo

**Goal:** 

Technical demo to show my DirectX 12 basic/junior-level programming skills

**Description:**

Hallo, my name is Dmitriy and i want to be Engine/Rendering Engineer. This Project is to put together and upgrade my C++ and DirectX 12 programming skills.
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
   * Dynamic Shadows
   * Normal Mapping
   * Water simulation on Computer Shaders


![Hamil](Hamil/doc/hamil-logo.png "Hamil")

# List of next-in-line TODOs for Hamil
* Improve ```gx``` module
   * Rewrite ```gx::VertexFormat``` to better handle instancing
   * Rewrite ```gx::Pipeline``` for easier extensibility and better diffing
   * Move binding ```gx::Program```s into ```gx::Pipeline``` state
   * Support Vulkan as a backend along with/in place of (?) OpenGL
* Simple compiler/preprocessor for GPU shaders
   * Extend the current custom ```#pragma``` directive support?
   * Make it an Eugene module?
* Improve ```sched::Job```/```sched::WorkerPool```
   * Run Jobs in a fiber/coroutine context
   * Support input dependency tracking and barriers
   * Add ```sched::ParallelForJob``` job type to abstract running a function over a range of elements
* Add ```hm::System``` facility to implement the Systems part of the ECS pattern
   * Support iteration over a union of ```hm::Component```s in ```hm::ComponentStore```
   * ...as well as optionally present ```hm::Component```s
   * ...this can be implemented using either ```template```-metaprogramming or in ```Eugene.componentgen```
   * Support data dependencies between Systems
   * Schedule running their code in ```sched::WorkerPool```s
* Improvements to ```ek::ViewVisibility```
   * Use Masked Occlusion culling technique
   * Use GPU Compute shaders
* Support coarse-grained BVH/k-d tree/portal based geometry culling in ```ek``` as a first step in choosing visible objects# Hamil next in line TODO list
* Improve ```gx``` module
   * Rewrite ```gx::VertexFormat``` to better handle instancing
   * Rewrite ```gx::Pipeline``` for easier extensibility and better diffing
   * Move binding ```gx::Program```s into ```gx::Pipeline``` state
   * Support Vulkan as a backend along with/in place of (?) OpenGL
* Simple compiler/preprocessor for GPU shaders
   * Extend the current custom ```#pragma``` directive support?
   * Make it an Eugene module?
* Improve ```sched::Job```/```sched::WorkerPool```
   * Run Jobs in a fiber/coroutine context
   * Support input dependency tracking and barriers
   * Add ```sched::ParallelForJob``` job type to abstract running a function over a range of elements
* Add ```hm::System``` facility to implement the Systems part of the ECS pattern
   * Support iteration over a union of ```hm::Component```s in ```hm::ComponentStore```
   * ...as well as optionally present ```hm::Component```s
   * ...this can be implemented using either ```template```-metaprogramming or in ```Eugene.componentgen```
   * Support data dependencies between Systems
   * Schedule running their code in ```sched::WorkerPool```s
* Improvements to ```ek::ViewVisibility```
   * Use Masked Occlusion culling technique
   * Use GPU Compute shaders
* Support coarse-grained BVH/k-d tree/portal based geometry culling in ```ek``` as a first step in choosing visible objects


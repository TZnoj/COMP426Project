# Multicore Systems Assignment
**Project Description**  
The task for this project was to implement Conrad's Game of Life, but have it multithreaded by using CPU and GPU cores. This particular implementation was the Multi Species variant in which the different species do not interact with one and another.  
**My Implementation**  
I used OpenCL to be able to assign tasks to my CPU and GPU cores. Unfortunately, as the species didn't interact with each other, there were a lot of comparison checks in the kernel code which reduced the optimization. I was able to increase performance by choosing appropriate work group sizes.

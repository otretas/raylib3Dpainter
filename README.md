# raylib3Dpainter
Just a simple raylib model viewer where you can paint and save the diffuse texture

Tested with a build with 17.5.4 x64. To load a model pass the path to the model as the first argument and the directory to the shaders as the second. It will look for vs.gsls and fs.gsls in the shader directory.
This isn't the best tool to draw as it does not consider neighbour triangles, so, when creating the model's UV coordinates make sure each element has some considerable space in between.

Controls:
Left Click Mouse - rotates around the target
Right Click Mouse - moves the target in the XZ plane
Mouse Wheel - zooms in/out
Q - draws red circles where mouse is pointing
W - draws white circles where mouse is pointing (as to erase)
A - increases size of drawing circles
Z - decreases size of drawing circles


Using parts of code and modified them to my needs from other places such as:

orbital controls - https://gist.github.com/JeffM2501/000787070aef421a00c02ae4cf799ea1

bounding volume hierarchy - https://github.com/jbikker/bvh_article

raycast painting concept - https://bedroomcoders.co.uk/painting-on-a-3d-mesh-with-raylib/

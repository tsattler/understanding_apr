# Utils
This directory provides source code for utility functions.

Please note that we provide this functionality for convenience. We do not plan to provide a full toolbox that easily compiles under all systems.

## Creating Sparse 3D Models from the Training Images Using COLMAP
Some visual localization algorithms such as [Active Search](https://github.com/tsattler/vps) require a Structure-from-Motion (SfM) model of the scene. 
We provide functionality to create such models from the images, camera poses, and camera intrinsics that we provide for our datasets. 
In order to run this software, you will need to install [COLMAP](https://github.com/colmap/colmap).

### Preparation
* Compile PoseNet2EmptyColmap.cc via the provided script: ``sh compile.sh``.
* Edit the file ``create_colmap_model_from_training_data.sh``: Adjust the variables ``colmap_exe``, ``posenet2colmap_exe``, and ``py_file`` based on your system. 

### Creating Sparse 3D Models
Move the file ``create_colmap_model_from_training_data.sh`` into the directory into which you extracted one of our datasets. Then run ``sh create_colmap_model_from_training_data.sh``. 
The result will be a sparse 3D model stored in ``training_model/``.

#! /bin/bash
# Enter the path to the colmap exectubale on your system.
colmap_exe="/home/torsat/code/colmap/colmap/build/src/exe/colmap"
# Enter the path to the PoseNet2EmptyColmap.
posenet2colmap_exe="/home/torsat/code/understanding_apr/utils/PoseNet2EmptyColmap"
# Enter the path to the export_image_names_and_ids.py file.
py_file="/home/torsat/code/understanding_apr/utils/export_image_names_and_ids.py"

mkdir training_model
mkdir empty_model

# Extracts the intrinsics.
intrinsics=$(cat intrinsics.txt | grep PINHOLE | awk -F ' ' '{print $5", "$6", "$7", "$8}')

# Extracts features for the training and test images.
cat dataset_train.txt | grep jpg | awk -F ' ' '{print $1}' > list_images.txt
$colmap_exe feature_extractor --database_path database.db --image_path ./ --ImageReader.camera_model PINHOLE --ImageReader.single_camera 1 --ImageReader.camera_params "$intrinsics" --SiftExtraction.max_num_orientations 1 --SiftExtraction.upright 1 --image_list_path list_images.txt

# Exports the image names and their image ids.
python $py_file --database_path database.db --output_file image_names_and_ids.txt

# Prepares the data for colmap.
$posenet2colmap_exe dataset_train.txt image_names_and_ids.txt empty_model/ intrinsics.txt

# Runs feature matching.
$colmap_exe matches_importer --database_path database.db --match_list_path empty_model/matches.txt

# Triangulates the 3D structure.
$colmap_exe point_triangulator --database_path database.db --image_path ./ --input_path empty_model/ --output_path training_model/ --Mapper.ba_refine_focal_length 0 --Mapper.ba_refine_extra_params 0 --Mapper.ba_refine_principal_point 0

# Cleanup
rm -rf empty_model/

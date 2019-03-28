#include <vector>
#include <set>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdint.h>
#include <string>
#include <cstdlib>
#include <map>

#include <Eigen/Core>
#include <Eigen/Geometry>

struct Camera {
  std::string image_name;
  Eigen::Quaterniond orientation;
  Eigen::Vector3d position;
};

bool LoadFromList(const std::string& list_file, std::vector<Camera>* images) {
  std::ifstream ifs(list_file.c_str(), std::ios::in);
  if (!ifs.is_open()) {
    std::cerr << " ERROR: Cannot read the image list from " << list_file
              << std::endl;
    return false;
  }
  std::string line;

  // Skips the first three lines containing the header.
  for (int i = 0; i < 3; ++i) {
    std::getline(ifs, line);
  }

  int counter = 0;

  while (std::getline(ifs, line)) {
    std::stringstream s_stream(line);

    Camera img;
    s_stream >> img.image_name >> img.position[0] >> img.position[1]
             >> img.position[2] >> img.orientation.w() >> img.orientation.x()
             >> img.orientation.y() >> img.orientation.z();

    ++counter;
    if (counter % 100 == 0) {
      std::cout << "\r " << counter << std::flush;
    }
    images->push_back(img);
  }

  ifs.close();

  return true;
}

double ComputeRotError(const Eigen::Quaterniond& q1,
                       const Eigen::Quaterniond& q2) {
  Eigen::Quaterniond estimated_q(q2);
  estimated_q.normalize();
  Eigen::Matrix3d R1 = estimated_q.toRotationMatrix();
  Eigen::Quaterniond gt_q(q1);
  gt_q.normalize();
  Eigen::Matrix3d R2 = gt_q.toRotationMatrix();
  Eigen::Matrix3d R_rel = R1 * R2.inverse();
  Eigen::AngleAxisd aax(R_rel);
  return aax.angle() * 180.0 / M_PI;
}

int main (int argc, char **argv)
{
  if (argc < 5)
  {
    std::cout << "________________________________________________________________________________________________" << std::endl;
    std::cout << " -                                                                                            - " << std::endl;
    std::cout << " -         Given a dataset_train.txt file in PoseNet's file format, creates an empty 3D model - " << std::endl;
    std::cout << " -         in COLMAP's text format. Also writes out a list of camera pairs to be matched.     - " << std::endl;
    std::cout << " -                               2018 by Torsten Sattler (sattlert@inf.ethz.ch)               - " << std::endl;
    std::cout << " -                                                                                            - " << std::endl;
    std::cout << " - Usage: PoseNet2EmptyColmap dataset_train name_id_list outdir shared_cam_file               - " << std::endl;
    std::cout << " - Parameters:                                                                                - " << std::endl;
    std::cout << " -  - dataset_train                                                                           - " << std::endl;
    std::cout << " -     Textfile containing the names and poses of the training images.                        - " << std::endl;
    std::cout << " -                                                                                            - " << std::endl;
    std::cout << " -  - name_id_list                                                                            - " << std::endl;
    std::cout << " -     Textfile containing the names of the cameras and their image ids in the Colmap         - " << std::endl;
    std::cout << " -     database. This file can be generated using a suitable exporter from Colmap.            - " << std::endl;
    std::cout << " -                                                                                            - " << std::endl;
    std::cout << " -  - outdir                                                                                  - " << std::endl;
    std::cout << " -     Output directory into which the cameras.txt, images.txt, and points3D.txt files will   - " << std::endl;
    std::cout << " -     be written. Includes trailing /. Will also contain a list of images to be matched.     - " << std::endl;
    std::cout << " -                                                                                            - " << std::endl;
    std::cout << " -  - shared_cam_file                                                                         - " << std::endl;
    std::cout << " -     If specified, the filename of a cameras.txt file (in Colmap's text format) that stores - " << std::endl;
    std::cout << " -     a single camera that will be used for all images.                                      - " << std::endl;
    std::cout << " -                                                                                            - " << std::endl;
    std::cout << "________________________________________________________________________________________________" << std::endl;
    return 1;
  }
  
  ////
  // Loads the data of the shared cameras.
  std::vector<std::string> shared_cam_data;
  int shared_cam_id = -1;
  {
    std::ifstream ifs(argv[4], std::ios::in);
    if (!ifs.is_open()) {
      std::cerr << " ERROR: Cannot read the shared camera from " << argv[4]
                << std::endl;
      return -1;
    }
    std::string line;
    while (std::getline(ifs, line)) {
      if (line.empty())
        continue;
      shared_cam_data.push_back(line);
//      std::cout << line << std::endl;
    }

    ifs.close();

    // Get the camera id of the first camera specified in the file.
    for (int i = 0; i < static_cast<int>(shared_cam_data.size()); ++i) {
      if (!shared_cam_data[i].empty()) {
        if (shared_cam_data[i][0] != '#') {
          std::stringstream s_stream(shared_cam_data[i]);
          s_stream >> shared_cam_id;
          break;
        }
      }
    }
    if (shared_cam_id == -1) {
      std::cerr << " ERROR: Could not find a valid camera id" << std::endl;
      return -1;
    } else {
      std::cout << " Using shared camera with id " << shared_cam_id
                << std::endl;
    }
  }

  ////
  // Loads the PoseNet file.
  std::vector<Camera> images_train;
  std::string dataset_train_file(argv[1]);
  if (!LoadFromList(dataset_train_file, &images_train)) {
    std::cerr << " ERROR: Loading training images failed " << std::endl;
    return -1;
  }

  ////
  // Finds pairs of images within 10m and 45 degree of orientation that should be
  // matched.
  int num_images = static_cast<int>(images_train.size());
  std::vector<std::pair<std::string, std::string> > image_matching_pairs;
  for (int i = 0; i < num_images; ++i) {
    for (int j = i + 1; j < num_images; ++j) {
      double pos_error = (images_train[i].position - images_train[j].position)
          .norm();
      if (pos_error > 10.0) continue;

      double rot_error = ComputeRotError(images_train[i].orientation,
                                         images_train[j].orientation);
      if (rot_error > 45.0) continue;

      image_matching_pairs.push_back(
          std::make_pair(images_train[i].image_name,
                         images_train[j].image_name));
    }
  }
  std::cout << " Found " << image_matching_pairs.size() << " pairs for "
            << num_images << " images" << std::endl;

  ////
  // Loads the list of image name - image id associations from the colmap
  // database.
  std::map<std::string, int> cam_names_to_id;
  {
    std::ifstream list_names_ids(argv[2], std::ios::in);
    std::string line;
    while (std::getline(list_names_ids, line)) {
      if (line.empty())
        continue;

      std::stringstream s_stream(line);

      std::string name;
      int id = 0;
      s_stream >> name >> id;

      cam_names_to_id[name] = id;
    }
    list_names_ids.close();
  }

  ////
  // Stores the camera data in a file.
  {
    std::string camera_file(argv[3]);
    camera_file.append("cameras.txt");
    std::ofstream ofs(camera_file.c_str(), std::ios::out);

    if (!ofs.is_open()) {
      std::cerr << " ERROR: Cannot write the cameras to " << camera_file
                << std::endl;
      return -1;
    }

    for (size_t i = 0; i < shared_cam_data.size(); ++i) {
      ofs << shared_cam_data[i] << std::endl;
    }

    ofs.close();
  }

  ////
  // Stores the image data in a file.
  {
    std::string camera_file(argv[3]);
    camera_file.append("images.txt");
    std::ofstream ofs(camera_file.c_str(), std::ios::out);

    if (!ofs.is_open()) {
      std::cerr << " ERROR: Cannot write the images to " << camera_file
          << std::endl;
      return -1;
    }

    ofs << "# Image list with two lines of data per image:" << std::endl;
    ofs << "#   IMAGE_ID, QW, QX, QY, QZ, TX, TY, TZ, CAMERA_ID, NAME" << std::endl;
    ofs << "#   POINTS2D[] as (X, Y, POINT3D_ID)" << std::endl;
    ofs << "# Number of images: " << num_images
        << " , mean observations per image: 0.0" << std::endl;
    for (int i = 0; i < num_images; ++i) {
      Eigen::Matrix3d R = images_train[i].orientation.toRotationMatrix();
      Eigen::Vector3d c(images_train[i].position[0],
                        images_train[i].position[1],
                        images_train[i].position[2]);
      Eigen::Vector3d t = -R * c;

      const int cam_id = shared_cam_id;

      Eigen::Quaterniond q(R);
      ofs << cam_names_to_id[images_train[i].image_name] << " " << q.w() << " "
          << q.x() << " " << q.y() << " " << q.z() << " " << t[0] << " " << t[1]
          << " " << t[2] << " " << cam_id << " " << images_train[i].image_name
          << std::endl;
      // Empty line for features
      ofs << std::endl;
    }

    ofs.close();
  }

  ////
  // Creates an empty file with the point positions.
  {
    std::string camera_file(argv[3]);
    camera_file.append("points3D.txt");
    std::ofstream ofs(camera_file.c_str(), std::ios::out);

    if (!ofs.is_open()) {
      std::cerr << " ERROR: Cannot write the points to " << camera_file
                << std::endl;
      return -1;
    }

    ofs << "# 3D point list with one line of data per point:" << std::endl;
    ofs << "#   POINT3D_ID, X, Y, Z, R, G, B, ERROR, TRACK[] as (IMAGE_ID, POINT2D_IDX)"
        << std::endl;
    ofs << "#   POINTS2D[] as (X, Y, POINT3D_ID)" << std::endl;
    ofs << "# Number of points: -, mean track length: 0.0" << std::endl;
    ofs.close();
  }

  ////
  // Creates a text file containing the ids of the images to be matched.
  {
    std::string camera_file(argv[3]);
    camera_file.append("matches.txt");
    std::ofstream ofs(camera_file.c_str(), std::ios::out);

    if (!ofs.is_open()) {
      std::cerr << " ERROR: Cannot write the matches to " << camera_file
                << std::endl;
      return -1;
    }

    int num_matches = static_cast<int>(image_matching_pairs.size());
    for (int i = 0; i < num_matches; ++i) {
      ofs << image_matching_pairs[i].first << " "
          << image_matching_pairs[i].second << std::endl;
    }
    ofs.close();
  }

  return 0;
}


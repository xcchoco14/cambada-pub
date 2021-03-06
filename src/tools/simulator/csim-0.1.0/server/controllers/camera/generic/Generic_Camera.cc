/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Desc: SickLMS200 Laser controller.
 * Author: Nathan Koenig
 * Date: 01 Feb 2007
 * SVN info: $Id: Generic_Camera.cc 8433 2009-11-22 20:15:37Z natepak $
 */

#include <algorithm>
#include <assert.h>

#include "Sensor.hh"
#include "Global.hh"
#include "XMLConfig.hh"
#include "Simulator.hh"
#include "gazebo.h"
#include "GazeboError.hh"
#include "ControllerFactory.hh"
#include "MonoCameraSensor.hh"
#include "Generic_Camera.hh"

using namespace gazebo;

GZ_REGISTER_STATIC_CONTROLLER("generic_camera", Generic_Camera);

////////////////////////////////////////////////////////////////////////////////
// Constructor
Generic_Camera::Generic_Camera(Entity *parent)
    : Controller(parent)
{
  this->myParent = dynamic_cast<MonoCameraSensor*>(this->parent);

  if (!this->myParent)
    gzthrow("Generic_Camera controller requires a Camera Sensor as its parent");
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
Generic_Camera::~Generic_Camera()
{
}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void Generic_Camera::LoadChild(XMLConfigNode *node)
{
  this->cameraIface = dynamic_cast<CameraIface*>(this->GetIface("camera"));
}

////////////////////////////////////////////////////////////////////////////////
/// Save the controller.
void Generic_Camera::SaveChild(std::string &prefix, std::ostream &stream)
{

}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void Generic_Camera::InitChild()
{
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void Generic_Camera::UpdateChild()
{

  // do this first so there's chance for sensor to run 1 frame after activate
  if (this->myParent->IsActive())
    this->PutCameraData();

  // activate if iface open
  if (this->cameraIface->Lock(1))
  {
    if (this->cameraIface->GetOpenCount() > 0)
      this->myParent->SetActive(true);
    else
      this->myParent->SetActive(false);

    //std::cout << " camera open count " << this->cameraIface->GetOpenCount() << std::endl;
    this->cameraIface->Unlock();
  }
  //std::cout << " camera     active " << this->myParent->IsActive() << std::endl;

}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void Generic_Camera::FiniChild()
{
}

////////////////////////////////////////////////////////////////////////////////
// Put laser data to the interface
void Generic_Camera::PutCameraData()
{
  CameraData *data = this->cameraIface->data;
  const unsigned char *src;
  unsigned char *dst;
  Pose3d cameraPose;

  this->cameraIface->Lock(1);

  // Data timestamp
  data->head.time = Simulator::Instance()->GetSimTime().Double();

  data->width = this->myParent->GetImageWidth();
  data->height = this->myParent->GetImageHeight();
  data->image_size = data->width * data->height * this->myParent->GetImageDepth();

  // GetFOV() returns radians
  data->hfov = *(this->myParent->GetHFOV());
  data->vfov = *(this->myParent->GetVFOV());

  // Set the pose of the camera
  cameraPose = this->myParent->GetWorldPose();
  data->camera_pose.pos.x = cameraPose.pos.x;
  data->camera_pose.pos.y = cameraPose.pos.y;
  data->camera_pose.pos.z = cameraPose.pos.z;
  data->camera_pose.roll = cameraPose.rot.GetRoll();
  data->camera_pose.pitch = cameraPose.rot.GetPitch();
  data->camera_pose.yaw = cameraPose.rot.GetYaw();

  // Make sure there is room to store the image
  assert (data->image_size <= sizeof(data->image));

  // Copy the pixel data to the interface
  src = this->myParent->GetImageData(0);
  dst = data->image;

  boost::recursive_mutex::scoped_lock mr_lock(*Simulator::Instance()->GetMRMutex());
  memcpy(dst, src, data->image_size);

  this->myParent->EnableSaveFrame( data->saveFrames );

  this->cameraIface->Unlock();

  // New data is available
  this->cameraIface->Post();


}


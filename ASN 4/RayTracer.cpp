/*
  RayTracer.cpp

  CS3388 Assignment 4

  By: Andrew Simpson
  SN: 250 633 280
  EM: asimps53@uwo.ca

  Class that implements the ray tracing algorithm discussed in class.
  User just calls traceImage().
*/

#include "RenderUtils.h"
#include "RayTracer.h"
#include "ImageManip.h"

void RayTracer::traceImage(cv::Mat &renderImage, float W, float H, float N, 
    std::vector<GenericObject*> &objects, Camera &cam, 
    cv::Scalar &backColour, std::vector<GenericLight*> &lights,
    PointLight &ambientLight)
{
  for (int r = 0; r < renderImage.rows; r++)
  {
    for (int c = 0; c < renderImage.cols; c++)
    {
      // Trace ray for row r, column c
      float u = -W + W*((float)2 * c) / renderImage.cols;
      float v = -H + H*((float)2 * r) / renderImage.rows;
      // Debugging trick...
      // Use image watch to find a bad pixel, then this
      // to stop at it, then debug that pixel.
      //if (r == 235 && c == 312){
      //  c = c;
      //}
      // Negate v to turn image right way up
      v = -v;
      // Trace a ray
      cv::Scalar colour = RayTracer::traceRay(u, v, N, objects, cam, backColour, lights, ambientLight);
      // Draw
      ImageManip::plot(renderImage, c, r, colour, false);
    }
  }

  return;
}

cv::Scalar RayTracer::traceRay(float u, float v, float N, std::vector<GenericObject*> &objects, 
    Camera &cam, cv::Scalar &backColour, std::vector<GenericLight*> &lights, PointLight &ambientLight)
{
  // Get the ray's direction vector  
  static cv::Mat d = cv::Mat::zeros(4, 1, CV_32FC1);
  d = - N*cam.n + u*cam.u + v*cam.v;
  d.at<float>(3) = 0;

  // Check for intersections
  float t = 0; // Needs to be initialized even though it will be overwritten
  GenericObject* obj = intersectRay(cam.e, d, t, objects);

  // Check if there was an intersection
  if (obj == NULL)
  {
    // No intersections
    return backColour;
  }
  // Get light intensity for each colour
  cv::Scalar colour = cv::Scalar(0,0,0);

  // Loop through all lights
  for (int i = 0; i < lights.size(); i++){
    GenericLight &light = *lights[i]; // Gets the current light

    cv::Mat intersection = cam.e + d*t;      // Get the intersection point
    cv::Mat s = light.toLight(intersection); // Vector to light from intersection

    // Do shadow ray:
    if (!shadowRay(intersection + s, -s, objects)) // Returns true if it really is a shadow ray
    {
      // Unit vectors for lighting effects
      RenderUtils::homoNormalize(s); // Normalize s
      cv::Mat v = RenderUtils::homoNormalize(cam.e - intersection); // Vector to camera from intersection
      cv::Mat norm = obj->normal(intersection);  // Normal vector at intersection
      cv::Mat r = -s + 2 * RenderUtils::homoDot(s, norm)*norm; // 

      // Diffuse lighting
      float dotProd = RenderUtils::homoDot(norm, s);
      if (dotProd > 0)
      {
        colour[0] += obj->rho_d[0] * light.intensity[0] * dotProd * 255;
        colour[1] += obj->rho_d[1] * light.intensity[1] * dotProd * 255;
        colour[2] += obj->rho_d[2] * light.intensity[2] * dotProd * 255;
      }

      // These checks were never used
      //for (int i = 0; i < 3; i++)
      //{
      //  if (colour[i] < 0)
      //  {
      //    colour[i] = 0;
      //  }
      //}

      // Specular lighting
      float f = 2;
      dotProd = RenderUtils::homoDot(r, v);
      if (dotProd > 0)
      {
        colour[0] += obj->rho_s[0] * light.intensity[0] * pow(dotProd, f) * 255;
        colour[1] += obj->rho_s[1] * light.intensity[1] * pow(dotProd, f) * 255;
        colour[2] += obj->rho_s[2] * light.intensity[2] * pow(dotProd, f) * 255;
      }

      // Never used
      //for (int i = 0; i < 3; i++)
      //{
      //  if (colour[i] < 0)
      //  {
      //    colour[i] = 0;
      //  }
      //}
    }
  }

  // Ambient lighting
  colour[0] += obj->rho_a[0] * ambientLight.intensity[0] * 255;
  colour[1] += obj->rho_a[1] * ambientLight.intensity[1] * 255;
  colour[2] += obj->rho_a[2] * ambientLight.intensity[2] * 255;

  return colour;
}

GenericObject* RayTracer::intersectRay(cv::Mat e, cv::Mat d, float &t, 
    std::vector<GenericObject*> &objects)
{
  // Keep track of the nearest intersection
  GenericObject* closestObject = NULL;

  // Calculate intersection with each object
  float thisIntersection; // Intersection distance of current iteration
  for (int i = 0; i < objects.size(); i++)
  {
    if (objects[i]->intersect(e, d, thisIntersection) && 
      ((closestObject == NULL) || (thisIntersection < t)))
    {
      // Make sure the intersection was in front of the camera:
      t = thisIntersection;
      closestObject = objects[i];
    }
  }
  return closestObject;
}

bool RayTracer::shadowRay(cv::Mat lightLoc, cv::Mat toPoint,
    std::vector<GenericObject*> &objects)
{
  // Calculate if there is an intersection with another object
  float thisIntersection; // Intersection distance of current iteration
  for (int i = 0; i < objects.size(); i++)
  {
    // Check if the intersection is between the point and the light.
    // Note that I don't check if the intersection is at a distance > 0,
    // this causes weird self-shadowing artifacts.
    // I'm assuming this is due to rounding errors.
    //
    // Another trick: I do this back wards, to get the largest distance
    // from the object, as otherwise self-shadowing glitches are more
    // apparent on the backs of objects... It's hard to explain in a
    // comment...
    if (objects[i]->intersect(lightLoc, toPoint, thisIntersection) &&
      ((0.001 < thisIntersection)&&(0.999 > thisIntersection)))
    {
      // Intersection between point and light
      return true;
    }
  }
  // No intersection was found
  return false;
}
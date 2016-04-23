#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define VK_ASSERT( a ) do { VkResult res = a; assert(VK_SUCCESS==res); }while(0)
#define VK_SUCCEED( a ) ( VK_SUCCESS == a )
#define VK_RETURN_IF_FAILED( a ) do{ VkResult res = a; assert(VK_SUCCESS==res); if( !VK_SUCCEED(res) ) return false; }while(0)

struct Vector3 {
	float x, y, z;
};

struct Vector2 {
	float u, v;
};



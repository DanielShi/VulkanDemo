#pragma once
#include <memory>
#include <algorithm>
#include <sstream>
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_ASSERT( a ) do { VkResult res = a; assert(VK_SUCCESS==res); }while(0)
#define VK_SUCCEED( a ) ( VK_SUCCESS == a )
#define VK_RETURN_IF_FAILED( a ) do{ VkResult res = a; assert(VK_SUCCESS==res); if( !VK_SUCCEED(res) ) return false; }while(0)

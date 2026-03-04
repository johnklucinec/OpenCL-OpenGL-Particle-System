// ==================================================
// OpenCL Helper Functions
// Adapted from Mike Bailey (Oregon State University)
// Updated to C++17 - functionality unchanged
// ==================================================

#pragma once

#include <CL/cl.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>


// ==================================================
// OpenCL Vendor IDs:
// ==================================================
namespace VendorID {
constexpr cl_uint AMD    = 0x1002;
constexpr cl_uint INTEL  = 0x8086;
constexpr cl_uint NVIDIA = 0x10de;
constexpr cl_uint APPLE  = 0x1027f00;
}  // namespace VendorID


// ==================================================
// Error Code Lookup Table:
// ==================================================
struct ErrorCode
{
  cl_int           statusCode;
  std::string_view meaning;
};

constexpr std::array ErrorCodes = {
    ErrorCode{CL_SUCCESS,                         ""                                   },
    ErrorCode{CL_DEVICE_NOT_FOUND,                "Device Not Found"                   },
    ErrorCode{CL_DEVICE_NOT_AVAILABLE,            "Device Not Available"               },
    ErrorCode{CL_COMPILER_NOT_AVAILABLE,          "Compiler Not Available"             },
    ErrorCode{CL_MEM_OBJECT_ALLOCATION_FAILURE,   "Memory Object Allocation Failure"   },
    ErrorCode{CL_OUT_OF_RESOURCES,                "Out of resources"                   },
    ErrorCode{CL_OUT_OF_HOST_MEMORY,              "Out of Host Memory"                 },
    ErrorCode{CL_PROFILING_INFO_NOT_AVAILABLE,    "Profiling Information Not Available"},
    ErrorCode{CL_MEM_COPY_OVERLAP,                "Memory Copy Overlap"                },
    ErrorCode{CL_IMAGE_FORMAT_MISMATCH,           "Image Format Mismatch"              },
    ErrorCode{CL_IMAGE_FORMAT_NOT_SUPPORTED,      "Image Format Not Supported"         },
    ErrorCode{CL_BUILD_PROGRAM_FAILURE,           "Build Program Failure"              },
    ErrorCode{CL_MAP_FAILURE,                     "Map Failure"                        },
    ErrorCode{CL_INVALID_VALUE,                   "Invalid Value"                      },
    ErrorCode{CL_INVALID_DEVICE_TYPE,             "Invalid Device Type"                },
    ErrorCode{CL_INVALID_PLATFORM,                "Invalid Platform"                   },
    ErrorCode{CL_INVALID_DEVICE,                  "Invalid Device"                     },
    ErrorCode{CL_INVALID_CONTEXT,                 "Invalid Context"                    },
    ErrorCode{CL_INVALID_QUEUE_PROPERTIES,        "Invalid Queue Properties"           },
    ErrorCode{CL_INVALID_COMMAND_QUEUE,           "Invalid Command Queue"              },
    ErrorCode{CL_INVALID_HOST_PTR,                "Invalid Host Pointer"               },
    ErrorCode{CL_INVALID_MEM_OBJECT,              "Invalid Memory Object"              },
    ErrorCode{CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, "Invalid Image Format Descriptor"    },
    ErrorCode{CL_INVALID_IMAGE_SIZE,              "Invalid Image Size"                 },
    ErrorCode{CL_INVALID_SAMPLER,                 "Invalid Sampler"                    },
    ErrorCode{CL_INVALID_BINARY,                  "Invalid Binary"                     },
    ErrorCode{CL_INVALID_BUILD_OPTIONS,           "Invalid Build Options"              },
    ErrorCode{CL_INVALID_PROGRAM,                 "Invalid Program"                    },
    ErrorCode{CL_INVALID_PROGRAM_EXECUTABLE,      "Invalid Program Executable"         },
    ErrorCode{CL_INVALID_KERNEL_NAME,             "Invalid Kernel Name"                },
    ErrorCode{CL_INVALID_KERNEL_DEFINITION,       "Invalid Kernel Definition"          },
    ErrorCode{CL_INVALID_KERNEL,                  "Invalid Kernel"                     },
    ErrorCode{CL_INVALID_ARG_INDEX,               "Invalid Argument Index"             },
    ErrorCode{CL_INVALID_ARG_VALUE,               "Invalid Argument Value"             },
    ErrorCode{CL_INVALID_ARG_SIZE,                "Invalid Argument Size"              },
    ErrorCode{CL_INVALID_KERNEL_ARGS,             "Invalid Kernel Arguments"           },
    ErrorCode{CL_INVALID_WORK_DIMENSION,          "Invalid Work Dimension"             },
    ErrorCode{CL_INVALID_WORK_GROUP_SIZE,         "Invalid Work Group Size"            },
    ErrorCode{CL_INVALID_WORK_ITEM_SIZE,          "Invalid Work Item Size"             },
    ErrorCode{CL_INVALID_GLOBAL_OFFSET,           "Invalid Global Offset"              },
    ErrorCode{CL_INVALID_EVENT_WAIT_LIST,         "Invalid Event Wait List"            },
    ErrorCode{CL_INVALID_EVENT,                   "Invalid Event"                      },
    ErrorCode{CL_INVALID_OPERATION,               "Invalid Operation"                  },
    ErrorCode{CL_INVALID_GL_OBJECT,               "Invalid GL Object"                  },
    ErrorCode{CL_INVALID_BUFFER_SIZE,             "Invalid Buffer Size"                },
    ErrorCode{CL_INVALID_MIP_LEVEL,               "Invalid MIP Level"                  },
    ErrorCode{CL_INVALID_GLOBAL_WORK_SIZE,        "Invalid Global Work Size"           },
};


// ==================================================
// Global Variables (to be set by SelectOpenclDevice):
// ==================================================
inline cl_platform_id Platform;
inline cl_device_id   Device;


// ==================================================
// Function Prototypes:
// ==================================================
namespace CLUtils {

bool             IsCLExtensionSupported(const char*, cl_device_id);
void             PrintCLError(cl_int, const char*, FILE* = stderr);
void             PrintOpenclInfo();
void             SelectOpenclDevice(cl_platform_id*, cl_device_id*);
std::string_view Vendor(cl_uint);
std::string_view Type(cl_device_type);

}  // namespace CLUtils


namespace CLUtils {

// ==================================================
// Check if OpenCL Extension is Supported:
// ==================================================
inline bool IsCLExtensionSupported(const char* extension, cl_device_id device)
{
  // See if the extension is bogus:
  if(extension == nullptr || extension[0] == '\0')
    return false;

  if(std::strchr(extension, ' ') != nullptr)
    return false;

  // Get the full list of extensions:
  size_t extensionSize;
  clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr, &extensionSize);
  std::vector<char> extensions(extensionSize);
  clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, extensionSize, extensions.data(), nullptr);

  const char* start = extensions.data();
  while(true)
  {
    const char* where = std::strstr(start, extension);
    if(where == nullptr)
      return false;

    const char* terminator = where + std::strlen(extension);  // Points to what should be the separator

    if(*terminator == ' ' || *terminator == '\0' || *terminator == '\r' || *terminator == '\n')
      return true;

    start = terminator;
  }
}


// ==================================================
// Print OpenCL Error Message:
// ==================================================
inline void PrintCLError(cl_int errorCode, const char* prefix, FILE* fp)
{
  if(errorCode == CL_SUCCESS)
    return;

  std::string_view meaning = "Unknown Error";
  for(const auto& error : ErrorCodes)
  {
    if(errorCode == error.statusCode)
    {
      meaning = error.meaning;
      break;
    }
  }

  std::fprintf(fp, "%s %.*s\n", prefix, static_cast<int>(meaning.size()), meaning.data());
}


// ==================================================
// Print All OpenCL Platform and Device Information:
// ==================================================
inline void PrintOpenclInfo()
{
  cl_int status;  // Returned status from OpenCL calls (test against CL_SUCCESS)

  std::fprintf(stderr, "PrintInfo:\n");

  // Find out how many platforms are attached here and get their ids:
  cl_uint numPlatforms;
  status = clGetPlatformIDs(0, nullptr, &numPlatforms);
  if(status != CL_SUCCESS)
    std::fprintf(stderr, "clGetPlatformIDs failed (1)\n");

  std::fprintf(stderr, "Number of Platforms = %d\n", numPlatforms);

  std::vector<cl_platform_id> platforms(numPlatforms);
  status = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
  if(status != CL_SUCCESS)
    std::fprintf(stderr, "clGetPlatformIDs failed (2)\n");

  for(cl_uint p = 0; p < numPlatforms; p++)
  {
    std::fprintf(stderr, "Platform #%d:\n", p);

    auto printPlatformInfo = [&](cl_platform_info param, const char* label) {
      size_t size;
      clGetPlatformInfo(platforms[p], param, 0, nullptr, &size);
      std::vector<char> str(size);
      clGetPlatformInfo(platforms[p], param, size, str.data(), nullptr);
      std::fprintf(stderr, "\t%s = '%s'\n", label, str.data());
    };

    printPlatformInfo(CL_PLATFORM_NAME, "Name   ");
    printPlatformInfo(CL_PLATFORM_VENDOR, "Vendor ");
    printPlatformInfo(CL_PLATFORM_VERSION, "Version");
    printPlatformInfo(CL_PLATFORM_PROFILE, "Profile");

    // Find out how many devices are attached to each platform and get their ids:
    cl_uint numDevices;
    status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if(status != CL_SUCCESS)
      std::fprintf(stderr, "clGetDeviceIDs failed (1)\n");

    std::fprintf(stderr, "\tNumber of Devices = %d\n", numDevices);

    std::vector<cl_device_id> devices(numDevices);
    status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
    if(status != CL_SUCCESS)
      std::fprintf(stderr, "clGetDeviceIDs failed (2)\n");

    for(cl_uint d = 0; d < numDevices; d++)
    {
      std::fprintf(stderr, "\tDevice #%d:\n", d);

      cl_device_type type;
      clGetDeviceInfo(devices[d], CL_DEVICE_TYPE, sizeof(type), &type, nullptr);
      std::fprintf(stderr, "\t\tType = 0x%04x = ", static_cast<unsigned int>(type));
      switch(type)
      {
        case CL_DEVICE_TYPE_CPU:
          std::fprintf(stderr, "CL_DEVICE_TYPE_CPU\n");
          break;
        case CL_DEVICE_TYPE_GPU:
          std::fprintf(stderr, "CL_DEVICE_TYPE_GPU\n");
          break;
        case CL_DEVICE_TYPE_ACCELERATOR:
          std::fprintf(stderr, "CL_DEVICE_TYPE_ACCELERATOR\n");
          break;
        default:
          std::fprintf(stderr, "Other...\n");
          break;
      }

      cl_uint vendorID;
      clGetDeviceInfo(devices[d], CL_DEVICE_VENDOR_ID, sizeof(vendorID), &vendorID, nullptr);
      std::fprintf(stderr, "\t\tDevice Vendor ID = 0x%04x ", vendorID);
      switch(vendorID)
      {
        case VendorID::AMD:
          std::fprintf(stderr, "(AMD)\n");
          break;
        case VendorID::INTEL:
          std::fprintf(stderr, "(INTEL)\n");
          break;
        case VendorID::NVIDIA:
          std::fprintf(stderr, "(NVIDIA)\n");
          break;
        case VendorID::APPLE:
          std::fprintf(stderr, "(APPLE)\n");
          break;
        default:
          std::fprintf(stderr, "(?)\n");
      }

      cl_uint ui;
      clGetDeviceInfo(devices[d], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(ui), &ui, nullptr);
      std::fprintf(stderr, "\t\tDevice Maximum Compute Units = %d\n", ui);

      clGetDeviceInfo(devices[d], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(ui), &ui, nullptr);
      std::fprintf(stderr, "\t\tDevice Maximum Work Item Dimensions = %d\n", ui);

      std::array<size_t, 3> sizes = {0, 0, 0};
      clGetDeviceInfo(devices[d], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(sizes), sizes.data(), nullptr);
      std::fprintf(stderr, "\t\tDevice Maximum Work Item Sizes = %zu x %zu x %zu\n", sizes[0], sizes[1], sizes[2]);

      size_t workGroupSize;
      clGetDeviceInfo(devices[d], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(workGroupSize), &workGroupSize, nullptr);
      std::fprintf(stderr, "\t\tDevice Maximum Work Group Size = %zu\n", workGroupSize);

      clGetDeviceInfo(devices[d], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(ui), &ui, nullptr);
      std::fprintf(stderr, "\t\tDevice Maximum Clock Frequency = %d MHz\n", ui);

      size_t extensionSize;
      clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, 0, nullptr, &extensionSize);
      std::vector<char> extensions(extensionSize);
      clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, extensionSize, extensions.data(), nullptr);

      std::fprintf(stderr, "\nDevice #%d's Extensions:\n", d);
      for(char& ch : extensions)
      {
        if(ch == ' ')
          ch = '\n';
      }
      std::fprintf(stderr, "%s\n", extensions.data());
    }
  }
  std::fprintf(stderr, "\n\n");
}


// ==================================================
// Select Best Available OpenCL Device:
// Priority order:
//   1. A GPU
//   2. An NVIDIA, AMD, or Apple GPU
//   3. An Intel GPU
//   4. An Intel CPU
// ==================================================
inline void SelectOpenclDevice(cl_platform_id* outPlatform, cl_device_id* outDevice)
{
  int            bestPlatform = -1;
  int            bestDevice   = -1;
  cl_device_type bestDeviceType;
  cl_uint        bestDeviceVendor;
  cl_int         status;  // Returned status from OpenCL calls (test against CL_SUCCESS)

  std::fprintf(stderr, "\nSelecting the OpenCL Platform and Device:\n");

  // Find out how many platforms are attached here and get their ids:
  cl_uint numPlatforms;
  status = clGetPlatformIDs(0, nullptr, &numPlatforms);
  if(status != CL_SUCCESS)
    std::fprintf(stderr, "clGetPlatformIDs failed (1)\n");

  std::vector<cl_platform_id> platforms(numPlatforms);
  status = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
  if(status != CL_SUCCESS)
    std::fprintf(stderr, "clGetPlatformIDs failed (2)\n");

  for(cl_uint p = 0; p < numPlatforms; p++)
  {
    // Find out how many devices are attached to each platform and get their ids:
    cl_uint numDevices;
    status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if(status != CL_SUCCESS)
      std::fprintf(stderr, "clGetDeviceIDs failed (1)\n");

    std::vector<cl_device_id> devices(numDevices);
    status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
    if(status != CL_SUCCESS)
      std::fprintf(stderr, "clGetDeviceIDs failed (2)\n");

    for(cl_uint d = 0; d < numDevices; d++)
    {
      cl_device_type type;
      cl_uint        vendor;

      clGetDeviceInfo(devices[d], CL_DEVICE_TYPE, sizeof(type), &type, nullptr);
      clGetDeviceInfo(devices[d], CL_DEVICE_VENDOR_ID, sizeof(vendor), &vendor, nullptr);

      // Select:
      if(bestPlatform < 0)  // Not yet holding anything -- we'll accept anything
      {
        bestPlatform     = static_cast<int>(p);
        bestDevice       = static_cast<int>(d);
        *outPlatform     = platforms[bestPlatform];
        *outDevice       = devices[bestDevice];
        bestDeviceType   = type;
        bestDeviceVendor = vendor;
      }
      else  // Holding something already -- can we do better?
      {
        if(bestDeviceType == CL_DEVICE_TYPE_CPU)  // Holding a CPU already -- switch to a GPU if possible
        {
          if(type == CL_DEVICE_TYPE_GPU)  // Found a GPU
          {
            // Switch to the GPU we just found
            bestPlatform     = static_cast<int>(p);
            bestDevice       = static_cast<int>(d);
            *outPlatform     = platforms[bestPlatform];
            *outDevice       = devices[bestDevice];
            bestDeviceType   = type;
            bestDeviceVendor = vendor;
          }
        }
        else  // Holding a GPU -- is a better GPU available?
        {
          if(bestDeviceVendor == VendorID::INTEL)  // Currently holding an Intel GPU
          {
            // We are assuming we just found a bigger, badder NVIDIA, AMD, or Apple GPU
            bestPlatform     = static_cast<int>(p);
            bestDevice       = static_cast<int>(d);
            *outPlatform     = platforms[bestPlatform];
            *outDevice       = devices[bestDevice];
            bestDeviceType   = type;
            bestDeviceVendor = vendor;
          }
        }
      }
    }
  }

  if(bestPlatform < 0)
  {
    std::fprintf(stderr, "Found no OpenCL devices!\n");
  }
  else
  {
    std::fprintf(stderr, "I have selected Platform #%d, Device #%d\n", bestPlatform, bestDevice);
    auto vendorName = Vendor(bestDeviceVendor);
    auto typeName   = Type(bestDeviceType);
    std::fprintf(stderr, "Vendor = %.*s, Type = %.*s\n", static_cast<int>(vendorName.size()), vendorName.data(),
                 static_cast<int>(typeName.size()), typeName.data());
  }
}


// ==================================================
// Get Vendor Name from Vendor ID:
// ==================================================
inline std::string_view Vendor(cl_uint v)
{
  switch(v)
  {
    case VendorID::AMD:
      return "AMD";
    case VendorID::INTEL:
      return "INTEL";
    case VendorID::NVIDIA:
      return "NVIDIA";
    case VendorID::APPLE:
      return "APPLE";
  }
  return "Unknown";
}


// ==================================================
// Get Device Type Name:
// ==================================================
inline std::string_view Type(cl_device_type t)
{
  switch(t)
  {
    case CL_DEVICE_TYPE_CPU:
      return "CL_DEVICE_TYPE_CPU";
    case CL_DEVICE_TYPE_GPU:
      return "CL_DEVICE_TYPE_GPU";
    case CL_DEVICE_TYPE_ACCELERATOR:
      return "CL_DEVICE_TYPE_ACCELERATOR";
  }
  return "Unknown";
}

}  // namespace CLUtils

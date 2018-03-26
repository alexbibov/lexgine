//This header contains miscelleneous functionality employed by various parts of Lexgine

#ifndef LEXGINE_CORE_MISC_MISC_H

#include <stdint.h>
#include <cmath>
#include <string>
#include <list>
#include <vector>

#include <d3d12.h>

#include "optional.h"

#include "../../../3rd_party/half/half.h"
#include "datetime.h"

namespace lexgine { namespace core { namespace misc {

//! Version of the renderer
enum class EngineAPI
{
    Direct3D12,
    Vulkan,
    Metal,
    OpenGL45    // not sure if ever gets implemented
};


//! Enumerates frame references used to define rotation transforms
enum class RotationFrame { LOCAL, GLOBAL };


//Implements triplet as generalization of std::pair yet more convenient then std::tuple
template<typename T1, typename T2, typename T3> struct triplet
{
    using value_type_1 = T1;
    using value_type_2 = T2;
    using value_type_3 = T3;

    T1 first;
    T2 second;
    T3 third;
};


//Enumerates standard object rendering modes
#define TW_RENDERING_MODE_DEFAULT            0		//draws object using default rendering mode. Most objects are required to support this mode.
#define TW_RENDERING_MODE_WIREFRAME          1		//draws object using "wire-frame" mode. Most objects are required to support this mode.
#define TW_RENDERING_MODE_WIREFRAME_COMBINED 2		//combines TW_RENDERING_MODE_DEFAULT and TW_RENDERING_MODE_WIREFRAME_COMBINED into single rendering request.
#define TW_RENDERING_MODE_SILHOUETTE		 3		//draws dark silhouette of the object. This mode is mainly used to render crepuscular  rays in post-processing pass.


//Enumerates some standard return codes
#define TW_INVALID_RETURN_VALUE 0xFFFFFFFF	//constant encoding an invalid value returned by some functions on failure


//! Helper template function that allows to check if provided value equals to any of values from the given list.
//! Note that all types from the list must be comparable with the reference value (i.e. must implement the corresponding == operators)
template<typename T1, typename T2, typename ... Rest>
bool equalsAny(T1 const& reference_expression, T2 const& val0, Rest ... tail)
{
    if (reference_expression == val0) return true;
    else return equalsAny(reference_expression, tail...);
}

// Case of empty comparison list, always assumed to have negative yield
template<typename T>
bool equalsAny(T const& reference_expression) { return false; }


//! Helper: generic dereference. Works correctly regardless of whether the underlying type is an actualpointer
template<typename T> struct dereference {
    static inline T const& resolve(T const& v) { return v; }
    static inline T const& resolve(T const* v) { return *v; }
};


//! special index value used to mark end of the currently assembled primitive (such as triangle strip) and
//! beginning of a new one
// NOTICE_TO_DEVELOPER: this value was selected to fit requirements of Direct3D12, Vulkan, and OpenGL
// Direct3D12 can use this one as well as 0xFFFF, Vulkan supports only this value, OpenGL can use any user-defined value but this one makes the most sense
// CHECK WHAT IS THE BEST SOLUTION FOR METAL
uint32_t const PrimitiveRestartIndex = 0xFFFFFFFF;


//! Default stream that gets rasterized
uint32_t const DefaultRasterizedStream = 0;
uint32_t const NoRasterizationStream = D3D12_SO_NO_RASTERIZED_STREAM;


/*! Converts single-byte string to multi-byte string assuming that the string can contain only characters from the basic ASCII table
 This function will not work properly for international strings
*/
std::wstring asciiStringToWstring(std::string const& str);

/*! Converts multi-byte string to single-byte string assuming that provided multi-byte string contains only ASCII characters.
 This functions will yield incorrect results for international characters
*/
std::string wstringToAsciiString(std::wstring const& wstr);

//! Reads ASCII text data from provided source file and returns them as an optional string (with undefined value in case of failure)
Optional<std::string> readAsciiTextFromSourceFile(std::string const& source_file);

//! Checks if requested file exists and returns 'true' in case of success
bool doesFileExist(std::string const& file_path);

/*! Retrieves size of requested file. Might be useful when loading binary data into pre-allocated memory chunks.
 In a case if the file does not exist or cannot be accessed, the function returns 0
 */
unsigned long long getFileSize(std::string const& file_path);

/*! Reads binary data from specified source file into provided memory chunk. It is assumed that there was enough memory capacity allocated
 for the chunk to receive the whole data. Use getFileSize(...) to determine required capacity. If the function succeeds, it returns 'true'.
 If the source file cannot be read, the function returns 'false'
 */
bool readBinaryDataFromSourceFile(std::string const& file_path, void* destination_memory_address);

//! Writes provided binary data to file. Returns 'true' in case of success and 'false' in case of failure
bool writeBinaryDataToFile(std::string const& file_path, void* source_memory_address, size_t data_size);


enum class DataFormat
{
    float64,
    float32,
    float16,
    int64,
    int32,
    int16,
    uint64,
    uint32,
    uint16,
    unknown
};


/*! Retrieves date and time of the last update applied to the given file. Returned time stamp is represented
    using UTC time format
*/
Optional<DateTime> getFileLastUpdatedTimeStamp(std::string const& file_path);

//! Retrieves list of files from given directory that match provided pattern (can include wildcards and question signs)
std::list<std::string> getFilesInDirectory(std::string const& directory_name, std::string const& name_pattern);


}}}

#define LEXGINE_CORE_MISC_MISC_H
#endif
#include <cstdint>
#include <iterator>
#include <memory>

#ifndef XML2BIN_BASEDEFS_H
#define XML2BIN_BASEDEFS_H

template <class T>
constexpr auto pi = T(3.141592653589793238);

#if !defined(__GNUC__) || __GNUC__ > 7
#define CPP17_FILESYSTEM_SUPPORT 1
#endif

typedef enum _tag_Units:std::uint32_t
{
	CHU_MILLIMETERS,
	CHU_METERS, /*default*/
	CHU_KILOMETERS,
	CHU_INVALID_METRIC
} Units, MetricId;

enum ObjectTypeId
{
	ObjectPoly,
	ObjectSource,
	ObjectPlain
};

enum class ConstantDomainDataId
{
	SurfaceLand, SurfaceWater
};

#endif //XML2BIN_BASEDEFS_H

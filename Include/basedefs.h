#include <cstdint>

#ifndef XML2BIN_BASEDEFS_H
#define XML2BIN_BASEDEFS_H

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

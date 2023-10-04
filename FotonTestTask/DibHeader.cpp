#include "DibHeader.h"
#include "BmpDownscaling.h"

DibHeader::DibHeader()
{
}

DibHeader::DibHeader(const RequiredTiffData& tiffData)
{
	imageHeight = tiffData.destLengthPx;
	imageWidth = tiffData.destWidthPx;
	imageSize = tiffData.destStrideBytes * tiffData.destLengthPx;
}

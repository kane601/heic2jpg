// heic2jpeg.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "Heic2jpeg.h"
#include "StringUtil.h"
#include "VideoDecoder.h"
#include "Heic2jpegMacroDef.h"

using namespace cv;
using namespace HEIF;

CHeic2jpeg::CHeic2jpeg()
	:m_heifReader(HEIF::Reader::Create())
	, m_chunkData(NULL)
{
}

CHeic2jpeg::~CHeic2jpeg()
{
	HEIF::Reader::Destroy(m_heifReader);
	m_heifReader = NULL;
	ARRYDELETE(m_chunkData);
}

#define TEST_RESULT(cmd,errorCode)\
{\
	result = (cmd);\
	if (HEIF::ErrorCode::OK != result)\
	{\
	printf("%s:%d %s got %x\n",__FUNCTION__,__LINE__,#cmd,result);\
	return errorCode;\
	};\
}

int CHeic2jpeg::SetHeifFilePath(const char* heifPath)
{
	HEIF::ErrorCode result = HEIF::ErrorCode::OK;
	TEST_RESULT((m_heifReader->initialize(heifPath)), -1);

	HEIF::FileInformation fileInformation;
	TEST_RESULT((m_heifReader->getFileInformation(fileInformation)), -2);
	return 0;
}

ITL_ERT CHeic2jpeg::heif2thumbnail(const wchar_t *wzSrc, const wchar_t *wzDst)
{
	CCritLock Lock(m_Datalock);
	ITL_ERT eRet = ITL_ERT_SUCCESS;
	do
	{
		ERR_BREAK(NULL == wzSrc || NULL == wzDst, eRet, ITL_ERT_INVALUD_PARS);
		std::string inputFileName = ToString(wzSrc);
		std::string outputFileName = ToString(wzDst);
		ERR_BREAK(0 != SetHeifFilePath(inputFileName.c_str()), eRet, ITL_ERT_INSTANCE_HEIF_FAIL);

		HEIF::FileInformation fileInformation;
		HEIF::ErrorCode result = HEIF::ErrorCode::OK;
		ERR_BREAK(result != m_heifReader->getFileInformation(fileInformation), eRet, ITL_ERT_GETINFO_FAIL);

		const auto metaBoxFeatures = fileInformation.rootMetaBoxInformation.features;
		if (!(metaBoxFeatures & HEIF::MetaBoxFeatureEnum::HasThumbnails)) 
		{
			eRet = ITL_ERT_NO_THUMBNAIL_DATA;
			break;
		}

		HEIF::Array<HEIF::ImageId> gridIdArray;
		ERR_BREAK(result != m_heifReader->getItemListByType("grid", gridIdArray), eRet, ITL_ERT_NO_THUMBNAIL_DATA);

		const HEIF::ImageId masterId = gridIdArray[0].get();
		ERR_BREAK(result != m_heifReader->getReferencedToItemListByType(masterId, "thmb", gridIdArray), eRet, ITL_ERT_NO_THUMBNAIL_DATA);

		eRet = DecodeImageId2File(gridIdArray[0].get(), outputFileName.c_str());
	} while (0);

	return 	eRet;
}

ITL_ERT CHeic2jpeg::DecodeImageId2File(DWORD imgId, const char* outFilePath)
{
	ITL_ERT eRet = ITL_ERT_SUCCESS;
	do 
	{
		ERR_BREAK(NULL == m_heifReader || NULL == outFilePath, eRet, ITL_ERT_INSTANCE_HEIF_FAIL);
		const HEIF::ImageId  imageId = (const HEIF::ImageId)imgId;
		uint64_t memoryBufferSize = 0;
		HEIF::ErrorCode result = m_heifReader->getItemDataWithDecoderParameters(imageId, 0, memoryBufferSize);
		ERR_BREAK(memoryBufferSize <= 0 || memoryBufferSize > 64 * 1024 * 1024LL, eRet, ITL_ERT_GET_DECODERPARA_FAIL);

		memoryBufferSize = (memoryBufferSize + 4096 - 1) & (~(4096 - 1));
		BYTE*  chunkData = new BYTE[memoryBufferSize];
		ERR_BREAK(NULL == chunkData, eRet, ITL_ERT_NEW_MEMORY_FAIL);
		memset(chunkData, 0, memoryBufferSize);
		ERR_BREAK(ErrorCode::OK != m_heifReader->getItemDataWithDecoderParameters(imageId, chunkData, memoryBufferSize), eRet, ITL_ERT_GET_DECODERPARA_FAIL);

		VideoDecoder videoDecode;
		ERR_BREAK(videoDecode.SetDecodeId(AV_CODEC_ID_HEVC) < 0, eRet, ITL_ERT_INIT_CODEC_FAIL);

		videoDecode.SendVideoData(chunkData, memoryBufferSize);
		ARRYDELETE(chunkData);

		BYTE* rgbData = 0;
		int rgbDataLen = 0;
		ERR_BREAK(videoDecode.GetRgb24Data(&rgbData, rgbDataLen) < 0, eRet, ITL_ERT_RECEIVE_FRAME_FAIL);

		cv::Size chunkSize(videoDecode.m_videoWidth, videoDecode.m_videoHeight);
		cv::Mat rgbMat(chunkSize, CV_8UC3);
		memcpy(rgbMat.data, rgbData, rgbDataLen);

		cv::transpose(rgbMat, rgbMat);
		cv::flip(rgbMat, rgbMat, 1);
		ERR_BREAK(!cv::imwrite((std::string)outFilePath, rgbMat), eRet, ITL_ERT_WRITE_INMAGE_FAIL);
	} while (0);

	return 	eRet;
}

ITL_ERT CHeic2jpeg::heif2jpeg(const wchar_t *wzSrc, const wchar_t *wzDst)
{
	CCritLock Lock(m_Datalock);
	ITL_ERT eRet = ITL_ERT_SUCCESS;
	do 
	{
		ERR_BREAK(NULL == wzSrc || NULL == wzDst, eRet, ITL_ERT_INVALUD_PARS);
		std::string inputFileName = ToString(wzSrc);
		std::string outputFileName = ToString(wzDst);

		ERR_BREAK(0 != SetHeifFilePath(inputFileName.c_str()), eRet, ITL_ERT_INSTANCE_HEIF_FAIL);

		HEIF::FileInformation fileInformation;
		HEIF::ErrorCode result = HEIF::ErrorCode::OK;
		ERR_BREAK(result != m_heifReader->getFileInformation(fileInformation), eRet, ITL_ERT_GETINFO_FAIL);

		if (!(fileInformation.features & FileFeatureEnum::HasSingleImage ||
			fileInformation.features & FileFeatureEnum::HasImageCollection))
		{
#ifdef _DEBUG
			cout << "The file don't have images in the Metabox. " << endl;
#endif
			eRet = ITL_ERT_NO_IMAGE_DATA;
			break;
		}

		HEIF::Array<HEIF::ImageId> imgIdArray; 
		HEIF::Grid gridItem;
		if (HEIF::ErrorCode::OK == m_heifReader->getItemListByType("grid", imgIdArray)
			&& fileInformation.features & FileFeatureEnum::HasImageCollection
			&& HEIF::ErrorCode::OK == m_heifReader->getItem(imgIdArray[0].get(), gridItem))
		{
			eRet = ConvertGridItem2File(&gridItem, outputFileName.c_str());
			break;
		}

		ERR_BREAK(HEIF::ErrorCode::OK != m_heifReader->getMasterImages(imgIdArray), eRet, ITL_ERT_GET_METABOX_FAIL);
		eRet = DecodeImageId2File(imgIdArray[0].get(), outputFileName.c_str());
	} while (0);

	return eRet;
}

ITL_ERT CHeic2jpeg::ConvertGridItem2File(void* grid, const char* outFilePath)
{
	ITL_ERT eRet = ITL_ERT_SUCCESS;
	do 
	{
		ERR_BREAK(NULL == m_heifReader || NULL == grid || NULL == outFilePath, eRet, ITL_ERT_INSTANCE_HEIF_FAIL);
		VideoDecoder videoDecode;
		ERR_BREAK(videoDecode.SetDecodeId(AV_CODEC_ID_HEVC) < 0, eRet, ITL_ERT_INIT_CODEC_FAIL);

		HEIF::Grid* gridItems = (HEIF::Grid*)grid;
		int outputWidth = (gridItems->outputWidth + 0x200 - 1) &(~(0x200 - 1));
		int outputHeight = (gridItems->outputHeight + 0x200 - 1) &(~(0x200 - 1));
		HEIF::ErrorCode result = HEIF::ErrorCode::OK;
		int left = 0; int top = 0; uint64_t lastMemBufferSize = 0;

		cv::Mat3b mainMat(outputHeight, outputWidth, cv::Vec3b(0, 0, 0));
		for (auto imgId : gridItems->imageIds)
		{
			uint64_t memoryBufferSize = 0;
			result = m_heifReader->getItemDataWithDecoderParameters(imgId, 0, memoryBufferSize);
			if (memoryBufferSize <= 0 || memoryBufferSize > 64 * 1024 * 1024LL)
				break;

			memoryBufferSize = (memoryBufferSize + 4096 - 1) & (~(4096 - 1));
			if (memoryBufferSize > lastMemBufferSize)
			{
				lastMemBufferSize = memoryBufferSize;
				ARRYDELETE(m_chunkData);
				m_chunkData = new BYTE[memoryBufferSize];
			}
			result = m_heifReader->getItemDataWithDecoderParameters(imgId, m_chunkData, memoryBufferSize);
			if (HEIF::ErrorCode::OK != result)
				break;

			videoDecode.SendVideoData(m_chunkData, memoryBufferSize);
			BYTE* rgbData = 0; int rgbDataLen = 0;
			if (videoDecode.GetRgb24Data(&rgbData, rgbDataLen) < 0)
				break;

			cv::Size chunkSize(videoDecode.m_videoWidth, videoDecode.m_videoHeight);
			cv::Rect targetRect(left, top, chunkSize.width, chunkSize.height);
			cv::Mat3b  subMat(videoDecode.m_videoWidth, videoDecode.m_videoHeight, cv::Vec3b(0, 0, 0));
			memcpy(subMat.data, rgbData, rgbDataLen);
			subMat.copyTo(mainMat(targetRect));

			left += chunkSize.width;
			if (left >= gridItems->outputWidth)
			{
				left = 0;
				top += chunkSize.height;
			}
		}
		ARRYDELETE(m_chunkData);

		cv::Mat3b  imgDst = mainMat(Rect(0, 0, gridItems->outputWidth, gridItems->outputHeight));
		cv::transpose(imgDst, imgDst);
		cv::flip(imgDst, imgDst, 1);
		ERR_BREAK(!cv::imwrite((std::string)outFilePath, imgDst), eRet, ITL_ERT_WRITE_INMAGE_FAIL);
	} while (0);

	return eRet;
}
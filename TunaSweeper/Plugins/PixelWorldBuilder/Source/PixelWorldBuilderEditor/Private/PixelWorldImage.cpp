#include "PixelWorldCore.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    uint32 BigEndian32(const uint8* P)
    {
        return (uint32(P[0])<<24)|(uint32(P[1])<<16)|(uint32(P[2])<<8)|P[3];
    }

    bool LoadPng(const FString& Path, bool bHeight, FImage& Image, FText& Error)
    {
        const FString Full=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(),Path);
        const int64 FileSize=IFileManager::Get().FileSize(*Full);
        TArray<uint8> Bytes;
        if(Path.IsEmpty() || FileSize<33 || FileSize>64*1024*1024 || !FFileHelper::LoadFileToArray(Bytes,*Full))
        { Error=PixelWorld::Text(TEXT("Error.Read")); return false; }
        const uint8 Signature[]={137,80,78,71,13,10,26,10};
        if(FMemory::Memcmp(Bytes.GetData(),Signature,8)!=0 || BigEndian32(Bytes.GetData()+8)!=13 || FMemory::Memcmp(Bytes.GetData()+12,"IHDR",4)!=0)
        { Error=PixelWorld::Text(TEXT("Error.Png")); return false; }
        const uint32 Width=BigEndian32(Bytes.GetData()+16), Height=BigEndian32(Bytes.GetData()+20);
        if(Width==0 || Height==0 || Width>2048 || Height>2048)
        { Error=PixelWorld::Text(TEXT("Error.ImageSize")); return false; }
        // Inspect IHDR before any allocation or bit-depth conversion. The raw-data route
        // preserves encoded samples; FImage::GammaSpace is metadata, not a conversion.
        const bool bFormat=bHeight ? (Bytes[24]==16 && Bytes[25]==0) :
            (Bytes[24]==8 && (Bytes[25]==2 || Bytes[25]==6 || Bytes[25]==3));
        if(!bFormat) { Error=PixelWorld::Text(bHeight?TEXT("Error.HeightFormat"):TEXT("Error.ColorFormat")); return false; }
        if(!FImageUtils::DecompressImage(Bytes.GetData(),Bytes.Num(),Image))
        { Error=PixelWorld::Text(TEXT("Error.Png")); return false; }
        if(Image.SizeX!=Width || Image.SizeY!=Height || Image.NumSlices!=1 || Image.Format!=(bHeight?ERawImageFormat::G16:ERawImageFormat::BGRA8))
        { Error=PixelWorld::Text(bHeight?TEXT("Error.HeightFormat"):TEXT("Error.ColorFormat")); return false; }
        return true;
    }
}

bool PixelWorld::ReadColorPng(const FString& Path, FPixelWorldImage& Out, FText& Error)
{
    Out={}; Error=FText::GetEmpty();
    FImage Image;
    if(!LoadPng(Path,false,Image,Error)) return false;
    Out.Width=Image.SizeX; Out.Height=Image.SizeY;
    const auto Pixels=Image.AsBGRA8();
    Out.Pixels.Append(Pixels.GetData(),static_cast<int32>(Pixels.Num()));
    return true;
}

bool PixelWorld::ReadHeightPng(const FString& Path, int32 Width, int32 Height, TArray<uint16>& Out, FText& Error)
{
    FImage Image;
    if(!LoadPng(Path,true,Image,Error)) return false;
    if(Image.SizeX!=Width || Image.SizeY!=Height)
    { Error=Text(TEXT("Error.HeightFormat")); return false; }
    const auto Values=Image.AsG16();
    Out.Append(Values.GetData(),static_cast<int32>(Values.Num()));
    return true;
}

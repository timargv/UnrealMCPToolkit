// Copyright (c) 2026 Timur Ragimkhanov. Licensed under the MIT License.

#include "Tools/TakeScreenshotTool.h"

#include "ModelContextProtocolToolResults.h"
#include "Editor.h"                          // GEditor
#include "UnrealClient.h"                    // FViewport
#include "ImageCore.h"                       // FImage, FImageView, ERawImageFormat
#include "ImageUtils.h"                      // FImageUtils
#include "Misc/FileHelper.h"                 // FFileHelper
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFileManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "JsonDomBuilder.h"

namespace
{
	// --- Context-safety knobs -------------------------------------------------
	constexpr int32 DefaultWidth   = 1280;
	constexpr int32 MinWidth       = 256;
	constexpr int32 MaxWidth       = 3840;
	constexpr int32 JpegQuality    = 70;

	/**
	 * Hard cap on the encoded byte size that may be inlined as base64 (256 KB).
	 * Above this the image is never embedded — only its path is returned — so a
	 * large screenshot can never flood the model's context window.
	 */
	constexpr int64 InlineByteCap  = 256 * 1024;  // strict cap: inline base64 only for small images
}

FString FTakeScreenshotTool::GetName() const
{
	return TEXT("take_screenshot");
}

FString FTakeScreenshotTool::GetDescription() const
{
	return TEXT(
		"Captures the active Unreal Editor viewport, downscaled (aspect preserved) "
		"to a maximum width, encodes it as JPEG or PNG and saves it under "
		"<Project>/Saved/Screenshots/MCP/. By default returns a small text result "
		"with the absolute file path, pixel dimensions and file size in KB (no "
		"base64) so it is safe for the model's context. Pass 'inline': true to also "
		"embed the image, but it is embedded only when small enough; otherwise the "
		"path is returned with a note to open the file. Optional parameters: 'width' "
		"(integer, default 1280, clamped 256..3840), 'format' (\"jpg\" or \"png\", "
		"default \"jpg\"), 'inline' (boolean, default false).");
}

TSharedPtr<FJsonObject> FTakeScreenshotTool::GetInputJsonSchema() const
{
	FJsonDomBuilder::FObject WidthProp;
	WidthProp.Set(TEXT("type"), TEXT("integer"));
	WidthProp.Set(TEXT("description"),
		TEXT("Maximum long-edge width in pixels (aspect preserved, never upscaled). Default 1280, clamped 256..3840."));

	FJsonDomBuilder::FArray FormatEnum;
	FormatEnum.Add(TEXT("jpg"));
	FormatEnum.Add(TEXT("png"));

	FJsonDomBuilder::FObject FormatProp;
	FormatProp.Set(TEXT("type"), TEXT("string"));
	FormatProp.Set(TEXT("enum"), FormatEnum);
	FormatProp.Set(TEXT("description"), TEXT("Output image format: \"jpg\" (default, smaller) or \"png\" (lossless)."));

	FJsonDomBuilder::FObject InlineProp;
	InlineProp.Set(TEXT("type"), TEXT("boolean"));
	InlineProp.Set(TEXT("description"),
		TEXT("When true, also embed the image inline if it is below the size cap; otherwise only the file path is returned. Default false."));

	FJsonDomBuilder::FObject Props;
	Props.Set(TEXT("width"), WidthProp);
	Props.Set(TEXT("format"), FormatProp);
	Props.Set(TEXT("inline"), InlineProp);

	FJsonDomBuilder::FObject Schema;
	Schema.Set(TEXT("type"), TEXT("object"));
	Schema.Set(TEXT("properties"), Props);

	return Schema.AsJsonObject().ToSharedPtr();
}

FModelContextProtocolToolResult FTakeScreenshotTool::Run(const TSharedPtr<FJsonObject>& Params)
{
	using namespace UE::ModelContextProtocol;

	// --- Parse parameters (all optional, clamped) ----------------------------
	int32 MaxLongEdge = DefaultWidth;
	if (Params.IsValid())
	{
		double WidthValue = 0.0;
		if (Params->TryGetNumberField(TEXT("width"), WidthValue))
		{
			MaxLongEdge = FMath::Clamp(FMath::RoundToInt(WidthValue), MinWidth, MaxWidth);
		}
	}

	FString Format = TEXT("jpg");
	if (Params.IsValid())
	{
		FString FormatValue;
		if (Params->TryGetStringField(TEXT("format"), FormatValue))
		{
			FormatValue = FormatValue.ToLower();
			if (FormatValue == TEXT("png"))
			{
				Format = TEXT("png");
			}
			else if (FormatValue == TEXT("jpg") || FormatValue == TEXT("jpeg"))
			{
				Format = TEXT("jpg");
			}
			// any other value silently falls back to the jpg default
		}
	}

	bool bInline = false;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("inline"), bInline);
	}

	// --- Capture the active editor viewport (synchronous) --------------------
	FViewport* Viewport = GEditor ? GEditor->GetActiveViewport() : nullptr;
	if (!Viewport)
	{
		return MakeErrorResult(TEXT("No active editor viewport to capture. Focus a level viewport and try again."));
	}

	const FIntPoint Size = Viewport->GetSizeXY();
	if (Size.X <= 0 || Size.Y <= 0)
	{
		return MakeErrorResult(TEXT("Active editor viewport has a zero size."));
	}

	// FViewport::ReadPixels flushes rendering commands internally, so the pixel
	// buffer is fully populated when this returns — no delegate / next-frame wait.
	TArray<FColor> Pixels;
	if (!Viewport->ReadPixels(Pixels) || Pixels.Num() < Size.X * Size.Y)
	{
		return MakeErrorResult(TEXT("Failed to read pixels from the active editor viewport."));
	}

	// --- Downscale (aspect preserved, never upscale) -------------------------
	const int32 LongEdge = FMath::Max(Size.X, Size.Y);
	const double Scale = (LongEdge > MaxLongEdge) ? double(MaxLongEdge) / double(LongEdge) : 1.0;
	const int32 DstW = FMath::Max(1, FMath::RoundToInt(Size.X * Scale));
	const int32 DstH = FMath::Max(1, FMath::RoundToInt(Size.Y * Scale));

	const FImageView SrcView(Pixels.GetData(), Size.X, Size.Y, EGammaSpace::sRGB);

	FImage OutImage;
	if (DstW != Size.X || DstH != Size.Y)
	{
		// FImageView has no ResizeTo; copy into an FImage, then use FImage::ResizeTo (member).
		FImage SrcImage;
		SrcView.CopyTo(SrcImage, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		SrcImage.ResizeTo(OutImage, DstW, DstH, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	}
	else
	{
		SrcView.CopyTo(OutImage, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	}

	// --- Encode to memory (so we can measure size before deciding to inline) --
	const bool bIsPng = (Format == TEXT("png"));
	const TCHAR* FormatExt = bIsPng ? TEXT("png") : TEXT("jpg");
	const int32 Quality = bIsPng ? 0 : JpegQuality;

	TArray64<uint8> EncodedBytes;
	if (!FImageUtils::CompressImage(EncodedBytes, FormatExt, OutImage, Quality) || EncodedBytes.Num() == 0)
	{
		return MakeErrorResult(FString::Printf(TEXT("Failed to encode the screenshot as %s."), FormatExt));
	}

	// --- Write to disk (synchronous; file is complete once this returns) ------
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("MCP");
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);

	const FString FileName = FString::Printf(
		TEXT("Viewport_%s.%s"),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),
		FormatExt);
	const FString RelPath = Dir / FileName;

	if (!FFileHelper::SaveArrayToFile(EncodedBytes, *RelPath))
	{
		return MakeErrorResult(FString::Printf(TEXT("Failed to write the screenshot to '%s'."), *RelPath));
	}

	const FString AbsPath = FPaths::ConvertRelativePathToFull(RelPath);
	const int64 ByteSize = EncodedBytes.Num();
	const double SizeKB = double(ByteSize) / 1024.0;

	// --- Build the result (context-safe: path-first, never a blind base64) ----
	FString Message = FString::Printf(
		TEXT("Screenshot saved.\nPath: %s\nDimensions: %dx%d px\nFormat: %s\nSize: %.1f KB"),
		*AbsPath, DstW, DstH, FormatExt, SizeKB);

	if (!bInline)
	{
		return MakeTextResult(Message);
	}

	// Inline requested: only embed if the encoded image is below the hard cap.
	if (ByteSize > InlineByteCap)
	{
		Message += FString::Printf(
			TEXT("\nInline image omitted (%.0f KB > %.0f KB cap) — read the file at the path above."),
			SizeKB, double(InlineByteCap) / 1024.0);
		return MakeTextResult(Message);
	}

	// Build an image content block, then add a sibling text block so the path /
	// dimensions travel with the inline image. The public helpers each build a
	// single-block result, so we merge them on the raw JSON object the result
	// already exposes (FModelContextProtocolToolResult : FJsonObjectWrapper).
	const FString MimeType = bIsPng ? TEXT("image/png") : TEXT("image/jpeg");
	// ByteSize is below the 256 KB cap here, so it fits an int32 view safely.
	FModelContextProtocolToolResult Result = MakeImageResult(
		MimeType, TArrayView<uint8>(EncodedBytes.GetData(), static_cast<int32>(ByteSize)));

	if (Result.JsonObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* ContentArray = nullptr;
		if (Result.JsonObject->TryGetArrayField(TEXT("content"), ContentArray) && ContentArray)
		{
			TArray<TSharedPtr<FJsonValue>> NewContent = *ContentArray;
			NewContent.Add(MakeShared<FJsonValueObject>(MakeTextContentObject(Message)));
			Result.JsonObject->SetArrayField(TEXT("content"), NewContent);
		}
	}

	return Result;
}

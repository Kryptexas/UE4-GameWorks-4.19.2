// Copyright 2017 Google Inc.

#include "GoogleARCoreCameraImageBlitter.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DDynamic.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GoogleARCorePassthroughCameraRenderer.h"

#if PLATFORM_ANDROID
#include "AndroidApplication.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGoogleARCoreImageBlitter, Log, All);

FGoogleARCoreDeviceCameraBlitter::FGoogleARCoreDeviceCameraBlitter()
  : CurrentCameraCopy(0)
  , BlitShaderProgram(0)
  , BlitShaderProgram_Uniform_CameraTexture(0)
  , BlitShaderProgram_Attribute_InPos(0)
  , FrameBufferObject(0)
  , VertexBufferObject(0)
{
}

FGoogleARCoreDeviceCameraBlitter::~FGoogleARCoreDeviceCameraBlitter()
{
	for(int32 i = 0; i < CameraCopies.Num(); i++)
	{
		delete CameraCopies[i];
	}
}

UTexture *FGoogleARCoreDeviceCameraBlitter::GetLastCameraImageTexture()
{
	if(CameraCopies.Num())
	{
		int32 LastCameraImage = static_cast<int32>(CurrentCameraCopy) - 1;
		if(LastCameraImage < 0)
		{
			LastCameraImage = CameraCopies.Num() - 1;
		}
		return CameraCopies[LastCameraImage];
	}
	return nullptr;
}

void FGoogleARCoreDeviceCameraBlitter::LateInit(FIntPoint ImageSize)
{
#if PLATFORM_ANDROID
	FIntPoint CameraSize = ImageSize;

	// Create a ring buffer of UTexture2Ds to store snapshots of
	// the camera texture, if we don't already have them.
	if(!CameraCopies.Num())
	{
		for(uint32 i = 0; i < 5; i++)
		{
			// Make the texture itself.
			UTexture *CameraCopy = UTexture2D::CreateTransient(CameraSize.X, CameraSize.Y, PF_R8G8B8A8);
			check(CameraCopy);
			CameraCopy->AddToRoot();
			FTextureResource *resource = CameraCopy->CreateResource();
			CameraCopy->UpdateResource();
			CameraCopies.Add(CameraCopy);

			// Make a place to store the OpenGL texture ID, read
			// back from the RHI within the render thread.
			uint32 *TextureId = new uint32;
			*TextureId = 0;
			CameraCopyIds.Add(TextureId);

			// We have to get the OpenGL texture ID on the render
			// thread, after the native resource has been
			// initialized.
			ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
				InitCameraBlitter,
				FTexture2DResource *, resource, ((FTexture2DResource *)resource),
				uint32 *, TextureId, TextureId,
				FIntPoint, CameraSize, CameraSize,
				{
					resource->InitRHI();
					*TextureId = *reinterpret_cast<uint32*>(resource->GetTexture2DRHI()->GetNativeResource());
				});
		}
	}

	// Compile and link the shader program.
	if(!BlitShaderProgram)
	{
		GLint CompileSuccess = 0;

		// Compile VS.
		const char *VsText =
			"#version 300 es\n"
			"#extension GL_OES_EGL_image_external_essl3 : require\n"
			"precision mediump float;\n"
			"in vec2 inPos;\n"
			"out vec2 uvs;\n"
			"void main() {\n"
			"	uvs = inPos;\n"
			"	vec2 clipSpace = inPos * 2.0 - vec2(1.0, 1.0);\n"
			"	gl_Position = vec4(clipSpace, 0.0, 1.0);\n"
			"}\n";
		GLuint Vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(Vs, 1, &VsText, NULL);
		glCompileShader(Vs);
		glGetShaderiv(Vs, GL_COMPILE_STATUS, &CompileSuccess);
		if(!CompileSuccess) {
			GLint ShaderLogLength = 0;
			glGetShaderiv(Vs, GL_INFO_LOG_LENGTH, &ShaderLogLength);
			char *ShaderLog = new char[ShaderLogLength + 1];
			glGetShaderInfoLog(Vs, ShaderLogLength, &ShaderLogLength, ShaderLog);
			ShaderLog[ShaderLogLength] = 0;
			UE_LOG(LogGoogleARCoreImageBlitter, Error, TEXT("Shader compile error: %s"), *FString(ShaderLog));
			check(CompileSuccess);
		}

		// Compile FS.
		const char *FsText =
			"#version 300 es\n"
			"#extension GL_OES_EGL_image_external_essl3 : require\n"
			"precision mediump float;\n"
			"uniform samplerExternalOES cameraTexture;\n"
			"in vec2 uvs;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"void main() {\n"
			"	outColor.xyz = texture(cameraTexture, uvs).rgb;\n"
			"	outColor.w = 1.0;\n"
			"}\n";
		GLuint Fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(Fs, 1, &FsText, NULL);
		glCompileShader(Fs);
		glGetShaderiv(Fs, GL_COMPILE_STATUS, &CompileSuccess);
		if(!CompileSuccess)
		{
			GLint ShaderLogLength = 0;
			glGetShaderiv(Fs, GL_INFO_LOG_LENGTH, &ShaderLogLength);
			char *ShaderLog = new char[ShaderLogLength + 1];
			glGetShaderInfoLog(Fs, ShaderLogLength, &ShaderLogLength, ShaderLog);
			ShaderLog[ShaderLogLength] = 0;
			UE_LOG(LogGoogleARCoreImageBlitter, Error, TEXT("Shader compile error: %s"), *FString(ShaderLog));
			check(CompileSuccess);
		}

		// Link program.
		BlitShaderProgram = glCreateProgram();
		glAttachShader(BlitShaderProgram, Vs);
		glAttachShader(BlitShaderProgram, Fs);
		glLinkProgram(BlitShaderProgram);

		glGetProgramiv(BlitShaderProgram, GL_LINK_STATUS, &CompileSuccess);
		if(!CompileSuccess)
		{
			GLint ShaderLogLength = 0;
			glGetProgramiv(BlitShaderProgram, GL_INFO_LOG_LENGTH, &ShaderLogLength);
			char *ShaderLog = new char[ShaderLogLength + 1];
			glGetProgramInfoLog(Fs, ShaderLogLength, &ShaderLogLength, ShaderLog);
			ShaderLog[ShaderLogLength] = 0;
			UE_LOG(LogGoogleARCoreImageBlitter, Error, TEXT("Shader link error: %s"), *FString(ShaderLog));
			check(CompileSuccess);
		}

		// Get uniform and attribute locations.
		BlitShaderProgram_Uniform_CameraTexture = glGetUniformLocation(BlitShaderProgram, "cameraTexture");
		BlitShaderProgram_Attribute_InPos = glGetAttribLocation(BlitShaderProgram, "inPos");

		// Clean up unneeded references.
		glDeleteShader(Vs);
		glDeleteShader(Fs);

		// Sanity check.
		check(glGetError() == GL_NO_ERROR);
	}

	// Create FBO.
	if(!FrameBufferObject)
	{
		glGenFramebuffers(1, &FrameBufferObject);
	}

	// Create VBO.
	if(!VertexBufferObject)
	{
		glGenBuffers(1, &VertexBufferObject);
	}
#endif
}

void FGoogleARCoreDeviceCameraBlitter::DoBlit(uint32_t CameraTextureId, FIntPoint ImageSize)
{
#if PLATFORM_ANDROID
	FIntPoint CameraSize = ImageSize;

	LateInit(ImageSize);

	if (CameraCopyIds.Num())
	{
		uint32 TextureId = *CameraCopyIds[CurrentCameraCopy];

		if (TextureId)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferObject);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureId, 0);

			glViewport(0, 0, CameraSize.X, CameraSize.Y);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glDisable(GL_STENCIL_TEST);

			glUseProgram(BlitShaderProgram);

			glEnableVertexAttribArray(BlitShaderProgram_Attribute_InPos);

			float Verts[6 * 2] = {
				0.0f, 1.0f,
				1.0f, 1.0f,
				1.0f, 0.0f,

				0.0f, 1.0f,
				1.0f, 0.0f,
				0.0f, 0.0f
			};

			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);

			glVertexAttribPointer(BlitShaderProgram_Attribute_InPos, 2, GL_FLOAT, false, 0, 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, CameraTextureId);
			glUniform1i(BlitShaderProgram_Uniform_CameraTexture, 0);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
			glDisableVertexAttribArray(BlitShaderProgram_Attribute_InPos);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glFlush();

		}

		// Update the dynamic material instance with the newest
		// texture.
		UMaterialInstanceDynamic *DynMat = Cast<UMaterialInstanceDynamic>(
			GetDefault<UGoogleARCoreCameraOverlayMaterialLoader>()->DefaultCameraOverlayMaterial);
		DynMat->SetTextureParameterValue(FName("CameraTexture"), CameraCopies[CurrentCameraCopy]);

		CurrentCameraCopy++;
		CurrentCameraCopy %= CameraCopies.Num();
	}
#endif
}

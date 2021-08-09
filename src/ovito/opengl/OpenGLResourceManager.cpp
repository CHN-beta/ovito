////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/core/Core.h>
#include "OpenGLResourceManager.h"

#include <QThreadStorage>
#include <QOpenGLContextGroup>

namespace Ovito {

static QThreadStorage<OpenGLResourceManager*> glContextManagerStorage;

OpenGLResourceManager* OpenGLResourceManager::instance() 
{
	if(!glContextManagerStorage.hasLocalData()) {
		glContextManagerStorage.setLocalData(new OpenGLResourceManager());
	}
	return glContextManagerStorage.localData();
}

/******************************************************************************
* Create an OpenGL texture object for a QImage.
******************************************************************************/
QOpenGLTexture* OpenGLResourceManager::uploadImage(const QImage& image, ResourceFrameHandle resourceFrame, QOpenGLTexture::MipMapGeneration genMipMaps)
{
	OVITO_ASSERT(!image.isNull());

    // Check if this image has already been uploaded to the GPU.
	RendererResourceKey<OpenGLResourceManager, quint64, QOpenGLContextGroup*> cacheKey{ image.cacheKey(), QOpenGLContextGroup::currentContextGroup() };
    std::unique_ptr<QOpenGLTexture>& texture = lookup<std::unique_ptr<QOpenGLTexture>>(cacheKey, resourceFrame);

	// Create the texture object.
    if(!texture) {
		texture = std::make_unique<QOpenGLTexture>(image, genMipMaps);
		if(genMipMaps == QOpenGLTexture::DontGenerateMipMaps) {
			texture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
		}
	}

	return texture.get();
}

}	// End of namespace

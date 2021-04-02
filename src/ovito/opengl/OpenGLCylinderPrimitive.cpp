////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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
#include "OpenGLCylinderPrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLCylinderPrimitive::OpenGLCylinderPrimitive(OpenGLSceneRenderer* renderer, CylinderPrimitive::Shape shape, ShadingMode shadingMode, RenderingQuality renderingQuality) :
	CylinderPrimitive(shape, shadingMode, renderingQuality)
{
#if 0
	QString prefix = renderer->glcontext()->isOpenGLES() ? QStringLiteral(":/openglrenderer_gles") : QStringLiteral(":/openglrenderer");

	// Initialize OpenGL shaders.
	if(shadingMode == NormalShading) {
		if(renderingQuality == HighQuality && shape == CylinderShape) {
			if(!renderer->useGeometryShaders()) {
				_shader = renderer->loadShaderProgram(
						"cylinder_raytraced",
						prefix + "/glsl/cylinder/cylinder_raytraced_tri.vs",
						prefix + "/glsl/cylinder/cylinder_raytraced.fs");
			}
			else {
				_shader = renderer->loadShaderProgram(
						"cylinder_geomshader_raytraced",
						prefix + "/glsl/cylinder/cylinder_raytraced.vs",
						prefix + "/glsl/cylinder/cylinder_raytraced.fs",
						prefix + "/glsl/cylinder/cylinder_raytraced.gs");
			}
		}
		else {
			_shader = renderer->loadShaderProgram(
					"arrow_shaded",
					prefix + "/glsl/arrows/shaded.vs",
					prefix + "/glsl/arrows/shaded.fs");
		}
	}
	else if(shadingMode == FlatShading) {
		if(!renderer->useGeometryShaders() || shape != CylinderShape) {
			_shader = renderer->loadShaderProgram(
					"arrow_flat",
					prefix + "/glsl/arrows/flat_tri.vs",
					prefix + "/glsl/arrows/flat.fs");
		}
		else {
			_shader = renderer->loadShaderProgram(
					"cylinder_geomshader_flat",
					prefix + "/glsl/arrows/flat.vs",
					prefix + "/glsl/arrows/flat.fs",
					prefix + "/glsl/cylinder/flat.gs");
		}
	}

	OVITO_ASSERT(_shader != nullptr);
#endif
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLCylinderPrimitive::render(OpenGLSceneRenderer* renderer)
{
	return;
	OVITO_ASSERT(basePositions());
	OVITO_ASSERT(headPositions());

	// Update primitive count.
	_primitiveCount = basePositions()->size();
	if(_primitiveCount <= 0)
		return;

	// Fill the OpenGL buffers with data.
	fillBuffers(renderer);

	// Bind OpenGL shader and set up GL state.
	renderer->glEnable(GL_CULL_FACE);
	renderer->glCullFace(GL_BACK);
	if(!_shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	// Activate OpenGL blending mode when rendering translucent elements.
	if(!renderer->isPicking() && transparencies()) {
		renderer->glEnable(GL_BLEND);
		renderer->glBlendEquation(GL_FUNC_ADD);
		renderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR, GL_ONE);
	}

	// Choose rendering technique.
	if(shadingMode() == NormalShading) {
		if(renderingQuality() == HighQuality && shape() == CylinderShape)
			renderWithElementInfo(renderer);
		else
			renderWithNormals(renderer);
	}
	else if(shadingMode() == FlatShading) {
		renderWithElementInfo(renderer);
	}

	// Reset state.
	_shader->release();
	renderer->glDisable(GL_CULL_FACE);

	// Deactivate blend mode after rendering translucent elements.
	if(!renderer->isPicking() && transparencies()) {
		renderer->glDisable(GL_BLEND);
	}
}

/******************************************************************************
* Renders the geometry as triangle mesh with normals.
******************************************************************************/
void OpenGLCylinderPrimitive::renderWithNormals(OpenGLSceneRenderer* renderer)
{
	_shader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());
	_shader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));
	if(!renderer->isPicking())
		_shader->setUniformValue("normal_matrix", (QMatrix3x3)(renderer->modelViewTM().linear().inverse().transposed()));

	if(renderer->isPicking()) {
		GLint pickingBaseID = renderer->registerSubObjectIDs(_primitiveCount);
		_shader->setUniformValue("picking_base_id", pickingBaseID);
	}

	_verticesWithNormals.bindPositions(renderer, _shader, offsetof(VertexWithNormal, pos));
	if(!renderer->isPicking()) {
		_verticesWithNormals.bindNormals(renderer, _shader, offsetof(VertexWithNormal, normal));
		_verticesWithNormals.bindColors(renderer, _shader, 4, offsetof(VertexWithNormal, color));
	}

	if(!QOpenGLContext::currentContext()->isOpenGLES()) {
		int stripPrimitivesPerElement = _stripPrimitiveVertexCounts.size() / _primitiveCount;
		int stripVerticesPerElement = std::accumulate(_stripPrimitiveVertexCounts.begin(), _stripPrimitiveVertexCounts.begin() + stripPrimitivesPerElement, 0);
		OVITO_CHECK_OPENGL(renderer, _shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
		OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_STRIP, _stripPrimitiveVertexStarts.data(), _stripPrimitiveVertexCounts.data(), stripPrimitivesPerElement * _primitiveCount));

		int fanPrimitivesPerElement = _fanPrimitiveVertexCounts.size() / _primitiveCount;
		int fanVerticesPerElement = std::accumulate(_fanPrimitiveVertexCounts.begin(), _fanPrimitiveVertexCounts.begin() + fanPrimitivesPerElement, 0);
		OVITO_CHECK_OPENGL(renderer, _shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
		OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_FAN, _fanPrimitiveVertexStarts.data(), _fanPrimitiveVertexCounts.data(), fanPrimitivesPerElement * _primitiveCount));
	}
	else {
		OVITO_CHECK_OPENGL(renderer, _shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, _indicesPerElement * _primitiveCount, GL_UNSIGNED_INT, _trianglePrimitiveVertexIndices.data()));
	}
	_verticesWithNormals.detachPositions(renderer, _shader);
	if(!renderer->isPicking()) {
		_verticesWithNormals.detachNormals(renderer, _shader);
		_verticesWithNormals.detachColors(renderer, _shader);
	}
}

/******************************************************************************
* Renders the geometry as with extra information passed to the vertex _shader.
******************************************************************************/
void OpenGLCylinderPrimitive::renderWithElementInfo(OpenGLSceneRenderer* renderer)
{
	_shader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());
	_shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());
	_shader->setUniformValue("modelview_uniform_scale", (float)pow(std::abs(renderer->modelViewTM().determinant()), (FloatType(1.0/3.0))));
	_shader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));
	_shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	_shader->setUniformValue("inverse_projection_matrix", (QMatrix4x4)renderer->projParams().inverseProjectionMatrix);
	_shader->setUniformValue("is_perspective", renderer->projParams().isPerspective);

	AffineTransformation viewModelTM = renderer->modelViewTM().inverse();
	Vector3 eye_pos = viewModelTM.translation();
	_shader->setUniformValue("eye_pos", eye_pos.x(), eye_pos.y(), eye_pos.z());
	Vector3 viewDir = viewModelTM * Vector3(0,0,1);
	_shader->setUniformValue("parallel_view_dir", viewDir.x(), viewDir.y(), viewDir.z());

	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	_shader->setUniformValue("viewport_origin", (float)viewportCoords[0], (float)viewportCoords[1]);
	_shader->setUniformValue("inverse_viewport_size", 2.0f / (float)viewportCoords[2], 2.0f / (float)viewportCoords[3]);

	if(renderer->isPicking()) {
		GLint pickingBaseID = renderer->registerSubObjectIDs(_primitiveCount);
		_shader->setUniformValue("picking_base_id", pickingBaseID);
		_shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement);
	}

	_verticesWithElementInfo.bindPositions(renderer, _shader, offsetof(VertexWithElementInfo, pos));
	_verticesWithElementInfo.bind(renderer, _shader, "cylinder_base", GL_FLOAT, offsetof(VertexWithElementInfo, base), 3, sizeof(VertexWithElementInfo));
	_verticesWithElementInfo.bind(renderer, _shader, "cylinder_head", GL_FLOAT, offsetof(VertexWithElementInfo, head), 3, sizeof(VertexWithElementInfo));
	_verticesWithElementInfo.bind(renderer, _shader, "cylinder_radius", GL_FLOAT, offsetof(VertexWithElementInfo, radius), 1, sizeof(VertexWithElementInfo));
	if(!renderer->isPicking())
		_verticesWithElementInfo.bindColors(renderer, _shader, 4, offsetof(VertexWithElementInfo, color));

	if(renderer->useGeometryShaders() && (shadingMode() == FlatShading || renderingQuality() == HighQuality) && shape() == CylinderShape) {
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_POINTS, 0, _primitiveCount));
	}
	else {
		if(!QOpenGLContext::currentContext()->isOpenGLES()) {
			int stripPrimitivesPerElement = _stripPrimitiveVertexCounts.size() / _primitiveCount;
			OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_STRIP, _stripPrimitiveVertexStarts.data(), _stripPrimitiveVertexCounts.data(), stripPrimitivesPerElement * _primitiveCount));

			int fanPrimitivesPerElement = _fanPrimitiveVertexCounts.size() / _primitiveCount;
			OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArrays(GL_TRIANGLE_FAN, _fanPrimitiveVertexStarts.data(), _fanPrimitiveVertexCounts.data(), fanPrimitivesPerElement * _primitiveCount));
		}
		else {
			OVITO_CHECK_OPENGL(renderer, _shader->setUniformValue("verticesPerElement", (GLint)_verticesPerElement));
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, _indicesPerElement * _primitiveCount, GL_UNSIGNED_INT, _trianglePrimitiveVertexIndices.data()));
		}		
	}

	_verticesWithElementInfo.detachPositions(renderer, _shader);
	_verticesWithElementInfo.detach(renderer, _shader, "cylinder_base");
	_verticesWithElementInfo.detach(renderer, _shader, "cylinder_head");
	_verticesWithElementInfo.detach(renderer, _shader, "cylinder_radius");
	if(!renderer->isPicking())
		_verticesWithElementInfo.detachColors(renderer, _shader);
}

/******************************************************************************
* Creates and fills the OpenGL VBO buffers with data.
******************************************************************************/
void OpenGLCylinderPrimitive::fillBuffers(OpenGLSceneRenderer* renderer)
{
	if(_verticesWithNormals.isCreated() || _verticesWithElementInfo.isCreated())
		return;

	bool renderMesh = true;

	// Determine the number of triangle strips and triangle fans required to render N primitives.
	int stripsPerElement;
	int fansPerElement;
	int verticesPerStrip;
	int verticesPerFan;
	if(shadingMode() == NormalShading) {
		verticesPerStrip = _cylinderSegments * 2 + 2;
		verticesPerFan = _cylinderSegments;
		if(shape() == ArrowShape) {
			stripsPerElement = 2;
			fansPerElement = 2;
		}
		else {
			stripsPerElement = 1;
			fansPerElement = 2;
			if(renderingQuality() == HighQuality) {
				if(renderer->useGeometryShaders()) {
					verticesPerStrip = 1;
					stripsPerElement = 1;
				}
				else {
					verticesPerStrip = 14;
				}
				fansPerElement = verticesPerFan = 0;
				renderMesh = false;
			}
		}
	}
	else if(shadingMode() == FlatShading) {
		fansPerElement = 1;
		stripsPerElement = 0;
		verticesPerStrip = 0;
		if(shape() == ArrowShape)
			verticesPerFan = 7;
		else
			verticesPerFan = 4;
		if(renderer->useGeometryShaders() && shape() == CylinderShape) {
			verticesPerFan = 1;
		}
		renderMesh = false;
	}
	else OVITO_ASSERT(false);

	_verticesPerElement = stripsPerElement * verticesPerStrip + fansPerElement * verticesPerFan;

	// Allocate VBOs.
	if(renderMesh) {
		_verticesWithNormals.create(QOpenGLBuffer::StaticDraw, _primitiveCount, _verticesPerElement);
		_mappedVerticesWithNormals = _verticesWithNormals.map();
	}
	else {
		_verticesWithElementInfo.create(QOpenGLBuffer::StaticDraw, _primitiveCount, _verticesPerElement);
		_mappedVerticesWithElementInfo = _verticesWithElementInfo.map();
	}

	if(!QOpenGLContext::currentContext()->isOpenGLES()) {
		// Prepare arrays to be passed to the glMultiDrawArrays() function.
		_stripPrimitiveVertexStarts.resize(_primitiveCount * stripsPerElement);
		_stripPrimitiveVertexCounts.resize(_primitiveCount * stripsPerElement);
		_fanPrimitiveVertexStarts.resize(_primitiveCount * fansPerElement);
		_fanPrimitiveVertexCounts.resize(_primitiveCount * fansPerElement);
		std::fill(_stripPrimitiveVertexCounts.begin(), _stripPrimitiveVertexCounts.end(), verticesPerStrip);
		std::fill(_fanPrimitiveVertexCounts.begin(), _fanPrimitiveVertexCounts.end(), verticesPerFan);
		auto ps_strip = _stripPrimitiveVertexStarts.begin();
		auto ps_fan = _fanPrimitiveVertexStarts.begin();
		for(GLint index = 0, baseIndex = 0; index < _primitiveCount; index++) {
			for(int p = 0; p < stripsPerElement; p++, baseIndex += verticesPerStrip)
				*ps_strip++ = baseIndex;
			for(int p = 0; p < fansPerElement; p++, baseIndex += verticesPerFan)
				*ps_fan++ = baseIndex;
		}
	}
	else {
		// Prepare list of vertex indices needed for glDrawElements() call.
		_indicesPerElement = 3 * (stripsPerElement * std::max(verticesPerStrip - 2, 0) + fansPerElement * std::max(verticesPerFan - 2, 0));
		_trianglePrimitiveVertexIndices.resize(_indicesPerElement * _primitiveCount);
		auto pvi = _trianglePrimitiveVertexIndices.begin();
		for(int index = 0, baseIndex = 0; index < _primitiveCount; index++) {
			for(int p = 0; p < stripsPerElement; p++, baseIndex += verticesPerStrip) {
				for(int u = 2; u < verticesPerStrip; u++) {
					if((u & 1) == 0) {
						*pvi++ = baseIndex + u - 2;
						*pvi++ = baseIndex + u - 1;
						*pvi++ = baseIndex + u - 0;
					}
					else {
						*pvi++ = baseIndex + u - 0;
						*pvi++ = baseIndex + u - 1;
						*pvi++ = baseIndex + u - 2;
					}
				}
			}
			for(int p = 0; p < fansPerElement; p++, baseIndex += verticesPerFan) {
				for(int u = 2; u < verticesPerFan; u++) {
					*pvi++ = baseIndex;
					*pvi++ = baseIndex + u - 1;
					*pvi++ = baseIndex + u - 0;
				}
			}
		}
		OVITO_ASSERT(pvi == _trianglePrimitiveVertexIndices.end());
	}

	// Precompute cos() and sin() functions.
	if(shadingMode() == NormalShading) {
		_cosTable.resize(_cylinderSegments + 1);
		_sinTable.resize(_cylinderSegments + 1);
		for(int i = 0; i <= _cylinderSegments; i++) {
			float angle = (FLOATTYPE_PI * 2 / _cylinderSegments) * i;
			_cosTable[i] = std::cos(angle);
			_sinTable[i] = std::sin(angle);
		}
	}

	ConstDataBufferAccess<Point3> basePositionsBuffer(basePositions());
	ConstDataBufferAccess<Point3> headPositionsBuffer(headPositions());
	ConstDataBufferAccess<Color> colorsBuffer(colors());
	ConstDataBufferAccess<FloatType> transparenciesBuffer(transparencies());
	ConstDataBufferAccess<FloatType> radiiBuffer(radii());

	for(int index = 0; index < _primitiveCount; index++) {
		const Point3& base = basePositionsBuffer[index];
		const Point3& head = headPositionsBuffer[index];
		ColorA color = colorsBuffer ? colorsBuffer[index] : uniformColor();
		color.a() = transparenciesBuffer ? (FloatType(1) - transparenciesBuffer[index]) : FloatType(1);
		FloatType radius = radiiBuffer ? radiiBuffer[index] : uniformRadius();
		if(shape() == ArrowShape)
			createArrowElement(renderer, (Point_3<float>)base, (Point_3<float>)head, (ColorAT<float>)color, (float)radius);
		else
			createCylinderElement(renderer, (Point_3<float>)base, (Point_3<float>)head, (ColorAT<float>)color, (float)radius);
	}

	if(_mappedVerticesWithNormals)
		_verticesWithNormals.unmap();
	if(_mappedVerticesWithElementInfo)
		_verticesWithElementInfo.unmap();
}

/******************************************************************************
* Creates the geometry for a single cylinder element.
******************************************************************************/
void OpenGLCylinderPrimitive::createCylinderElement(OpenGLSceneRenderer* renderer, const Point_3<float>& base, const Point_3<float>& head, const ColorAT<float>& color, float radius)
{
	if(renderer->useGeometryShaders() && (shadingMode() == FlatShading || renderingQuality() == HighQuality)) {
		OVITO_ASSERT(_mappedVerticesWithElementInfo);
		OVITO_ASSERT(_verticesPerElement == 1);
		_mappedVerticesWithElementInfo->pos = base;
		_mappedVerticesWithElementInfo->base = base;
		_mappedVerticesWithElementInfo->head = head;
		_mappedVerticesWithElementInfo->color = color;
		_mappedVerticesWithElementInfo->radius = radius;
		++_mappedVerticesWithElementInfo;
		return;
	}

	if(shadingMode() == NormalShading) {

		// Build local coordinate system.
		Vector_3<float> t, u, v;
		t = head - base;
		float length = t.length();
		if(length != 0) {
			t /= length;
			if(t.y() != 0 || t.x() != 0)
				u = Vector_3<float>(t.y(), -t.x(), 0);
			else
				u = Vector_3<float>(-t.z(), 0, t.x());
			u.normalize();
			v = u.cross(t);
		}
		else {
			t.setZero();
			u.setZero();
			v.setZero();
		}

		ColorAT<float> c = color;
		Point_3<float> v1 = base;
		Point_3<float> v2 = head;

		if(renderingQuality() != HighQuality) {
			OVITO_ASSERT(_mappedVerticesWithNormals);
			VertexWithNormal*& vertex = _mappedVerticesWithNormals;

			// Generate vertices for cylinder mantle.
			for(int i = 0; i <= _cylinderSegments; i++) {
				Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
				Vector_3<float> d = n * radius;
				vertex->pos = v1 + d;
				vertex->normal = n;
				vertex->color = c;
				vertex++;
				vertex->pos = v2 + d;
				vertex->normal = n;
				vertex->color = c;
				vertex++;
			}

			// Generate vertices for first cylinder cap.
			for(int i = 0; i < _cylinderSegments; i++) {
				Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
				Vector_3<float> d = n * radius;
				vertex->pos = v1 + d;
				vertex->normal = Vector_3<float>(0,0,-1);
				vertex->color = c;
				vertex++;
			}

			// Generate vertices for second cylinder cap.
			for(int i = _cylinderSegments - 1; i >= 0; i--) {
				Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
				Vector_3<float> d = n * radius;
				vertex->pos = v2 + d;
				vertex->normal = Vector_3<float>(0,0,1);
				vertex->color = c;
				vertex++;
			}
		}
		else {
			// Create bounding box geometry around cylinder for raytracing.
			OVITO_ASSERT(_mappedVerticesWithElementInfo);
			VertexWithElementInfo*& vertex = _mappedVerticesWithElementInfo;
			OVITO_ASSERT(_verticesPerElement == 14);
			u *= radius;
			v *= radius;
			Point_3<float> corners[8] = {
					v1 - u - v,
					v1 - u + v,
					v1 + u - v,
					v1 + u + v,
					v2 - u - v,
					v2 - u + v,
					v2 + u + v,
					v2 + u - v
			};
			const static size_t stripIndices[14] = { 3,2,6,7,4,2,0,3,1,6,5,4,1,0 };
			for(int i = 0; i < 14; i++, vertex++) {
				vertex->pos = corners[stripIndices[i]];
				vertex->base = v1;
				vertex->head = v2;
				vertex->color = c;
				vertex->radius = radius;
			}
		}
	}
	else if(shadingMode() == FlatShading) {

		Vector_3<float> t = head - base;
		float length = t.length();
		if(length != 0)
			t /= length;
	
		ColorAT<float> c = color;

		OVITO_ASSERT(_mappedVerticesWithElementInfo);
		VertexWithElementInfo*& vertex = _mappedVerticesWithElementInfo;
		vertex[0].pos = Point_3<float>(0, radius, 0);
		vertex[1].pos = Point_3<float>(0, -radius, 0);
		vertex[2].pos = Point_3<float>(length, -radius, 0);
		vertex[3].pos = Point_3<float>(length, radius, 0);
		for(int i = 0; i < _verticesPerElement; i++, ++vertex) {
			vertex->base = base;
			vertex->head = base + t;
			vertex->color = c;
		}
	}
}

/******************************************************************************
* Creates the geometry for a single arrow element.
******************************************************************************/
void OpenGLCylinderPrimitive::createArrowElement(OpenGLSceneRenderer* renderer, const Point_3<float>& base, const Point_3<float>& head, const ColorAT<float>& color, float radius)
{
	const float arrowHeadRadius = radius * 2.5f;
	const float arrowHeadLength = arrowHeadRadius * 1.8f;

	if(shadingMode() == NormalShading) {

		// Build local coordinate system.
		Vector_3<float> t, u, v;
		t = head - base;
		float length = t.length();
		if(length != 0) {
			t /= length;
			if(t.y() != 0 || t.x() != 0)
				u = Vector_3<float>(t.y(), -t.x(), 0);
			else
				u = Vector_3<float>(-t.z(), 0, t.x());
			u.normalize();
			v = u.cross(t);
		}
		else {
			t.setZero();
			u.setZero();
			v.setZero();
		}

		ColorAT<float> c = color;
		Point_3<float> v1 = base;
		Point_3<float> v2;
		Point_3<float> v3 = head;
		float r;
		if(length > arrowHeadLength) {
			v2 = v1 + t * (length - arrowHeadLength);
			r = arrowHeadRadius;
		}
		else {
			v2 = v1;
			r = arrowHeadRadius * length / arrowHeadLength;
			// Hide cylinder:
			radius = 0;
		}

		OVITO_ASSERT(_mappedVerticesWithNormals);
		VertexWithNormal*& vertex = _mappedVerticesWithNormals;

		// Generate vertices for cylinder.
		for(int i = 0; i <= _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * radius;
			vertex->pos = v1 + d;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
			vertex->pos = v2 + d;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
		}

		// Generate vertices for head cone.
		for(int i = 0; i <= _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * r;
			vertex->pos = v2 + d;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
			vertex->pos = v3;
			vertex->normal = n;
			vertex->color = c;
			vertex++;
		}

		// Generate vertices for cylinder cap.
		for(int i = 0; i < _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * radius;
			vertex->pos = v1 + d;
			vertex->normal = Vector_3<float>(0,0,-1);
			vertex->color = c;
			vertex++;
		}

		// Generate vertices for cone cap.
		for(int i = 0; i < _cylinderSegments; i++) {
			Vector_3<float> n = _cosTable[i] * u + _sinTable[i] * v;
			Vector_3<float> d = n * r;
			vertex->pos = v2 + d;
			vertex->normal = Vector_3<float>(0,0,-1);
			vertex->color = c;
			vertex++;
		}
	}
	else if(shadingMode() == FlatShading) {

		Vector_3<float> t = head - base;
		float length = t.length();
		if(length != 0)
			t /= length;

		ColorAT<float> c = color;

		OVITO_ASSERT(_mappedVerticesWithElementInfo);
		VertexWithElementInfo*& vertices = _mappedVerticesWithElementInfo;
		OVITO_ASSERT(_verticesPerElement == 7);

		if(length > arrowHeadLength) {
			vertices[0].pos = Point_3<float>(length, 0, 0);
			vertices[1].pos = Point_3<float>(length - arrowHeadLength, arrowHeadRadius, 0);
			vertices[2].pos = Point_3<float>(length - arrowHeadLength, radius, 0);
			vertices[3].pos = Point_3<float>(0, radius, 0);
			vertices[4].pos = Point_3<float>(0, -radius, 0);
			vertices[5].pos = Point_3<float>(length - arrowHeadLength, -radius, 0);
			vertices[6].pos = Point_3<float>(length - arrowHeadLength, -arrowHeadRadius, 0);
		}
		else {
			float r = arrowHeadRadius * length / arrowHeadLength;
			vertices[0].pos = Point_3<float>(length, 0, 0);
			vertices[1].pos = Point_3<float>(0, r, 0);
			vertices[2].pos = Point_3<float>::Origin();
			vertices[3].pos = Point_3<float>::Origin();
			vertices[4].pos = Point_3<float>::Origin();
			vertices[5].pos = Point_3<float>::Origin();
			vertices[6].pos = Point_3<float>(0, -r, 0);
		}
		for(int i = 0; i < _verticesPerElement; i++, ++vertices) {
			vertices->base = base;
			vertices->head = base + t;
			vertices->color = c;
		}
	}
}

}	// End of namespace

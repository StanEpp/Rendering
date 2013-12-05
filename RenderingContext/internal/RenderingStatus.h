/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DATA_H_
#define RENDERING_DATA_H_

#include "../RenderingParameters.h"
#include <Geometry/Matrix4x4.h>
#include <bitset>
#include <cassert>
#include <deque>
#include <vector>
#include <cstdint>

namespace Rendering {
class Shader;

//! (internal) Used by shaders and the renderingContext to track the state of shader (and openGL) dependent properties.
class RenderingStatus {

	//!	@name General
	//	@{
	private:
		Shader * shader;
		bool initialized;

	public:
		explicit RenderingStatus(Shader * _shader = nullptr) : 
			shader(_shader), 
			initialized(false),
			cameraCheckNumber(0),
			cameraMatrix(),
			cameraInverseMatrix(),
			lightsCheckNumber(0),
			lights(),
			lightsEnabled(0),
			materialCheckNumber(0),
			materialEnabled(false),
			material(),
			modelViewMatrixCheckNumber(0),
			modelViewMatrix(),
			pointParameters(),
			projectionMatrixCheckNumber(0),
			projectionMatrix(),
			textureUnitUsagesCheckNumber(0),
			textureUnitUsages(MAX_TEXTURES, TexUnitUsageParameter::GENERAL_PURPOSE) {
		}
		Shader * getShader() 						{	return shader;	}
		bool isInitialized()const					{	return initialized;	}
		void markInitialized()						{	initialized=true;	}
	//	@}

	// -------------------------------

	//!	@name Camera Matrix
	//	@{
	private:
		uint32_t cameraCheckNumber;
		Geometry::Matrix4x4f cameraMatrix;
		Geometry::Matrix4x4f cameraInverseMatrix;
	public:
		bool cameraInverseMatrixChanged(const RenderingStatus & actual) const {
			return (cameraCheckNumber == actual.cameraCheckNumber) ? false :
					cameraInverseMatrix != actual.cameraInverseMatrix;
		}
		const Geometry::Matrix4x4f & getCameraInverseMatrix() const {
			return cameraInverseMatrix;
		}
		const Geometry::Matrix4x4f & getCameraMatrix() const {
			return cameraMatrix;
		}
		void setCameraInverseMatrix(const Geometry::Matrix4x4f & matrix) {
			cameraInverseMatrix = matrix;
			cameraMatrix = matrix.inverse();
			++cameraCheckNumber;
		}
		void updateCameraMatrix(const RenderingStatus & actual) {
			cameraInverseMatrix = actual.cameraInverseMatrix;
			cameraMatrix = actual.cameraMatrix;
			cameraCheckNumber = actual.cameraCheckNumber;
		}
	//	@}

	// ------

	//!	@name Lights
	//	@{
	public:
		static const uint8_t MAX_LIGHTS = 8;
	private:
		uint32_t lightsCheckNumber;
		//! Storage of light parameters.
		LightParameters lights[MAX_LIGHTS];

		//! Status of the lights (1 = enabled, 0 = disabled).
		std::bitset<MAX_LIGHTS> lightsEnabled;
	public:
		//! Return the number of lights that are currently enabled.
		uint8_t getNumEnabledLights() const {
			return lightsEnabled.count();
		}
		//! Of the lights that are enabled, return the one with the given index.
		const LightParameters & getEnabledLight(uint8_t index) const {
			assert(index < getNumEnabledLights());
			uint_fast8_t pos = 0;
			// Find first enabled light.
			while(!lightsEnabled[pos]) {
				++pos;
			}
			// Find next enabled light, until the index'th enabled light is found.
			while(index > 0) {
				while(!lightsEnabled[pos]) {
					++pos;
				}
				--index;
				++pos;
			}
			return lights[pos];
		}
		//! Enable the light given by its parameters. Return the number that can be used to disable it.
		uint8_t enableLight(const LightParameters & light) {
			assert(getNumEnabledLights() < MAX_LIGHTS);
			uint_fast8_t pos = 0;
			while(lightsEnabled[pos]) {
				++pos;
			}
			++lightsCheckNumber;
			lights[pos] = light;
			lightsEnabled[pos] = true;
			return pos;
		}
		//! Disable the light with the given number.
		void disableLight(uint8_t lightNumber) {
			assert(lightsEnabled[lightNumber]);
			++lightsCheckNumber;
			lightsEnabled[lightNumber] = false;
		}
		//! Return @c true, if the light with the given light number is enabled.
		bool isLightEnabled(uint8_t lightNumber) const {
			return lightsEnabled[lightNumber];
		}
		bool lightsChanged(const RenderingStatus & actual) const {
			if (lightsCheckNumber == actual.lightsCheckNumber)
				return false;
			if (lightsEnabled != actual.lightsEnabled)
				return true;
			for (uint_fast8_t i = 0; i < getNumEnabledLights(); ++i) {
				if (getEnabledLight(i) != actual.getEnabledLight(i)) {
					return true;
				}
			}
			return false;
		}
		void updateLights(const RenderingStatus & actual) {
			lightsEnabled = actual.lightsEnabled;
			lightsCheckNumber = actual.lightsCheckNumber;
		}
		void updateLightParameter(uint8_t lightNumber, const LightParameters & light) {
			assert(lightNumber < MAX_LIGHTS);
			lights[lightNumber] = light;
		}
	//	@}

	// ------

	//!	@name Materials
	//	@{
	private:
		uint32_t materialCheckNumber;
		bool materialEnabled;
		MaterialParameters material;

	public:
		bool isMaterialEnabled()const								{	return materialEnabled;	}
		const MaterialParameters & getMaterialParameters()const	{	return material;	}
		bool materialChanged(const RenderingStatus & actual) const {
			return (materialCheckNumber == actual.materialCheckNumber) ? false :
						(materialEnabled != actual.materialEnabled || material != actual.material);
		}
		void setMaterial(const MaterialParameters & mat) {
			material = mat;
			materialEnabled = true;
			++materialCheckNumber;
		}
		void updateMaterial(const RenderingStatus & actual) {
			materialEnabled=actual.materialEnabled;
			material=actual.material;
			materialCheckNumber=actual.materialCheckNumber;
		}
		void disableMaterial() {
			materialEnabled = false;
			++materialCheckNumber;
		}
	//	@}

	// ------

	//!	@name Modelview Matrix
	//	@{
	private:
		uint32_t modelViewMatrixCheckNumber;
		Geometry::Matrix4x4f modelViewMatrix;

	public:
		const Geometry::Matrix4x4f & getModelViewMatrix() const 				{	return modelViewMatrix;	}
		void setModelViewMatrix(const Geometry::Matrix4x4f & matrix) {
			modelViewMatrix = matrix;
			++modelViewMatrixCheckNumber;
		}
		bool modelViewMatrixChanged(const RenderingStatus & actual) const {
			return (modelViewMatrixCheckNumber == actual.modelViewMatrixCheckNumber) ? false :
					modelViewMatrix != actual.modelViewMatrix;
		}
		void multModelViewMatrix(const Geometry::Matrix4x4f & matrix) {
			modelViewMatrix *= matrix;
			++modelViewMatrixCheckNumber;
		}
		void updateModelViewMatrix(const RenderingStatus & actual) {
			modelViewMatrix = actual.modelViewMatrix;
			modelViewMatrixCheckNumber = actual.modelViewMatrixCheckNumber;
		}
	//	@}

	// ------

	//!	@name Point
	//	@{
	private:
		PointParameters pointParameters;
	public:
		bool pointParametersChanged(const RenderingStatus & actual) const {
			return pointParameters != actual.pointParameters;
		}
		const PointParameters & getPointParameters() const {
			return pointParameters;
		}
		void setPointParameters(const PointParameters & p) {
			pointParameters = p;
		}
	//	@}

	// ------

	//!	@name Projection Matrix
	//	@{
	private:
		uint32_t projectionMatrixCheckNumber;
		Geometry::Matrix4x4f projectionMatrix;

	public:
		void setProjectionMatrix(const Geometry::Matrix4x4f & matrix) {
			projectionMatrix = matrix;
			++projectionMatrixCheckNumber;
		}
		const Geometry::Matrix4x4f & getProjectionMatrix() const 				{	return projectionMatrix;	}
		void updateProjectionMatrix(const RenderingStatus & actual) {
			projectionMatrix = actual.projectionMatrix;
			projectionMatrixCheckNumber = actual.projectionMatrixCheckNumber;
		}
		bool projectionMatrixChanged(const RenderingStatus & actual) const {
			return (projectionMatrixCheckNumber == actual.projectionMatrixCheckNumber) ? false :
					projectionMatrix != actual.projectionMatrix;
		}
	//	@}

	// ------

	//!	@name Texture Units
	//	@{
	private:
		uint32_t textureUnitUsagesCheckNumber;
		std::vector<TexUnitUsageParameter> textureUnitUsages;

	public:
		void setTextureUnitUsage(uint8_t unit, TexUnitUsageParameter use) {
			++textureUnitUsagesCheckNumber;
			textureUnitUsages.at(unit) = use;
		}
		const TexUnitUsageParameter & getTextureUnitUsage(uint8_t unit) const {
			return textureUnitUsages.at(unit);
		}
		bool textureUnitsChanged(const RenderingStatus & actual) const {
			return (textureUnitUsagesCheckNumber == actual.textureUnitUsagesCheckNumber) ? false : 
					textureUnitUsages != actual.textureUnitUsages;
		}
		void updateTextureUnits(const RenderingStatus & actual) {
			textureUnitUsages = actual.textureUnitUsages;
			textureUnitUsagesCheckNumber = actual.textureUnitUsagesCheckNumber;
		}

};

}

#endif /* RENDERING_DATA_H_ */
/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Context.h"

#include "Platform.h"
#include "Device.h"
#include "CLUtils.h"

#include "../Helper.h"

#include <Util/Macros.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wpedantic)
COMPILER_WARN_OFF(-Wold-style-cast)
COMPILER_WARN_OFF(-Wcast-qual)
COMPILER_WARN_OFF(-Wshadow)
COMPILER_WARN_OFF(-Wstack-protector)
#include <CL/cl.hpp>
COMPILER_WARN_POP


#if defined __APPLE__ || defined(MACOSX)
#else
    #if defined _WIN32
		#include <gl/gl.h> //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    #else
        //needed for context sharing functions
        #include <GL/glx.h>
    #endif
#endif

namespace Rendering {
namespace CL {

inline
std::vector<cl_context_properties> getContextProperties(const cl::Platform& platform, bool shareGLContext) {
	if(shareGLContext) {


		#if defined (__APPLE__) || defined(MACOSX)
			CGLContextObj kCGLContext = CGLGetCurrentContext();
			CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
			return {
				CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, reinterpret_cast<cl_context_properties>(kCGLShareGroup),
				CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()),
				0
			};
		#else
			#if defined _WIN32 // Win32
				return {
					CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(wglGetCurrentContext()),
					CL_WGL_HDC_KHR, reinterpret_cast<cl_context_properties>(wglGetCurrentDC()),
					CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()),
					0
				};
			#else
				return {
					CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(glXGetCurrentContext()),
					CL_GLX_DISPLAY_KHR, reinterpret_cast<cl_context_properties>(glXGetCurrentDisplay()),
					CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()),
					0
				};
			#endif
		#endif
	} else {
		return {CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()), 0};
	}
}

Context::Context(uint32_t device_type, bool shareGLContext /*= false*/) : Util::ReferenceCounter<Context>(),
		platform(nullptr), glInterop(shareGLContext) {
	cl_int err;
	auto pfAndDev = getFirstPlatformAndDeviceFor(device_type);
	platform = std::get<0>(pfAndDev);
	auto cprops = getContextProperties(*platform->_internal(), shareGLContext);
	context.reset(new cl::Context(static_cast<cl_device_type>(device_type), cprops.data(), nullptr, nullptr, &err));
	if(err != CL_SUCCESS)
		WARN("Could not create context (" + getErrorString(err) + ")");
	FAIL_IF(err != CL_SUCCESS);
}

Context::Context(Platform* _platform, uint32_t device_type, bool shareGLContext /*= false*/) : Util::ReferenceCounter<Context>(),
		platform(_platform), glInterop(shareGLContext) {
	cl_int err;
	auto cprops = getContextProperties(*platform->_internal(), shareGLContext);
	context.reset(new cl::Context(static_cast<cl_device_type>(device_type), cprops.data(), nullptr, nullptr, &err));
	if(err != CL_SUCCESS)
		WARN("Could not create context (" + getErrorString(err) + ")");
	FAIL_IF(err != CL_SUCCESS);
}

Context::~Context() = default;

Context::Context(Platform* _platform, const std::vector<DeviceRef>& _devices, bool shareGLContext /*= false*/) : Util::ReferenceCounter<Context>(),
		platform(_platform), devices(_devices), glInterop(shareGLContext) {

	cl_int err;
	auto cprops = getContextProperties(*platform->_internal(), shareGLContext);
	std::vector<cl::Device> cl_devices;
	for(auto device : devices)
		cl_devices.push_back(*device->_internal());
	context.reset(new cl::Context(cl_devices, cprops.data(), nullptr, nullptr, &err));
	if(err != CL_SUCCESS)
		WARN("Could not create context (" + getErrorString(err) + ")");
	FAIL_IF(err != CL_SUCCESS);
}

Context::Context(Platform* _platform, Device* _device, bool shareGLContext /*= false*/) : Util::ReferenceCounter<Context>(),
		platform(_platform), devices(std::vector<DeviceRef>{_device}), glInterop(shareGLContext) {
	cl_int err;
	auto cprops = getContextProperties(*platform->_internal(), shareGLContext);
	std::vector<cl::Device> cl_devices;
	context.reset(new cl::Context(*_device->_internal(), cprops.data(), nullptr, nullptr, &err));
	if(err != CL_SUCCESS)
		WARN("Could not create context (" + getErrorString(err) + ")");
	FAIL_IF(err != CL_SUCCESS);
}

Context::Context(const Context& _context) : Util::ReferenceCounter<Context>(),
		context(new cl::Context(*_context.context.get())), platform(_context.platform), devices(_context.devices), glInterop(_context.glInterop) { }

//Context::Context(Context&& context) = default;
//
//Context& Context::operator=(Context&&) = default;

std::vector<intptr_t> Context::getProperties() const {
	return context->getInfo<CL_CONTEXT_PROPERTIES>();
}

std::vector<DeviceRef> Context::getDevices() {
	if(devices.empty()) {
		std::vector<cl::Device> cl_devices = context->getInfo<CL_CONTEXT_DEVICES>();
		for(auto device : cl_devices)
			devices.push_back(new Device(platform.get(), &device));
	}
	return devices;
}


} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

// wlroots.hpp
//
// Single include point for all wlroots/wayland C headers.
// Handles two recurring C++ interop problems:
//   1. wlroots headers aren't wrapped in extern "C", so symbols get
//      C++-mangled unless we do it ourselves here.
//   2. A few headers use "class" / "namespace" / "static" which are
//		reserved keywords in C++ and fail to compile as-is.
//
// Include this instead of any individual <wlr/...> header.

#pragma once

#include <wayland-server-core.h>

#define class class_
#define namespace namespace_
#define static

extern "C" {
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_subcompositor.h>
}

#undef class
#undef namespace
#undef static

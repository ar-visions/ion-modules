#pragma once
#include <dx/dx.hpp>
#include <media/color.hpp>
#include <media/canvas.hpp>

typedef var &Event;

/// compile-time transition controls on members; certain ones you want and certain ones you may not want.
/// it will be important to allow transitions on Region
template <class T>
struct uxTransitions { static const bool enabled = false; };

/// add this decoration under classes that are transitionable; they dont need to be of a base because it references any T
template <> struct uxTransitions < float   > { static const bool enabled = true; };
template <> struct uxTransitions < double  > { static const bool enabled = true; };
template <> struct uxTransitions < int8_t  > { static const bool enabled = true; };
template <> struct uxTransitions < int16_t > { static const bool enabled = true; };
template <> struct uxTransitions < int32_t > { static const bool enabled = true; };
template <> struct uxTransitions < int64_t > { static const bool enabled = true; };
template <> struct uxTransitions <uint8_t  > { static const bool enabled = true; };
template <> struct uxTransitions <uint16_t > { static const bool enabled = true; };
template <> struct uxTransitions <uint32_t > { static const bool enabled = true; };
template <> struct uxTransitions <uint64_t > { static const bool enabled = true; };
template <> struct uxTransitions <  rgba   > { static const bool enabled = true; };
template <> struct uxTransitions <  vec2   > { static const bool enabled = true; };
template <> struct uxTransitions <  vec3   > { static const bool enabled = true; };
template <> struct uxTransitions <  vec4   > { static const bool enabled = true; };
template <> struct uxTransitions <  m44    > { static const bool enabled = true; };

#include <ux/region.hpp>
#include <ux/node.hpp>

/*
#include <ux/app.hpp>
#include <ux/edit.hpp>
#include <ux/list.hpp>
#include <ux/label.hpp>
#include <ux/button.hpp>
*/

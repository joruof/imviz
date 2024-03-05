#pragma once

void checkAssertion(bool expr, const char* exprStr);

#define IM_ASSERT(_EXPR) checkAssertion(_EXPR, #_EXPR)
#define IMGUI_DEFINE_MATH_OPERATORS

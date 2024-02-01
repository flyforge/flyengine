#pragma once

#ifndef PL_CONCAT

/// \brief Concatenates two strings, even when the strings are macros themselves
#  define PL_CONCAT(x, y) PL_CONCAT_HELPER(x, y)
#  define PL_CONCAT_HELPER(x, y) PL_CONCAT_HELPER2(x, y)
#  define PL_CONCAT_HELPER2(x, y) x##y

#endif

#ifndef PL_STRINGIZE

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#  define PL_STRINGIZE(str) PL_STRINGIZE_HELPER(str)
#  define PL_STRINGIZE_HELPER(x) #x

#endif

#ifndef PL_ON

/// \brief Used in conjunction with PL_ENABLED and PL_DISABLED for safe checks. Define something to PL_ON or PL_OFF to work with those macros.
#  define PL_ON =

/// \brief Used in conjunction with PL_ENABLED and PL_DISABLED for safe checks. Define something to PL_ON or PL_OFF to work with those macros.
#  define PL_OFF !

/// \brief Used in conjunction with PL_ON and PL_OFF for safe checks. Use #if PL_ENABLED(x) or #if PL_DISABLED(x) in conditional compilation.
#  define PL_ENABLED(x) (1 PL_CONCAT(x, =) 1)

/// \brief Used in conjunction with PL_ON and PL_OFF for safe checks. Use #if PL_ENABLED(x) or #if PL_DISABLED(x) in conditional compilation.
#  define PL_DISABLED(x) (1 PL_CONCAT(x, =) 2)

/// \brief Checks whether x AND y are both defined as PL_ON or PL_OFF. Usually used to check whether configurations overlap, to issue an error.
#  define PL_IS_NOT_EXCLUSIVE(x, y) ((1 PL_CONCAT(x, =) 1) == (1 PL_CONCAT(y, =) 1))

/// \brief #TODO_SHADER Right now these are only used in the ST_SetsSlots unit test. We will need to decide what the best separation and naming for these sets is once the renderer can make actual use of these to improve performance.
# define SET_FRAME 0
# define SET_RENDER_PASS 1
# define SET_MATERIAL 2
# define SET_DRAW_CALL 3
# define SLOT_AUTO AUTO

/// \brief Binds the resource to the given set and slot. Note that this does not produce valid HLSL code, the code will instead be patched by the shader compiler.
#  define BIND_RESOURCE(Slot, Set) : register(PL_CONCAT(x, Slot), PL_CONCAT(space, Set))

/// \brief Binds the resource to the given set. Note that this does not produce valid HLSL code, the code will instead be patched by the shader compiler.
#define BIND_SET(Set) BIND_RESOURCE(SLOT_AUTO, Set)

#endif

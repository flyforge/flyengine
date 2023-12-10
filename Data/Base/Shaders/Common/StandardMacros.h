#pragma once

# define SET_FRAME 0
# define SET_RENDER_PASS 1
# define SET_MATERIAL 2
# define SET_DRAW_CALL 3

#define RESOURCE(name, Slot, Set) RES_##Name_##Slot_##Set

#ifndef PLASMA_CONCAT

/// \brief Concatenates two strings, even when the strings are macros themselves
#define PLASMA_CONCAT(x,y) PLASMA_CONCAT_HELPER(x,y)
#define PLASMA_CONCAT_HELPER(x,y) PLASMA_CONCAT_HELPER2(x,y)
#define PLASMA_CONCAT_HELPER2(x,y) x##y

#endif

#ifndef PLASMA_STRINGIZE

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define PLASMA_STRINGIZE(str) PLASMA_STRINGIZE_HELPER(str)
#define PLASMA_STRINGIZE_HELPER(x) #x

#endif

#ifndef PLASMA_ON

/// \brief Used in conjunction with PLASMA_ENABLED and PLASMA_DISABLED for safe checks. Define something to PLASMA_ON or PLASMA_OFF to work with those macros.
#define PLASMA_ON =

/// \brief Used in conjunction with PLASMA_ENABLED and PLASMA_DISABLED for safe checks. Define something to PLASMA_ON or PLASMA_OFF to work with those macros.
#define PLASMA_OFF !

/// \brief Used in conjunction with PLASMA_ON and PLASMA_OFF for safe checks. Use #if PLASMA_ENABLED(x) or #if PLASMA_DISABLED(x) in conditional compilation.
#define PLASMA_ENABLED(x) (1 PLASMA_CONCAT(x,=) 1)

/// \brief Used in conjunction with PLASMA_ON and PLASMA_OFF for safe checks. Use #if PLASMA_ENABLED(x) or #if PLASMA_DISABLED(x) in conditional compilation.
#define PLASMA_DISABLED(x) (1 PLASMA_CONCAT(x,=) 2)

/// \brief Checks whether x AND y are both defined as PLASMA_ON or PLASMA_OFF. Usually used to check whether configurations overlap, to issue an error.
#define PLASMA_IS_NOT_EXCLUSIVE(x, y) ((1 PLASMA_CONCAT(x,=) 1) == (1 PLASMA_CONCAT(y,=) 1))

#endif

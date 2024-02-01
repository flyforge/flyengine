#include <Utilities/UtilitiesPCH.h>

PL_STATICLINK_LIBRARY(Utilities)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(Utilities_Resources_ConfigFileResource);
}

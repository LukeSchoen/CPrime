#include "external_lifecycle.h"

int external_lifecycle_destroyed;

ExternalLifecycle::ExternalLifecycle()
{
  this->value = 9;
}

ExternalLifecycle::~ExternalLifecycle()
{
  external_lifecycle_destroyed = this->value;
}


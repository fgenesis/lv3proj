#ifndef FALCON_BASE_MODULE_H
#define FALCON_BASE_MODULE_H

Falcon::Module *FalconBaseModule_create(void);

// this does absolutely nothing.
FALCON_FUNC fal_NullFunc(Falcon::VMachine *vm);

void forbidden_init(Falcon::VMachine *vm);

#endif
